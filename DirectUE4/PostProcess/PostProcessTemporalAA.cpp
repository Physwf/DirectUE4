#include "PostProcessTemporalAA.h"
#include "DeferredShading.h"
#include "SceneRenderTargetParameters.h"
#include "SceneFilterRendering.h"
#include "PostProcessTonemap.h"
#include "PostProcessing.h"
#include "GPUProfiler.h"

static float CatmullRom(float x)
{
	float ax = FMath::Abs(x);
	if (ax > 1.0f)
		return ((-0.5f * ax + 2.5f) * ax - 4.0f) *ax + 2.0f;
	else
		return (1.5f * ax - 2.5f) * ax*ax + 1.0f;
}


struct FTemporalAAParameters
{
public:
	FPostProcessPassParameters PostprocessParameter;
	FSceneTextureShaderParameters SceneTextureParameters;
	FShaderParameter SampleWeights;
	FShaderParameter PlusWeights;
	FShaderParameter DitherScale;
	FShaderParameter VelocityScaling;
	FShaderParameter CurrentFrameWeight;
	FShaderParameter ScreenPosAbsMax;
	FShaderParameter ScreenPosToHistoryBufferUV;
	FShaderResourceParameter HistoryBuffer[FTemporalAAHistory::kRenderTargetCount];
	FShaderResourceParameter HistoryBufferSampler[FTemporalAAHistory::kRenderTargetCount];
	FShaderParameter HistoryBufferSize;
	FShaderParameter HistoryBufferUVMinMax;
	FShaderParameter MaxViewportUVAndSvPositionToViewportUV;
	FShaderParameter OneOverHistoryPreExposure;
	FShaderParameter ViewportUVToInputBufferUV;

	void Bind(const FShader::CompiledShaderInitializerType& Initializer)
	{
		const FShaderParameterMap& ParameterMap = Initializer.ParameterMap;
		PostprocessParameter.Bind(ParameterMap);
		SceneTextureParameters.Bind(Initializer);
		SampleWeights.Bind(ParameterMap, ("SampleWeights"));
		PlusWeights.Bind(ParameterMap, ("PlusWeights"));
		DitherScale.Bind(ParameterMap, ("DitherScale"));
		VelocityScaling.Bind(ParameterMap, ("VelocityScaling"));
		CurrentFrameWeight.Bind(ParameterMap, ("CurrentFrameWeight"));
		ScreenPosAbsMax.Bind(ParameterMap, ("ScreenPosAbsMax"));
		ScreenPosToHistoryBufferUV.Bind(ParameterMap, ("ScreenPosToHistoryBufferUV"));
		HistoryBuffer[0].Bind(ParameterMap, ("HistoryBuffer0"));
		HistoryBuffer[1].Bind(ParameterMap, ("HistoryBuffer1"));
		HistoryBufferSampler[0].Bind(ParameterMap, ("HistoryBuffer0Sampler"));
		HistoryBufferSampler[1].Bind(ParameterMap, ("HistoryBuffer1Sampler"));
		HistoryBufferSize.Bind(ParameterMap, ("HistoryBufferSize"));
		HistoryBufferUVMinMax.Bind(ParameterMap, ("HistoryBufferUVMinMax"));
		MaxViewportUVAndSvPositionToViewportUV.Bind(ParameterMap, ("MaxViewportUVAndSvPositionToViewportUV"));
		OneOverHistoryPreExposure.Bind(ParameterMap, ("OneOverHistoryPreExposure"));
		ViewportUVToInputBufferUV.Bind(ParameterMap, ("ViewportUVToInputBufferUV"));
	}

