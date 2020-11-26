#pragma once

#include "StaticMesh.h"
#include "Camera.h"
#include "SceneView.h"
#include "MeshBach.h"
#include "StaticMeshDrawList.h"
#include "DepthOnlyRendering.h"
#include "SceneManagement.h"
#include "DeferredShading.h"
#include <memory>

Vector4 CreateInvDeviceZToWorldZTransform(const FMatrix& ProjMatrix);

class FSceneRenderTargets;
class UPrimitiveComponent;
class FLightSceneInfo;
class FLightSceneInfoCompact;


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

class FCachedShadowMapData
{
public:
	FWholeSceneProjectedShadowInitializer Initializer;
	FShadowMapRenderTargetsRefCounted ShadowMap;
	float LastUsedTime;
	bool bCachedShadowMapHasPrimitives;

	FCachedShadowMapData(const FWholeSceneProjectedShadowInitializer& InInitializer, float InLastUsedTime) :
		Initializer(InInitializer),
		LastUsedTime(InLastUsedTime),
		bCachedShadowMapHasPrimitives(true)
	{}
};

class FScene
{
public:
	void AddPrimitive(UPrimitiveComponent* Primitive);
	void RemovePrimitive(UPrimitiveComponent* Primitive);
	void UpdatePrimitiveTransform(UPrimitiveComponent* Component);
	void AddLight(class ULightComponent* Light);
	void RemoveLight(class ULightComponent* Light);


	void AddLightSceneInfo(FLightSceneInfo* LightSceneInfo);
	void AddPrimitiveSceneInfo(FPrimitiveSceneInfo* PrimitiveSceneInfo);

	void UpdatePrimitiveTransform(FPrimitiveSceneProxy* PrimitiveSceneProxy, const FBoxSphereBounds& WorldBounds, const FBoxSphereBounds& LocalBounds, const FMatrix& LocalToWorld, const FVector& OwnerPosition);
public:


	TStaticMeshDrawList<FPositionOnlyDepthDrawingPolicy> PositionOnlyDepthDrawList;
	TStaticMeshDrawList<FDepthDrawingPolicy> DepthDrawList;

	//TStaticMeshDrawList<FDepthDrawingPolicy> MaskedDepthDrawList;
	/** Base pass draw list - no light map */
	//TStaticMeshDrawList<TBasePassDrawingPolicy<FUniformLightMapPolicy> > BasePassUniformLightMapPolicyDrawList[EBasePass_MAX];
	/** Base pass draw list - self shadowed translucency*/
	//TStaticMeshDrawList<TBasePassDrawingPolicy<FSelfShadowedTranslucencyPolicy> > BasePassSelfShadowedTranslucencyDrawList[EBasePass_MAX];
	//TStaticMeshDrawList<TBasePassDrawingPolicy<FSelfShadowedCachedPointIndirectLightingPolicy> > BasePassSelfShadowedCachedPointIndirectTranslucencyDrawList[EBasePass_MAX];
	//TStaticMeshDrawList<TBasePassDrawingPolicy<FSelfShadowedVolumetricLightmapPolicy> > BasePassSelfShadowedVolumetricLightmapTranslucencyDrawList[EBasePass_MAX];

	/** hit proxy draw list (includes both opaque and translucent objects) */
	//TStaticMeshDrawList<FHitProxyDrawingPolicy> HitProxyDrawList;

	/** hit proxy draw list, with only opaque objects */
	//TStaticMeshDrawList<FHitProxyDrawingPolicy> HitProxyDrawList_OpaqueOnly;

	/** draw list for motion blur velocities */
	//TStaticMeshDrawList<FVelocityDrawingPolicy> VelocityDrawList;

	/** Draw list used for rendering whole scene shadow depths. */
	//TStaticMeshDrawList<FShadowDepthDrawingPolicy<false> > WholeSceneShadowDepthDrawList;

	/** Draw list used for rendering whole scene reflective shadow maps.  */
	//TStaticMeshDrawList<FShadowDepthDrawingPolicy<true> > WholeSceneReflectiveShadowMapDrawList;


	//std::vector<MeshBatch> AllBatches;
	std::vector<FPrimitiveSceneInfo*> Primitives;
	std::vector<FLightSceneInfoCompact*> Lights;

	class AtmosphericFogSceneInfo* AtmosphericFog;

	FLightSceneInfo* SimpleDirectionalLight;

	std::map<int32, FCachedShadowMapData> CachedShadowMaps;
};
