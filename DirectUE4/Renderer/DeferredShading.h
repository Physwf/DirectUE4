#pragma once

#include "D3D11RHI.h"
#include "RenderTargetPool.h"
#include "SceneView.h"
#include "Shader.h"
#include "RenderTargets.h"
#include "LightSceneInfo.h"
#include "DepthOnlyRendering.h"

void InitShading();

class FProjectedShadowInfo;
class FLightSceneInfo;
struct FDrawingPolicyRenderState;
enum EBasePassDrawListType;

class FShadowMapRenderTargetsRefCounted
{
public:
	std::vector<ComPtr<PooledRenderTarget>> ColorTargets;
	ComPtr<PooledRenderTarget> DepthTarget;

	bool IsValid() const
	{
		if (DepthTarget)
		{
			return true;
		}
		else
		{
			return ColorTargets.size() > 0;
		}
	}

	FIntPoint GetSize() const
	{
		const PooledRenderTargetDesc* Desc = NULL;

		if (DepthTarget)
		{
			Desc = &DepthTarget->GetDesc();
		}
		else
		{
			assert(ColorTargets.size() > 0);
			Desc = &ColorTargets[0]->GetDesc();
		}

		return Desc->Extent;
	}

	void Release()
	{
		for (uint32 i = 0; i < ColorTargets.size(); i++)
		{
			ColorTargets[i].Reset();
		}

		ColorTargets.clear();

		DepthTarget.Reset();
	}
};
struct FSortedShadowMapAtlas
{
	FShadowMapRenderTargetsRefCounted RenderTargets;
	std::vector<FProjectedShadowInfo*> Shadows;
};

struct FSortedShadowMaps
{
	/** Visible shadows sorted by their shadow depth map render target. */
	std::vector<FSortedShadowMapAtlas> ShadowMapAtlases;

	std::vector<FSortedShadowMapAtlas> RSMAtlases;

	std::vector<FSortedShadowMapAtlas> ShadowMapCubemaps;

	FSortedShadowMapAtlas PreshadowCache;

	std::vector<FSortedShadowMapAtlas> TranslucencyShadowMapAtlases;

	void Release();
};

/** Information about a visible light which is specific to the view it's visible in. */
class FVisibleLightViewInfo
{
public:

	/** The dynamic primitives which are both visible and affected by this light. */
	//std::vector<FPrimitiveSceneInfo*> VisibleDynamicLitPrimitives;

	/** Whether each shadow in the corresponding FVisibleLightInfo::AllProjectedShadows array is visible. */
	//FSceneBitArray ProjectedShadowVisibilityMap;

	/** The view relevance of each shadow in the corresponding FVisibleLightInfo::AllProjectedShadows array. */
	//std::vector<FPrimitiveViewRelevance> ProjectedShadowViewRelevanceMap;

	/** true if this light in the view frustum (dir/sky lights always are). */
	uint32 bInViewFrustum : 1;

	/** List of CSM shadow casters. Used by mobile renderer for culling primitives receiving static + CSM shadows */
	//FMobileCSMSubjectPrimitives MobileCSMSubjectPrimitives;

	/** Initialization constructor. */
	FVisibleLightViewInfo()
		: bInViewFrustum(false)
	{}
};

/** Information about a visible light which isn't view-specific. */
class FVisibleLightInfo
{
public:

	/** Projected shadows allocated on the scene rendering mem stack. */
	std::vector<FProjectedShadowInfo*> MemStackProjectedShadows;

	/** All visible projected shadows, output of shadow setup.  Not all of these will be rendered. */
	std::vector<FProjectedShadowInfo*> AllProjectedShadows;

	/** Shadows to project for each feature that needs special handling. */
	std::vector<FProjectedShadowInfo*> ShadowsToProject;
	std::vector<FProjectedShadowInfo*> CapsuleShadowsToProject;
	std::vector<FProjectedShadowInfo*> RSMsToProject;

	/** All visible projected preshdows.  These are not allocated on the mem stack so they are refcounted. */
	std::vector<std::shared_ptr<FProjectedShadowInfo>> ProjectedPreShadows;

	/** A list of per-object shadows that were occluded. We need to track these so we can issue occlusion queries for them. */
	std::vector<FProjectedShadowInfo*> OccludedPerObjectShadows;
};
const int32 GMaxForwardShadowCascades = 4;

