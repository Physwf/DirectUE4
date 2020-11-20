/*
LightMapRendering.h
BEGIN_UNIFORM_BUFFER_STRUCT(FPrecomputedLightingParameters, )
	UNIFORM_MEMBER(FVector, IndirectLightingCachePrimitiveAdd) // FCachedVolumeIndirectLightingPolicy
	UNIFORM_MEMBER(FVector, IndirectLightingCachePrimitiveScale) // FCachedVolumeIndirectLightingPolicy
	UNIFORM_MEMBER(FVector, IndirectLightingCacheMinUV) // FCachedVolumeIndirectLightingPolicy
	UNIFORM_MEMBER(FVector, IndirectLightingCacheMaxUV) // FCachedVolumeIndirectLightingPolicy
	UNIFORM_MEMBER(FVector4, PointSkyBentNormal) // FCachedPointIndirectLightingPolicy
	UNIFORM_MEMBER_EX(float, DirectionalLightShadowing, EShaderPrecisionModifier::Half) // FCachedPointIndirectLightingPolicy
	UNIFORM_MEMBER(FVector4, StaticShadowMapMasks) // TDistanceFieldShadowsAndLightMapPolicy
	UNIFORM_MEMBER(FVector4, InvUniformPenumbraSizes) // TDistanceFieldShadowsAndLightMapPolicy
	UNIFORM_MEMBER_ARRAY(FVector4, IndirectLightingSHCoefficients0, [3]) // FCachedPointIndirectLightingPolicy
	UNIFORM_MEMBER_ARRAY(FVector4, IndirectLightingSHCoefficients1, [3]) // FCachedPointIndirectLightingPolicy
	UNIFORM_MEMBER(FVector4,	IndirectLightingSHCoefficients2) // FCachedPointIndirectLightingPolicy
	UNIFORM_MEMBER_EX(FVector4, IndirectLightingSHSingleCoefficient, EShaderPrecisionModifier::Half) // FCachedPointIndirectLightingPolicy used in forward Translucent
	UNIFORM_MEMBER(FVector4, LightMapCoordinateScaleBias) // TLightMapPolicy
	UNIFORM_MEMBER(FVector4, ShadowMapCoordinateScaleBias) // TDistanceFieldShadowsAndLightMapPolicy
	UNIFORM_MEMBER_ARRAY_EX(FVector4, LightMapScale, [MAX_NUM_LIGHTMAP_COEF], EShaderPrecisionModifier::Half) // TLightMapPolicy
	UNIFORM_MEMBER_ARRAY_EX(FVector4, LightMapAdd, [MAX_NUM_LIGHTMAP_COEF], EShaderPrecisionModifier::Half) // TLightMapPolicy
	UNIFORM_MEMBER_TEXTURE(Texture2D, LightMapTexture) // TLightMapPolicy
	UNIFORM_MEMBER_TEXTURE(Texture2D, SkyOcclusionTexture) // TLightMapPolicy
	UNIFORM_MEMBER_TEXTURE(Texture2D, AOMaterialMaskTexture) // TLightMapPolicy
	UNIFORM_MEMBER_TEXTURE(Texture3D, IndirectLightingCacheTexture0) // FCachedVolumeIndirectLightingPolicy
	UNIFORM_MEMBER_TEXTURE(Texture3D, IndirectLightingCacheTexture1) // FCachedVolumeIndirectLightingPolicy
	UNIFORM_MEMBER_TEXTURE(Texture3D, IndirectLightingCacheTexture2) // FCachedVolumeIndirectLightingPolicy
	UNIFORM_MEMBER_TEXTURE(Texture2D, StaticShadowTexture) // 
	UNIFORM_MEMBER_SAMPLER(SamplerState, LightMapSampler) // TLightMapPolicy
	UNIFORM_MEMBER_SAMPLER(SamplerState, SkyOcclusionSampler) // TLightMapPolicy
	UNIFORM_MEMBER_SAMPLER(SamplerState, AOMaterialMaskSampler) // TLightMapPolicy
	UNIFORM_MEMBER_SAMPLER(SamplerState, IndirectLightingCacheTextureSampler0) // FCachedVolumeIndirectLightingPolicy
	UNIFORM_MEMBER_SAMPLER(SamplerState, IndirectLightingCacheTextureSampler1) // FCachedVolumeIndirectLightingPolicy
	UNIFORM_MEMBER_SAMPLER(SamplerState, IndirectLightingCacheTextureSampler2) // FCachedVolumeIndirectLightingPolicy
	UNIFORM_MEMBER_SAMPLER(SamplerState, StaticShadowTextureSampler) // TDistanceFieldShadowsAndLightMapPolicy
END_UNIFORM_BUFFER_STRUCT( FPrecomputedLightingParameters )
*/

