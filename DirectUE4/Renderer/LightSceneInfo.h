#pragma once

#include "UnrealMath.h"
#include "LightComponent.h"

class ULightComponent;

class FLightSceneInfoCompact
{
public:
	class FLightSceneInfo * LightSceneInfo;

	uint32 LightType : LightType_NumBits;
};

class FLightSceneInfo
{
public:
	class FLightSceneProxy* Proxy;

	class FScene* Scene;


	FLightSceneInfo(FLightSceneProxy* InProxy);
	virtual ~FLightSceneInfo();

	void AddToScene();
	void RemoveFromScene();

};