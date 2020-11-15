#include "AtmosphereRendering.h"
#include "RenderTargets.h"
#include "Scene.h"
#include "DeferredShading.h"
#include "GPUProfiler.h"
#include "DirectXTex.h"
using namespace DirectX;

ID3D11Buffer* AtmosphereFogVertexBuffer;
ID3D11Buffer* AtmosphereFogIndexBuffer;
ID3D11InputLayout* AtmosphereFogInputLayout;

ID3DBlob* AtmosphereFogVSBytecode;
ID3DBlob* AtmosphereFogPSBytecode;
ID3D11VertexShader* AtmosphereFogVertexShader;
ID3D11PixelShader* AtmosphereFogPixelShader;
ID3D11RasterizerState* AtmosphereFogRasterState;
ID3D11BlendState* AtmosphereFogBlendState;

ID3D11ShaderResourceView* SceneDepthSRV;
ID3D11SamplerState* SceneDepthSampler;

ID3D11ShaderResourceView* AtmosphereTransmittanceSRV;
ID3D11SamplerState* AtmosphereTransmittanceTextureSampler;
ID3D11ShaderResourceView* AtmosphereIrradianceSRV;
ID3D11SamplerState* AtmosphereIrradianceTextureSampler;
ID3D11ShaderResourceView* AtmosphereInscatterSRV;
ID3D11SamplerState* AtmosphereInscatterTextureSampler;

std::map<std::string, ParameterAllocation> AtmosphereFogVSParams;
std::map<std::string, ParameterAllocation> AtmosphereFogPSParams;


bool ShouldRenderAtmosphere(/*const FSceneViewFamily& Family*/)
{
	return true;
}

void InitAtomosphereFog()
{
	/*
	Vector2 Vertices[] = {
		Vector2(-1,-1),
		Vector2(-1,+1),
		Vector2(+1,+1),
		Vector2(+1,-1),
	};
	uint16 Indices[] = {
		0, 1, 2,
		0, 2, 3
	};

	AtmosphereFogVertexBuffer = CreateVertexBuffer(false, sizeof(Vertices), Vertices);
	AtmosphereFogIndexBuffer = CreateIndexBuffer(Indices, sizeof(Indices));

	AtmosphereFogVSBytecode = CompileVertexShader(TEXT("./Shaders/AtmosphericFogShader.hlsl"), "VS_Main");
	AtmosphereFogPSBytecode = CompilePixelShader(TEXT("./Shaders/AtmosphericFogShader.hlsl"), "PS_Main");
	GetShaderParameterAllocations(AtmosphereFogVSBytecode, AtmosphereFogVSParams);
	GetShaderParameterAllocations(AtmosphereFogPSBytecode, AtmosphereFogPSParams);
	D3D11_INPUT_ELEMENT_DESC InputDesc[] = 
	{
		"ATTRIBUTE",0,DXGI_FORMAT_R32G32_FLOAT,0,0, D3D11_INPUT_PER_VERTEX_DATA,0,
	};
	AtmosphereFogInputLayout = CreateInputLayout(InputDesc, sizeof(InputDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC), AtmosphereFogVSBytecode);

	AtmosphereFogVertexShader = CreateVertexShader(AtmosphereFogVSBytecode);
	AtmosphereFogPixelShader = CreatePixelShader(AtmosphereFogPSBytecode);

	AtmosphereFogRasterState = TStaticRasterizerState<>::GetRHI();
	AtmosphereFogBlendState = TStaticBlendState<FALSE,TRUE, D3D11_COLOR_WRITE_ENABLE_ALL, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ONE>::GetRHI();

	RenderTargets& SceneContext = RenderTargets::Get();
	SceneDepthSRV = CreateShaderResourceView2D(SceneContext.GetSceneDepthTexture(), DXGI_FORMAT_R24_UNORM_X8_TYPELESS,1,0);
	SceneDepthSampler = TStaticSamplerState<>::GetRHI();

	{
		TexMetadata Metadata;
		ScratchImage sImage;
		if (S_OK == LoadFromDDSFile(TEXT("./dx11demo/AtmosphereTransmittanceTexture.dds"), DDS_FLAGS_NONE, &Metadata, sImage))
		{
			CreateShaderResourceView(D3D11Device, sImage.GetImages(), 1, Metadata, &AtmosphereTransmittanceSRV);
			AtmosphereTransmittanceTextureSampler = TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT>::GetRHI();
		}
	}
	{
		TexMetadata Metadata;
		ScratchImage sImage;
		if (S_OK == LoadFromDDSFile(TEXT("./dx11demo/AtmosphereIrradianceTexture.dds"), DDS_FLAGS_NONE, &Metadata, sImage))
		{
			CreateShaderResourceView(D3D11Device, sImage.GetImages(), 1, Metadata, &AtmosphereIrradianceSRV);
			AtmosphereIrradianceTextureSampler = TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT>::GetRHI();
		}
	}
	{
		TexMetadata Metadata;
		ScratchImage sImage;
		if (S_OK == LoadFromDDSFile(TEXT("./dx11demo/AtmosphereInscatterTexture.dds"), DDS_FLAGS_NONE, &Metadata, sImage))
		{
			CreateShaderResourceView(D3D11Device, sImage.GetImages(), 32, Metadata, &AtmosphereInscatterSRV);
			AtmosphereInscatterTextureSampler = TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT>::GetRHI();
		}
	}
	*/
}

