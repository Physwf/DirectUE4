#include "LightRendering.h"
#include "Scene.h"
#include "RenderTargets.h"
#include "DeferredShading.h"
#include "GPUProfiler.h"

#pragma pack(push)
#pragma pack(1)
struct DeferredLightUniforms
{
	Vector LightPosition;
	float LightInvRadius;
	Vector LightColor;
	float LightFalloffExponent;
	Vector NormalizedLightDirection;
	float DeferredLightUniformsPading01;
	Vector NormalizedLightTangent;
	float DeferredLightUniformsPading02;
	Vector2 SpotAngles;
	float SpecularScale;
	float SourceRadius;
	float SoftSourceRadius;
	float SourceLength;
	float ContactShadowLength;
	float DeferredLightUniformsPading03;
	Vector2 DistanceFadeMAD;
	float DeferredLightUniformsPading04;
	float DeferredLightUniformsPading05;
	Vector4 ShadowMapChannelMask;
	unsigned int ShadowedBits;
	unsigned int LightingChannelMask;
	float VolumetricScatteringIntensity;
	float DeferredLightUniformsPading06;
};
#pragma pack(pop)

struct RectangleVertex
{
	Vector2 Position;
	Vector2 UV;
};

struct DrawRectangleParameters
{
	Vector4 PosScaleBias;
	Vector4 UVScaleBias;
	Vector4 InvTargetSizeAndTextureSize;
};
/*
0------1
|		|
2------3
*/

struct ScreenRectangle
{
	RectangleVertex Vertices[4];
	int Indices[6] = { 0,1,2,2,1,3 };

	DrawRectangleParameters UniformPrameters;

	ID3D11Buffer* VertexBuffer = NULL;
	ID3D11Buffer* IndexBuffer = NULL;
	ID3D11Buffer* DrawRectangleParameters = NULL;

	ScreenRectangle(float W, float H)
	{
		Vertices[0].Position = Vector2(1.0f, 1.0f);
		Vertices[0].UV = Vector2(1.0f, 1.0f);
		Vertices[1].Position = Vector2(0.0f, 1.0f);
		Vertices[1].UV = Vector2(0.0f, 1.0f);
		Vertices[2].Position = Vector2(1.0f, 0.0f);
		Vertices[2].UV = Vector2(1.0f, 0.0f);
		Vertices[3].Position = Vector2(0.0f, 0.0f);
		Vertices[3].UV = Vector2(0.0f, 0.0f);

		UniformPrameters.PosScaleBias = Vector4(W, H, 0.0f, 0.0f);
		UniformPrameters.UVScaleBias = Vector4(W, H, 0.0f, 0.0f);
		UniformPrameters.InvTargetSizeAndTextureSize = Vector4(1.0f / W, 1.0f / H, 1.0f / W, 1.0f / H);
	}

	~ScreenRectangle()
	{
		ReleaseResource();
	}

	void InitResource()
	{
		VertexBuffer = CreateVertexBuffer(false, sizeof(Vertices), Vertices);
		IndexBuffer = CreateIndexBuffer(Indices, sizeof(Indices));
		DrawRectangleParameters = CreateConstantBuffer(false, sizeof(UniformPrameters), &UniformPrameters);
	}

	void ReleaseResource()
	{
		if (VertexBuffer)
		{
			VertexBuffer->Release();
		}
		if (IndexBuffer)
		{
			IndexBuffer->Release();
		}
	}
};

ScreenRectangle ScreenRect((float)WindowWidth, (float)WindowHeight);

DeferredLightUniforms DLU;
ID3D11Buffer* DeferredLightUniformBuffer;
ID3D11Buffer* VSourceTexture;

ID3D11InputLayout* RectangleInputLayout;

