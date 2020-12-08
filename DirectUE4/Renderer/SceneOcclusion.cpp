#include "SceneOcclusion.h"
#include "SceneFilterRendering.h"
#include "RenderTargets.h"
#include "DeferredShading.h"
#include "GPUProfiler.h"
#include "GlobalShader.h"
#include "PostProcessing.h"
#include "SceneRenderTargetParameters.h"

#define MAXNumMips 10
ID3D11Texture2D* HZBRTVTexture;
ID3D11Texture2D* HZBSRVTexture;
ID3D11RenderTargetView* HZBRTVs[MAXNumMips];
ID3D11ShaderResourceView* HZBSRV;
ID3D11ShaderResourceView* HZBSRVs[MAXNumMips];
ID3D11SamplerState* HZBSamplers[MAXNumMips];
uint32 NumMips;
FIntPoint HZBSize;

ID3D11BlendState* BlendState;
ID3D11RasterizerState* RasterizerState;
ID3D11DepthStencilState* DepthStencilState;

static ID3D11ShaderResourceView* SceneDepthSRV;
static ID3D11SamplerState* SceneDepthSampler;

ID3DBlob* PSBytecode0;
ID3DBlob* PSBytecode1;
ID3D11PixelShader* PS0;
ID3D11PixelShader* PS1;
std::map<std::string, ParameterAllocation> PSParams0;
std::map<std::string, ParameterAllocation> PSParams1;

template< uint32 Stage >
class THZBBuildPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(THZBBuildPS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		//return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine("STAGE", Stage);
		//OutEnvironment.SetRenderTargetOutputFormat(0, PF_R32_FLOAT);
	}

	THZBBuildPS() {}

public:
	FShaderParameter				InvSizeParameter;
	FShaderParameter				InputUvFactorAndOffsetParameter;
	FShaderParameter				InputViewportMaxBoundParameter;
	FSceneTextureShaderParameters	SceneTextureParameters;
	FShaderResourceParameter		TextureParameter;
	FShaderResourceParameter		TextureParameterSampler;

	THZBBuildPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		InvSizeParameter.Bind(Initializer.ParameterMap, ("InvSize"));
		InputUvFactorAndOffsetParameter.Bind(Initializer.ParameterMap, ("InputUvFactorAndOffset"));
		InputViewportMaxBoundParameter.Bind(Initializer.ParameterMap, ("InputViewportMaxBound"));
		SceneTextureParameters.Bind(Initializer);
		TextureParameter.Bind(Initializer.ParameterMap, ("Texture"));
		TextureParameterSampler.Bind(Initializer.ParameterMap, ("TextureSampler"));
	}

	void SetParameters(const FViewInfo& View)
	{
		ID3D11PixelShader* const ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(ShaderRHI, View.ViewUniformBuffer.get());
		FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();

		const FIntPoint GBufferSize = SceneContext.GetBufferSizeXY();
		const Vector2 InvSize(1.0f / float(GBufferSize.X), 1.0f / float(GBufferSize.Y));
		const Vector4 InputUvFactorAndOffset(
			float(2 * View.HZBMipmap0Size.X) / float(GBufferSize.X),
			float(2 * View.HZBMipmap0Size.Y) / float(GBufferSize.Y),
			float(View.ViewRect.Min.X) / float(GBufferSize.X),
			float(View.ViewRect.Min.Y) / float(GBufferSize.Y)
		);
		const Vector2 InputViewportMaxBound(
			float(View.ViewRect.Max.X) / float(GBufferSize.X) - 0.5f * InvSize.X,
			float(View.ViewRect.Max.Y) / float(GBufferSize.Y) - 0.5f * InvSize.Y
		);
		SetShaderValue(ShaderRHI, InvSizeParameter, InvSize);
		SetShaderValue(ShaderRHI, InputUvFactorAndOffsetParameter, InputUvFactorAndOffset);
		SetShaderValue(ShaderRHI, InputViewportMaxBoundParameter, InputViewportMaxBound);

		SceneTextureParameters.Set(ShaderRHI, ESceneTextureSetupMode::SceneDepth);
	}

	void SetParameters(const FSceneView& View, const FIntPoint& Size, ID3D11ShaderResourceView* ShaderResourceView)
	{
		ID3D11PixelShader* const ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(ShaderRHI, View.ViewUniformBuffer.get());

		const Vector2 InvSize(1.0f / Size.X, 1.0f / Size.Y);
		SetShaderValue(ShaderRHI, InvSizeParameter, InvSize);

		//SetTextureParameter( ShaderRHI, TextureParameter, TextureParameterSampler, TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(), Texture );

		SetSRVParameter(ShaderRHI, TextureParameter, ShaderResourceView);
		SetSamplerParameter(ShaderRHI, TextureParameterSampler, TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI());
	}
};