	template <typename TShaderRHIParamRef>
	void SetParameters(
		const TShaderRHIParamRef ShaderRHI,
		const FRenderingCompositePassContext& Context,
		const FTemporalAAHistory& InputHistory,
		const FTAAPassParameters& PassParameters,
		bool bUseDither,
		const FIntPoint& SrcSize)
	{
		SceneTextureParameters.Set(ShaderRHI, ESceneTextureSetupMode::All);

		float ResDivisor = PassParameters.ResolutionDivisor;
		float ResDivisorInv = 1.0f / ResDivisor;

		// PS params
		{
			float JitterX = Context.View.TemporalJitterPixels.X;
			float JitterY = Context.View.TemporalJitterPixels.Y;

			static const float SampleOffsets[9][2] =
			{
				{ -1.0f, -1.0f },
			{ 0.0f, -1.0f },
			{ 1.0f, -1.0f },
			{ -1.0f,  0.0f },
			{ 0.0f,  0.0f },
			{ 1.0f,  0.0f },
			{ -1.0f,  1.0f },
			{ 0.0f,  1.0f },
			{ 1.0f,  1.0f },
			};

			float FilterSize = 1.0f;// CVarTemporalAAFilterSize.GetValueOnRenderThread();
			int32 bCatmullRom = 0;// CVarTemporalAACatmullRom.GetValueOnRenderThread();

			float Weights[9];
			float WeightsPlus[5];
			float TotalWeight = 0.0f;
			float TotalWeightLow = 0.0f;
			float TotalWeightPlus = 0.0f;
			for (int32 i = 0; i < 9; i++)
			{
				float PixelOffsetX = SampleOffsets[i][0] - JitterX * ResDivisorInv;
				float PixelOffsetY = SampleOffsets[i][1] - JitterY * ResDivisorInv;

				PixelOffsetX /= FilterSize;
				PixelOffsetY /= FilterSize;

				if (bCatmullRom)
				{
					Weights[i] = CatmullRom(PixelOffsetX) * CatmullRom(PixelOffsetY);
					TotalWeight += Weights[i];
				}
				else
				{
					// Normal distribution, Sigma = 0.47
					Weights[i] = FMath::Exp(-2.29f * (PixelOffsetX * PixelOffsetX + PixelOffsetY * PixelOffsetY));
					TotalWeight += Weights[i];
				}
			}

			WeightsPlus[0] = Weights[1];
			WeightsPlus[1] = Weights[3];
			WeightsPlus[2] = Weights[4];
			WeightsPlus[3] = Weights[5];
			WeightsPlus[4] = Weights[7];
			TotalWeightPlus = Weights[1] + Weights[3] + Weights[4] + Weights[5] + Weights[7];

			for (int32 i = 0; i < 9; i++)
			{
				SetShaderValue(ShaderRHI, SampleWeights, Weights[i] / TotalWeight, i);
			}

			for (int32 i = 0; i < 5; i++)
			{
				SetShaderValue(ShaderRHI, PlusWeights, WeightsPlus[i] / TotalWeightPlus, i);
			}
		}

		SetShaderValue(ShaderRHI, DitherScale, bUseDither ? 1.0f : 0.0f);

		const bool bIgnoreVelocity = (Context.View.ViewState && Context.View.ViewState->bSequencerIsPaused);
		SetShaderValue(ShaderRHI, VelocityScaling, bIgnoreVelocity ? 0.0f : 1.0f);

		SetShaderValue(ShaderRHI, CurrentFrameWeight,0.04f/* CVarTemporalAACurrentFrameWeight.GetValueOnRenderThread()*/);

		// Set history shader parameters.
		{
			FIntPoint ReferenceViewportOffset = InputHistory.ViewportRect.Min;
			FIntPoint ReferenceViewportExtent = InputHistory.ViewportRect.Size();
			FIntPoint ReferenceBufferSize = InputHistory.ReferenceBufferSize;

			float InvReferenceBufferSizeX = 1.f / float(InputHistory.ReferenceBufferSize.X);
			float InvReferenceBufferSizeY = 1.f / float(InputHistory.ReferenceBufferSize.Y);

			Vector4 ScreenPosToPixelValue(
				ReferenceViewportExtent.X * 0.5f * InvReferenceBufferSizeX,
				-ReferenceViewportExtent.Y * 0.5f * InvReferenceBufferSizeY,
				(ReferenceViewportExtent.X * 0.5f + ReferenceViewportOffset.X) * InvReferenceBufferSizeX,
				(ReferenceViewportExtent.Y * 0.5f + ReferenceViewportOffset.Y) * InvReferenceBufferSizeY);
			SetShaderValue(ShaderRHI, ScreenPosToHistoryBufferUV, ScreenPosToPixelValue);

			FIntPoint ViewportOffset = ReferenceViewportOffset / PassParameters.ResolutionDivisor;
			FIntPoint ViewportExtent = FIntPoint::DivideAndRoundUp(ReferenceViewportExtent, PassParameters.ResolutionDivisor);
			FIntPoint BufferSize = ReferenceBufferSize / PassParameters.ResolutionDivisor;

			Vector2 ScreenPosAbsMaxValue(1.0f - 1.0f / float(ViewportExtent.X), 1.0f - 1.0f / float(ViewportExtent.Y));
			SetShaderValue(ShaderRHI, ScreenPosAbsMax, ScreenPosAbsMaxValue);

			float InvBufferSizeX = 1.f / float(BufferSize.X);
			float InvBufferSizeY = 1.f / float(BufferSize.Y);

			Vector4 HistoryBufferUVMinMaxValue(
				(ViewportOffset.X + 0.5f) * InvBufferSizeX,
				(ViewportOffset.Y + 0.5f) * InvBufferSizeY,
				(ViewportOffset.X + ViewportExtent.X - 0.5f) * InvBufferSizeX,
				(ViewportOffset.Y + ViewportExtent.Y - 0.5f) * InvBufferSizeY);

			SetShaderValue(ShaderRHI, HistoryBufferUVMinMax, HistoryBufferUVMinMaxValue);

			Vector4 HistoryBufferSizeValue(BufferSize.X, BufferSize.Y, InvBufferSizeX, InvBufferSizeY);
			SetShaderValue(ShaderRHI, HistoryBufferSize, HistoryBufferSizeValue);

			for (uint32 i = 0; i < FTemporalAAHistory::kRenderTargetCount; i++)
			{
				if (InputHistory.RT[i].Get()!=NULL)
				{
					SetTextureParameter(
						ShaderRHI,
						HistoryBuffer[i], HistoryBufferSampler[i],
						TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT>::GetRHI(),
						InputHistory.RT[i]->ShaderResourceTexture->GetShaderResourceView());
				}
			}
		}

		{
			Vector4 MaxViewportUVAndSvPositionToViewportUVValue(
				(PassParameters.OutputViewRect.Width() - 0.5f * ResDivisor) / float(PassParameters.OutputViewRect.Width()),
				(PassParameters.OutputViewRect.Height() - 0.5f * ResDivisor) / float(PassParameters.OutputViewRect.Height()),
				ResDivisor / float(PassParameters.OutputViewRect.Width()),
				ResDivisor / float(PassParameters.OutputViewRect.Height()));

			SetShaderValue(ShaderRHI, MaxViewportUVAndSvPositionToViewportUV, MaxViewportUVAndSvPositionToViewportUVValue);
		}

		const float OneOverHistoryPreExposureValue = 1.f / FMath::Max<float>(SMALL_NUMBER, InputHistory.IsValid() ? InputHistory.SceneColorPreExposure : Context.View.PreExposure);
		SetShaderValue(ShaderRHI, OneOverHistoryPreExposure, OneOverHistoryPreExposureValue);

		{
			float InvSizeX = 1.0f / float(SrcSize.X);
			float InvSizeY = 1.0f / float(SrcSize.Y);
			Vector4 ViewportUVToBufferUVValue(
				ResDivisorInv * PassParameters.InputViewRect.Width() * InvSizeX,
				ResDivisorInv * PassParameters.InputViewRect.Height() * InvSizeY,
				ResDivisorInv * PassParameters.InputViewRect.Min.X * InvSizeX,
				ResDivisorInv * PassParameters.InputViewRect.Min.Y * InvSizeY);

			SetShaderValue(ShaderRHI, ViewportUVToInputBufferUV, ViewportUVToBufferUVValue);
		}

	}
};

