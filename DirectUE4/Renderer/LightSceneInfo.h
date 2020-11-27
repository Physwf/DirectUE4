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

class FLightSceneInfo
{
public:
	class FLightSceneProxy* Proxy;

	FLightPrimitiveInteraction* DynamicInteractionOftenMovingPrimitiveList;

	FLightPrimitiveInteraction* DynamicInteractionStaticPrimitiveList;

	class FScene* Scene;

	int32 Id;

	FLightSceneInfo(FLightSceneProxy* InProxy);
	virtual ~FLightSceneInfo();

	void AddToScene();

	void CreateLightPrimitiveInteraction(const FLightSceneInfoCompact& LightSceneInfoCompact, const FPrimitiveSceneInfoCompact& PrimitiveSceneInfoCompact);

	void RemoveFromScene();

};