ID3DBlob* LightPassVSByteCode;
ID3DBlob* LightPassPSByteCode;
//Scene  depth
ID3D11ShaderResourceView* LightPassSceneDepthSRV;
ID3D11SamplerState* LightPassSceneDepthSamplerState;
//G-Buffer
ID3D11ShaderResourceView* LightPassGBufferSRV[6];
ID3D11SamplerState* LightPassGBufferSamplerState[6];
//Screen Space AO, Custom Depth, Custom Stencil
ID3D11Texture2D* LightPassScreenSapceAOTexture;
ID3D11Texture2D* LightPassCustomDepthTexture;
ID3D11Texture2D* LightPassCustomStencialTexture;
ID3D11ShaderResourceView* LightPassScreenSapceAOSRV;
ID3D11ShaderResourceView* LightPassCustomDepthSRV;
ID3D11ShaderResourceView* LightPassCustomStencialSRV;
ID3D11SamplerState* LightPassScreenSapceAOSamplerState;
ID3D11SamplerState* LightPassCustomDepthSamplerState;
ID3D11SamplerState* LightPassCustomStencialState;

ID3D11VertexShader* LightPassVS;
ID3D11PixelShader* LightPassPS;
ID3D11RasterizerState* LightPassRasterizerState;
ID3D11BlendState* LightPassBlendState;
ID3D11DepthStencilState* LightPassDepthStencilState;
ID3D11ShaderResourceView* LightAttenuationSRV;
ID3D11SamplerState* LightAttenuationSampleState;

extern ID3D11Texture2D* BasePassSceneColorRT;
extern ID3D11Texture2D* BasePassGBufferRT[6];
extern ID3D11Texture2D* SceneDepthRT;
extern ID3D11Texture2D* ShadowProjectionRT;


std::map<std::string, ParameterAllocation> LightPassVSParams;
std::map<std::string, ParameterAllocation> LightPassPSParams;