IMPLEMENT_SHADER_TYPE(template<>, THZBBuildPS<0>, ("HZBOcclusion.dusf"), ("HZBBuildPS"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>, THZBBuildPS<1>, ("HZBOcclusion.dusf"), ("HZBBuildPS"), SF_Pixel);

void InitHZB()
{
	/*
	const int32 NumMipsX = Math::Max(Math::CeilToInt(Math::Log2(float(WindowWidth))) - 1, 1);
	const int32 NumMipsY = Math::Max(Math::CeilToInt(Math::Log2(float(WindowHeight))) - 1, 1);
	NumMips = Math::Max(NumMipsX, NumMipsY);

	HZBSize = IntPoint(1 << NumMipsX, 1 << NumMipsY);
	
	HZBRTVTexture = CreateTexture2D(HZBSize.X, HZBSize.Y, DXGI_FORMAT_R16_FLOAT, true, true, false, NumMips);
	HZBSRVTexture = CreateTexture2D(HZBSize.X, HZBSize.Y, DXGI_FORMAT_R16_FLOAT, true, true, false, NumMips);
	for (uint32 i = 0; i < NumMips; ++i)
	{
		HZBRTVs[i] = CreateRenderTargetView2D(HZBRTVTexture, DXGI_FORMAT_R16_FLOAT, i);
		HZBSRVs[i] = CreateShaderResourceView2D(HZBSRVTexture, DXGI_FORMAT_R16_FLOAT, NumMips, i);
	}
	HZBSRV = HZBSRVs[0];
	D3D_SHADER_MACRO Macro0 = { "STAGE","0" };
	D3D_SHADER_MACRO Macro1 = { "STAGE","1" };
	PSBytecode0 = CompilePixelShader(TEXT("./Shaders/HZBOcclusion.dusf"), "HZBBuildPS", &Macro0, 1);
	PSBytecode1 = CompilePixelShader(TEXT("./Shaders/HZBOcclusion.dusf"), "HZBBuildPS", &Macro1, 1);
	GetShaderParameterAllocations(PSBytecode0, PSParams0);
	GetShaderParameterAllocations(PSBytecode1, PSParams1);
	PS0 = CreatePixelShader(PSBytecode0);
	PS1 = CreatePixelShader(PSBytecode1);

	RenderTargets& SceneContext = RenderTargets::Get();
	SceneDepthSRV = CreateShaderResourceView2D(SceneContext.GetSceneDepthTexture(), DXGI_FORMAT_R24_UNORM_X8_TYPELESS, 1, 0);
	SceneDepthSampler = TStaticSamplerState<>::GetRHI();

	BlendState = TStaticBlendState<>::GetRHI();
	RasterizerState = TStaticRasterizerState<>::GetRHI();
	DepthStencilState = TStaticDepthStencilState<FALSE, D3D11_COMPARISON_ALWAYS>::GetRHI();
	*/
}

void BuildHZB(FViewInfo& View)
{
	const int32 NumMipsX = FMath::Max(FMath::CeilToInt(FMath::Log2(float(View.ViewRect.Width()))) - 1, 1);
	const int32 NumMipsY = FMath::Max(FMath::CeilToInt(FMath::Log2(float(View.ViewRect.Height()))) - 1, 1);
	const uint32 NumMips = FMath::Max(NumMipsX, NumMipsY);

	const FIntPoint HZBSize(1 << NumMipsX, 1 << NumMipsY);
	View.HZBMipmap0Size = HZBSize;
	
	PooledRenderTargetDesc Desc(PooledRenderTargetDesc::Create2DDesc(HZBSize, PF_R16F, FClearValueBinding::None, TexCreate_None, TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_NoFastClear, false, NumMips));
	//Desc.Flags |= GFastVRamConfig.HZB;
	GRenderTargetPool.FindFreeElement(Desc, View.HZB, TEXT("HZB"));

	PooledRenderTarget& HZBRenderTarget = *View.HZB.Get();

	D3D11DeviceContext->OMSetBlendState(TStaticBlendState<>::GetRHI(), NULL, 0xffffffff);
	D3D11DeviceContext->RSSetState(TStaticRasterizerState<>::GetRHI());
	D3D11DeviceContext->OMSetDepthStencilState(TStaticDepthStencilState<TRUE,D3D11_COMPARISON_ALWAYS>::GetRHI(),0);

	FD3D11Texture2D* HZBRenderTargetRef = HZBRenderTarget.TargetableTexture.get();
	//mip0
	{
		SCOPED_DRAW_EVENT_FORMAT(BuildHZB, TEXT("HZB SetupMip 0 %dx%d"), HZBSize.X, HZBSize.Y);

		SetRenderTarget(HZBRenderTargetRef, NULL, 0);

		TShaderMapRef<FPostProcessVS> VertexShader(View.ShaderMap);
		TShaderMapRef<THZBBuildPS<0>> PixelShader(View.ShaderMap);

		ID3D11InputLayout* InputLayout = GetInputLayout(GetFilterInputDelcaration().get(), VertexShader->GetCode().Get());
		D3D11DeviceContext->IASetInputLayout(InputLayout);
		D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
		D3D11DeviceContext->PSSetShader(PixelShader->GetPixelShader(), 0, 0);
		D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		PixelShader->SetParameters(View);

		D3D11_VIEWPORT VP = { 0, 0, (float)HZBSize.X, (float)HZBSize.Y, 0, 1.f };
		D3D11DeviceContext->RSSetViewports(1, &VP);

		DrawRectangle(
			0, 0, 
			HZBSize.X, HZBSize.Y, 
			View.ViewRect.Min.X, View.ViewRect.Min.Y, 
			View.ViewRect.Width(), View.ViewRect.Height(), 
			HZBSize, FSceneRenderTargets::Get().GetBufferSizeXY(),
			*VertexShader,1
			);

		RHICopyToResolveTarget(View.HZB->TargetableTexture.get(), View.HZB->ShaderResourceTexture.get(),FResolveParams());
	}

	FIntPoint SrcSize = HZBSize;
	FIntPoint DstSize = SrcSize / 2;

	SCOPED_DRAW_EVENT_FORMAT(BuildHZB, TEXT("HZB SetupMips Mips:1..%d %dx%d"), NumMips - 1, DstSize.X, DstSize.Y);

	TShaderMapRef< FPostProcessVS >	VertexShader(View.ShaderMap);
	TShaderMapRef< THZBBuildPS<1> >	PixelShader(View.ShaderMap);

	// Downsampling...
	for (uint8 MipIndex = 1; MipIndex < NumMips; MipIndex++)
	{
		DstSize.X = FMath::Max(DstSize.X, 1);
		DstSize.Y = FMath::Max(DstSize.Y, 1);

		SetRenderTarget(View.HZB->TargetableTexture.get(), NULL, false, false, false, MipIndex);

		ID3D11InputLayout* InputLayout = GetInputLayout(GetFilterInputDelcaration().get(), VertexShader->GetCode().Get());
		D3D11DeviceContext->IASetInputLayout(InputLayout);
		D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
		D3D11DeviceContext->PSSetShader(PixelShader->GetPixelShader(), 0, 0);
		D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


		PixelShader->SetParameters(View, SrcSize, HZBRenderTarget.MipSRVs[MipIndex - 1].Get());

		D3D11_VIEWPORT VP = { 0, 0, (float)DstSize.X, (float)DstSize.Y, 0, 1.f };
		D3D11DeviceContext->RSSetViewports(1, &VP);

		DrawRectangle(
			0, 0,
			DstSize.X, DstSize.Y,
			0, 0,
			SrcSize.X, SrcSize.Y,
			DstSize,
			SrcSize,
			*VertexShader,
			1);

		SrcSize /= 2;
		DstSize /= 2;

		RHICopyToResolveTarget(View.HZB->TargetableTexture.get(), View.HZB->ShaderResourceTexture.get(),FResolveParams());
	}
}

void FSceneRenderer::RenderHzb()
{
	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
	{
		SCOPED_DRAW_EVENT_FORMAT(RenderHzb, TEXT("Views %d"), ViewIndex);
		FViewInfo& View = Views[ViewIndex];
		BuildHZB(View);
	}
}