#ifndef __UniformBuffer_PrecomputedLighting_Definition__
#define __UniformBuffer_PrecomputedLighting_Definition__

cbuffer PrecomputedLightingParameters
{
    float3 PrecomputedLighting_IndirectLightingCachePrimitiveAdd;// FCachedVolumeIndirectLightingPolicy
    float Pading01;
    float3 PrecomputedLighting_IndirectLightingCachePrimitiveScale;// FCachedVolumeIndirectLightingPolicy
    float Pading02;
    float3 PrecomputedLighting_IndirectLightingCacheMinUV;// FCachedVolumeIndirectLightingPolicy
    float Pading03;
    float3 PrecomputedLighting_IndirectLightingCacheMaxUV;// FCachedVolumeIndirectLightingPolicy
    float Pading04;
    float4 PrecomputedLighting_PointSkyBentNormal;// FCachedPointIndirectLightingPolicy
    float PrecomputedLighting_DirectionalLightShadowing;// FCachedPointIndirectLightingPolicy
    float3 Pading05;
    float4 PrecomputedLighting_StaticShadowMapMasks;// TDistanceFieldShadowsAndLightMapPolicy
    float4 PrecomputedLighting_InvUniformPenumbraSizes;// TDistanceFieldShadowsAndLightMapPolicy
    float4 PrecomputedLighting_IndirectLightingSHCoefficients0[3]; // FCachedPointIndirectLightingPolicy
    float4 PrecomputedLighting_IndirectLightingSHCoefficients1[3]; // FCachedPointIndirectLightingPolicy
    float4 PrecomputedLighting_IndirectLightingSHCoefficients2;// FCachedPointIndirectLightingPolicy
    float4 PrecomputedLighting_IndirectLightingSHSingleCoefficient;// FCachedPointIndirectLightingPolicy
    float4 PrecomputedLighting_LightMapCoordinateScaleBias; // TLightMapPolicy
    float4 PrecomputedLighting_ShadowMapCoordinateScaleBias;// TDistanceFieldShadowsAndLightMapPolicy
    float4 PrecomputedLighting_LightMapScale[2];// TLightMapPolicy
    float4 PrecomputedLighting_LightMapAdd[2];// TLightMapPolicy
};

Texture2D PrecomputedLighting_LightMapTexture;// TLightMapPolicy
Texture2D PrecomputedLighting_SkyOcclusionTexture;// TLightMapPolicy
Texture2D PrecomputedLighting_AOMaterialMaskTexture;// TLightMapPolicy
Texture2D PrecomputedLighting_IndirectLightingCacheTexture0;// FCachedVolumeIndirectLightingPolicy
Texture2D PrecomputedLighting_IndirectLightingCacheTexture1;// FCachedVolumeIndirectLightingPolicy
Texture2D PrecomputedLighting_IndirectLightingCacheTexture2;// FCachedVolumeIndirectLightingPolicy
Texture2D PrecomputedLighting_StaticShadowTexture;
SamplerState PrecomputedLighting_LightMapSampler;// TLightMapPolicy
SamplerState PrecomputedLighting_SkyOcclusionSampler;// TLightMapPolicy
SamplerState PrecomputedLighting_AOMaterialMaskSampler;// TLightMapPolicy
SamplerState PrecomputedLighting_IndirectLightingCacheTextureSampler0;// FCachedVolumeIndirectLightingPolicy
SamplerState PrecomputedLighting_IndirectLightingCacheTextureSampler1;// FCachedVolumeIndirectLightingPolicy
SamplerState PrecomputedLighting_IndirectLightingCacheTextureSampler2;// FCachedVolumeIndirectLightingPolicy
SamplerState PrecomputedLighting_StaticShadowTextureSampler;// TDistanceFieldShadowsAndLightMapPolicy

