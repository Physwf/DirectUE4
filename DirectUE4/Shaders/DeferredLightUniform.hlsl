//DeferredLightUniform
#ifndef __UniformBuffer_DeferredLightUniform_Definition__
#define __UniformBuffer_DeferredLightUniform_Definition__

cbuffer DeferredLightUniform : register(b2)
{
    float3 DeferredLightUniform_LightPosition;
    float DeferredLightUniform_LightInvRadius;
    float3 DeferredLightUniform_LightColor;
    float DeferredLightUniform_LightFalloffExponent;
    float3 DeferredLightUniform_NormalizedLightDirection;
    float DeferredLightUniformsPading01;
    float3 DeferredLightUniform_NormalizedLightTangent;
    float DeferredLightUniformsPading02;
    float2 DeferredLightUniform_SpotAngles;
    float DeferredLightUniform_SpecularScale;
    float DeferredLightUniform_SourceRadius;
    float DeferredLightUniform_SoftSourceRadius;
    float DeferredLightUniform_SourceLength;
    float DeferredLightUniform_ContactShadowLength;
    float DeferredLightUniformsPading03;
    float2 DeferredLightUniform_DistanceFadeMAD;
    float DeferredLightUniformsPading04;
	float DeferredLightUniformsPading05;
    float4 DeferredLightUniform_ShadowMapChannelMask;
    uint DeferredLightUniform_ShadowedBits;
    uint DeferredLightUniform_LightingChannelMask;
    float DeferredLightUniform_VolumetricScatteringIntensity;
    float DeferredLightUniformsPading06;
};
Texture2D DeferredLightUniform_VSourceTexture;
static const struct
{
    float3 LightPosition;
    float LightInvRadius;
    float3 LightColor;
    float LightFalloffExponent;
    float3 NormalizedLightDirection;
    float3 NormalizedLightTangent;
    float2 SpotAngles;
    float SpecularScale;
    float SourceRadius;
    float SoftSourceRadius;
    float SourceLength;
    float ContactShadowLength;
    float2 DistanceFadeMAD;
    float4 ShadowMapChannelMask;
    uint ShadowedBits;
    uint LightingChannelMask;
    float VolumetricScatteringIntensity;
    Texture2D VSourceTexture;
} DeferredLightUniform =
{
    DeferredLightUniform_LightPosition,
    DeferredLightUniform_LightInvRadius,
    DeferredLightUniform_LightColor,
    DeferredLightUniform_LightFalloffExponent,
    DeferredLightUniform_NormalizedLightDirection,
    DeferredLightUniform_NormalizedLightTangent,
    DeferredLightUniform_SpotAngles,
    DeferredLightUniform_SpecularScale,
    DeferredLightUniform_SourceRadius,
    DeferredLightUniform_SoftSourceRadius,
    DeferredLightUniform_SourceLength,
    DeferredLightUniform_ContactShadowLength,
    DeferredLightUniform_DistanceFadeMAD,
    DeferredLightUniform_ShadowMapChannelMask,
    DeferredLightUniform_ShadowedBits,
    DeferredLightUniform_LightingChannelMask,
    DeferredLightUniform_VolumetricScatteringIntensity,
    DeferredLightUniform_VSourceTexture
};
#endif