void InitLightPass()
{
	LightPassVSByteCode = CompileVertexShader(TEXT("./Shaders/DeferredLightVertexShader.hlsl"), "VS_Main");
	LightPassPSByteCode = CompilePixelShader(TEXT("./Shaders/DeferredLightPixelShader.hlsl"), "PS_Main");
	GetShaderParameterAllocations(LightPassVSByteCode, LightPassVSParams);
	GetShaderParameterAllocations(LightPassPSByteCode, LightPassPSParams);

	D3D11_INPUT_ELEMENT_DESC RectangleInputDesc[] =
	{
		{ "ATTRIBUTE",	0,	DXGI_FORMAT_R32G32_FLOAT,	0, 0,  D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "ATTRIBUTE",	1,	DXGI_FORMAT_R32G32_FLOAT,	0, 8,  D3D11_INPUT_PER_VERTEX_DATA,0 },
	};

	ScreenRect.InitResource();

	RectangleInputLayout = CreateInputLayout(RectangleInputDesc, sizeof(RectangleInputDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC), LightPassVSByteCode);
	LightPassVS = CreateVertexShader(LightPassVSByteCode);
	LightPassPS = CreatePixelShader(LightPassPSByteCode);

	RenderTargets& SceneContext = RenderTargets::Get();

	LightPassSceneDepthSRV = CreateShaderResourceView2D(SceneContext.GetSceneDepthTexture(), DXGI_FORMAT_R24_UNORM_X8_TYPELESS, 1, 0);
	LightPassSceneDepthSamplerState = TStaticSamplerState<>::GetRHI();

	LightPassGBufferSRV[0] = CreateShaderResourceView2D(SceneContext.GetGBufferATexture(), DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0);
	LightPassGBufferSRV[1] = CreateShaderResourceView2D(SceneContext.GetGBufferBTexture(), DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0);
	LightPassGBufferSRV[2] = CreateShaderResourceView2D(SceneContext.GetGBufferCTexture(), DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0);

	LightPassGBufferSamplerState[0] = TStaticSamplerState<>::GetRHI();
	LightPassGBufferSamplerState[1] = TStaticSamplerState<>::GetRHI();
	LightPassGBufferSamplerState[2] = TStaticSamplerState<>::GetRHI();

	LightPassRasterizerState = TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_NONE, FALSE, FALSE>::GetRHI();
	LightPassDepthStencilState = TStaticDepthStencilState<false, D3D11_COMPARISON_ALWAYS>::GetRHI();
	LightPassBlendState = TStaticBlendState<FALSE,TRUE, D3D11_COLOR_WRITE_ENABLE_ALL, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ONE>::GetRHI();
	
	LightAttenuationSRV = CreateShaderResourceView2D(ShadowProjectionRT, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0);
	LightAttenuationSampleState = TStaticSamplerState<>::GetRHI();

	/*

	DLU.LightPosition = Vector(0.0f, 0.0f, 0.0f);
	DLU.LightInvRadius = 0.0f;
	DLU.LightColor = DirLight.Color;
	DLU.NormalizedLightDirection = -DirLight.Direction;
	DLU.NormalizedLightTangent = -DirLight.Direction;
	DLU.SpotAngles = Vector2(0.0f, 0.0f);
	DLU.SpecularScale = 1.0f;
	DLU.SourceRadius = std::sin(0.5f * DirLight.LightSourceAngle / 180.f * 3.14f);
	DLU.SoftSourceRadius = std::sin(0.5f * DirLight.LightSourceSoftAngle / 180.f * 3.14f);
	DLU.SourceLength = 0.0f;
	DLU.ContactShadowLength;
	//const FVector2D FadeParams = LightSceneInfo->Proxy->GetDirectionalLightDistanceFadeParameters(View.GetFeatureLevel(), LightSceneInfo->IsPrecomputedLightingValid(), View.MaxShadowCascades);
	// use MAD for efficiency in the shader
	//DeferredLightUniformsValue.DistanceFadeMAD = FVector2D(FadeParams.Y, -FadeParams.X * FadeParams.Y);
	DLU.DistanceFadeMAD = Vector2(0.5f, 0.5f);
	//UE4 LightRender.h
	//DeferredLightUniformsValue.ShadowedBits = LightSceneInfo->Proxy->CastsStaticShadow() || bHasLightFunction ? 1 : 0;
	//DeferredLightUniformsValue.ShadowedBits |= LightSceneInfo->Proxy->CastsDynamicShadow() && View.Family->EngineShowFlags.DynamicShadows ? 3 : 0;
	DLU.ShadowedBits = 2;
	DeferredLightUniformBuffer = CreateConstantBuffer(false, sizeof(DLU), &DLU);
	*/
}

