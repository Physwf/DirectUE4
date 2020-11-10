#pragma once

#include "Mesh.h"
#include "Light.h"
#include "Camera.h"
#include "SceneView.h"
#include <memory>

Vector4 CreateInvDeviceZToWorldZTransform(const Matrix& ProjMatrix);

class RenderTargets;

class ViewInfo : public SceneView
{
public:
	/* Final position of the view in the final render target (in pixels), potentially scaled by ScreenPercentage */
	IntRect ViewRect;

	/** Cached view uniform shader parameters, to allow recreating the view uniform buffer without having to fill out the entire struct. */
	ViewUniformShaderParameters* CachedViewUniformShaderParameters;

	/** A map from primitive ID to a boolean visibility value. */
	//FSceneBitArray PrimitiveVisibilityMap;

	/** Bit set when a primitive is known to be unoccluded. */
	//FSceneBitArray PrimitiveDefinitelyUnoccludedMap;

	/** A map from primitive ID to a boolean is fading value. */
	//FSceneBitArray PotentiallyFadingPrimitiveMap;

	/** Primitive fade uniform buffers, indexed by packed primitive index. */
	//TArray<FUniformBufferRHIParamRef, SceneRenderingAllocator> PrimitiveFadeUniformBuffers;

	/** A map from primitive ID to the primitive's view relevance. */
	//TArray<FPrimitiveViewRelevance, SceneRenderingAllocator> PrimitiveViewRelevanceMap;

	/** A map from static mesh ID to a boolean visibility value. */
	//FSceneBitArray StaticMeshVisibilityMap;

	/** A map from static mesh ID to a boolean occluder value. */
	//FSceneBitArray StaticMeshOccluderMap;

	/** A map from static mesh ID to a boolean velocity visibility value. */
	//FSceneBitArray StaticMeshVelocityMap;

	/** A map from static mesh ID to a boolean shadow depth visibility value. */
	//FSceneBitArray StaticMeshShadowDepthMap;

	/** A map from static mesh ID to a boolean dithered LOD fade out value. */
	//FSceneBitArray StaticMeshFadeOutDitheredLODMap;

	/** A map from static mesh ID to a boolean dithered LOD fade in value. */
	//FSceneBitArray StaticMeshFadeInDitheredLODMap;

	/** An array of batch element visibility masks, valid only for meshes
	set visible in either StaticMeshVisibilityMap or StaticMeshShadowDepthMap. */
	//TArray<uint64, SceneRenderingAllocator> StaticMeshBatchVisibility;

	/** The dynamic primitives visible in this view. */
	//TArray<const FPrimitiveSceneInfo*, SceneRenderingAllocator> VisibleDynamicPrimitives;

	/** List of visible primitives with dirty precomputed lighting buffers */
	//TArray<FPrimitiveSceneInfo*, SceneRenderingAllocator> DirtyPrecomputedLightingBufferPrimitives;

	/** View dependent global distance field clipmap info. */
	//FGlobalDistanceFieldInfo GlobalDistanceFieldInfo;

	/** Set of translucent prims for this view */
	//FTranslucentPrimSet TranslucentPrimSet;

	/** Set of distortion prims for this view */
	//FDistortionPrimSet DistortionPrimSet;

	/** Set of mesh decal prims for this view */
	//FMeshDecalPrimSet MeshDecalPrimSet;

	/** Set of CustomDepth prims for this view */
	//FCustomDepthPrimSet CustomDepthSet;

	/** Primitives with a volumetric material. */
	//FVolumetricPrimSet VolumetricPrimSet;

	/** A map from light ID to a boolean visibility value. */
	//TArray<FVisibleLightViewInfo, SceneRenderingAllocator> VisibleLightInfos;

	/** The view's batched elements. */
	//FBatchedElements BatchedViewElements;

	/** The view's batched elements, above all other elements, for gizmos that should never be occluded. */
	//FBatchedElements TopBatchedViewElements;

	/** The view's mesh elements. */
	//TIndirectArray<FMeshBatch> ViewMeshElements;

	/** The view's mesh elements for the foreground (editor gizmos and primitives )*/
	//TIndirectArray<FMeshBatch> TopViewMeshElements;

	/** The dynamic resources used by the view elements. */
	//TArray<FDynamicPrimitiveResource*> DynamicResources;