namespace
{

	class FTAAPassConfigDim : SHADER_PERMUTATION_ENUM_CLASS("TAA_PASS_CONFIG", ETAAPassConfig);
	class FTAAFastDim : SHADER_PERMUTATION_BOOL("TAA_FAST");
	class FTAAResponsiveDim : SHADER_PERMUTATION_BOOL("TAA_RESPONSIVE");
	class FTAACameraCutDim : SHADER_PERMUTATION_BOOL("TAA_CAMERA_CUT");
	class FTAAScreenPercentageDim : SHADER_PERMUTATION_INT("TAA_SCREEN_PERCENTAGE_RANGE", 3);
	class FTAAUpsampleFilteredDim : SHADER_PERMUTATION_BOOL("TAA_UPSAMPLE_FILTERED");
	class FTAADownsampleDim : SHADER_PERMUTATION_BOOL("TAA_DOWNSAMPLE");

}

class FPostProcessTemporalAAPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FPostProcessTemporalAAPS);

	using FPermutationDomain = TShaderPermutationDomain<
		FTAAPassConfigDim,
		FTAAFastDim,
		FTAAResponsiveDim,
		FTAACameraCutDim>;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		FPermutationDomain PermutationVector(Parameters.PermutationId);

		// TAAU is compute shader only.
		if (IsTAAUpsamplingConfig(PermutationVector.Get<FTAAPassConfigDim>()))
		{
			return false;
		}

		// Fast dimensions is only for Main and Diaphragm DOF.
		if (PermutationVector.Get<FTAAFastDim>() &&
			!IsMainTAAConfig(PermutationVector.Get<FTAAPassConfigDim>()) &&
			!IsDOFTAAConfig(PermutationVector.Get<FTAAPassConfigDim>()))
		{
			return false;
		}

		// Responsive dimension is only for Main.
		if (PermutationVector.Get<FTAAResponsiveDim>() && !SupportsResponsiveDim(PermutationVector))
		{
			return false;
		}

		return true;// IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
	}

	static bool SupportsResponsiveDim(const FPermutationDomain& PermutationVector)
	{
		return PermutationVector.Get<FTAAPassConfigDim>() == ETAAPassConfig::Main;
	}

	/** Default constructor. */
	FPostProcessTemporalAAPS() {}

	/** Initialization constructor. */
	FPostProcessTemporalAAPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		Parameter.Bind(Initializer);
	}

	void SetParameters(
		const FRenderingCompositePassContext& Context,
		const FTemporalAAHistory& InputHistory,
		const FTAAPassParameters& PassParameters,
		bool bUseDither,
		const FIntPoint& SrcSize)
	{
		ID3D11PixelShader* const ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(ShaderRHI, Context.View.ViewUniformBuffer.get());

		Parameter.PostprocessParameter.SetPS(ShaderRHI, Context);

		Parameter.SetParameters(ShaderRHI, Context, InputHistory, PassParameters, bUseDither, SrcSize);
	}

	FTemporalAAParameters Parameter;
};

