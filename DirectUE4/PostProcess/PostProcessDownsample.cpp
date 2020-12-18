#include "PostProcessDownsample.h"
#include "GPUProfiler.h"
#include "SceneFilterRendering.h"

const int32 GDownsampleTileSizeX = 8;
const int32 GDownsampleTileSizeY = 8;

/** Encapsulates the post processing down sample pixel shader. */
template <uint32 Method, uint32 ManuallyClampUV>
class FPostProcessDownsamplePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessDownsamplePS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return Method != 2 || /*IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4)*/true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(("METHOD"), Method);
		OutEnvironment.SetDefine(("MANUALLY_CLAMP_UV"), ManuallyClampUV);
	}

	/** Default constructor. */
	FPostProcessDownsamplePS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FSceneTextureShaderParameters SceneTextureParameters;
	FShaderParameter DownsampleParams;
	FShaderParameter BufferBilinearUVMinMax;


	/** Initialization constructor. */
	FPostProcessDownsamplePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		SceneTextureParameters.Bind(Initializer);
		DownsampleParams.Bind(Initializer.ParameterMap, ("DownsampleParams"));
		BufferBilinearUVMinMax.Bind(Initializer.ParameterMap, ("BufferBilinearUVMinMax"));
	}

	void SetParameters(const FRenderingCompositePassContext& Context, const PooledRenderTargetDesc* InputDesc, const FIntPoint& SrcSize, const FIntRect& SrcRect)
	{
		ID3D11PixelShader* const ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(ShaderRHI, Context.View.ViewUniformBuffer.get());
		SceneTextureParameters.Set(ShaderRHI, ESceneTextureSetupMode::All);

		// filter only if needed for better performance
		ID3D11SamplerState* Filter = (Method == 2) ?
			TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI() :
			TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI();

		{
			float PixelScale = (Method == 2) ? 0.5f : 1.0f;

			Vector4 DownsampleParamsValue(PixelScale / InputDesc->Extent.X, PixelScale / InputDesc->Extent.Y, 0, 0);
			SetShaderValue(ShaderRHI, DownsampleParams, DownsampleParamsValue);
		}

		PostprocessParameter.SetPS(ShaderRHI, Context, Filter);

		if (ManuallyClampUV)
		{
			Vector4 BufferBilinearUVMinMaxValue(
				(SrcRect.Min.X + 0.5f) / SrcSize.X, (SrcRect.Min.Y + 0.5f) / SrcSize.Y,
				(SrcRect.Max.X - 0.5f) / SrcSize.X, (SrcRect.Max.Y - 0.5f) / SrcSize.Y);
			SetShaderValue(ShaderRHI, BufferBilinearUVMinMax, BufferBilinearUVMinMaxValue);
		}
	}

	static const char* GetSourceFilename()
	{
		return ("PostProcessDownsample.dusf");
	}

	static const char* GetFunctionName()
	{
		return ("MainPS");
	}
};

// #define avoids a lot of code duplication
#define VARIATION1(A,B) typedef FPostProcessDownsamplePS<A, B> FPostProcessDownsamplePS##A##B; \
	IMPLEMENT_SHADER_TYPE2(FPostProcessDownsamplePS##A##B, SF_Pixel);

VARIATION1(0, 0) VARIATION1(1, 0) VARIATION1(2, 0)
VARIATION1(0, 1) VARIATION1(1, 1) VARIATION1(2, 1)
#undef VARIATION1

/** Encapsulates the post processing down sample vertex shader. */
class FPostProcessDownsampleVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessDownsampleVS, Global);
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	/** Default constructor. */
	FPostProcessDownsampleVS() {}

	/** Initialization constructor. */
	FPostProcessDownsampleVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
	}

	void SetParameters(const FRenderingCompositePassContext& Context)
	{
		ID3D11VertexShader* const ShaderRHI = GetVertexShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(ShaderRHI, Context.View.ViewUniformBuffer.get());

		const PooledRenderTargetDesc* InputDesc = Context.Pass->GetInputDesc(ePId_Input0);

		if (!InputDesc)
		{
			// input is not hooked up correctly
			return;
		}
	}
};