	/** Gathered in initviews from all the primitives with dynamic view relevance, used in each mesh pass. */
	//TArray<FMeshBatchAndRelevance, SceneRenderingAllocator> DynamicMeshElements;

	// [PrimitiveIndex] = end index index in DynamicMeshElements[], to support GetDynamicMeshElementRange()
	//TArray<uint32, SceneRenderingAllocator> DynamicMeshEndIndices;

	//TArray<FMeshBatchAndRelevance, SceneRenderingAllocator> DynamicEditorMeshElements;

	// Used by mobile renderer to determine whether static meshes will be rendered with CSM shaders or not.
	//FMobileCSMVisibilityInfo MobileCSMVisibilityInfo;

	// Primitive CustomData
// 	TArray<const FPrimitiveSceneInfo*, SceneRenderingAllocator> PrimitivesWithCustomData;	// Size == Amount of Primitive With Custom Data
// 	FSceneBitArray UpdatedPrimitivesWithCustomData;
// 	TArray<FMemStackBase, SceneRenderingAllocator> PrimitiveCustomDataMemStack; // Size == 1 global stack + 1 per visibility thread (if multithread)

																				/** Parameters for exponential height fog. */
// 	FVector4 ExponentialFogParameters;
// 	FVector ExponentialFogColor;
// 	float FogMaxOpacity;
// 	FVector4 ExponentialFogParameters3;
// 	FVector2D SinCosInscatteringColorCubemapRotation;

// 	UTexture* FogInscatteringColorCubemap;
// 	FVector FogInscatteringTextureParameters;

	/** Parameters for directional inscattering of exponential height fog. */
// 	bool bUseDirectionalInscattering;
// 	float DirectionalInscatteringExponent;
// 	float DirectionalInscatteringStartDistance;
// 	FVector InscatteringLightDirection;
// 	FLinearColor DirectionalInscatteringColor;

	/** Translucency lighting volume properties. */
// 	FVector TranslucencyLightingVolumeMin[TVC_MAX];
// 	float TranslucencyVolumeVoxelSize[TVC_MAX];
// 	FVector TranslucencyLightingVolumeSize[TVC_MAX];

	/** Temporal jitter at the pixel scale. */
	//FVector2D TemporalJitterPixels;

	/** true if all PrimitiveVisibilityMap's bits are set to false. */
	//uint32 bHasNoVisiblePrimitive : 1;

	/** true if the view has at least one mesh with a translucent material. */
	//uint32 bHasTranslucentViewMeshElements : 1;
	/** Indicates whether previous frame transforms were reset this frame for any reason. */
	//uint32 bPrevTransformsReset : 1;
	/** Whether we should ignore queries from last frame (useful to ignoring occlusions on the first frame after a large camera movement). */
	//uint32 bIgnoreExistingQueries : 1;
	/** Whether we should submit new queries this frame. (used to disable occlusion queries completely. */
	//uint32 bDisableQuerySubmissions : 1;
	/** Whether we should disable distance-based fade transitions for this frame (usually after a large camera movement.) */
	//uint32 bDisableDistanceBasedFadeTransitions : 1;
	/** Whether the view has any materials that use the global distance field. */
	//uint32 bUsesGlobalDistanceField : 1;
	//uint32 bUsesLightingChannels : 1;
	//uint32 bTranslucentSurfaceLighting : 1;
	/** Whether the view has any materials that read from scene depth. */
	//uint32 bUsesSceneDepth : 1;
	/**
	* true if the scene has at least one decal. Used to disable stencil operations in the mobile base pass when the scene has no decals.
	* TODO: Right now decal visibility is computed right before rendering them. Ideally it should be done in InitViews and this flag should be replaced with list of visible decals
	*/
	//uint32 bSceneHasDecals : 1;
	/** Bitmask of all shading models used by primitives in this view */
	//uint16 ShadingModelMaskInView;

	// Previous frame view info to use for this view.
	//FPreviousViewInfo PrevViewInfo;

	/** An intermediate number of visible static meshes.  Doesn't account for occlusion until after FinishOcclusionQueries is called. */
	//int32 NumVisibleStaticMeshElements;

	/** Frame's exposure. Always > 0. */
	//float PreExposure;