void SceneRenderer::RenderAtmosphereFog()
{
	/*
	SCOPED_DRAW_EVENT_FORMAT(RenderAtmosphereFog, TEXT("AtmosphereFog"));

	RenderTargets& SceneContext = RenderTargets::Get();
	SceneContext.BeginRenderingSceneColor();

	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ++ViewIndex)
	{
		SceneView& View = Views[ViewIndex];

		UINT Stride = sizeof(Vector2);
		UINT Offset = 0;
		D3D11DeviceContext->IASetVertexBuffers(0, 1, &AtmosphereFogVertexBuffer, &Stride, &Offset);
		D3D11DeviceContext->IASetIndexBuffer(AtmosphereFogIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
		D3D11DeviceContext->IASetInputLayout(AtmosphereFogInputLayout);
		D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		D3D11DeviceContext->VSSetShader(AtmosphereFogVertexShader, 0, 0);
		D3D11DeviceContext->PSSetShader(AtmosphereFogPixelShader, 0, 0);

		const ParameterAllocation& VSViewParam = AtmosphereFogVSParams["View"];
		D3D11DeviceContext->VSSetConstantBuffers(VSViewParam.BufferIndex, 1, &View.ViewUniformBuffer);
		const ParameterAllocation& PSViewParam = AtmosphereFogPSParams["View"];
		D3D11DeviceContext->PSSetConstantBuffers(PSViewParam.BufferIndex, 1, &View.ViewUniformBuffer);

		const ParameterAllocation& SceneDepthTextureParam = AtmosphereFogPSParams["SceneTexturesStruct_SceneDepthTexture"];
		const ParameterAllocation& SceneDepthTextureSamplerParam = AtmosphereFogPSParams["SceneTexturesStruct_SceneDepthTextureSampler"];
		const ParameterAllocation& AtmosphereTransmittanceTextureParam = AtmosphereFogPSParams["AtmosphereTransmittanceTexture"];
		const ParameterAllocation& AtmosphereTransmittanceTextureSamplerParam = AtmosphereFogPSParams["AtmosphereTransmittanceTextureSampler"];
		const ParameterAllocation& AtmosphereIrradianceTextureParam = AtmosphereFogPSParams["AtmosphereIrradianceTexture"];
		const ParameterAllocation& AtmosphereIrradianceTextureSamplerParam = AtmosphereFogPSParams["AtmosphereIrradianceTextureSampler"];
		const ParameterAllocation& AtmosphereInscatterTextureParam = AtmosphereFogPSParams["AtmosphereInscatterTexture"];
		const ParameterAllocation& AtmosphereInscatterTextureSamplerParam = AtmosphereFogPSParams["AtmosphereInscatterTextureSampler"];
		D3D11DeviceContext->PSSetShaderResources(SceneDepthTextureParam.BaseIndex, SceneDepthTextureParam.Size, &SceneDepthSRV);
		D3D11DeviceContext->PSSetSamplers(SceneDepthTextureSamplerParam.BaseIndex, SceneDepthTextureSamplerParam.Size, &SceneDepthSampler);
		D3D11DeviceContext->PSSetShaderResources(AtmosphereTransmittanceTextureParam.BaseIndex, AtmosphereTransmittanceTextureParam.Size, &AtmosphereTransmittanceSRV);
		D3D11DeviceContext->PSSetSamplers(AtmosphereTransmittanceTextureSamplerParam.BaseIndex, AtmosphereTransmittanceTextureSamplerParam.Size, &AtmosphereTransmittanceTextureSampler);
		D3D11DeviceContext->PSSetShaderResources(AtmosphereIrradianceTextureParam.BaseIndex, AtmosphereIrradianceTextureParam.Size, &AtmosphereIrradianceSRV);
		D3D11DeviceContext->PSSetSamplers(AtmosphereIrradianceTextureSamplerParam.BaseIndex, AtmosphereTransmittanceTextureSamplerParam.Size, &AtmosphereIrradianceTextureSampler);
		D3D11DeviceContext->PSSetShaderResources(AtmosphereInscatterTextureParam.BaseIndex, AtmosphereInscatterTextureParam.Size, &AtmosphereInscatterSRV);
		D3D11DeviceContext->PSSetSamplers(AtmosphereInscatterTextureSamplerParam.BaseIndex, AtmosphereInscatterTextureSamplerParam.Size, &AtmosphereInscatterTextureSampler);

		D3D11DeviceContext->RSSetState(AtmosphereFogRasterState);
		D3D11DeviceContext->RSSetViewports(1, &GViewport);
		D3D11DeviceContext->OMSetBlendState(AtmosphereFogBlendState, NULL, 0xffffffff);

		D3D11DeviceContext->DrawIndexed(6, 0, 0);

		ID3D11ShaderResourceView* NULLSRV = NULL;
		ID3D11SamplerState* NULLSampler = NULL;
		D3D11DeviceContext->PSSetShaderResources(SceneDepthTextureParam.BaseIndex, SceneDepthTextureParam.Size, &NULLSRV);
		D3D11DeviceContext->PSSetSamplers(SceneDepthTextureSamplerParam.BaseIndex, SceneDepthTextureSamplerParam.Size, &NULLSampler);
		D3D11DeviceContext->PSSetShaderResources(AtmosphereTransmittanceTextureParam.BaseIndex, AtmosphereTransmittanceTextureParam.Size, &NULLSRV);
		D3D11DeviceContext->PSSetSamplers(AtmosphereTransmittanceTextureSamplerParam.BaseIndex, AtmosphereTransmittanceTextureSamplerParam.Size, &NULLSampler);
		D3D11DeviceContext->PSSetShaderResources(AtmosphereIrradianceTextureParam.BaseIndex, AtmosphereIrradianceTextureParam.Size, &NULLSRV);
		D3D11DeviceContext->PSSetSamplers(AtmosphereIrradianceTextureSamplerParam.BaseIndex, AtmosphereTransmittanceTextureSamplerParam.Size, &NULLSampler);
		D3D11DeviceContext->PSSetShaderResources(AtmosphereTransmittanceTextureSamplerParam.BaseIndex, AtmosphereTransmittanceTextureSamplerParam.Size, &NULLSRV);
		D3D11DeviceContext->PSSetSamplers(AtmosphereTransmittanceTextureSamplerParam.BaseIndex, AtmosphereTransmittanceTextureSamplerParam.Size, &NULLSampler);

	}
	*/
}

AtmosphericFogSceneInfo::AtmosphericFogSceneInfo(/*UAtmosphericFogComponent* InComponent,*/ const FScene* InScene)
{
	SunMultiplier = 1.0f;
	FogMultiplier = 1.0f;
	InvDensityMultiplier = 1.0f;
	DensityOffset = 0.0f;
	GroundOffset = -98975.89844f;
	DistanceScale = 1.0f;
	AltitudeScale = 1.0f;
	RHeight = 8.0f;
	StartDistance = 0.15f;
	DistanceOffset = 0.0f;
	SunDiscScale = 1.0f;
	DefaultSunColor = { 2.75f, 2.75f, 2.75f, 2.75f };
	DefaultSunDirection = { -0.39815f, -0.05403f, 0.91573f };
	RenderFlag = 0;
	InscatterAltitudeSampleNum = 32;
}

AtmosphericFogSceneInfo::~AtmosphericFogSceneInfo()
{

}
