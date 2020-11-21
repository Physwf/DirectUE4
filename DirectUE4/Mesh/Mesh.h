#pragma once

#include "UnrealMath.h"
#include "PrimitiveUniformBufferParameters.h"
#include <vector>

struct FPrimitiveViewRelevance
{
	uint32 bStaticRelevance : 1;
	uint32 bDynamicRelevance : 1;
};

class FSceneView;
class FStaticMesh;
class SceneViewFamily;
class FScene;

class MeshPrimitive
{
	friend class FScene;

public:
	MeshPrimitive();
	virtual ~MeshPrimitive() {}

	virtual void Register(FScene* Scene);
	virtual void UnRegister(FScene* Scene);

	virtual void InitResources();
	virtual void ReleaseResources();

	void ConditionalUpdateUniformBuffer();

	void UpdateUniformBuffer();
	//FPrimitiveSceneInfo
	/** Adds the primitive to the scene. */
	void AddToScene(FScene* Scene/*FRHICommandListImmediate& RHICmdList, bool bUpdateStaticDrawLists, bool bAddToStaticDrawLists = true*/);
	/** Removes the primitive from the scene. */
	void RemoveFromScene(FScene* Scene/*bool bUpdateStaticDrawLists*/);
	/** Adds the primitive's static meshes to the scene. */
	void AddStaticMeshes(FScene* Scene/*FRHICommandListImmediate& RHICmdList, bool bUpdateStaticDrawLists = true*/);
	/** Removes the primitive's static meshes from the scene. */
	void RemoveStaticMeshes(FScene* Scene);

	//FPrimitiveSceneProxy
	virtual void DrawStaticElements(/*FStaticPrimitiveDrawInterface* PDI*/) {};
	virtual void GetDynamicMeshElements(const std::vector<const FSceneView*>& Views, const SceneViewFamily& ViewFamily, uint32 VisibilityMap/*, FMeshElementCollector& Collector*/) const = 0;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const = 0;
	//FPrimitiveSceneProxy
	inline const FMatrix& GetLocalToWorld() const { return LocalToWorld; }
	//inline const FBoxSphereBounds& GetBounds() const { return Bounds; }
	//inline const FBoxSphereBounds& GetLocalBounds() const { return LocalBounds; }
	inline const TUniformBuffer<FPrimitiveUniformShaderParameters>& GetUniformBuffer() const
	{
		return UniformBuffer;
	}
	//FPrimitiveSceneInfo
	/** The primitive's static meshes. */
	std::vector<FStaticMesh*> StaticMeshes;

private:
	void SetTransform(const FMatrix& InLocalToWorld, const FBoxSphereBounds& InBounds, const FBoxSphereBounds& InLocalBounds, FVector InActorPosition);

	FMatrix LocalToWorld;

	/** The primitive's bounds. */
	FBoxSphereBounds Bounds;

	/** The primitive's local space bounds. */
	FBoxSphereBounds LocalBounds;

	/** The component's actor's position. */
	FVector ActorPosition;

	uint32 bIsLocalToWorldDeterminantNegative : 1;

	bool bNeedsUniformBufferUpdate;
	TUniformBuffer<FPrimitiveUniformShaderParameters> UniformBuffer;
};