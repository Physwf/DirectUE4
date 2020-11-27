#pragma once

#include "MeshBach.h"

class FLightSceneInfo;
class FPrimitiveSceneInfo;
class FScene;

class FLightPrimitiveInteraction
{
public:

	/** Creates an interaction for a light-primitive pair. */
	static void Create(FLightSceneInfo* LightSceneInfo, FPrimitiveSceneInfo* PrimitiveSceneInfo);
	static void Destroy(FLightPrimitiveInteraction* LightPrimitiveInteraction);

	// Accessors.
	bool HasShadow() const { return bCastShadow; }
	bool IsLightMapped() const { return bLightMapped; }
	bool IsDynamic() const { return bIsDynamic; }
	bool IsShadowMapped() const { return bIsShadowMapped; }
	bool IsUncachedStaticLighting() const { return bUncachedStaticLighting; }
	bool HasTranslucentObjectShadow() const { return bHasTranslucentObjectShadow; }
	bool HasInsetObjectShadow() const { return bHasInsetObjectShadow; }
	bool CastsSelfShadowOnly() const { return bSelfShadowOnly; }
	bool IsES2DynamicPointLight() const { return bES2DynamicPointLight; }
	FLightSceneInfo* GetLight() const { return LightSceneInfo; }
	int32 GetLightId() const { return LightId; }
	FPrimitiveSceneInfo* GetPrimitiveSceneInfo() const { return PrimitiveSceneInfo; }
	FLightPrimitiveInteraction* GetNextPrimitive() const { return NextPrimitive; }
	FLightPrimitiveInteraction* GetNextLight() const { return NextLight; }

	template<typename T> friend struct std::hash;

	/** Clears cached shadow maps, if possible */
	void FlushCachedShadowMapData();

private:
	/** The light which affects the primitive. */
	FLightSceneInfo * LightSceneInfo;

	/** The primitive which is affected by the light. */
	FPrimitiveSceneInfo* PrimitiveSceneInfo;

	/** A pointer to the NextPrimitive member of the previous interaction in the light's interaction list. */
	FLightPrimitiveInteraction** PrevPrimitiveLink;

	/** The next interaction in the light's interaction list. */
	FLightPrimitiveInteraction* NextPrimitive;

	/** A pointer to the NextLight member of the previous interaction in the primitive's interaction list. */
	FLightPrimitiveInteraction** PrevLightLink;

	/** The next interaction in the primitive's interaction list. */
	FLightPrimitiveInteraction* NextLight;

	/** The index into Scene->Lights of the light which affects the primitive. */
	int32 LightId;

	/** True if the primitive casts a shadow from the light. */
	uint32 bCastShadow : 1;

	/** True if the primitive has a light-map containing the light. */
	uint32 bLightMapped : 1;

	/** True if the interaction is dynamic. */
	uint32 bIsDynamic : 1;

	/** Whether the light's shadowing is contained in the primitive's static shadow map. */
	uint32 bIsShadowMapped : 1;

	/** True if the interaction is an uncached static lighting interaction. */
	uint32 bUncachedStaticLighting : 1;

	/** True if the interaction has a translucent per-object shadow. */
	uint32 bHasTranslucentObjectShadow : 1;

	/** True if the interaction has an inset per-object shadow. */
	uint32 bHasInsetObjectShadow : 1;

	/** True if the primitive only shadows itself. */
	uint32 bSelfShadowOnly : 1;

	/** True this is an ES2 dynamic point light interaction. */
	uint32 bES2DynamicPointLight : 1;

	/** Initialization constructor. */
	FLightPrimitiveInteraction(FLightSceneInfo* InLightSceneInfo, FPrimitiveSceneInfo* InPrimitiveSceneInfo,
		bool bIsDynamic, bool bInLightMapped, bool bInIsShadowMapped, bool bInHasTranslucentObjectShadow, bool bInHasInsetObjectShadow);

	/** Hide dtor */
	~FLightPrimitiveInteraction();

};

namespace std
{
	template<> struct hash<FLightPrimitiveInteraction*>
	{
		size_t operator()(FLightPrimitiveInteraction* Interaction)
		{
			return (uint32)Interaction->LightId;
		}
	};
}
/**
* A mesh which is defined by a primitive at scene segment construction time and never changed.
* Lights are attached and detached as the segment containing the mesh is added or removed from a scene.
*/
class FStaticMesh : public FMeshBatch
{
public:

	/**
	* An interface to a draw list's reference to this static mesh.
	* used to remove the static mesh from the draw list without knowing the draw list type.
	*/
	class FDrawListElementLink// : public FRefCountedObject
	{
	public:
		virtual bool IsInDrawList(const class FStaticMeshDrawListBase* DrawList) const = 0;
		virtual void Remove(const bool bUnlinkMesh) = 0;
	};

	/** The screen space size to draw this primitive at */
	//float ScreenSize;

	/** The render info for the primitive which created this mesh. */
	FPrimitiveSceneInfo* PrimitiveSceneInfo;

	/** The index of the mesh in the scene's static meshes array. */
	int32 Id;

	/** Index of the mesh into the scene's StaticMeshBatchVisibility array. */
	int32 BatchVisibilityId;

	// Constructor/destructor.
	FStaticMesh(
		FPrimitiveSceneInfo* InPrimitiveSceneInfo,
		const FMeshBatch& InMesh//,
								//float InScreenSize,
								//FHitProxyId InHitProxyId
	) :
		FMeshBatch(InMesh),
		//ScreenSize(InScreenSize),
		PrimitiveSceneInfo(InPrimitiveSceneInfo),
		//Id(INDEX_NONE),
		BatchVisibilityId(-1)
	{
		//BatchHitProxyId = InHitProxyId;
	}

	~FStaticMesh();

	/** Adds a link from the mesh to its entry in a draw list. */
	void LinkDrawList(/*FDrawListElementLink* Link*/);

	/** Removes a link from the mesh to its entry in a draw list. */
	void UnlinkDrawList(/*FDrawListElementLink* Link*/);

	/** Adds the static mesh to the appropriate draw lists in a scene. */
	void AddToDrawLists(/*FRHICommandListImmediate& RHICmdList,*/ FScene* Scene);

	/** Removes the static mesh from all draw lists. */
	void RemoveFromDrawLists();

	/** Returns true if the mesh is linked to the given draw list. */
	bool IsLinkedToDrawList(const FStaticMeshDrawListBase* DrawList) const;

private:
	/** Links to the draw lists this mesh is an element of. */
	//TArray<TRefCountPtr<FDrawListElementLink> > DrawListLinks;

	/** Private copy constructor. */
	FStaticMesh(const FStaticMesh& InStaticMesh) :
		FMeshBatch(InStaticMesh),
		//ScreenSize(InStaticMesh.ScreenSize),
		PrimitiveSceneInfo(InStaticMesh.PrimitiveSceneInfo),
		Id(InStaticMesh.Id),
		BatchVisibilityId(InStaticMesh.BatchVisibilityId)
	{}
};
