#pragma once

#include "D3D11RHI.h"

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

struct alignas(16) FPrecomputedLightingParameters
{
	FPrecomputedLightingParameters()
	{
		ConstructUniformBufferInfo(*this);
	}

	struct ConstantStruct
	{
		FVector IndirectLightingCachePrimitiveAdd; // FCachedVolumeIndirectLightingPolicy
		float Padding001;
		FVector IndirectLightingCachePrimitiveScale; // FCachedVolumeIndirectLightingPolicy
		float Padding002;
		FVector IndirectLightingCacheMinUV; // FCachedVolumeIndirectLightingPolicy
		float Padding003;
		FVector IndirectLightingCacheMaxUV; // FCachedVolumeIndirectLightingPolicy
		float Padding004;
		Vector4 PointSkyBentNormal; // FCachedPointIndirectLightingPolicy
		float DirectionalLightShadowing;// EShaderPrecisionModifier::Half // FCachedPointIndirectLightingPolicy
		FVector Pading005;
		Vector4 StaticShadowMapMasks; // TDistanceFieldShadowsAndLightMapPolicy
		Vector4 InvUniformPenumbraSizes; // TDistanceFieldShadowsAndLightMapPolicy
		Vector4 IndirectLightingSHCoefficients0[3]; // FCachedPointIndirectLightingPolicy
		Vector4 IndirectLightingSHCoefficients1[3]; // FCachedPointIndirectLightingPolicy
		Vector4 IndirectLightingSHCoefficients2; // FCachedPointIndirectLightingPolicy
		Vector4 IndirectLightingSHSingleCoefficient;// EShaderPrecisionModifier::Half; // FCachedPointIndirectLightingPolicy used in forward Translucent
		Vector4 LightMapCoordinateScaleBias; // TLightMapPolicy
		Vector4 ShadowMapCoordinateScaleBias; // TDistanceFieldShadowsAndLightMapPolicy
		Vector4 LightMapScale[MAX_NUM_LIGHTMAP_COEF];// EShaderPrecisionModifier::Half; // TLightMapPolicy
		Vector4 LightMapAdd[MAX_NUM_LIGHTMAP_COEF];// EShaderPrecisionModifier::Half; // TLightMapPolicy
	};

	ComPtr<ID3D11ShaderResourceView> LightMapTexture; // TLightMapPolicy
	ComPtr<ID3D11ShaderResourceView> SkyOcclusionTexture; // TLightMapPolicy
	ComPtr<ID3D11ShaderResourceView> AOMaterialMaskTexture; // TLightMapPolicy
	ComPtr<ID3D11ShaderResourceView> IndirectLightingCacheTexture0; //Texture3D  FCachedVolumeIndirectLightingPolicy
	ComPtr<ID3D11ShaderResourceView> IndirectLightingCacheTexture1; //Texture3D FCachedVolumeIndirectLightingPolicy
	ComPtr<ID3D11ShaderResourceView> IndirectLightingCacheTexture2; //Texture3D FCachedVolumeIndirectLightingPolicy
	ComPtr<ID3D11ShaderResourceView> StaticShadowTexture; // 
	ComPtr<ID3D11SamplerState> LightMapSampler; // TLightMapPolicy
	ComPtr<ID3D11SamplerState> SkyOcclusionSampler; // TLightMapPolicy
	ComPtr<ID3D11SamplerState> AOMaterialMaskSampler; // TLightMapPolicy
	ComPtr<ID3D11SamplerState> IndirectLightingCacheTextureSampler0; // FCachedVolumeIndirectLightingPolicy
	ComPtr<ID3D11SamplerState> IndirectLightingCacheTextureSampler1; // FCachedVolumeIndirectLightingPolicy
	ComPtr<ID3D11SamplerState> IndirectLightingCacheTextureSampler2; // FCachedVolumeIndirectLightingPolicy
	ComPtr<ID3D11SamplerState> StaticShadowTextureSampler; // TDistanceFieldShadowsAndLightMapPolicy

	static std::string GetConstantBufferName()
	{
		return "PrecomputedLightingBuffer";
	}
#define ADD_RES(StructName, MemberName) List.insert(std::make_pair(std::string(#StructName) + "_" + std::string(#MemberName),StructName.MemberName))
	static std::map<std::string, ComPtr<ID3D11ShaderResourceView>> GetSRVs(const FPrecomputedLightingParameters& PrecomputedLightingBuffer)
	{
		std::map<std::string, ComPtr<ID3D11ShaderResourceView>> List;
		ADD_RES(PrecomputedLightingBuffer, LightMapTexture);
		ADD_RES(PrecomputedLightingBuffer, SkyOcclusionTexture);
		ADD_RES(PrecomputedLightingBuffer, AOMaterialMaskTexture);
		ADD_RES(PrecomputedLightingBuffer, IndirectLightingCacheTexture0);
		ADD_RES(PrecomputedLightingBuffer, IndirectLightingCacheTexture1);
		ADD_RES(PrecomputedLightingBuffer, IndirectLightingCacheTexture2);
		ADD_RES(PrecomputedLightingBuffer, StaticShadowTexture);
		return List;
	}
	static std::map<std::string, ComPtr<ID3D11SamplerState>> GetSamplers(const FPrecomputedLightingParameters& PrecomputedLightingBuffer)
	{
		std::map<std::string, ComPtr<ID3D11SamplerState>> List;
		ADD_RES(PrecomputedLightingBuffer, LightMapSampler);
		ADD_RES(PrecomputedLightingBuffer, SkyOcclusionSampler);
		ADD_RES(PrecomputedLightingBuffer, AOMaterialMaskSampler);
		ADD_RES(PrecomputedLightingBuffer, IndirectLightingCacheTextureSampler0);
		ADD_RES(PrecomputedLightingBuffer, IndirectLightingCacheTextureSampler1);
		ADD_RES(PrecomputedLightingBuffer, IndirectLightingCacheTextureSampler2);
		ADD_RES(PrecomputedLightingBuffer, StaticShadowTextureSampler);
		return List;
	}
	static std::map<std::string, ComPtr<ID3D11UnorderedAccessView>> GetUAVs(const FPrecomputedLightingParameters& PrecomputedLightingBuffer)
	{
		std::map<std::string, ComPtr<ID3D11UnorderedAccessView>> List;
		return List;
	}
#undef ADD_RES
};
