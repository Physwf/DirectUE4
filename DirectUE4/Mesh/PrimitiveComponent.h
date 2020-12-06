#pragma once

#include "UnrealMath.h"
#include "PrimitiveUniformBufferParameters.h"
#include "SceneComponent.h"

#include <vector>


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

	virtual FMatrix GetRenderMatrix() const;

	virtual FPrimitiveSceneProxy* CreateSceneProxy()
	{
		return NULL;
	}
	virtual class UMaterial* GetMaterial(int32 ElementIndex) const = 0;

	FPrimitiveSceneProxy* SceneProxy;
protected:
	virtual void CreateRenderState_Concurrent() override;
	virtual void SendRenderTransform_Concurrent() override;
	virtual void DestroyRenderState_Concurrent() override;
};