#include "BasePassRendering.h"
#include "Scene.h"
#include "StaticMesh.h"
#include "RenderTargets.h"
#include "DeferredShading.h"
#include "GPUProfiler.h"
#include "DirectXTex.h"
using namespace DirectX;

/** The number of coefficients that are stored for each light sample. */
static const int32 NUM_STORED_LIGHTMAP_COEF = 4;

/** The number of directional coefficients which the lightmap stores for each light sample. */
static const int32 NUM_HQ_LIGHTMAP_COEF = 2;

/** The number of simple coefficients which the lightmap stores for each light sample. */
static const int32 NUM_LQ_LIGHTMAP_COEF = 2;

/** The index at which simple coefficients are stored in any array containing all NUM_STORED_LIGHTMAP_COEF coefficients. */
static const int32 LQ_LIGHTMAP_COEF_INDEX = 2;

/** The maximum value between NUM_LQ_LIGHTMAP_COEF and NUM_HQ_LIGHTMAP_COEF. */
static const int32 MAX_NUM_LIGHTMAP_COEF = 2;

#pragma pack(push)
#pragma pack(1)
struct PrecomputedLightingUniform
{
	FVector IndirectLightingCachePrimitiveAdd;// FCachedVolumeIndirectLightingPolicy
	float Pading01;
	FVector IndirectLightingCachePrimitiveScale;// FCachedVolumeIndirectLightingPolicy
	float Pading02;
	FVector IndirectLightingCacheMinUV;// FCachedVolumeIndirectLightingPolicy
	float Pading03;
	FVector IndirectLightingCacheMaxUV;// FCachedVolumeIndirectLightingPolicy
	float Pading04;
	Vector4 PointSkyBentNormal;// FCachedPointIndirectLightingPolicy
	float DirectionalLightShadowing;// FCachedPointIndirectLightingPolicy
	FVector Pading05;
	Vector4 StaticShadowMapMasks;// TDistanceFieldShadowsAndLightMapPolicy
	Vector4 InvUniformPenumbraSizes;// TDistanceFieldShadowsAndLightMapPolicy
	Vector4 IndirectLightingSHCoefficients0[3]; // FCachedPointIndirectLightingPolicy
	Vector4 IndirectLightingSHCoefficients1[3]; // FCachedPointIndirectLightingPolicy
	Vector4 IndirectLightingSHCoefficients2;// FCachedPointIndirectLightingPolicy
	Vector4 IndirectLightingSHSingleCoefficient;// FCachedPointIndirectLightingPolicy
	Vector4 LightMapCoordinateScaleBias; // TLightMapPolicy LightmapCoordinateScale = {X=0.484375000 Y=0.968750000 } LightmapCoordinateBias = {X=0.00781250000 Y=0.0156250000 }
	Vector4 ShadowMapCoordinateScaleBias;// TDistanceFieldShadowsAndLightMapPolicy
	Vector4 LightMapScale[MAX_NUM_LIGHTMAP_COEF];// TLightMapPolicy
	Vector4 LightMapAdd[MAX_NUM_LIGHTMAP_COEF];// TLightMapPolicy
};
#pragma pack(pop)

PrecomputedLightingUniform PrecomputedLightingParameters;

ID3D11InputLayout* MeshInputLayout;

//PrecomputedLightParameters
ID3D11ShaderResourceView* PrecomputedLighting_LightMapTexture;
ID3D11ShaderResourceView* PrecomputedLighting_SkyOcclusionTexture;
ID3D11ShaderResourceView* PrecomputedLighting_AOMaterialMaskTexture;
ID3D11ShaderResourceView* PrecomputedLighting_IndirectLightingCacheTexture0;
ID3D11ShaderResourceView* PrecomputedLighting_IndirectLightingCacheTexture1;
ID3D11ShaderResourceView* PrecomputedLighting_IndirectLightingCacheTexture2;
ID3D11ShaderResourceView* PrecomputedLighting_StaticShadowTexture;
ID3D11SamplerState* PrecomputedLighting_LightMapSampler;
ID3D11SamplerState* PrecomputedLighting_SkyOcclusionSampler;
ID3D11SamplerState* PrecomputedLighting_AOMaterialMaskSampler;
ID3D11SamplerState* PrecomputedLighting_IndirectLightingCacheTextureSampler0;
ID3D11SamplerState* PrecomputedLighting_IndirectLightingCacheTextureSampler1;
ID3D11SamplerState* PrecomputedLighting_IndirectLightingCacheTextureSampler2;
ID3D11SamplerState* PrecomputedLighting_StaticShadowTextureSampler;