IMPLEMENT_GLOBAL_SHADER(FPostProcessTemporalAAPS, "PostProcessTemporalAA.dusf", "MainPS", SF_Pixel);

void DrawPixelPassTemplate(
	FRenderingCompositePassContext& Context,
	const FPostProcessTemporalAAPS::FPermutationDomain& PermutationVector,
	const FIntPoint SrcSize,
	const FIntRect ViewRect,
	const FTemporalAAHistory& InputHistory,
	const FTAAPassParameters& PassParameters,
	const bool bUseDither,
	ID3D11DepthStencilState* DepthStencilState)
{
	//FGraphicsPipelineStateInitializer GraphicsPSOInit;
	//Context.RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	ID3D11BlendState* BlendState = TStaticBlendState<>::GetRHI();
	ID3D11RasterizerState* RasterizerState = TStaticRasterizerState<>::GetRHI();

	TShaderMapRef< FPostProcessTonemapVS > VertexShader(Context.GetShaderMap());
	TShaderMapRef< FPostProcessTemporalAAPS > PixelShader(Context.GetShaderMap(), PermutationVector);

// 	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
// 	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
// 	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
// 	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	D3D11DeviceContext->IASetInputLayout(GetInputLayout(GetFilterInputDelcaration().get(), VertexShader->GetCode().Get()));
	D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
	D3D11DeviceContext->PSSetShader(PixelShader->GetPixelShader(), 0, 0);
	D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//SetGraphicsPipelineState(Context.RHICmdList, GraphicsPSOInit);
	VertexShader->SetVS(Context);
	PixelShader->SetParameters(Context, InputHistory, PassParameters, bUseDither, SrcSize);

	DrawRectangle(
		0, 0,
		ViewRect.Width(), ViewRect.Height(),
		ViewRect.Min.X, ViewRect.Min.Y,
		ViewRect.Width(), ViewRect.Height(),
		ViewRect.Size(),
		SrcSize,
		*VertexShader);
}