static const struct 
{
    float3 IndirectLightingCachePrimitiveAdd;
    float3 IndirectLightingCachePrimitiveScale;
    float3 IndirectLightingCacheMinUV;
    float3 IndirectLightingCacheMaxUV;
    float4 PointSkyBentNormal;
    float DirectionalLightShadowing;
    float4 StaticShadowMapMasks;
    float4 InvUniformPenumbraSizes;
    float4 IndirectLightingSHCoefficients0[3];
    float4 IndirectLightingSHCoefficients1[3];
    float4 IndirectLightingSHCoefficients2;
    float4 IndirectLightingSHSingleCoefficient;
    float4 LightMapCoordinateScaleBias;
    float4 ShadowMapCoordinateScaleBias;
    float4 LightMapScale[2];
    float4 LightMapAdd[2];
    Texture2D LightMapTexture;
    Texture2D SkyOcclusionTexture;
    Texture2D AOMaterialMaskTexture;
    Texture2D IndirectLightingCacheTexture0;
    Texture2D IndirectLightingCacheTexture1;
    Texture2D IndirectLightingCacheTexture2;
    Texture2D StaticShadowTexture;
    SamplerState LightMapSampler;
    SamplerState SkyOcclusionSampler;
    SamplerState AOMaterialMaskSampler;
    SamplerState IndirectLightingCacheTextureSampler0;
    SamplerState IndirectLightingCacheTextureSampler1;
    SamplerState IndirectLightingCacheTextureSampler2;
    SamplerState StaticShadowTextureSampler;
} PrecomputedLightingBuffer =
{
    PrecomputedLighting_IndirectLightingCachePrimitiveAdd,// FCachedVolumeIndirectLightingPolicy
    PrecomputedLighting_IndirectLightingCachePrimitiveScale,// FCachedVolumeIndirectLightingPolicy
    PrecomputedLighting_IndirectLightingCacheMinUV,// FCachedVolumeIndirectLightingPolicy
    PrecomputedLighting_IndirectLightingCacheMaxUV,// FCachedVolumeIndirectLightingPolicy
    PrecomputedLighting_PointSkyBentNormal,// FCachedPointIndirectLightingPolicy
    PrecomputedLighting_DirectionalLightShadowing,// FCachedPointIndirectLightingPolicy
    PrecomputedLighting_StaticShadowMapMasks,// TDistanceFieldShadowsAndLightMapPolicy
    PrecomputedLighting_InvUniformPenumbraSizes,// TDistanceFieldShadowsAndLightMapPolicy
    {
        PrecomputedLighting_IndirectLightingSHCoefficients0[0], // FCachedPointIndirectLightingPolicy
        PrecomputedLighting_IndirectLightingSHCoefficients0[1], // FCachedPointIndirectLightingPolicy
        PrecomputedLighting_IndirectLightingSHCoefficients0[2], // FCachedPointIndirectLightingPolicy
    },
    {
        PrecomputedLighting_IndirectLightingSHCoefficients1[0], // FCachedPointIndirectLightingPolicy
        PrecomputedLighting_IndirectLightingSHCoefficients1[1], // FCachedPointIndirectLightingPolicy
        PrecomputedLighting_IndirectLightingSHCoefficients1[2], // FCachedPointIndirectLightingPolicy
    },
    PrecomputedLighting_IndirectLightingSHCoefficients2,// FCachedPointIndirectLightingPolicy
    PrecomputedLighting_IndirectLightingSHSingleCoefficient,// FCachedPointIndirectLightingPolicy
    PrecomputedLighting_LightMapCoordinateScaleBias, // TLightMapPolicy
    PrecomputedLighting_ShadowMapCoordinateScaleBias,// TDistanceFieldShadowsAndLightMapPolicy
    { 
        PrecomputedLighting_LightMapScale[0],// TLightMapPolicy
        PrecomputedLighting_LightMapScale[1],// TLightMapPolicy
    },
    { 
        PrecomputedLighting_LightMapAdd[0],// TLightMapPolicy
        PrecomputedLighting_LightMapAdd[1],// TLightMapPolicy
    }, 
    PrecomputedLighting_LightMapTexture,// TLightMapPolicy
    PrecomputedLighting_SkyOcclusionTexture,// TLightMapPolicy
    PrecomputedLighting_AOMaterialMaskTexture,// TLightMapPolicy
    PrecomputedLighting_IndirectLightingCacheTexture0,// FCachedVolumeIndirectLightingPolicy
    PrecomputedLighting_IndirectLightingCacheTexture1,// FCachedVolumeIndirectLightingPolicy
    PrecomputedLighting_IndirectLightingCacheTexture2,// FCachedVolumeIndirectLightingPolicy
    PrecomputedLighting_StaticShadowTexture,
    PrecomputedLighting_LightMapSampler,// TLightMapPolicy
    PrecomputedLighting_SkyOcclusionSampler,// TLightMapPolicy
    PrecomputedLighting_AOMaterialMaskSampler,// TLightMapPolicy
    PrecomputedLighting_IndirectLightingCacheTextureSampler0,// FCachedVolumeIndirectLightingPolicy
    PrecomputedLighting_IndirectLightingCacheTextureSampler1,// FCachedVolumeIndirectLightingPolicy
    PrecomputedLighting_IndirectLightingCacheTextureSampler2,// FCachedVolumeIndirectLightingPolicy
    PrecomputedLighting_StaticShadowTextureSampler,// TDistanceFieldShadowsAndLightMapPolicy
};

#endif