	/** Mip bias to apply in material's samplers. */
	//float MaterialTextureMipBias;

	/** Precomputed visibility data, the bits are indexed by VisibilityId of a primitive component. */
	//const uint8* PrecomputedVisibilityData;

	//FOcclusionQueryBatcher IndividualOcclusionQueries;
	//FOcclusionQueryBatcher GroupedOcclusionQueries;

	// Hierarchical Z Buffer
	//TRefCountPtr<IPooledRenderTarget> HZB;

	//int32 NumBoxReflectionCaptures;
	//int32 NumSphereReflectionCaptures;
	//float FurthestReflectionCaptureDistance;
	//TUniformBufferRef<FReflectionCaptureShaderData> ReflectionCaptureUniformBuffer;

	/** Used when there is no view state, buffers reallocate every frame. */
	//TUniquePtr<FForwardLightingViewResources> ForwardLightingResourcesStorage;

	//FVolumetricFogViewResources VolumetricFogResources;

	// Size of the HZB's mipmap 0
	// NOTE: the mipmap 0 is downsampled version of the depth buffer
	IntPoint HZBMipmap0Size;

	/** Used by occlusion for percent unoccluded calculations. */
	//float OneOverNumPossiblePixels;

	// Mobile gets one light-shaft, this light-shaft.
	//FVector4 LightShaftCenter;
	//FLinearColor LightShaftColorMask;
	//FLinearColor LightShaftColorApply;
	//bool bLightShaftUse;

	//FHeightfieldLightingViewInfo HeightfieldLightingViewInfo;

	//TShaderMap<FGlobalShaderType>* ShaderMap;

	//bool bIsSnapshot;

	// Optional stencil dithering optimization during prepasses
	//bool bAllowStencilDither;

	/** Custom visibility query for view */
	//ICustomVisibilityQuery* CustomVisibilityQuery;

	ViewInfo(const ViewInitOptions& InitOptions);
	explicit ViewInfo(const SceneView* InView);
	//TArray<FPrimitiveSceneInfo*, SceneRenderingAllocator> IndirectShadowPrimitives;
	/**
	* Destructor.
	*/
	~ViewInfo();

	/** Returns the size of view rect after primary upscale ( == only with secondary screen percentage). */
	//FIntPoint GetSecondaryViewRectSize() const;

	/** Returns whether the view requires a secondary upscale. */
// 	bool RequiresSecondaryUpscale() const
// 	{
// 		return UnscaledViewRect.Size() != GetSecondaryViewRectSize();
// 	}

	/** Creates ViewUniformShaderParameters given a set of view transforms. */
	void SetupUniformBufferParameters(
		RenderTargets& SceneContext,
		const ViewMatrices& InViewMatrices,
		const ViewMatrices& InPrevViewMatrices,
		Box* OutTranslucentCascadeBoundsArray,
		int32 NumTranslucentCascades,
		ViewUniformShaderParameters& ViewUniformParameters) const;

	/** Recreates ViewUniformShaderParameters, taking the view transform from the View Matrices */
	inline void SetupUniformBufferParameters(
		RenderTargets& SceneContext,
		Box* OutTranslucentCascadeBoundsArray,
		int32 NumTranslucentCascades,
		ViewUniformShaderParameters& ViewUniformParameters) const
	{
		SetupUniformBufferParameters(SceneContext,
			mViewMatrices,
			mViewMatrices,/*PrevViewInfo.ViewMatrices,*/
			OutTranslucentCascadeBoundsArray,
			NumTranslucentCascades,
			ViewUniformParameters);
	}

	//void SetupDefaultGlobalDistanceFieldUniformBufferParameters(FViewUniformShaderParameters& ViewUniformShaderParameters) const;
	//void SetupGlobalDistanceFieldUniformBufferParameters(FViewUniformShaderParameters& ViewUniformShaderParameters) const;
	//void SetupVolumetricFogUniformBufferParameters(FViewUniformShaderParameters& ViewUniformShaderParameters) const;

	void InitRHIResources();

	/** Determines distance culling and fades if the state changes */
	//bool IsDistanceCulled(float DistanceSquared, float MaxDrawDistance, float MinDrawDistance, const FPrimitiveSceneInfo* PrimitiveSceneInfo);

