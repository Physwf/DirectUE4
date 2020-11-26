#pragma once

#include "UnrealMath.h"
#include "PrimitiveUniformBufferParameters.h"
#include "SceneComponent.h"

#include <vector>

struct FPrimitiveViewRelevance
{
	uint32 bStaticRelevance : 1;
	uint32 bDynamicRelevance : 1;
};

class FSceneView;
class FStaticMesh;
class FSceneViewFamily;
class FScene;
class AActor;
class FPrimitiveSceneProxy;

class UPrimitiveComponent : public USceneComponent
{
	friend class FScene;
public:
	UPrimitiveComponent(AActor* InOwner);
	virtual ~UPrimitiveComponent() {}

	virtual void Register() override;
	virtual void Unregister() override;

	virtual FMatrix GetRenderMatrix() const;

	virtual FPrimitiveSceneProxy* CreateSceneProxy()
	{
		return NULL;
	}

	virtual void SendRenderTransform_Concurrent() override;

	FPrimitiveSceneProxy* SceneProxy;
private:
};