#pragma once

#include "UnrealMath.h"
#include "PrimitiveSceneProxy.h"

#include <vector>

class FPrimitiveSceneInfo;
class FPrimitiveSceneProxy;
class FScene;
class FViewInfo;
class UPrimitiveComponent;
class FStaticMesh;

/** Flags needed for shadow culling.  These are pulled out of the FPrimitiveSceneProxy so we can do rough culling before dereferencing the proxy. */
struct FPrimitiveFlagsCompact
{
	/** True if the primitive casts dynamic shadows. */
	uint32 bCastDynamicShadow : 1;
	/** True if the primitive will cache static lighting. */
	uint32 bStaticLighting : 1;
	/** True if the primitive casts static shadows. */
	uint32 bCastStaticShadow : 1;
	FPrimitiveFlagsCompact() {}
	FPrimitiveFlagsCompact(const FPrimitiveSceneProxy* Proxy);
};

/** The information needed to determine whether a primitive is visible. */
class FPrimitiveSceneInfoCompact
{
public:
	FPrimitiveSceneInfo * PrimitiveSceneInfo;
	FPrimitiveSceneProxy* Proxy;
	FBoxSphereBounds Bounds;
	float MinDrawDistance;
	float MaxDrawDistance;
	/** Used for precomputed visibility */
	int32 VisibilityId;
	FPrimitiveFlagsCompact PrimitiveFlagsCompact;

	FPrimitiveSceneInfoCompact() {}
	/** Initialization constructor. */
	FPrimitiveSceneInfoCompact(FPrimitiveSceneInfo* InPrimitiveSceneInfo);
};

class FPrimitiveSceneInfo
{
public:

	/** The render proxy for the primitive. */
	FPrimitiveSceneProxy * Proxy;

	/**
	* Id for the component this primitive belongs to.
	* This will stay the same for the lifetime of the component, so it can be used to identify the component across re-registers.
	*/
	FPrimitiveComponentId PrimitiveComponentId;


	std::vector<FStaticMesh*> StaticMeshes;

	std::shared_ptr<FUniformBuffer> IndirectLightingCacheUniformBuffer;

	class FLightPrimitiveInteraction* LightList;

	FScene* Scene;

	FPrimitiveSceneInfo(UPrimitiveComponent* InPrimitive, FScene* InScene);

	/** Destructor. */
	~FPrimitiveSceneInfo();

	void AddToScene(bool bUpdateStaticDrawLists, bool bAddToStaticDrawLists = true);

	/** Removes the primitive from the scene. */
	void RemoveFromScene(bool bUpdateStaticDrawLists);

	void AddStaticMeshes(bool bUpdateStaticDrawLists = true);
	void RemoveStaticMeshes();

	void SetNeedsUniformBufferUpdate(bool bInNeedsUniformBufferUpdate)
	{
		bNeedsUniformBufferUpdate = bInNeedsUniformBufferUpdate;
	}
	bool NeedsUpdateStaticMeshes()
	{
		return bNeedsStaticMeshUpdate;
	}
	/** return true if we need to call LazyUpdateForRendering */
	bool NeedsUniformBufferUpdate() const
	{
		return bNeedsUniformBufferUpdate;
	}
	/** return true if we need to call ConditionalLazyUpdateForRendering */
	bool NeedsLazyUpdateForRendering()
	{
		return NeedsUniformBufferUpdate() || NeedsUpdateStaticMeshes();
	}

	void UpdateStaticMeshes(bool bReAddToDrawLists = true);

	void ConditionalUpdateStaticMeshes()
	{
		if (NeedsUpdateStaticMeshes())
		{
			UpdateStaticMeshes();
		}
	}
	void UpdateUniformBuffer();
	bool NeedsPrecomputedLightingBufferUpdate()
	{
		return bPrecomputedLightingBufferDirty;
	}
	void MarkPrecomputedLightingBufferDirty()
	{
		bPrecomputedLightingBufferDirty = true;
	}
	void UpdatePrecomputedLightingBuffer();

	/** Updates the primitive's uniform buffer. */
	void ConditionalUpdateUniformBuffer()
	{
		if (bNeedsUniformBufferUpdate)
		{
			UpdateUniformBuffer();
		}
	}
	void ConditionalLazyUpdateForRendering()
	{
		ConditionalUpdateUniformBuffer();
		ConditionalUpdateStaticMeshes();
	}
	int32 GetIndex() const { return PackedIndex; }
private:
	bool bNeedsUniformBufferUpdate;
	bool bPrecomputedLightingBufferDirty;
	bool bNeedsStaticMeshUpdate;
	friend class FScene;

	int32 PackedIndex;
};