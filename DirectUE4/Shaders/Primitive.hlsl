//Primitive
#ifndef __UniformBuffer_Primitive_Definition__
#define __UniformBuffer_Primitive_Definition__
cbuffer	Primitive : register(b1)
{
    float4x4 Primitive_LocalToWorld;
	float4x4 Primitive_WorldToLocal;
	float4 Primitive_ObjectWorldPositionAndRadius;
	float3 Primitive_ObjectBounds;
	float Primitive_LocalToWorldDeterminantSign;
	float3 Primitive_ActorWorldPosition;
	float Primitive_DecalReceiverMask;
	float Primitive_PerObjectGBufferData;
	float Primitive_UseSingleSampleShadowFromStationaryLights;
	float Primitive_UseVolumetricLightmapShadowFromStationaryLights;
	float Primitive_UseEditorDepthTest;
	float4 Primitive_ObjectOrientation;
	float4 Primitive_NonUniformScale;
	float4 Primitive_InvNonUniformScale;
	float3 Primitive_LocalObjectBoundsMin;
	float PrePadding_Primitive_252;
	float3 Primitive_LocalObjectBoundsMax;
	uint Primitive_LightingChannelMask;
	float Primitive_LpvBiasMultiplier;
};
static const struct
{
    float4x4 LocalToWorld;
	float4x4 WorldToLocal;
	float4 ObjectWorldPositionAndRadius;
	float3 ObjectBounds;
	float LocalToWorldDeterminantSign;
	float3 ActorWorldPosition;
	float DecalReceiverMask;
	float PerObjectGBufferData;
	float UseSingleSampleShadowFromStationaryLights;
	float UseVolumetricLightmapShadowFromStationaryLights;
	float UseEditorDepthTest;
	float4 ObjectOrientation;
	float4 NonUniformScale;
	float4 InvNonUniformScale;
	float3 LocalObjectBoundsMin;
	float3 LocalObjectBoundsMax;
	uint LightingChannelMask;
	float LpvBiasMultiplier;
} Primitive = { 
	Primitive_LocalToWorld, 
	Primitive_WorldToLocal, 
	Primitive_ObjectWorldPositionAndRadius,
	Primitive_ObjectBounds,
	Primitive_LocalToWorldDeterminantSign,
	Primitive_ActorWorldPosition,
	Primitive_DecalReceiverMask,
	Primitive_PerObjectGBufferData, 
	Primitive_UseSingleSampleShadowFromStationaryLights,
	Primitive_UseVolumetricLightmapShadowFromStationaryLights, 
	Primitive_UseEditorDepthTest,
	Primitive_ObjectOrientation,
	Primitive_NonUniformScale,
	Primitive_InvNonUniformScale,
	Primitive_LocalObjectBoundsMin,
	Primitive_LocalObjectBoundsMax,
	Primitive_LightingChannelMask,
	Primitive_LpvBiasMultiplier
};
#endif