struct alignas(16) FForwardLightData
{
	FForwardLightData()
	{
		ConstructUniformBufferInfo(*this);
	}

	struct ConstantStruct
	{
		uint32 NumLocalLights;
		uint32 NumReflectionCaptures;
		uint32 HasDirectionalLight;
		uint32 NumGridCells;
		FIntVector CulledGridSize;
		uint32 MaxCulledLightsPerCell;
		uint32 LightGridPixelSizeShift;
		FVector LightGridZParams;
		FVector DirectionalLightDirection;
		float Pading01;
		FVector DirectionalLightColor;
		float DirectionalLightVolumetricScatteringIntensity;
		uint32 DirectionalLightShadowMapChannelMask;
		Vector2 DirectionalLightDistanceFadeMAD;
		uint32 NumDirectionalLightCascades;
		Vector4 CascadeEndDepths;
		FMatrix DirectionalLightWorldToShadowMatrix[GMaxForwardShadowCascades];
		Vector4 DirectionalLightShadowmapMinMax[GMaxForwardShadowCascades];
		Vector4 DirectionalLightShadowmapAtlasBufferSize;
		float DirectionalLightDepthBias;
		uint32 DirectionalLightUseStaticShadowing;
		float Pading02;
		float Pading03;
		Vector4 DirectionalLightStaticShadowBufferSize;
		FMatrix DirectionalLightWorldToStaticShadow;
	} Constants;
	
	ID3D11ShaderResourceView* DirectionalLightShadowmapAtlas;
	ID3D11SamplerState* ShadowmapSampler;
	ID3D11ShaderResourceView* DirectionalLightStaticShadowmap;
	ID3D11SamplerState* StaticShadowmapSampler;
	ID3D11ShaderResourceView* ForwardLocalLightBuffer; //StrongTypedBuffer<float4>
	ID3D11ShaderResourceView* NumCulledLightsGrid; //StrongTypedBuffer<uint>
	ID3D11ShaderResourceView* CulledLightDataGrid;// StrongTypedBuffer<uint>

	static std::string GetConstantBufferName()
	{
		return "ForwardLightData";
	}

#define ADD_RES(StructName, MemberName) List.insert(std::make_pair(std::string(#StructName) + "_" + std::string(#MemberName),StructName.MemberName))
	static std::map<std::string, ID3D11ShaderResourceView*> GetSRVs(const FForwardLightData& ForwardLightData)
	{
		std::map<std::string, ID3D11ShaderResourceView*> List;
		ADD_RES(ForwardLightData, DirectionalLightShadowmapAtlas);
		ADD_RES(ForwardLightData, DirectionalLightStaticShadowmap);
		ADD_RES(ForwardLightData, ForwardLocalLightBuffer);
		ADD_RES(ForwardLightData, NumCulledLightsGrid);
		ADD_RES(ForwardLightData, CulledLightDataGrid);
		return List;
	}
	static std::map<std::string, ID3D11SamplerState*> GetSamplers(const FForwardLightData& ForwardLightData)
	{
		std::map<std::string, ID3D11SamplerState*> List;
		ADD_RES(ForwardLightData, ShadowmapSampler);
		ADD_RES(ForwardLightData, StaticShadowmapSampler);
		return List;
	}
	static std::map<std::string, ID3D11UnorderedAccessView*> GetUAVs(const FForwardLightData& ForwardLightData)
	{
		std::map<std::string, ID3D11UnorderedAccessView*> List;
		return List;
	}
#undef ADD_RES
};

static const int32 GMaxNumReflectionCaptures = 341;

struct alignas(16) FReflectionCaptureShaderData
{
	FReflectionCaptureShaderData()
	{
		ConstructUniformBufferInfo(*this);
	}
	struct ContantStruct
	{
		Vector4 PositionAndRadius[GMaxNumReflectionCaptures];
		// R is brightness, G is array index, B is shape
		Vector4 CaptureProperties[GMaxNumReflectionCaptures];
		Vector4 CaptureOffsetAndAverageBrightness[GMaxNumReflectionCaptures];
		// Stores the box transform for a box shape, other data is packed for other shapes
		FMatrix BoxTransform[GMaxNumReflectionCaptures];
		Vector4 BoxScales[GMaxNumReflectionCaptures];
	} Constants;
	