ID3D11Buffer* PrecomputedLightUniformBuffer;

ID3DBlob* BasePassVSBytecode;
ID3DBlob* BasePassPSBytecode;

ID3D11VertexShader* BasePassVS;
ID3D11PixelShader* BasePassPS;
ID3D11RasterizerState* BasePassRasterizerState;
ID3D11BlendState* BasePassBlendState;
ID3D11DepthStencilState* BasePassDepthStencilState;

//material
ID3D11ShaderResourceView* BaseColorSRV;
ID3D11SamplerState* BaseColorSampler;

extern ID3D11DepthStencilView* SceneDepthDSV;

std::map<std::string, ParameterAllocation> BasePassVSParams;
std::map<std::string, ParameterAllocation> BasePassPSParams;


void InitBasePass()
{
	/*
	//LightMapRendering.cpp
	const Vector2 LightmapCoordinateScale = { 0.484375000 ,0.968750000 };// LightMapInteraction.GetCoordinateScale();// LightmapCoordinateScale = { X = 0.484375000 Y = 0.968750000 } LightmapCoordinateBias = { X = 0.00781250000 Y = 0.0156250000 }
	const Vector2 LightmapCoordinateBias = { 0.00781250000 ,0156250000 }; //LightMapInteraction.GetCoordinateBias();
	PrecomputedLightingParameters.LightMapCoordinateScaleBias = Vector4(LightmapCoordinateScale.X, LightmapCoordinateScale.Y, LightmapCoordinateBias.X, LightmapCoordinateBias.Y);

	PrecomputedLightingParameters.ShadowMapCoordinateScaleBias = Vector4(0.968750000, 0.968750000, 0.0156250000, 0.0156250000);//{ X = 0.968750000 Y = 0.968750000 Z = 0.0156250000 W = 0.0156250000 }

	bool bAllowHighQualityLightMaps = true;
	const uint32 NumCoef = bAllowHighQualityLightMaps ? NUM_HQ_LIGHTMAP_COEF : NUM_LQ_LIGHTMAP_COEF;
	//const FVector4* Scales = LightMapInteraction.GetScaleArray();Scales = 0x0000006554e6b040 {X=0.0207918882 Y=0.0144486427 Z=0.0207914710 W=6.19691181} Scales[CoefIndex] = {X=0.915465474 Y=0.976352394 Z=0.944339991 W = 0.00000000}
	//const FVector4* Adds = LightMapInteraction.GetAddArray();//Adds = 0x0000006554e6b060 {X=0.986185253 Y=0.995151460 Z=0.986185730 W = -5.75000048} Adds[CoefIndex] = {X=-0.465408266 Y=-0.519816577 Z=-0.434536219 W = 0.282094985}
	Vector4 Scales[] = { { 0.0207918882f ,0.0144486427f ,0.0207914710f ,6.19691181f },{ 0.915465474f , 0.976352394f , 0.944339991f , 0.00000000f } };
	Vector4 Adds[] = { { 0.986185253f , 0.995151460f, 0.986185730f , -5.75000048f },{ -0.465408266f , -0.519816577f , -0.434536219f , 0.282094985f } };
	for (uint32 CoefIndex = 0; CoefIndex < NumCoef; ++CoefIndex)
	{
		PrecomputedLightingParameters.LightMapScale[CoefIndex] = Scales[CoefIndex];
		PrecomputedLightingParameters.LightMapAdd[CoefIndex] = Adds[CoefIndex];
	}
	PrecomputedLightUniformBuffer = CreateConstantBuffer(false, sizeof(PrecomputedLightingParameters), &PrecomputedLightingParameters);

	//Base Pass
	BasePassVSBytecode = CompileVertexShader(TEXT("./Shaders/BasePassVertexShader.hlsl"), "VS_Main");
	BasePassPSBytecode = CompilePixelShader(TEXT("./Shaders/BasePassPixelShader.hlsl"), "PS_Main");
	GetShaderParameterAllocations(BasePassVSBytecode, BasePassVSParams);
	GetShaderParameterAllocations(BasePassPSBytecode, BasePassPSParams);

	D3D11_INPUT_ELEMENT_DESC InputDesc[] =
	{
		{ "ATTRIBUTE",	0,	DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 0,  D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "ATTRIBUTE",	1,	DXGI_FORMAT_R32G32B32_FLOAT,	0, 16, D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "ATTRIBUTE",	2,	DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 32, D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "ATTRIBUTE",	3,	DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 48, D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "ATTRIBUTE",	4,	DXGI_FORMAT_R32G32_FLOAT,		0, 64, D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "ATTRIBUTE",	15,	DXGI_FORMAT_R32G32_FLOAT,		0, 72, D3D11_INPUT_PER_VERTEX_DATA,0 },
	};

	MeshInputLayout = CreateInputLayout(InputDesc, sizeof(InputDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC), BasePassVSBytecode);
	BasePassVS = CreateVertexShader(BasePassVSBytecode);
	BasePassPS = CreatePixelShader(BasePassPSBytecode);
	
	BasePassRasterizerState = TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_BACK, FALSE, FALSE>::GetRHI();
	BasePassDepthStencilState = TStaticDepthStencilState<TRUE, D3D11_COMPARISON_GREATER>::GetRHI();
	BasePassBlendState = TStaticBlendState<>::GetRHI();

	{
		//CreateWICTextureFromFile(D3D11Device, D3D11DeviceContext, TEXT("./shaderBallNoCrease/checkerA.tga"), (ID3D11Resource**)&BaseColor, &BaseColorSRV);
		// 		int Width, Height, nChanels;
		// 		unsigned char* Data = stbi_load("./shaderBallNoCrease/checkerA.tga", &Width, &Height, &nChanels, 4);
		// 		if (Data)
		// 		{
		// 			CreateWICTextureFromMemory(D3D11Device, D3D11DeviceContext, Data, Width*Height*nChanels,(ID3D11Resource**)&BaseColor, &BaseColorSRV);
		// 			//BaseColor = CreateTexture2D(Width, Height, DXGI_FORMAT_,1,Data);
		// 		}

		TexMetadata Metadata;
		ScratchImage sImage;
		if (S_OK == LoadFromTGAFile(TEXT("./shaderBallNoCrease/checkerA.tga"), TGA_FLAGS_FORCE_SRGB, &Metadata, sImage))
		{
			CreateShaderResourceView(D3D11Device, sImage.GetImages(), 1, Metadata, &BaseColorSRV);
			BaseColorSampler = TStaticSamplerState<>::GetRHI();
		}
	}
	{
		{
			TexMetadata Metadata;
			ScratchImage sImage;
			if (S_OK == LoadFromDDSFile(TEXT("./dx11demo/PrecomputedLightingBuffer_LightMapTexture.dds"), DDS_FLAGS_NONE, &Metadata, sImage))
			{
				CreateShaderResourceView(D3D11Device, sImage.GetImages(), Metadata.mipLevels, Metadata, &PrecomputedLighting_LightMapTexture);
				PrecomputedLighting_LightMapSampler = TStaticSamplerState<>::GetRHI();
			}
		}
		{
			TexMetadata Metadata;
			ScratchImage sImage;
			if (S_OK == LoadFromDDSFile(TEXT("./dx11demo/PrecomputedLightingBuffer_StaticShadowTexture.dds"), DDS_FLAGS_NONE, &Metadata, sImage))
			{
				CreateShaderResourceView(D3D11Device, sImage.GetImages(), Metadata.mipLevels, Metadata, &PrecomputedLighting_StaticShadowTexture);
				PrecomputedLighting_StaticShadowTextureSampler = TStaticSamplerState<>::GetRHI();
			}
		}
		{
			TexMetadata Metadata;
			ScratchImage sImage;
			if (S_OK == LoadFromDDSFile(TEXT("./dx11demo/PrecomputedLightingBuffer_SkyOcclusionTexture.dds"), DDS_FLAGS_NONE, &Metadata, sImage))
			{
				CreateShaderResourceView(D3D11Device, sImage.GetImages(), Metadata.mipLevels, Metadata, &PrecomputedLighting_SkyOcclusionTexture);
				PrecomputedLighting_SkyOcclusionSampler = TStaticSamplerState<>::GetRHI();
			}
		}
		// 		ID3D11ShaderResourceView* PrecomputedLighting_SkyOcclusionTexture;
		// 		ID3D11ShaderResourceView* PrecomputedLighting_AOMaterialMaskTexture;
		// 		ID3D11ShaderResourceView* PrecomputedLighting_IndirectLightingCacheTexture0;
		// 		ID3D11ShaderResourceView* PrecomputedLighting_IndirectLightingCacheTexture1;
		// 		ID3D11ShaderResourceView* PrecomputedLighting_IndirectLightingCacheTexture2;
		// 		ID3D11SamplerState* PrecomputedLighting_SkyOcclusionSampler;
		// 		ID3D11SamplerState* PrecomputedLighting_AOMaterialMaskSampler;
		// 		ID3D11SamplerState* PrecomputedLighting_IndirectLightingCacheTextureSampler0;
		// 		ID3D11SamplerState* PrecomputedLighting_IndirectLightingCacheTextureSampler1;
		// 		ID3D11SamplerState* PrecomputedLighting_IndirectLightingCacheTextureSampler2;
	}
	*/
}
void RenderBasePassView(FViewInfo& View)
{
	/*
	RenderTargets& SceneContext = RenderTargets::Get();
	SceneContext.BeginRenderingGBuffer(true);


	const ParameterAllocation& VSViewParam = BasePassVSParams.at("View");
	const ParameterAllocation& VSPrecomputedLightingParameters = BasePassVSParams.at("PrecomputedLightingParameters");
	D3D11DeviceContext->VSSetConstantBuffers(VSViewParam.BufferIndex, 1, &View.ViewUniformBuffer);
	D3D11DeviceContext->VSSetConstantBuffers(VSPrecomputedLightingParameters.BufferIndex, 1, &PrecomputedLightUniformBuffer);

	D3D11DeviceContext->RSSetState(BasePassRasterizerState);
	D3D11DeviceContext->OMSetBlendState(TStaticBlendState<>::GetRHI(), NULL, 0xffffffff);
	D3D11DeviceContext->RSSetViewports(1, &GViewport);
	D3D11DeviceContext->OMSetDepthStencilState(BasePassDepthStencilState, 0);

	const ParameterAllocation& PSViewParam = BasePassPSParams.at("View");
	const ParameterAllocation& PrecomputedLightingBuffer = BasePassPSParams.at("PrecomputedLightingParameters");
	const ParameterAllocation& BaseColorParam = BasePassPSParams.at("Material_BaseColor");
	const ParameterAllocation& BaseColorSamplerParam = BasePassPSParams.at("Material_BaseColorSampler");
	const ParameterAllocation& LightMapTextureParam = BasePassPSParams.at("PrecomputedLighting_LightMapTexture");
	const ParameterAllocation& LightMapSamplerParam = BasePassPSParams.at("PrecomputedLighting_LightMapSampler");
	const ParameterAllocation& SkyOcclusionTextureParam = BasePassPSParams.at("PrecomputedLighting_SkyOcclusionTexture");
	const ParameterAllocation& SkyOcclusionSamplerParam = BasePassPSParams.at("PrecomputedLighting_SkyOcclusionSampler");
	// 	const ParameterAllocation& StaticShadowTextureParam = BasePassParams["PrecomputedLighting_StaticShadowTexture"];
	// 	const ParameterAllocation& StaticShadowTextureSamplerParam = BasePassParams["PrecomputedLighting_StaticShadowTextureSampler"];

	D3D11DeviceContext->PSSetConstantBuffers(PSViewParam.BufferIndex, 1, &View.ViewUniformBuffer);
	D3D11DeviceContext->PSSetConstantBuffers(PrecomputedLightingBuffer.BufferIndex, 1, &PrecomputedLightUniformBuffer);
	D3D11DeviceContext->PSSetShaderResources(BaseColorParam.BaseIndex, BaseColorParam.Size, &BaseColorSRV);
	D3D11DeviceContext->PSSetSamplers(BaseColorSamplerParam.BaseIndex, BaseColorSamplerParam.Size, &BaseColorSampler);
	D3D11DeviceContext->PSSetShaderResources(LightMapTextureParam.BaseIndex, LightMapTextureParam.Size, &PrecomputedLighting_LightMapTexture);
	D3D11DeviceContext->PSSetSamplers(LightMapSamplerParam.BaseIndex, LightMapSamplerParam.Size, &PrecomputedLighting_LightMapSampler);
	D3D11DeviceContext->PSSetShaderResources(SkyOcclusionTextureParam.BaseIndex, SkyOcclusionTextureParam.Size, &PrecomputedLighting_SkyOcclusionTexture);
	D3D11DeviceContext->PSSetSamplers(SkyOcclusionSamplerParam.BaseIndex, SkyOcclusionSamplerParam.Size, &PrecomputedLighting_SkyOcclusionSampler);
	// 	D3D11DeviceContext->PSSetShaderResources(StaticShadowTextureParam.BaseIndex, StaticShadowTextureParam.Size, &PrecomputedLighting_StaticShadowTexture);
	// 	D3D11DeviceContext->PSSetSamplers(StaticShadowTextureSamplerParam.BaseIndex, StaticShadowTextureSamplerParam.Size, &PrecomputedLighting_StaticShadowTextureSampler);

	const ParameterAllocation& PrimitiveParam = BasePassVSParams.at("Primitive");
	const ParameterAllocation& PSPrimitiveParam = BasePassPSParams.at("Primitive");

	for (MeshBatch& MB : GScene->AllBatches)
	{
		D3D11DeviceContext->IASetInputLayout(MeshInputLayout);
		D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		UINT Stride = sizeof(LocalVertex);
		UINT Offset = 0;
		D3D11DeviceContext->IASetVertexBuffers(0, 1, &MB.VertexBuffer, &Stride, &Offset);
		D3D11DeviceContext->IASetIndexBuffer(MB.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		D3D11DeviceContext->VSSetShader(BasePassVS, 0, 0);
		D3D11DeviceContext->PSSetShader(BasePassPS, 0, 0);

		for (size_t Element = 0; Element < MB.Elements.size(); ++Element)
		{
			D3D11DeviceContext->VSSetConstantBuffers(PrimitiveParam.BufferIndex, 1, &MB.Elements[Element].PrimitiveUniformBuffer);
			D3D11DeviceContext->PSSetConstantBuffers(PSPrimitiveParam.BufferIndex, 1, &MB.Elements[Element].PrimitiveUniformBuffer);
			D3D11DeviceContext->DrawIndexed(MB.Elements[Element].NumTriangles * 3, MB.Elements[Element].FirstIndex, 0);
		}

	}
	*/
}

void SceneRenderer::RenderBasePass()
{
	/*
	SCOPED_DRAW_EVENT_FORMAT(RenderBasePass, TEXT("BasePass"));
	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
	{
		//SCOPED_CONDITIONAL_DRAW_EVENTF(RHICmdList, EventView, Views.Num() > 1, TEXT("View%d"), ViewIndex);
		ViewInfo& View = Views[ViewIndex];
		//SCOPED_GPU_MASK(RHICmdList, View.GPUMask);

		//TUniformBufferRef<FOpaqueBasePassUniformParameters> BasePassUniformBuffer;
		//CreateOpaqueBasePassUniformBuffer(RHICmdList, View, ForwardScreenSpaceShadowMask, BasePassUniformBuffer);
		RenderBasePassView(View);
	}
	*/
}
