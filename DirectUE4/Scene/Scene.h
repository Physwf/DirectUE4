#pragma once

#include "StaticMesh.h"
#include "Camera.h"
#include "SceneView.h"
#include "MeshBach.h"
#include "StaticMeshDrawList.h"
#include "DepthOnlyRendering.h"
#include "SceneManagement.h"
#include "DeferredShading.h"
#include "ShadowRendering.h"

#include <memory>

enum EBasePassDrawListType
{
	EBasePass_Default = 0,
	EBasePass_Masked,
	EBasePass_MAX
};

Vector4 CreateInvDeviceZToWorldZTransform(const FMatrix& ProjMatrix);

class FSceneRenderTargets;
class UPrimitiveComponent;
class FLightSceneInfo;
class FLightSceneInfoCompact;

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

/**
* Bounding information used to cull primitives in the scene.
*/
struct FPrimitiveBounds
{
	FBoxSphereBounds BoxSphereBounds;
	/** Square of the minimum draw distance for the primitive. */
	float MinDrawDistanceSq;
	/** Maximum draw distance for the primitive. */
	float MaxDrawDistance;
	/** Maximum cull distance for the primitive. This is only different from the MaxDrawDistance for HLOD.*/
	float MaxCullDistance;
};

struct FUpdateLightTransformParameters
{
	FMatrix LightToWorld;
	Vector4 Position;
};

class FScene
{
public:
	void AddPrimitive(UPrimitiveComponent* Primitive);
	void RemovePrimitive(UPrimitiveComponent* Primitive);
	void UpdatePrimitiveTransform(UPrimitiveComponent* Component);
	void UpdateLightTransform(ULightComponent* Light);
	void AddLight(class ULightComponent* Light);
	void RemoveLight(class ULightComponent* Light);


	void AddLightSceneInfo(FLightSceneInfo* LightSceneInfo);
	void AddPrimitiveSceneInfo(FPrimitiveSceneInfo* PrimitiveSceneInfo);

	void UpdateLightTransform_RenderThread(FLightSceneInfo* LightSceneInfo, const struct FUpdateLightTransformParameters& Parameters);
	void UpdatePrimitiveTransform_RenderThread(FPrimitiveSceneProxy* PrimitiveSceneProxy, const FBoxSphereBounds& WorldBounds, const FBoxSphereBounds& LocalBounds, const FMatrix& LocalToWorld, const FVector& OwnerPosition);
public:


	TStaticMeshDrawList<FPositionOnlyDepthDrawingPolicy> PositionOnlyDepthDrawList;
	TStaticMeshDrawList<FDepthDrawingPolicy> DepthDrawList;

	TStaticMeshDrawList<FDepthDrawingPolicy> MaskedDepthDrawList;
	/** Base pass draw list - no light map */
	TStaticMeshDrawList<TBasePassDrawingPolicy<FUniformLightMapPolicy> > BasePassUniformLightMapPolicyDrawList[EBasePass_MAX];
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
	TStaticMeshDrawList<FShadowDepthDrawingPolicy<false> > WholeSceneShadowDepthDrawList;

	/** Draw list used for rendering whole scene reflective shadow maps.  */
	TStaticMeshDrawList<FShadowDepthDrawingPolicy<true> > WholeSceneReflectiveShadowMapDrawList;

	template<typename LightMapPolicyType>
	TStaticMeshDrawList<TBasePassDrawingPolicy<LightMapPolicyType> >& GetBasePassDrawList(EBasePassDrawListType DrawType);

	//std::vector<MeshBatch> AllBatches;
	std::vector<FPrimitiveSceneInfo*> Primitives;
	std::vector<FPrimitiveSceneProxy*> PrimitiveSceneProxies;
	std::vector<FPrimitiveBounds> PrimitiveBounds;
	std::vector<FPrimitiveComponentId> PrimitiveComponentIds;

	std::vector<FLightSceneInfoCompact> Lights;


	std::vector<FPrimitiveSceneInfoCompact> PrimitiveOctree;

	class AtmosphericFogSceneInfo* AtmosphericFog;

	FSkyLightSceneProxy* SkyLight;

	FLightSceneInfo* SimpleDirectionalLight;

	std::map<int32, FCachedShadowMapData> CachedShadowMaps;


	bool ShouldRenderSkylightInBasePass(EBlendMode BlendMode) const
	{
		bool bRenderSkyLight = SkyLight && !SkyLight->bHasStaticLighting;

		if (IsTranslucentBlendMode(BlendMode))
		{
			// Both stationary and movable skylights are applied in base pass for translucent materials
			bRenderSkyLight = bRenderSkyLight
				&& (/*ReadOnlyCVARCache.bEnableStationarySkylight*/true || !SkyLight->bWantsStaticShadowing);
		}
		else
		{
			// For opaque materials, stationary skylight is applied in base pass but movable skylight
			// is applied in a separate render pass (bWantssStaticShadowing means stationary skylight)
			bRenderSkyLight = bRenderSkyLight
				&& ((/*ReadOnlyCVARCache.bEnableStationarySkylight*/true && SkyLight->bWantsStaticShadowing)
					/*|| (!SkyLight->bWantsStaticShadowing*/
						/*&& (IsAnyForwardShadingEnabled(GetShaderPlatform()) || IsMobilePlatform(GetShaderPlatform())))*/);
		}

		return bRenderSkyLight;
	}

	bool HasAtmosphericFog() const
	{
		return (AtmosphericFog != NULL); // Use default value when Sun Light is not existing
	}

};

// /**  */
// template<>
// TStaticMeshDrawList<TBasePassDrawingPolicy<FSelfShadowedTranslucencyPolicy> >& FScene::GetBasePassDrawList<FSelfShadowedTranslucencyPolicy>(EBasePassDrawListType DrawType)
// {
// 	return BasePassSelfShadowedTranslucencyDrawList[DrawType];
// }
// 
// /**  */
// template<>
// TStaticMeshDrawList<TBasePassDrawingPolicy<FSelfShadowedCachedPointIndirectLightingPolicy> >& FScene::GetBasePassDrawList<FSelfShadowedCachedPointIndirectLightingPolicy>(EBasePassDrawListType DrawType)
// {
// 	return BasePassSelfShadowedCachedPointIndirectTranslucencyDrawList[DrawType];
// }
// 
// template<>
// TStaticMeshDrawList<TBasePassDrawingPolicy<FSelfShadowedVolumetricLightmapPolicy> >& FScene::GetBasePassDrawList<FSelfShadowedVolumetricLightmapPolicy>(EBasePassDrawListType DrawType)
// {
// 	return BasePassSelfShadowedVolumetricLightmapTranslucencyDrawList[DrawType];
// }

/**  */
template<>
TStaticMeshDrawList<TBasePassDrawingPolicy<FUniformLightMapPolicy> >& FScene::GetBasePassDrawList<FUniformLightMapPolicy>(EBasePassDrawListType DrawType)
{
	return BasePassUniformLightMapPolicyDrawList[DrawType];
}

inline bool ShouldIncludeDomainInMeshPass(EMaterialDomain Domain)
{
	// Non-Surface domains can be applied to static meshes for thumbnails or material editor preview
	// Volume domain materials however must only be rendered in the voxelization pass
	return Domain != MD_Volume;
}