void SceneRenderer::RenderLight()
{
	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
	{
		ViewInfo& View = Views[ViewIndex];

		D3D11DeviceContext->RSSetState(LightPassRasterizerState);
		D3D11DeviceContext->RSSetViewports(1, &GViewport);
		D3D11DeviceContext->OMSetDepthStencilState(LightPassDepthStencilState, 0);
		D3D11DeviceContext->OMSetBlendState(LightPassBlendState, NULL, 0xffffffff);

		D3D11DeviceContext->IASetInputLayout(RectangleInputLayout);
		D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		UINT Stride = sizeof(RectangleVertex);
		UINT Offset = 0;
		D3D11DeviceContext->IASetVertexBuffers(0, 1, &ScreenRect.VertexBuffer, &Stride, &Offset);
		D3D11DeviceContext->IASetIndexBuffer(ScreenRect.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		D3D11DeviceContext->VSSetShader(LightPassVS, 0, 0);
		D3D11DeviceContext->PSSetShader(LightPassPS, 0, 0);

		const ParameterAllocation& VSViewParam = LightPassVSParams.at("View");
		const ParameterAllocation& DrawRectangleParametersParam = LightPassVSParams.at("DrawRectangleParameters");
		D3D11DeviceContext->VSSetConstantBuffers(VSViewParam.BufferIndex, 1, &View.ViewUniformBuffer);
		D3D11DeviceContext->VSSetConstantBuffers(DrawRectangleParametersParam.BufferIndex, 1, &ScreenRect.DrawRectangleParameters);

		const ParameterAllocation& LightAttenuationTextureParam = LightPassPSParams.at("LightAttenuationTexture");
		const ParameterAllocation& LightAttenuationTextureSamplerParam = LightPassPSParams.at("LightAttenuationTextureSampler");
		const ParameterAllocation& SceneDepthTextureParam = LightPassPSParams.at("SceneTexturesStruct_SceneDepthTexture");
		const ParameterAllocation& SceneDepthTextureSamplerParam = LightPassPSParams.at("SceneTexturesStruct_SceneDepthTextureSampler");
		const ParameterAllocation& GBufferATextureParam = LightPassPSParams.at("SceneTexturesStruct_GBufferATexture");
		const ParameterAllocation& GBufferATextureSamplerParam = LightPassPSParams.at("SceneTexturesStruct_GBufferATextureSampler");
		const ParameterAllocation& DeferredLightUniformParam = LightPassPSParams.at("DeferredLightUniform");
		const ParameterAllocation& PSViewParam = LightPassPSParams.at("View");

		D3D11DeviceContext->PSSetShaderResources(SceneDepthTextureParam.BaseIndex, SceneDepthTextureParam.Size, &LightPassSceneDepthSRV);
		D3D11DeviceContext->PSSetSamplers(SceneDepthTextureSamplerParam.BaseIndex, SceneDepthTextureSamplerParam.Size, &LightPassSceneDepthSamplerState);

		D3D11DeviceContext->PSSetShaderResources(GBufferATextureParam.BaseIndex, 3, LightPassGBufferSRV);
		D3D11DeviceContext->PSSetSamplers(GBufferATextureSamplerParam.BaseIndex, 3, LightPassGBufferSamplerState);

		D3D11DeviceContext->PSSetShaderResources(LightAttenuationTextureParam.BaseIndex, LightAttenuationTextureParam.Size, &LightAttenuationSRV);
		D3D11DeviceContext->PSSetSamplers(LightAttenuationTextureSamplerParam.BaseIndex, LightAttenuationTextureSamplerParam.Size, &LightAttenuationSampleState);

		D3D11DeviceContext->PSSetConstantBuffers(DeferredLightUniformParam.BufferIndex, 1, &DeferredLightUniformBuffer);
		D3D11DeviceContext->PSSetConstantBuffers(PSViewParam.BufferIndex, 1, &View.ViewUniformBuffer);


		D3D11DeviceContext->DrawIndexed(6, 0, 0);

		//reset
		ID3D11ShaderResourceView* SRV = NULL;
		ID3D11SamplerState* Sampler = NULL;

		D3D11DeviceContext->PSSetShaderResources(SceneDepthTextureParam.BaseIndex, SceneDepthTextureParam.Size, &SRV);
		D3D11DeviceContext->PSSetSamplers(SceneDepthTextureSamplerParam.BaseIndex, SceneDepthTextureSamplerParam.Size, &Sampler);

		D3D11DeviceContext->PSSetShaderResources(LightAttenuationTextureParam.BaseIndex, LightAttenuationTextureParam.Size, &SRV);
		D3D11DeviceContext->PSSetSamplers(LightAttenuationTextureSamplerParam.BaseIndex, LightAttenuationTextureSamplerParam.Size, &Sampler);

		for (int i = 0; i < 3; ++i)
		{
			D3D11DeviceContext->PSSetShaderResources(GBufferATextureParam.BaseIndex + i, 1, &SRV);
			D3D11DeviceContext->PSSetSamplers(GBufferATextureSamplerParam.BaseIndex + i, 1, &Sampler);
		}
	}
}

void SceneRenderer::RenderLights()
{
	SCOPED_DRAW_EVENT_FORMAT(RenderLights, TEXT("Lights"));

	RenderTargets& SceneContext = RenderTargets::Get();
	SceneContext.BeginRenderingSceneColor();
	RenderLight();
}

