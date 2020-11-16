#pragma once

#include "D3D11RHI.h"

struct alignas(16) FPrimitiveUniformShaderParameters
{
	struct ConstantStruct
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
	} Constants;

	static std::map<std::string, ComPtr<ID3D11ShaderResourceView>> GetSRVs(const FPrimitiveUniformShaderParameters& Primitive)
	{
		std::map<std::string, ComPtr<ID3D11ShaderResourceView>> List;
		return List;
	}
	static std::map<std::string, ComPtr<ID3D11SamplerState>> GetSamplers(const FPrimitiveUniformShaderParameters& Primitive)
	{
		std::map<std::string, ComPtr<ID3D11SamplerState>> List;
		return List;
	}
	static std::map<std::string, ComPtr<ID3D11UnorderedAccessView>> GetUAVs(const FPrimitiveUniformShaderParameters& Primitive)
	{
		std::map<std::string, ComPtr<ID3D11UnorderedAccessView>> List;
		return List;
	}
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
	Result.Constants.LocalToWorld = LocalToWorld;
	Result.Constants.WorldToLocal = LocalToWorld.Inverse();
	Result.Constants.ObjectWorldPositionAndRadius = Vector4(WorldBounds.Origin, WorldBounds.SphereRadius);
	Result.Constants.ObjectBounds = WorldBounds.BoxExtent;
	Result.Constants.LocalObjectBoundsMin = LocalBounds.GetBoxExtrema(0); // 0 == minimum
	Result.Constants.LocalObjectBoundsMax = LocalBounds.GetBoxExtrema(1); // 1 == maximum
	Result.Constants.ObjectOrientation = LocalToWorld.GetUnitAxis(EAxis::Z);
	Result.Constants.ActorWorldPosition = ActorPosition;
	Result.Constants.LightingChannelMask = LightingChannelMask;
	Result.Constants.LpvBiasMultiplier = LpvBiasMultiplier;

	{
		// Extract per axis scales from LocalToWorld transform
		Vector4 WorldX = Vector4(LocalToWorld.M[0][0], LocalToWorld.M[0][1], LocalToWorld.M[0][2], 0);
		Vector4 WorldY = Vector4(LocalToWorld.M[1][0], LocalToWorld.M[1][1], LocalToWorld.M[1][2], 0);
		Vector4 WorldZ = Vector4(LocalToWorld.M[2][0], LocalToWorld.M[2][1], LocalToWorld.M[2][2], 0);
		float ScaleX = Vector(WorldX).Size();
		float ScaleY = Vector(WorldY).Size();
		float ScaleZ = Vector(WorldZ).Size();
		Result.Constants.NonUniformScale = Vector4(ScaleX, ScaleY, ScaleZ, 0);
		Result.Constants.InvNonUniformScale = Vector4(
			ScaleX > KINDA_SMALL_NUMBER ? 1.0f / ScaleX : 0.0f,
			ScaleY > KINDA_SMALL_NUMBER ? 1.0f / ScaleY : 0.0f,
			ScaleZ > KINDA_SMALL_NUMBER ? 1.0f / ScaleZ : 0.0f,
			0.0f
		);
	}

	//Result.LocalToWorldDeterminantSign = Math::FloatSelect(LocalToWorld.RotDeterminant(), 1, -1);
	Result.Constants.DecalReceiverMask = bReceivesDecals ? 1.f : 0;
	Result.Constants.PerObjectGBufferData = (2 * (int32)bHasCapsuleRepresentation + (int32)bHasDistanceFieldRepresentation) / 3.0f;
	Result.Constants.UseSingleSampleShadowFromStationaryLights = bUseSingleSampleShadowFromStationaryLights ? 1.0f : 0.0f;
	Result.Constants.UseVolumetricLightmapShadowFromStationaryLights = bUseVolumetricLightmap && bUseSingleSampleShadowFromStationaryLights ? 1.0f : 0.0f;
	Result.Constants.UseEditorDepthTest = bUseEditorDepthTest ? 1.f : 0;
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