IMPLEMENT_SHADER_TYPE(, FPostProcessDownsampleVS, ("PostProcessDownsample.dusf"), ("MainDownsampleVS"), SF_Vertex);

FRCPassPostProcessDownsample::FRCPassPostProcessDownsample(EPixelFormat InOverrideFormat /*= PF_Unknown*/, uint32 InQuality /*= 1*/, bool bInIsComputePass /*= false*/, const char *InDebugName /*= ("Downsample")*/)
	: OverrideFormat(InOverrideFormat)
	, Quality(InQuality)
	, DebugName(InDebugName)
{

}

void FRCPassPostProcessDownsample::Process(FRenderingCompositePassContext& Context)
{
	const PooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	if (!InputDesc)
	{
		return;
	}

	const FViewInfo& View = Context.View;
	const FSceneViewFamily& ViewFamily = *(View.Family);

	FIntPoint SrcSize = InputDesc->Extent;
	FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;

	// e.g. 4 means the input texture is 4x smaller than the buffer size
	uint32 ScaleFactor = FMath::RoundUpToPowerOfTwo(FMath::DivideAndRoundUp(Context.ReferenceBufferSize.Y, SrcSize.Y));

	FIntRect SrcRect = FIntRect::DivideAndRoundUp(Context.SceneColorViewRect, ScaleFactor);
	FIntRect DestRect = FIntRect::DivideAndRoundUp(Context.SceneColorViewRect, ScaleFactor * 2);

	SCOPED_DRAW_EVENT_FORMAT(Downsample, TEXT("Downsample%s %dx%d"), bIsComputePass ? TEXT("Compute") : TEXT(""), DestRect.Width(), DestRect.Height());

	const PooledRenderTarget& DestRenderTarget = PassOutputs[0].RequestSurface(Context);
	const bool bIsDepthInputAvailable = (GetInputDesc(ePId_Input1) != nullptr);

	bool bManuallyClampUV = !(SrcRect.Min == FIntPoint::ZeroValue && SrcRect.Max == SrcSize);

	{
		// check if we have to clear the whole surface.
		// Otherwise perform the clear when the dest rectangle has been computed.
// 		auto FeatureLevel = Context.View.GetFeatureLevel();
// 		if (FeatureLevel == ERHIFeatureLevel::ES2 || FeatureLevel == ERHIFeatureLevel::ES3_1)
// 		{
// 			// Set the view family's render target/viewport.
// 			SetRenderTarget(Context.RHICmdList, DestRenderTarget.TargetableTexture, FTextureRHIRef(), ESimpleRenderTargetMode::EClearColorAndDepth);
// 			Context.SetViewportAndCallRHI(0, 0, 0.0f, DestSize.X, DestSize.Y, 1.0f);
// 		}
// 		else
		{
			// Set the view family's render target/viewport.
			//FRHIRenderTargetView RtView = FRHIRenderTargetView(DestRenderTarget.TargetableTexture, ERenderTargetLoadAction::ENoAction);
			//FRHISetRenderTargetsInfo Info(1, &RtView, FRHIDepthRenderTargetView());
			//Context.RHICmdList.SetRenderTargetsAndClear(Info);
			//Context.SetViewportAndCallRHI(0, 0, 0.0f, DestSize.X, DestSize.Y, 1.0f);
			SetRenderTarget(DestRenderTarget.TargetableTexture.get(), nullptr, false, false, false);
			RHISetViewport(0, 0, 0.0f, DestSize.X, DestSize.Y, 1.0f);
		}

		// InflateSize increases the size of the source/dest rectangle to compensate for bilinear reads and UIBlur pass requirements.
		int32 InflateSize;
		// if second input is hooked up

		if (bManuallyClampUV)
		{
			if (bIsDepthInputAvailable)
			{
				// also put depth in alpha
				InflateSize = 2;
				SetShader<2, true>(Context, InputDesc, SrcSize, SrcRect);
			}
			else if (Quality == 0)
			{
				SetShader<0, true>(Context, InputDesc, SrcSize, SrcRect);
				InflateSize = 1;
			}
			else
			{
				SetShader<1, true>(Context, InputDesc, SrcSize, SrcRect);
				InflateSize = 2;
			}
		}
		else
		{
			if (bIsDepthInputAvailable)
			{
				// also put depth in alpha
				InflateSize = 2;
				SetShader<2, false>(Context, InputDesc, SrcSize, SrcRect);
			}
			else if (Quality == 0)
			{
				SetShader<0, false>(Context, InputDesc, SrcSize, SrcRect);
				InflateSize = 1;
			}
			else
			{
				SetShader<1, false>(Context, InputDesc, SrcSize, SrcRect);
				InflateSize = 2;
			}
		}

		TShaderMapRef<FPostProcessDownsampleVS> VertexShader(Context.GetShaderMap());

		SrcRect = DestRect * 2;
		DrawRectangle(
			DestRect.Min.X, DestRect.Min.Y,
			DestRect.Width(), DestRect.Height(),
			SrcRect.Min.X, SrcRect.Min.Y,
			SrcRect.Width(), SrcRect.Height(),
			DestSize,
			SrcSize,
			*VertexShader//,
			/*EDRF_UseTriangleOptimization*/);

		RHICopyToResolveTarget(DestRenderTarget.TargetableTexture.get(), DestRenderTarget.ShaderResourceTexture.get(), FResolveParams());
	}
}