const wchar_t* const kTAAOutputNames[] = {
	TEXT("DOFTemporalAA"),
	TEXT("TemporalAA"),
	TEXT("SSRTemporalAA"),
	TEXT("LightShaftTemporalAA"),
	TEXT("TemporalAA"),
	TEXT("DOFTemporalAA"),
	TEXT("DOFTemporalAA"),
};

const wchar_t* const kTAAPassNames[] = {
	TEXT("LegacyDOF"),
	TEXT("Main"),
	TEXT("ScreenSpaceReflections"),
	TEXT("LightShaft"),
	TEXT("MainUpsampling"),
	TEXT("DiaphragmDOF"),
	TEXT("DiaphragmDOFUpsampling"),
};


//static_assert(ARRAY_COUNT(kTAAOutputNames) == int32(ETAAPassConfig::MAX), "Missing TAA output name.");
//static_assert(ARRAY_COUNT(kTAAPassNames) == int32(ETAAPassConfig::MAX), "Missing TAA pass name.");
static void TransitionShaderResources(FRenderingCompositePassContext& Context)
{
	TShaderMapRef< FPostProcessTonemapVS > VertexShader(Context.GetShaderMap());
	VertexShader->TransitionResources(Context);
}

FRCPassPostProcessTemporalAA::FRCPassPostProcessTemporalAA(
	const class FPostprocessContext& Context, 
	const FTAAPassParameters& InParameters,
	const FTemporalAAHistory& InInputHistory, 
	FTemporalAAHistory* OutOutputHistory)
	: Parameters(InParameters)
	, OutputExtent(0, 0)
	, InputHistory(InInputHistory)
	, OutputHistory(OutOutputHistory)
{
	bIsComputePass = Parameters.bIsComputePass;
	bPreferAsyncCompute = false;
}