	/** Gets the eye adaptation render target for this view. Same as GetEyeAdaptationRT */
	//IPooledRenderTarget* GetEyeAdaptation(FRHICommandList& RHICmdList) const;

// 	IPooledRenderTarget* GetEyeAdaptation() const
// 	{
// 		return GetEyeAdaptationRT();
// 	}
	/** Gets one of two eye adaptation render target for this view.
	* NB: will return null in the case that the internal view state pointer
	* (for the left eye in the stereo case) is null.
	*/
	//IPooledRenderTarget* GetEyeAdaptationRT(FRHICommandList& RHICmdList) const;
	//IPooledRenderTarget* GetEyeAdaptationRT() const;
	//IPooledRenderTarget* GetLastEyeAdaptationRT(FRHICommandList& RHICmdList) const;

	/**Swap the order of the two eye adaptation targets in the double buffer system */
	//void SwapEyeAdaptationRTs(FRHICommandList& RHICmdList) const;

	/** Tells if the eyeadaptation texture exists without attempting to allocate it. */
	//bool HasValidEyeAdaptation() const;

	/** Informs sceneinfo that eyedaptation has queued commands to compute it at least once and that it can be used */
	//void SetValidEyeAdaptation() const;

	/** Get the last valid exposure value for eye adapation. */
	//float GetLastEyeAdaptationExposure() const;

	/** Informs sceneinfo that tonemapping LUT has queued commands to compute it at least once */
	//void SetValidTonemappingLUT() const;

	/** Gets the tonemapping LUT texture, previously computed by the CombineLUTS post process,
	* for stereo rendering, this will force the post-processing to use the same texture for both eyes*/
	//const FTextureRHIRef* GetTonemappingLUTTexture() const;

	/** Gets the rendertarget that will be populated by CombineLUTS post process
	* for stereo rendering, this will force the post-processing to use the same render target for both eyes*/
	//FSceneRenderTargetItem* GetTonemappingLUTRenderTarget(FRHICommandList& RHICmdList, const int32 LUTSize, const bool bUseVolumeLUT, const bool bNeedUAV) const;

	//inline FVector GetPrevViewDirection() const { return PrevViewInfo.ViewMatrices.GetViewMatrix().GetColumn(2); }

	/** Create a snapshot of this view info on the scene allocator. */
	//FViewInfo* CreateSnapshot() const;

	/** Destroy all snapshots before we wipe the scene allocator. */
	//static void DestroyAllSnapshots();

	// Get the range in DynamicMeshElements[] for a given PrimitiveIndex
	// @return range (start is inclusive, end is exclusive)
// 	FInt32Range GetDynamicMeshElementRange(uint32 PrimitiveIndex) const
// 	{
// 		// inclusive
// 		int32 Start = (PrimitiveIndex == 0) ? 0 : DynamicMeshEndIndices[PrimitiveIndex - 1];
// 		// exclusive
// 		int32 AfterEnd = DynamicMeshEndIndices[PrimitiveIndex];
// 
// 		return FInt32Range(Start, AfterEnd);
// 	}

	/** Set the custom data associated with a primitive scene info.	*/
	//void SetCustomData(const FPrimitiveSceneInfo* InPrimitiveSceneInfo, void* InCustomData);
private:
	// Cache of TEXTUREGROUP_World to create view's samplers on render thread.
	// may not have a valid value if FViewInfo is created on the render thread.
	//ESamplerFilter WorldTextureGroupSamplerFilter;
	//bool bIsValidWorldTextureGroupSamplerFilter;

	//FSceneViewState* GetEffectiveViewState() const;

	/** Initialization that is common to the constructors. */
	//void Init();

	/** Calculates bounding boxes for the translucency lighting volume cascades. */
	//void CalcTranslucencyLightingVolumeBounds(FBox* InOutCascadeBoundsArray, int32 NumCascades) const;

	/** Sets the sky SH irradiance map coefficients. */
	void SetupSkyIrradianceEnvironmentMapConstants(Vector4* OutSkyIrradianceEnvironmentMap) const;
};

void UpdateView();

class Scene
{
public:
	void InitScene();
	void AddMesh(Mesh* InMesh);
public:
	std::vector<MeshBatch> AllBatches;

	class AtmosphericFogSceneInfo* AtmosphericFog;
};

extern Scene* GScene;