	static std::string GetConstantBufferName()
	{
		return "ReflectionCapture";
	}
	static std::map<std::string, ID3D11ShaderResourceView*> GetSRVs(const FReflectionCaptureShaderData& ReflectionCapture)
	{
		std::map<std::string, ID3D11ShaderResourceView*> List;
		return List;
	}
	static std::map<std::string, ID3D11SamplerState*> GetSamplers(const FReflectionCaptureShaderData& ReflectionCapture)
	{
		std::map<std::string, ID3D11SamplerState*> List;
		return List;
	}
	static std::map<std::string, ID3D11UnorderedAccessView*> GetUAVs(const FReflectionCaptureShaderData& ReflectionCapture)
	{
		std::map<std::string, ID3D11UnorderedAccessView*> List;
		return List;
	}
};
class FViewInfo : public FSceneView
{
public:
	/* Final position of the view in the final render target (in pixels), potentially scaled by ScreenPercentage */
	FIntRect ViewRect;

	/** Cached view uniform shader parameters, to allow recreating the view uniform buffer without having to fill out the entire struct. */
	FViewUniformShaderParameters* CachedViewUniformShaderParameters;

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
	std::vector<FVisibleLightViewInfo> VisibleLightInfos;

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
	float MaterialTextureMipBias;

	/** Precomputed visibility data, the bits are indexed by VisibilityId of a primitive component. */
	//const uint8* PrecomputedVisibilityData;

	//FOcclusionQueryBatcher IndividualOcclusionQueries;
	//FOcclusionQueryBatcher GroupedOcclusionQueries;

	// Hierarchical Z Buffer
	ComPtr<struct PooledRenderTarget> HZB;

	//int32 NumBoxReflectionCaptures;
	//int32 NumSphereReflectionCaptures;
	//float FurthestReflectionCaptureDistance;
	TUniformBufferPtr<FReflectionCaptureShaderData> ReflectionCaptureUniformBuffer;

	/** Used when there is no view state, buffers reallocate every frame. */
	//TUniquePtr<FForwardLightingViewResources> ForwardLightingResourcesStorage;

	//FVolumetricFogViewResources VolumetricFogResources;

	// Size of the HZB's mipmap 0
	// NOTE: the mipmap 0 is downsampled version of the depth buffer
	FIntPoint HZBMipmap0Size;

	/** Used by occlusion for percent unoccluded calculations. */
	//float OneOverNumPossiblePixels;

	// Mobile gets one light-shaft, this light-shaft.
	//FVector4 LightShaftCenter;
	//FLinearColor LightShaftColorMask;
	//FLinearColor LightShaftColorApply;
	//bool bLightShaftUse;

	//FHeightfieldLightingViewInfo HeightfieldLightingViewInfo;

	TShaderMap<FGlobalShaderType>* ShaderMap;

	bool bIsSnapshot;

	// Optional stencil dithering optimization during prepasses
	//bool bAllowStencilDither;

	/** Custom visibility query for view */
	//ICustomVisibilityQuery* CustomVisibilityQuery;

	FViewInfo(const ViewInitOptions& InitOptions);
	explicit FViewInfo(const FSceneView* InView);
	//TArray<FPrimitiveSceneInfo*, SceneRenderingAllocator> IndirectShadowPrimitives;
	/**
	* Destructor.
	*/
	~FViewInfo();

	/** Returns the size of view rect after primary upscale ( == only with secondary screen percentage). */
	//FIntPoint GetSecondaryViewRectSize() const;

	/** Returns whether the view requires a secondary upscale. */
	// 	bool RequiresSecondaryUpscale() const
	// 	{
	// 		return UnscaledViewRect.Size() != GetSecondaryViewRectSize();
	// 	}

	/** Creates ViewUniformShaderParameters given a set of view transforms. */
	void SetupUniformBufferParameters(
		FSceneRenderTargets& SceneContext,
		const FViewMatrices& InViewMatrices,
		const FViewMatrices& InPrevViewMatrices,
		FBox* OutTranslucentCascadeBoundsArray,
		int32 NumTranslucentCascades,
		FViewUniformShaderParameters& ViewUniformParameters) const;