void FRCPassPostProcessTemporalAA::Process(FRenderingCompositePassContext& Context)
{
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();

	const PooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);
	FIntPoint SrcSize = InputDesc->Extent;

	// Number of render target in TAA history.
	const int32 RenderTargetCount = IsDOFTAAConfig(Parameters.Pass) && false/*FPostProcessing::HasAlphaChannelSupport()*/ ? 2 : 1;

	const PooledRenderTarget* DestRenderTarget[2] = { nullptr, nullptr };
	DestRenderTarget[0] = &PassOutputs[0].RequestSurface(Context);
	if (RenderTargetCount == 2)
		DestRenderTarget[1] = &PassOutputs[1].RequestSurface(Context);

	//const PooledRenderTarget& DestDownsampled = /*bDownsamplePossible ? */PassOutputs[2].RequestSurface(Context) /*: PooledRenderTarget()*/;

	// Whether this is main TAA pass;
	bool bIsMainPass = Parameters.Pass == ETAAPassConfig::Main || Parameters.Pass == ETAAPassConfig::MainUpsampling;

	// Whether to use camera cut shader permutation or not.
	const bool bCameraCut = !InputHistory.IsValid() || Context.View.bCameraCut;

	// Whether to use responsive stencil test.
	bool bUseResponsiveStencilTest = Parameters.Pass == ETAAPassConfig::Main && !bIsComputePass && !bCameraCut;

	// Only use dithering if we are outputting to a low precision format
	const bool bUseDither = PassOutputs[0].RenderTargetDesc.Format != PF_FloatRGBA && bIsMainPass;

	// Src rectangle.
	FIntRect SrcRect = Parameters.InputViewRect;

	// Dest rectangle is same as source rectangle, unless Upsampling.
	FIntRect DestRect = Parameters.OutputViewRect;
	assert(IsTAAUpsamplingConfig(Parameters.Pass) || SrcRect == DestRect);

	// Name of the pass.
	const TCHAR* PassName = kTAAPassNames[static_cast<int32>(Parameters.Pass)];

	{
		assert(!IsTAAUpsamplingConfig(Parameters.Pass));

		FIntRect ViewRect = FIntRect::DivideAndRoundUp(DestRect, Parameters.ResolutionDivisor);
		FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;

		SCOPED_DRAW_EVENT_FORMAT(TemporalAA, TEXT("TAA %s PS%s %dx%d"),
			PassName, Parameters.bUseFast ? TEXT(" Fast") : TEXT(""), ViewRect.Width(), ViewRect.Height());

		//WaitForInputPassComputeFences(Context.RHICmdList);

		// Inform MultiGPU systems that we're starting to update this resource
		//Context.RHICmdList.BeginUpdateMultiFrameResource(DestRenderTarget[0]->ShaderResourceTexture);

		TransitionShaderResources(Context);

		// Setup render targets.
		{
// 			FRHIDepthRenderTargetView DepthRTV(SceneContext.GetSceneDepthTexture(),
// 				ERenderTargetLoadAction::ENoAction, ERenderTargetStoreAction::ENoAction,
// 				ERenderTargetLoadAction::ELoad, ERenderTargetStoreAction::ENoAction,
// 				FExclusiveDepthStencil::DepthRead_StencilWrite);

			FD3D11Texture2D* RTVs[2];

			RTVs[0] = DestRenderTarget[0]->TargetableTexture.get();

			// Inform MultiGPU systems that we're starting to update this resource
			//Context.RHICmdList.BeginUpdateMultiFrameResource(DestRenderTarget[0]->ShaderResourceTexture);

			if (RenderTargetCount == 2)
			{
				RTVs[1] = DestRenderTarget[1]->TargetableTexture.get();
				//Context.RHICmdList.BeginUpdateMultiFrameResource(DestRenderTarget[1]->ShaderResourceTexture);
			}
			SetRenderTargets(RenderTargetCount, RTVs, SceneContext.GetSceneDepthTexture().get(), false,false,true);
		}

		Context.SetViewportAndCallRHI(ViewRect);

		FPostProcessTemporalAAPS::FPermutationDomain PermutationVector;
		PermutationVector.Set<FTAAPassConfigDim>(Parameters.Pass);
		PermutationVector.Set<FTAAFastDim>(Parameters.bUseFast);
		PermutationVector.Set<FTAACameraCutDim>(bCameraCut);

		if (bUseResponsiveStencilTest)
		{
			// Normal temporal feedback
			// Draw to pixels where stencil == 0
			ID3D11DepthStencilState* DepthStencilState = TStaticDepthStencilState<
				false, D3D11_COMPARISON_ALWAYS,
				true, D3D11_COMPARISON_EQUAL, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP,
				false, D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP,
				STENCIL_TEMPORAL_RESPONSIVE_AA_MASK, STENCIL_TEMPORAL_RESPONSIVE_AA_MASK>::GetRHI();

			DrawPixelPassTemplate(
				Context, PermutationVector, SrcSize, ViewRect,
				InputHistory, Parameters, bUseDither,
				DepthStencilState);

			// Responsive feedback for tagged pixels
			// Draw to pixels where stencil != 0
			DepthStencilState = TStaticDepthStencilState<
				false, D3D11_COMPARISON_ALWAYS,
				true, D3D11_COMPARISON_NOT_EQUAL, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP,
				false, D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP,
				STENCIL_TEMPORAL_RESPONSIVE_AA_MASK, STENCIL_TEMPORAL_RESPONSIVE_AA_MASK>::GetRHI();

			PermutationVector.Set<FTAAResponsiveDim>(true);
			DrawPixelPassTemplate(
				Context, PermutationVector, SrcSize, ViewRect,
				InputHistory, Parameters, bUseDither,
				DepthStencilState);
		}
		else
		{
			DrawPixelPassTemplate(
				Context, PermutationVector, SrcSize, ViewRect,
				InputHistory, Parameters, bUseDither,
				TStaticDepthStencilState<false, D3D11_COMPARISON_ALWAYS>::GetRHI());
		}

		RHICopyToResolveTarget(DestRenderTarget[0]->TargetableTexture.get(), DestRenderTarget[0]->ShaderResourceTexture.get(), FResolveParams());

		if (RenderTargetCount == 2)
		{
			RHICopyToResolveTarget(DestRenderTarget[1]->TargetableTexture.get(), DestRenderTarget[1]->ShaderResourceTexture.get(), FResolveParams());
			//RHICmdList.EndUpdateMultiFrameResource(DestRenderTarget[1]->ShaderResourceTexture);
		}

		if (IsDOFTAAConfig(Parameters.Pass))
		{
			//Context.RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EGfxToCompute, DestRenderTarget[0]->UAV);

			if (RenderTargetCount == 2)
			{
				//Context.RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EGfxToCompute, DestRenderTarget[1]->UAV);
			}
		}

		// Inform MultiGPU systems that we've finished with this texture for this frame
		//Context.RHICmdList.EndUpdateMultiFrameResource(DestRenderTarget[0]->ShaderResourceTexture);
	}

	OutputHistory->SafeRelease();
	OutputHistory->RT[0] = PassOutputs[0].RenderTarget;
	OutputHistory->ViewportRect = DestRect;
	OutputHistory->ReferenceBufferSize = FSceneRenderTargets::Get().GetBufferSizeXY();
	OutputHistory->SceneColorPreExposure = Context.View.PreExposure;

	if (OutputExtent.X > 0)
	{
		OutputHistory->ReferenceBufferSize = OutputExtent;
	}

	// Changes the view rectangle of the scene color and reference buffer size when doing temporal upsample for the
	// following passes to still work.
	if (Parameters.Pass == ETAAPassConfig::MainUpsampling)
	{
		Context.SceneColorViewRect = DestRect;
		Context.ReferenceBufferSize = OutputExtent;
	}
}

