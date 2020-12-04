#pragma once

#include "UnrealMathFPU.h"
#include "LightComponent.h"
#include "PrimitiveSceneInfo.h"

class ULightComponent;
class FLightPrimitiveInteraction;
class FPrimitiveSceneProxy;

class FLightSceneInfoCompact
{
public:
	VectorRegister BoundingSphereVector;
	FLinearColor Color;
	// must not be 0
	FLightSceneInfo* LightSceneInfo;
	// e.g. LightType_Directional, LightType_Point or LightType_Spot
	uint32 LightType : LightType_NumBits;
	uint32 bCastDynamicShadow : 1;
	uint32 bCastStaticShadow : 1;
	uint32 bStaticLighting : 1;

	void Init(FLightSceneInfo* InLightSceneInfo);

	/** Default constructor. */
	FLightSceneInfoCompact() :
		LightSceneInfo(NULL)
	{}

	/** Initialization constructor. */
	FLightSceneInfoCompact(FLightSceneInfo* InLightSceneInfo)
	{
		Init(InLightSceneInfo);
	}

	bool AffectsPrimitive(const FBoxSphereBounds& PrimitiveBounds,  const FPrimitiveSceneProxy* PrimitiveSceneProxy) const;
};

/** Information for sorting lights. */
struct FSortedLightSceneInfo
{
	union
	{
		struct
		{
			// Note: the order of these members controls the light sort order!
			// Currently bUsesLightingChannels is the MSB and LightType is LSB
			/** The type of light. */
			uint32 LightType : LightType_NumBits;
			/** Whether the light has a texture profile. */
			uint32 bTextureProfile : 1;
			/** Whether the light uses a light function. */
			uint32 bLightFunction : 1;
			/** Whether the light casts shadows. */
			uint32 bShadowed : 1;
			/** Whether the light uses lighting channels. */
			uint32 bUsesLightingChannels : 1;
		} Fields;
		/** Sort key bits packed into an integer. */
		int32 Packed;
	} SortKey;

	const FLightSceneInfo* LightSceneInfo;

	/** Initialization constructor. */
	explicit FSortedLightSceneInfo(const FLightSceneInfo* InLightSceneInfo)
		: LightSceneInfo(InLightSceneInfo)
	{
		SortKey.Packed = 0;
	}
};

class FLightSceneInfo
{
public:
	class FLightSceneProxy* Proxy;

	FLightPrimitiveInteraction* DynamicInteractionOftenMovingPrimitiveList;

	FLightPrimitiveInteraction* DynamicInteractionStaticPrimitiveList;

	class FScene* Scene;

	int32 Id;

	int32 OctreeId;

	int32 NumUnbuiltInteractions;

	FLightSceneInfo(FLightSceneProxy* InProxy);
	virtual ~FLightSceneInfo();

	void AddToScene();

	void CreateLightPrimitiveInteraction(const FLightSceneInfoCompact& LightSceneInfoCompact, const FPrimitiveSceneInfoCompact& PrimitiveSceneInfoCompact);

	void RemoveFromScene();

	void Detach();

	int32 GetDynamicShadowMapChannel() const
	{
		if (Proxy->HasStaticShadowing())
		{
			// Stationary lights get a channel assigned by ReassignStationaryLightChannels
			return Proxy->GetPreviewShadowMapChannel();
		}

		// Movable lights get a channel assigned when they are added to the scene
		return DynamicShadowMapChannel;
	}
	bool IsPrecomputedLightingValid() const;
protected:
	int32 DynamicShadowMapChannel;
	uint32 bPrecomputedLightingIsValid : 1;
};