	/** Recreates ViewUniformShaderParameters, taking the view transform from the View Matrices */
	inline void SetupUniformBufferParameters(
		FSceneRenderTargets& SceneContext,
		FBox* OutTranslucentCascadeBoundsArray,
		int32 NumTranslucentCascades,
		FViewUniformShaderParameters& ViewUniformParameters) const
	{
		SetupUniformBufferParameters(SceneContext,
			ViewMatrices,
			ViewMatrices,/*PrevViewInfo.ViewMatrices,*/
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
	FViewInfo* CreateSnapshot() const;

	/** Destroy all snapshots before we wipe the scene allocator. */
	static void DestroyAllSnapshots();

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
	void Init();

	/** Calculates bounding boxes for the translucency lighting volume cascades. */
	//void CalcTranslucencyLightingVolumeBounds(FBox* InOutCascadeBoundsArray, int32 NumCascades) const;

	/** Sets the sky SH irradiance map coefficients. */
	void SetupSkyIrradianceEnvironmentMapConstants(Vector4* OutSkyIrradianceEnvironmentMap) const;
};

class FSceneRenderer
{
public:
	EDepthDrawingMode EarlyZPassMode;

	FScene* Scene;
	/** The view family being rendered.  This references the Views array. */
	FSceneViewFamily ViewFamily;

	FSceneRenderer(FSceneViewFamily& InViewFamily);

	std::vector<FVisibleLightInfo> VisibleLightInfos;

	FSortedShadowMaps SortedShadowsForShadowDepthPass;

	std::vector<FViewInfo> Views;

	void InitViewsPossiblyAfterPrepass();

	void InitDynamicShadows();

	void InitProjectedShadowVisibility();

	void GatherShadowDynamicMeshElements();

	void AllocateShadowDepthTargets();

	void ComputeViewVisibility();

	void AllocatePerObjectShadowDepthTargets(std::vector<FProjectedShadowInfo*>& Shadows);

	void AllocateCachedSpotlightShadowDepthTargets(std::vector<FProjectedShadowInfo*>& CachedShadows);

	void AllocateCSMDepthTargets(const std::vector<FProjectedShadowInfo*>& WholeSceneDirectionalShadows);

	void AllocateRSMDepthTargets(const std::vector<FProjectedShadowInfo*>& RSMShadows);

	void AllocateOnePassPointLightDepthTargets(const std::vector<FProjectedShadowInfo*>& WholeScenePointShadows);

	void AllocateTranslucentShadowDepthTargets(std::vector<FProjectedShadowInfo*>& TranslucentShadows);

	void CreateWholeSceneProjectedShadow(FLightSceneInfo* LightSceneInfo, uint32& NumPointShadowCachesUpdatedThisFrame, uint32& NumSpotShadowCachesUpdatedThisFrame);

	void PrepareViewRectsForRendering();
	void InitViews();
	void RenderPrePassView(FViewInfo& View, const FDrawingPolicyRenderState& DrawRenderState);
	void RenderPrePass();
	void RenderHzb();
	void RenderShadowDepthMaps();
	void RenderShadowDepthMapAtlases();
	bool RenderBasePassStaticData(FViewInfo& View, const FDrawingPolicyRenderState& DrawRenderState);
	bool RenderBasePassStaticDataType(FViewInfo& View, const FDrawingPolicyRenderState& DrawRenderState, const EBasePassDrawListType DrawType);
	void RenderBasePassDynamicData(const FViewInfo& View, const FDrawingPolicyRenderState& DrawRenderState, bool& bOutDirty);
	bool RenderBasePassView(FViewInfo& View, FExclusiveDepthStencil::Type BasePassDepthStencilAccess, const FDrawingPolicyRenderState& InDrawRenderState);
	void RenderBasePass(FExclusiveDepthStencil::Type BasePassDepthStencilAccess,struct PooledRenderTarget* ForwardScreenSpaceShadowMask);
	void RenderLights();
	void RenderLight();
	void RenderAtmosphereFog();

	void Render();

	static FIntPoint GetDesiredInternalBufferSize(const FSceneViewFamily& ViewFamily);
public:
	/** The view family being rendered.  This references the Views array. */

};