PooledRenderTargetDesc FRCPassPostProcessTemporalAA::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	PooledRenderTargetDesc Ret;

	switch (InPassOutputId)
	{
	case ePId_Output0: // main color output
	case ePId_Output1:

		Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;
		Ret.Flags &= ~(TexCreate_FastVRAM | TexCreate_Transient);
		Ret.Reset();
		//regardless of input type, PF_FloatRGBA is required to properly accumulate between frames for a good result.
		Ret.Format = PF_FloatRGBA;
		//Ret.DebugName = kTAAOutputNames[static_cast<int32>(Parameters.Pass)];
		Ret.AutoWritable = false;
		Ret.TargetableFlags &= ~(TexCreate_RenderTargetable | TexCreate_UAV);
		Ret.TargetableFlags |= bIsComputePass ? TexCreate_UAV : TexCreate_RenderTargetable;

		if (OutputExtent.X > 0)
		{
			assert(OutputExtent.X % Parameters.ResolutionDivisor == 0);
			assert(OutputExtent.Y % Parameters.ResolutionDivisor == 0);
			Ret.Extent = OutputExtent / Parameters.ResolutionDivisor;
		}

		// Need a UAV to resource transition from gfx to compute.
		if (IsDOFTAAConfig(Parameters.Pass))
		{
			Ret.TargetableFlags |= TexCreate_UAV;
		}

		break;

	case ePId_Output2: // downsampled color output

		if (!bDownsamplePossible)
		{
			break;
		}

		assert(bIsComputePass);

		Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;
		Ret.Flags &= ~(TexCreate_FastVRAM);
		Ret.Reset();

		if (Parameters.DownsampleOverrideFormat != PF_Unknown)
		{
			Ret.Format = Parameters.DownsampleOverrideFormat;
		}

		//Ret.DebugName = TEXT("SceneColorHalfRes");
		Ret.AutoWritable = false;
		Ret.TargetableFlags &= ~TexCreate_RenderTargetable;
		Ret.TargetableFlags |= TexCreate_UAV;

		if (OutputExtent.X > 0)
		{
			assert(OutputExtent.X % Parameters.ResolutionDivisor == 0);
			assert(OutputExtent.Y % Parameters.ResolutionDivisor == 0);
			Ret.Extent = OutputExtent / Parameters.ResolutionDivisor;
		}

		Ret.Extent = FIntPoint::DivideAndRoundUp(Ret.Extent, 2);
		Ret.Extent.X = FMath::Max(1, Ret.Extent.X);
		Ret.Extent.Y = FMath::Max(1, Ret.Extent.Y);

		break;

	default:
		assert(false);
	}

	return Ret;
}

