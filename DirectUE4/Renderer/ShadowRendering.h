#pragma once

#include "D3D11RHI.h"
#include "UnrealMath.h"
#include "SceneManagement.h"
#include "RenderTargetPool.h"
#include "LightSceneInfo.h"

#include <functional>

class FPrimitiveSceneInfo;
class FViewInfo;

void InitShadowDepthMapsPass();

enum EShadowDepthCacheMode
{
	SDCM_MovablePrimitivesOnly,
	SDCM_StaticPrimitivesOnly,
	SDCM_Uncached
};

class FShadowMapRenderTargets
{
public:
	std::vector<PooledRenderTarget*> ColorTargets;
	PooledRenderTarget* DepthTarget;

	FShadowMapRenderTargets() :
		DepthTarget(NULL)
	{}

	FIntPoint GetSize() const
	{
		if (DepthTarget)
		{
			return DepthTarget->GetDesc().Extent;
		}
		else
		{
			assert(ColorTargets.size() > 0);
			return ColorTargets[0]->GetDesc().Extent;
		}
	}
};
typedef std::function<void(bool bFirst)> FSetShadowRenderTargetFunction;
/**
* Information about a projected shadow.
*/
class FProjectedShadowInfo 
{
public:
	typedef std::vector<const FPrimitiveSceneInfo*> PrimitiveArrayType;

	/** The view to be used when rendering this shadow's depths. */
	FViewInfo * ShadowDepthView;

	/** The depth or color targets this shadow was rendered to. */
	FShadowMapRenderTargets RenderTargets;

	EShadowDepthCacheMode CacheMode;

	/** The main view this shadow must be rendered in, or NULL for a view independent shadow. */
	FViewInfo* DependentView;

	/** Index of the shadow into FVisibleLightInfo::AllProjectedShadows. */
	int32 ShadowId;

	/** A translation that is applied to world-space before transforming by one of the shadow matrices. */
	FVector PreShadowTranslation;

	/** The effective view matrix of the shadow, used as an override to the main view's view matrix when rendering the shadow depth pass. */
	FMatrix ShadowViewMatrix;

	/**
	* Matrix used for rendering the shadow depth buffer.
	* Note that this does not necessarily contain all of the shadow casters with CSM, since the vertex shader flattens them onto the near plane of the projection.
	*/
	FMatrix SubjectAndReceiverMatrix;
	FMatrix ReceiverMatrix;

	FMatrix InvReceiverMatrix;

	float InvMaxSubjectDepth;

	/**
	* Subject depth extents, in world space units.
	* These can be used to convert shadow depth buffer values back into world space units.
	*/
	float MaxSubjectZ;
	float MinSubjectZ;

	/** Frustum containing all potential shadow casters. */
	FConvexVolume CasterFrustum;
	FConvexVolume ReceiverFrustum;

	float MinPreSubjectZ;

	FSphere ShadowBounds;

	FShadowCascadeSettings CascadeSettings;

	/**
	* X and Y position of the shadow in the appropriate depth buffer.  These are only initialized after the shadow has been allocated.
	* The actual contents of the shadowmap are at X + BorderSize, Y + BorderSize.
	*/
	uint32 X;
	uint32 Y;

	/**
	* Resolution of the shadow, excluding the border.
	* The full size of the region allocated to this shadow is therefore ResolutionX + 2 * BorderSize, ResolutionY + 2 * BorderSize.
	*/
	uint32 ResolutionX;
	uint32 ResolutionY;

	/** Size of the border, if any, used to allow filtering without clamping for shadows stored in an atlas. */
	uint32 BorderSize;

	/** The largest percent of either the width or height of any view. */
	float MaxScreenPercent;

	/** Fade Alpha per view. */
	std::vector<float> FadeAlphas;

	/** Whether the shadow has been allocated in the shadow depth buffer, and its X and Y properties have been initialized. */
	uint32 bAllocated : 1;

	/** Whether the shadow's projection has been rendered. */
	uint32 bRendered : 1;

	/** Whether the shadow has been allocated in the preshadow cache, so its X and Y properties offset into the preshadow cache depth buffer. */
	uint32 bAllocatedInPreshadowCache : 1;

	/** Whether the shadow is in the preshadow cache and its depths are up to date. */
	uint32 bDepthsCached : 1;

	// redundant to LightSceneInfo->Proxy->GetLightType() == LightType_Directional, could be made ELightComponentType LightType
	uint32 bDirectionalLight : 1;

	/** Whether the shadow is a point light shadow that renders all faces of a cubemap in one pass. */
	uint32 bOnePassPointLightShadow : 1;

	/** Whether this shadow affects the whole scene or only a group of objects. */
	uint32 bWholeSceneShadow : 1;

	/** Whether the shadow needs to render reflective shadow maps. */
	uint32 bReflectiveShadowmap : 1;

	/** Whether this shadow should support casting shadows from translucent surfaces. */
	uint32 bTranslucentShadow : 1;

	/** Whether the shadow will be computed by ray tracing the distance field. */
	uint32 bRayTracedDistanceField : 1;

	/** Whether this is a per-object shadow that should use capsule shapes to shadow instead of the mesh's triangles. */
	uint32 bCapsuleShadow : 1;

	/** Whether the shadow is a preshadow or not.  A preshadow is a per object shadow that handles the static environment casting on a dynamic receiver. */
	uint32 bPreShadow : 1;

	/** To not cast a shadow on the ground outside the object and having higher quality (useful for first person weapon). */
	uint32 bSelfShadowOnly : 1;

	/** Whether the shadow is a per object shadow or not. */
	uint32 bPerObjectOpaqueShadow : 1;

	/** Whether turn on back-lighting transmission. */
	uint32 bTransmission : 1;

	//TBitArray<SceneRenderingBitArrayAllocator> StaticMeshWholeSceneShadowDepthMap;
	std::vector<uint64> StaticMeshWholeSceneShadowBatchVisibility;

	/** View projection matrices for each cubemap face, used by one pass point light shadows. */
	std::vector<FMatrix> OnePassShadowViewProjectionMatrices;

	/** Frustums for each cubemap face, used for object culling one pass point light shadows. */
	std::vector<FConvexVolume> OnePassShadowFrustums;

	/** Data passed from async compute begin to end. */
	//FComputeFenceRHIRef RayTracedShadowsEndFence;
	ComPtr<PooledRenderTarget> RayTracedShadowsRT;


	float ShaderDepthBias;
public:
	// default constructor
	FProjectedShadowInfo();

	/** for a whole-scene shadow. */
	void SetupWholeSceneProjection(
		FLightSceneInfo* InLightSceneInfo,
		FViewInfo* InDependentView,
		const FWholeSceneProjectedShadowInitializer& Initializer,
		uint32 InResolutionX,
		uint32 InResolutionY,
		uint32 InBorderSize,
		bool bInReflectiveShadowMap
	);

	void RenderDepth(class FSceneRenderer* SceneRenderer, FSetShadowRenderTargetFunction SetShadowRenderTargets/*, EShadowDepthRenderMode RenderMode*/);

	bool HasSubjectPrims() const;

	void SetupShadowDepthView(FSceneRenderer* SceneRenderer);

	const FLightSceneInfo& GetLightSceneInfo() const { return *LightSceneInfo; }
	const FLightSceneInfoCompact& GetLightSceneInfoCompact() const { return LightSceneInfoCompact; }

private:
	const FLightSceneInfo* LightSceneInfo;
	FLightSceneInfoCompact LightSceneInfoCompact;
};