PooledRenderTargetDesc FRCPassPostProcessDownsample::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	PooledRenderTargetDesc Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;

	Ret.Reset();

	Ret.Extent = FIntPoint::DivideAndRoundUp(Ret.Extent, 2);

	Ret.Extent.X = FMath::Max(1, Ret.Extent.X);
	Ret.Extent.Y = FMath::Max(1, Ret.Extent.Y);

	if (OverrideFormat != PF_Unknown)
	{
		Ret.Format = OverrideFormat;
	}

	Ret.TargetableFlags &= ~(TexCreate_RenderTargetable | TexCreate_UAV);
	Ret.TargetableFlags |= bIsComputePass ? TexCreate_UAV : TexCreate_RenderTargetable;
	//Ret.Flags |= GFastVRamConfig.Downsample;
	Ret.AutoWritable = false;
	//Ret.DebugName = DebugName;

	Ret.ClearValue = FClearValueBinding(FLinearColor(0, 0, 0, 0));

	return Ret;
}

template <uint32 Method, uint32 ManuallyClampUV>
void FRCPassPostProcessDownsample::SetShader(const FRenderingCompositePassContext& Context, const PooledRenderTargetDesc* InputDesc, const FIntPoint& SrcSize, const FIntRect& SrcRect)
{
	//FGraphicsPipelineStateInitializer GraphicsPSOInit;
	//Context.RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
// 	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
// 	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
// 	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
// 	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	ID3D11BlendState* BlendState = TStaticBlendState<>::GetRHI();
	ID3D11RasterizerState* RasterizerState = TStaticRasterizerState<>::GetRHI();
	ID3D11DepthStencilState* DepthStencilState = TStaticDepthStencilState<false, D3D11_COMPARISON_ALWAYS>::GetRHI();
	D3D11DeviceContext->OMSetBlendState(BlendState, NULL, 0xffffffff);
	D3D11DeviceContext->RSSetState(RasterizerState);
	D3D11DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);

	auto ShaderMap = Context.GetShaderMap();
	TShaderMapRef<FPostProcessDownsampleVS> VertexShader(ShaderMap);
	TShaderMapRef<FPostProcessDownsamplePS<Method, ManuallyClampUV> > PixelShader(ShaderMap);

// 	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
// 	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
// 	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
// 	SetGraphicsPipelineState(Context.RHICmdList, GraphicsPSOInit);

	D3D11DeviceContext->IASetInputLayout(GetInputLayout(GetFilterInputDelcaration().get(), VertexShader->GetCode().Get()));
	D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
	D3D11DeviceContext->PSSetShader(PixelShader->GetPixelShader(), 0, 0);

	PixelShader->SetParameters(Context, InputDesc, SrcSize, SrcRect);
	VertexShader->SetParameters(Context);
}
