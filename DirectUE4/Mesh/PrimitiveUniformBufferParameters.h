#pragma once

#include "D3D11RHI.h"

struct alignas(16) FPrimitiveUniformShaderParameters
{
	Matrix LocalToWorld;
	Matrix WorldToLocal;
	Vector4 ObjectWorldPositionAndRadius; // needed by some materials
	Vector ObjectBounds;
	float LocalToWorldDeterminantSign;
	Vector ActorWorldPosition;
	float DecalReceiverMask;
	float PerObjectGBufferData;
	float UseSingleSampleShadowFromStationaryLights;
	float UseVolumetricLightmapShadowFromStationaryLights;
	float UseEditorDepthTest;
	Vector4 ObjectOrientation;
	Vector4 NonUniformScale;
	Vector4 InvNonUniformScale;
	Vector LocalObjectBoundsMin;
	float PrePadding_Primitive_252;
	Vector LocalObjectBoundsMax;
	uint32 LightingChannelMask;
	float LpvBiasMultiplier;
};

/** Initializes the primitive uniform shader parameters. */
inline FPrimitiveUniformShaderParameters GetPrimitiveUniformShaderParameters(
	const Matrix& LocalToWorld,
	Vector ActorPosition,
	const FBoxSphereBounds& WorldBounds,
	const FBoxSphereBounds& LocalBounds,
	bool bReceivesDecals,
	bool bHasDistanceFieldRepresentation,
	bool bHasCapsuleRepresentation,
	bool bUseSingleSampleShadowFromStationaryLights,
	bool bUseVolumetricLightmap,
	bool bUseEditorDepthTest,
	uint32 LightingChannelMask,
	float LpvBiasMultiplier = 1.0f
)
{
	FPrimitiveUniformShaderParameters Result;
	Result.LocalToWorld = LocalToWorld;
	Result.WorldToLocal = LocalToWorld.Inverse();
	Result.ObjectWorldPositionAndRadius = Vector4(WorldBounds.Origin, WorldBounds.SphereRadius);
	Result.ObjectBounds = WorldBounds.BoxExtent;
	Result.LocalObjectBoundsMin = LocalBounds.GetBoxExtrema(0); // 0 == minimum
	Result.LocalObjectBoundsMax = LocalBounds.GetBoxExtrema(1); // 1 == maximum
	Result.ObjectOrientation = LocalToWorld.GetUnitAxis(EAxis::Z);
	Result.ActorWorldPosition = ActorPosition;
	Result.LightingChannelMask = LightingChannelMask;
	Result.LpvBiasMultiplier = LpvBiasMultiplier;

	{
		// Extract per axis scales from LocalToWorld transform
		Vector4 WorldX = Vector4(LocalToWorld.M[0][0], LocalToWorld.M[0][1], LocalToWorld.M[0][2], 0);
		Vector4 WorldY = Vector4(LocalToWorld.M[1][0], LocalToWorld.M[1][1], LocalToWorld.M[1][2], 0);
		Vector4 WorldZ = Vector4(LocalToWorld.M[2][0], LocalToWorld.M[2][1], LocalToWorld.M[2][2], 0);
		float ScaleX = Vector(WorldX).Size();
		float ScaleY = Vector(WorldY).Size();
		float ScaleZ = Vector(WorldZ).Size();
		Result.NonUniformScale = Vector4(ScaleX, ScaleY, ScaleZ, 0);
		Result.InvNonUniformScale = Vector4(
			ScaleX > KINDA_SMALL_NUMBER ? 1.0f / ScaleX : 0.0f,
			ScaleY > KINDA_SMALL_NUMBER ? 1.0f / ScaleY : 0.0f,
			ScaleZ > KINDA_SMALL_NUMBER ? 1.0f / ScaleZ : 0.0f,
			0.0f
		);
	}

	//Result.LocalToWorldDeterminantSign = Math::FloatSelect(LocalToWorld.RotDeterminant(), 1, -1);
	Result.DecalReceiverMask = bReceivesDecals ? 1.f : 0;
	Result.PerObjectGBufferData = (2 * (int32)bHasCapsuleRepresentation + (int32)bHasDistanceFieldRepresentation) / 3.0f;
	Result.UseSingleSampleShadowFromStationaryLights = bUseSingleSampleShadowFromStationaryLights ? 1.0f : 0.0f;
	Result.UseVolumetricLightmapShadowFromStationaryLights = bUseVolumetricLightmap && bUseSingleSampleShadowFromStationaryLights ? 1.0f : 0.0f;
	Result.UseEditorDepthTest = bUseEditorDepthTest ? 1.f : 0;
	return Result;
}

inline ID3D11Buffer* CreatePrimitiveUniformBufferImmediate(
	const Matrix& LocalToWorld,
	const FBoxSphereBounds& WorldBounds,
	const FBoxSphereBounds& LocalBounds,
	bool bReceivesDecals,
	bool bUseEditorDepthTest,
	float LpvBiasMultiplier = 1.0f
)
{
	FPrimitiveUniformShaderParameters Buffer = GetPrimitiveUniformShaderParameters(LocalToWorld, WorldBounds.Origin, WorldBounds, LocalBounds, bReceivesDecals, false, false, false, false, bUseEditorDepthTest, 1/*GetDefaultLightingChannelMask()*/, LpvBiasMultiplier);
	return CreateConstantBuffer(false, sizeof(Buffer), &Buffer);
}
