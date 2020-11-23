#include "SceneOcclusion.h"
#include "SceneFilterRendering.h"
#include "RenderTargets.h"
#include "DeferredShading.h"
#include "GPUProfiler.h"
#include "GlobalShader.h"

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

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(ShaderRHI, View.ViewUniformBuffer);
		RenderTargets& SceneContext = RenderTargets::Get();

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

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(ShaderRHI, View.ViewUniformBuffer);

		const Vector2 InvSize(1.0f / Size.X, 1.0f / Size.Y);
		SetShaderValue(ShaderRHI, InvSizeParameter, InvSize);

		//SetTextureParameter( ShaderRHI, TextureParameter, TextureParameterSampler, TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(), Texture );

		SetSRVParameter(ShaderRHI, TextureParameter, ShaderResourceView);
		SetSamplerParameter(ShaderRHI, TextureParameterSampler, TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI());
	}
};

IMPLEMENT_SHADER_TYPE(template<>, THZBBuildPS<0>, ("HZBOcclusion.hlsl"), ("HZBBuildPS"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>, THZBBuildPS<1>, ("HZBOcclusion.hlsl"), ("HZBBuildPS"), SF_Pixel);

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
	PSBytecode0 = CompilePixelShader(TEXT("./Shaders/HZBOcclusion.hlsl"), "HZBBuildPS", &Macro0, 1);
	PSBytecode1 = CompilePixelShader(TEXT("./Shaders/HZBOcclusion.hlsl"), "HZBBuildPS", &Macro1, 1);
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

	D3D11DeviceContext->OMSetBlendState(TStaticBlendState<>::GetRHI(), NULL, 0xffffffff);
	D3D11DeviceContext->RSSetState(TStaticRasterizerState<>::GetRHI());
	D3D11DeviceContext->OMSetDepthStencilState(TStaticDepthStencilState<TRUE,D3D11_COMPARISON_ALWAYS>::GetRHI(),0);


	FD3D11Texture2D* HZBRenderTarget = View.HZB->TargetableTexture.get();

	{
		SCOPED_DRAW_EVENT_FORMAT(BuildHZB, TEXT("HZB SetupMip 0 %dx%d"), HZBSize.X, HZBSize.Y);
		D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	}
	/*
	D3D11DeviceContext->OMSetRenderTargets(1, &HZBRTVs[0], NULL);
	D3D11DeviceContext->IASetInputLayout(GFilterInputLayout);
	D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	UINT Stride = sizeof(FilterVertex);
	UINT Offset = 0;
	D3D11DeviceContext->IASetVertexBuffers(0, 1, &GScreenRectangleVertexBuffer, &Stride, &Offset);
	D3D11DeviceContext->IASetIndexBuffer(GScreenRectangleIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	D3D11DeviceContext->VSSetShader(GCommonPostProcessVS, 0, 0);
	D3D11DeviceContext->PSSetShader(PS0, 0, 0);

	View.HZBMipmap0Size = HZBSize;

	const ParameterAllocation& InvSizeParam = PSParams0["InvSize"];
	const ParameterAllocation& InputUvFactorAndOffsetParam = PSParams0["InputUvFactorAndOffset"];
	const ParameterAllocation& InputViewportMaxBoundParam = PSParams0["InputViewportMaxBound"];
	const ParameterAllocation& SceneDepthTextureParam = PSParams0["SceneTexturesStruct_SceneDepthTexture"];
	const ParameterAllocation& SceneDepthTextureSamplerParam = PSParams0["SceneTexturesStruct_SceneDepthTextureSampler"];

	RenderTargets& SceneContext = RenderTargets::Get();
	const IntPoint GBufferSize = SceneContext.GetBufferSizeXY();
	const Vector2 InvSize(1.0f / float(GBufferSize.X), 1.0f / float(GBufferSize.Y));
	const Vector4 InputUvFactorAndOffset(
		float(2 * View.HZBMipmap0Size.X) / float(GBufferSize.X),
		float(2 * View.HZBMipmap0Size.Y) / float(GBufferSize.Y),
		float(View.ViewRect.Min.X) / float(GBufferSize.X),
		float(View.ViewRect.Min.Y) / float(GBufferSize.Y)
		//float(2 * HZBSize.X) / float(GBufferSize.X),
		//float(2 * HZBSize.Y) / float(GBufferSize.Y),
		//float(GViewport.TopLeftX) / float(GBufferSize.X),
		//float(GViewport.TopLeftY) / float(GBufferSize.Y)
	);
	const Vector2 InputViewportMaxBound(
		float(View.ViewRect.Max.X) / float(GBufferSize.X) - 0.5f * InvSize.X,
		float(View.ViewRect.Max.Y) / float(GBufferSize.Y) - 0.5f * InvSize.Y
// 		float(GViewport.TopLeftX + GViewport.Width) / float(GBufferSize.X) - 0.5f * InvSize.X,
// 		float(GViewport.TopLeftY + GViewport.Height) / float(GBufferSize.Y) - 0.5f * InvSize.Y
	);

	memcpy(GlobalConstantBufferData + InvSizeParam.BaseIndex, &InvSize, InvSizeParam.Size);
	memcpy(GlobalConstantBufferData + InputUvFactorAndOffsetParam.BaseIndex, &InputUvFactorAndOffset, InputUvFactorAndOffsetParam.Size);
	memcpy(GlobalConstantBufferData + InputViewportMaxBoundParam.BaseIndex, &InputViewportMaxBound, InputViewportMaxBoundParam.Size);
	D3D11DeviceContext->UpdateSubresource(GlobalConstantBuffer, 0, NULL, GlobalConstantBufferData, 0, 0);
	D3D11DeviceContext->PSSetConstantBuffers(InvSizeParam.BufferIndex, 1, &GlobalConstantBuffer);
	D3D11DeviceContext->PSSetShaderResources(SceneDepthTextureParam.BaseIndex, SceneDepthTextureParam.Size, &SceneDepthSRV);
	D3D11DeviceContext->PSSetSamplers(SceneDepthTextureSamplerParam.BaseIndex, SceneDepthTextureSamplerParam.Size, &SceneDepthSampler);

	D3D11DeviceContext->RSSetState(RasterizerState);
	D3D11_VIEWPORT Viewport = { 0,0,(float)HZBSize.X,(float)HZBSize.Y,0.f,1.f };
	D3D11DeviceContext->RSSetViewports(1, &Viewport);
	D3D11DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);
	D3D11DeviceContext->OMSetBlendState(BlendState, NULL, 0xffffffff);

	D3D11DeviceContext->DrawIndexed(6, 0, 0);

	D3D11DeviceContext->CopyResource(HZBSRVTexture, HZBRTVTexture);

	IntPoint SrcSize = HZBSize;
	IntPoint DstSize = SrcSize / 2;

	for (uint32 i = 1; i < NumMips; ++i)
	{
		DstSize.X = Math::Max(DstSize.X, 1);
		DstSize.Y = Math::Max(DstSize.Y, 1);

		D3D11DeviceContext->OMSetRenderTargets(1, &HZBRTVs[i], NULL);

		D3D11DeviceContext->PSSetShader(PS1, 0, 0);

		const ParameterAllocation& InvSizeParam = PSParams1["InvSize"];
		const ParameterAllocation& TextureParam = PSParams1["Texture"];
		const ParameterAllocation& TextureSamplerParam = PSParams1["TextureSampler"];

		memcpy(GlobalConstantBufferData + InvSizeParam.BaseIndex, &InvSize, InvSizeParam.Size);
		D3D11DeviceContext->UpdateSubresource(GlobalConstantBuffer, 0, NULL, GlobalConstantBufferData, 0, 0);
		D3D11DeviceContext->PSSetConstantBuffers(InvSizeParam.BufferIndex, 1, &GlobalConstantBuffer);

		D3D11DeviceContext->PSSetShaderResources(TextureParam.BufferIndex, TextureParam.Size, &HZBSRVs[i-1]);
		ID3D11SamplerState* TextureSampler = TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI();
		D3D11DeviceContext->PSSetSamplers(TextureSamplerParam.BufferIndex, TextureSamplerParam.Size, &TextureSampler);

		D3D11_VIEWPORT Viewport = { 0,0,(float)DstSize.X,(float)DstSize.Y,0.f,1.f };
		D3D11DeviceContext->RSSetViewports(1, &Viewport);

		D3D11DeviceContext->DrawIndexed(6, 0, 0);

		D3D11DeviceContext->CopyResource(HZBSRVTexture, HZBRTVTexture);

		SrcSize /= 2;
		DstSize /= 2;
	}

	ID3D11ShaderResourceView* SRV = NULL;
	ID3D11SamplerState* Sampler = NULL;
	D3D11DeviceContext->PSSetShaderResources(SceneDepthTextureParam.BaseIndex, SceneDepthTextureParam.Size, &SRV);
	D3D11DeviceContext->PSSetSamplers(SceneDepthTextureSamplerParam.BaseIndex, SceneDepthTextureSamplerParam.Size, &Sampler);
	*/
}

void SceneRenderer::RenderHzb()
{
	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
	{
		SCOPED_DRAW_EVENT_FORMAT(RenderHzb, TEXT("Views %d"), ViewIndex);
		FViewInfo& View = Views[ViewIndex];
		BuildHZB(View);
	}
}

