#include "DeferredShading.h"
#include "LightSceneInfo.h"
#include "ShadowRendering.h"
#include "Scene.h"
#include "PrimitiveSceneInfo.h"
#include <algorithm>

const static uint32 SHADOW_BORDER = 4;

typedef std::vector<FConvexVolume> FLightViewFrustumConvexHulls;

float CalculateShadowFadeAlpha(const float MaxUnclampedResolution, const uint32 ShadowFadeResolution, const uint32 MinShadowResolution)
{
	// NB: MaxUnclampedResolution < 0 will return FadeAlpha = 0.0f. 

	float FadeAlpha = 0.0f;
	// Shadow size is above fading resolution.
	if (MaxUnclampedResolution > ShadowFadeResolution)
	{
		FadeAlpha = 1.0f;
	}
	// Shadow size is below fading resolution but above min resolution.
	else if (MaxUnclampedResolution > MinShadowResolution)
	{
		const float Exponent = 0.25f;// CVarShadowFadeExponent.GetValueOnRenderThread();

		// Use the limit case ShadowFadeResolution = MinShadowResolution
		// to gracefully handle this case.
		if (MinShadowResolution >= ShadowFadeResolution)
		{
			const float SizeRatio = (float)(MaxUnclampedResolution - MinShadowResolution);
			FadeAlpha = 1.0f - FMath::Pow(SizeRatio, Exponent);
		}
		else
		{
			const float InverseRange = 1.0f / (ShadowFadeResolution - MinShadowResolution);
			const float FirstFadeValue = FMath::Pow(InverseRange, Exponent);
			const float SizeRatio = (float)(MaxUnclampedResolution - MinShadowResolution) * InverseRange;
			// Rescale the fade alpha to reduce the change between no fading and the first value, which reduces popping with small ShadowFadeExponent's
			FadeAlpha = (FMath::Pow(SizeRatio, Exponent) - FirstFadeValue) / (1.0f - FirstFadeValue);
		}
	}
	return FadeAlpha;
}

void ComputeWholeSceneShadowCacheModes(
	const FLightSceneInfo* LightSceneInfo,
	bool bCubeShadowMap,
	float RealTime,
	float ActualDesiredResolution,
	FScene* Scene,
	FWholeSceneProjectedShadowInitializer& InOutProjectedShadowInitializer,
	FIntPoint& InOutShadowMapSize,
	uint32& InOutNumPointShadowCachesUpdatedThisFrame,
	uint32& InOutNumSpotShadowCachesUpdatedThisFrame,
	int32& OutNumShadowMaps,
	EShadowDepthCacheMode* OutCacheModes)
{
	// Strategy:
	// - Try to fallback if over budget. Budget is defined as number of updates currently
	// - Only allow fallback for updates caused by resolution changes
	// - Always render if cache doesn't exist or has been released
	uint32* NumCachesUpdatedThisFrame = nullptr;
	uint32 MaxCacheUpdatesAllowed = 0;

	switch (LightSceneInfo->Proxy->GetLightType())
	{
	case LightType_Point:
	case LightType_Rect:
		NumCachesUpdatedThisFrame = &InOutNumPointShadowCachesUpdatedThisFrame;
		MaxCacheUpdatesAllowed = static_cast<uint32>(-1/*GMaxNumPointShadowCacheUpdatesPerFrame*/);
		break;
	case LightType_Spot:
		NumCachesUpdatedThisFrame = &InOutNumSpotShadowCachesUpdatedThisFrame;
		MaxCacheUpdatesAllowed = static_cast<uint32>(-1/*GMaxNumSpotShadowCacheUpdatesPerFrame*/);
		break;
	default:
		assert(false);//, TEXT("Directional light isn't handled here"));
		break;
	}

	//if (GCacheWholeSceneShadows && (!bCubeShadowMap || RHISupportsGeometryShaders(GShaderPlatformForFeatureLevel[Scene->GetFeatureLevel()]) || RHISupportsVertexShaderLayer(GShaderPlatformForFeatureLevel[Scene->GetFeatureLevel()])))
	{
		auto CachedShadowMapDataIt = Scene->CachedShadowMaps.find(LightSceneInfo->Id);

		if (CachedShadowMapDataIt != Scene->CachedShadowMaps.end())
		{
			FCachedShadowMapData* CachedShadowMapData = &CachedShadowMapDataIt->second;

			if (InOutProjectedShadowInitializer.IsCachedShadowValid(CachedShadowMapData->Initializer))
			{
				if (CachedShadowMapData->ShadowMap.IsValid() && CachedShadowMapData->ShadowMap.GetSize() == InOutShadowMapSize)
				{
					OutNumShadowMaps = 1;
					OutCacheModes[0] = SDCM_MovablePrimitivesOnly;
				}
				else
				{
					OutNumShadowMaps = 2;
					// Note: ShadowMap with static primitives rendered first so movable shadowmap can composite
					OutCacheModes[0] = SDCM_StaticPrimitivesOnly;
					OutCacheModes[1] = SDCM_MovablePrimitivesOnly;
					++*NumCachesUpdatedThisFrame;
				}
			}
			else
			{
				OutNumShadowMaps = 1;
				OutCacheModes[0] = SDCM_Uncached;
				CachedShadowMapData->ShadowMap.DepthTarget.Reset();// = NULL;
			}

			CachedShadowMapData->Initializer = InOutProjectedShadowInitializer;
			CachedShadowMapData->LastUsedTime = RealTime;
		}
		else
		{
			OutNumShadowMaps = 2;
			// Note: ShadowMap with static primitives rendered first so movable shadowmap can composite
			OutCacheModes[0] = SDCM_StaticPrimitivesOnly;
			OutCacheModes[1] = SDCM_MovablePrimitivesOnly;
			++*NumCachesUpdatedThisFrame;
			Scene->CachedShadowMaps.insert(std::make_pair(LightSceneInfo->Id, FCachedShadowMapData(InOutProjectedShadowInitializer, RealTime)) );
		}
	}
// 	else
// 	{
// 		OutNumShadowMaps = 1;
// 		OutCacheModes[0] = SDCM_Uncached;
// 		Scene->CachedShadowMaps.erase(LightSceneInfo->Id);
// 	}
}
FProjectedShadowInfo::FProjectedShadowInfo()
	: ShadowDepthView(NULL)
	, CacheMode(SDCM_Uncached)
	, DependentView(0)
	, ShadowId(-1)
	, PreShadowTranslation(0, 0, 0)
	, ShadowBounds(0)
	, X(0)
	, Y(0)
	, ResolutionX(0)
	, ResolutionY(0)
	, BorderSize(0)
	, MaxScreenPercent(1.0f)
	, bAllocated(false)
	, bRendered(false)
	, bAllocatedInPreshadowCache(false)
	, bDepthsCached(false)
	, bDirectionalLight(false)
	, bOnePassPointLightShadow(false)
	, bWholeSceneShadow(false)
	, bReflectiveShadowmap(false)
	, bTranslucentShadow(false)
	, bRayTracedDistanceField(false)
	, bCapsuleShadow(false)
	, bPreShadow(false)
	, bSelfShadowOnly(false)
	, bPerObjectOpaqueShadow(false)
	, bTransmission(false)
	, LightSceneInfo(0)
	//, ParentSceneInfo(0)
	, ShaderDepthBias(0.0f)
{
}
void FProjectedShadowInfo::SetupWholeSceneProjection(
	FLightSceneInfo* InLightSceneInfo, 
	FViewInfo* InDependentView, 
	const FWholeSceneProjectedShadowInitializer& Initializer, 
	uint32 InResolutionX, 
	uint32 InResolutionY, 
	uint32 InBorderSize, 
	bool bInReflectiveShadowMap)
{
	LightSceneInfo = InLightSceneInfo;
	LightSceneInfoCompact = InLightSceneInfo;
	DependentView = InDependentView;
	PreShadowTranslation = Initializer.PreShadowTranslation;
	CascadeSettings = Initializer.CascadeSettings;
	ResolutionX = InResolutionX;
	ResolutionY = InResolutionY;
	bDirectionalLight = InLightSceneInfo->Proxy->GetLightType() == LightType_Directional;
	bOnePassPointLightShadow = Initializer.bOnePassPointLightShadow;
	bRayTracedDistanceField = Initializer.bRayTracedDistanceField;
	bWholeSceneShadow = true;
	//bTransmission = InLightSceneInfo->Proxy->Transmission();
	bReflectiveShadowmap = bInReflectiveShadowMap;
	BorderSize = InBorderSize;

	FVector	XAxis, YAxis;
	Initializer.FaceDirection.FindBestAxisVectors(XAxis, YAxis);
	const FMatrix WorldToLightScaled = Initializer.WorldToLight * FScaleMatrix(Initializer.Scales);
	const FMatrix WorldToFace = WorldToLightScaled * FBasisVectorMatrix(-XAxis, YAxis, Initializer.FaceDirection.GetSafeNormal(), FVector::ZeroVector);

	MaxSubjectZ = WorldToFace.TransformPosition(Initializer.SubjectBounds.Origin).Z + Initializer.SubjectBounds.SphereRadius;
	MinSubjectZ = FMath::Max(MaxSubjectZ - Initializer.SubjectBounds.SphereRadius * 2, Initializer.MinLightW);

	if (bInReflectiveShadowMap)
	{
	}
	else
	{
		if (bDirectionalLight)
		{
			// Limit how small the depth range can be for smaller cascades
			// This is needed for shadow modes like subsurface shadows which need depth information outside of the smaller cascade depth range
			//@todo - expose this value to the ini
			const float DepthRangeClamp = 5000;
			MaxSubjectZ = FMath::Max(MaxSubjectZ, DepthRangeClamp);
			MinSubjectZ = FMath::Min(MinSubjectZ, -DepthRangeClamp);

			// Transform the shadow's position into shadowmap space
			const FVector TransformedPosition = WorldToFace.TransformPosition(-PreShadowTranslation);

			// Largest amount that the shadowmap will be downsampled to during sampling
			// We need to take this into account when snapping to get a stable result
			// This corresponds to the maximum kernel filter size used by subsurface shadows in ShadowProjectionPixelShader.usf
			const int32 MaxDownsampleFactor = 4;
			// Determine the distance necessary to snap the shadow's position to the nearest texel
			const float SnapX = FMath::Fmod(TransformedPosition.X, 2.0f * MaxDownsampleFactor / InResolutionX);
			const float SnapY = FMath::Fmod(TransformedPosition.Y, 2.0f * MaxDownsampleFactor / InResolutionY);
			// Snap the shadow's position and transform it back into world space
			// This snapping prevents sub-texel camera movements which removes view dependent aliasing from the final shadow result
			// This only maintains stable shadows under camera translation and rotation
			const FVector SnappedWorldPosition = WorldToFace.InverseFast().TransformPosition(TransformedPosition - FVector(SnapX, SnapY, 0.0f));
			PreShadowTranslation = -SnappedWorldPosition;
		}

		if (CascadeSettings.ShadowSplitIndex >= 0 && bDirectionalLight)
		{
			assert(InDependentView);

			ShadowBounds = InLightSceneInfo->Proxy->GetShadowSplitBounds(
				*InDependentView,
				bRayTracedDistanceField ? INDEX_NONE : CascadeSettings.ShadowSplitIndex,
				false/*InLightSceneInfo->IsPrecomputedLightingValid()*/,
				0);
		}
		else
		{
			ShadowBounds = FSphere(-Initializer.PreShadowTranslation, Initializer.SubjectBounds.SphereRadius);
		}

		// Any meshes between the light and the subject can cast shadows, also any meshes inside the subject region
		const FMatrix CasterMatrix = WorldToFace * FShadowProjectionMatrix(Initializer.MinLightW, MaxSubjectZ, Initializer.WAxis);
		GetViewFrustumBounds(CasterFrustum, CasterMatrix, true);
	}

	assert(MaxSubjectZ > MinSubjectZ);// , TEXT("MaxSubjectZ %f MinSubjectZ %f SubjectBounds.SphereRadius %f"), MaxSubjectZ, MinSubjectZ, Initializer.SubjectBounds.SphereRadius);

	const float ClampedMaxLightW = FMath::Min(MinSubjectZ + Initializer.MaxDistanceToCastInLightW, (float)HALF_WORLD_MAX);
	MinPreSubjectZ = Initializer.MinLightW;

	SubjectAndReceiverMatrix = WorldToFace * FShadowProjectionMatrix(MinSubjectZ, MaxSubjectZ, Initializer.WAxis);
	ReceiverMatrix = WorldToFace * FShadowProjectionMatrix(MinSubjectZ, ClampedMaxLightW, Initializer.WAxis);

	float MaxSubjectDepth = SubjectAndReceiverMatrix.TransformPosition(
		Initializer.SubjectBounds.Origin
		+ WorldToLightScaled.InverseFast().TransformVector(Initializer.FaceDirection) * Initializer.SubjectBounds.SphereRadius
	).Z;

	if (bOnePassPointLightShadow)
	{
		MaxSubjectDepth = Initializer.SubjectBounds.SphereRadius;
	}

	InvMaxSubjectDepth = 1.0f / MaxSubjectDepth;

	// Store the view matrix
	// Reorder the vectors to match the main view, since ShadowViewMatrix will be used to override the main view's view matrix during shadow depth rendering
	ShadowViewMatrix = Initializer.WorldToLight *
		FMatrix(
			FPlane(0, 0, 1, 0),
			FPlane(1, 0, 0, 0),
			FPlane(0, 1, 0, 0),
			FPlane(0, 0, 0, 1));

	InvReceiverMatrix = ReceiverMatrix.InverseFast();

	GetViewFrustumBounds(ReceiverFrustum, ReceiverMatrix, true);

	UpdateShaderDepthBias();
}

void FProjectedShadowInfo::UpdateShaderDepthBias()
{
	ShaderDepthBias = 0.f;
}

static float GMinScreenRadiusForShadowCaster = 0.01f;
void FProjectedShadowInfo::AddSubjectPrimitive(FPrimitiveSceneInfo* PrimitiveSceneInfo, std::vector<FViewInfo>* ViewArray, bool bRecordShadowSubjectForMobileShading)
{
	assert(!bRayTracedDistanceField);

	if (std::find(ReceiverPrimitives.begin(), ReceiverPrimitives.end(), PrimitiveSceneInfo) == ReceiverPrimitives.end()
		// Far cascade only casts from primitives marked for it
		&& (!CascadeSettings.bFarShadowCascade || PrimitiveSceneInfo->Proxy->CastsFarShadow()))
	{
		const FPrimitiveSceneProxy* Proxy = PrimitiveSceneInfo->Proxy;

		std::vector<FViewInfo*> Views;
		const bool bWholeSceneDirectionalShadow = IsWholeSceneDirectionalShadow();

		if (bWholeSceneDirectionalShadow)
		{
			Views.push_back(DependentView);
		}
		else
		{
// 			checkf(ViewArray,
// 				TEXT("bWholeSceneShadow=%d, CascadeSettings.ShadowSplitIndex=%d, bDirectionalLight=%s"),
// 				bWholeSceneShadow ? TEXT("true") : TEXT("false"),
// 				CascadeSettings.ShadowSplitIndex,
// 				bDirectionalLight ? TEXT("true") : TEXT("false"));

			for (uint32 ViewIndex = 0; ViewIndex < ViewArray->size(); ViewIndex++)
			{
				Views.push_back(&(*ViewArray)[ViewIndex]);
			}
		}

		uint32 ViewMask = 0;
		int32 PrimitiveId = PrimitiveSceneInfo->GetIndex();

		if (PrimitiveSceneInfo->NeedsLazyUpdateForRendering())
		{
			if (PrimitiveSceneInfo->NeedsUpdateStaticMeshes())
			{
				PrimitiveSceneInfo->ConditionalLazyUpdateForRendering();
			}
			else
			{
				PrimitiveSceneInfo->ConditionalLazyUpdateForRendering();
			}
		}

		const FBoxSphereBounds& Bounds = Proxy->GetBounds();
		bool bDrawingStaticMeshes = false;

		if (PrimitiveSceneInfo->StaticMeshes.size() > 0)
		{
			for (uint32 ViewIndex = 0, ViewCount = Views.size(); ViewIndex < ViewCount; ViewIndex++)
			{
				FViewInfo& CurrentView = *Views[ViewIndex];

				const float DistanceSquared = (Bounds.Origin - CurrentView.ShadowViewMatrices.GetViewOrigin()).SizeSquared();

				if (bWholeSceneShadow)
				{
					const bool bDrawShadowDepth = FMath::Square(Bounds.SphereRadius) > FMath::Square(GMinScreenRadiusForShadowCaster) * DistanceSquared * CurrentView.LODDistanceFactorSquared;
					if (!bDrawShadowDepth)
					{
						// cull object if it's too small to be considered as shadow caster
						continue;
					}
				}

				// Update visibility for meshes which weren't visible in the main views or were visible with static relevance
				//if (!CurrentView.PrimitiveVisibilityMap[PrimitiveId] || CurrentView.PrimitiveViewRelevanceMap[PrimitiveId].bStaticRelevance)
				{
					bDrawingStaticMeshes |= true;// ShouldDrawStaticMeshes(CurrentView, bCustomDataRelevance, PrimitiveSceneInfo);
				}
			}
		}

		if (bDrawingStaticMeshes)
		{
			if (!bWholeSceneDirectionalShadow)
			{
				// Add the primitive's static mesh elements to the draw lists.
				for (uint32 MeshIndex = 0; MeshIndex < PrimitiveSceneInfo->StaticMeshes.size(); MeshIndex++)
				{
					FStaticMesh& StaticMesh = *PrimitiveSceneInfo->StaticMeshes[MeshIndex];
					if (StaticMesh.CastShadow)
					{
						const FMaterialRenderProxy* MaterialRenderProxy = StaticMesh.MaterialRenderProxy;
						const FMaterial* Material = MaterialRenderProxy->GetMaterial();
						const EBlendMode BlendMode = Material->GetBlendMode();
						const EMaterialShadingModel ShadingModel = Material->GetShadingModel();

						if (/*Material->ShouldCastDynamicShadows()*/true || (bReflectiveShadowmap && Material->ShouldInjectEmissiveIntoLPV()))
						{
							const bool bTwoSided = false;// Material->IsTwoSided() || PrimitiveSceneInfo->Proxy->CastsShadowAsTwoSided();
							//OverrideWithDefaultMaterialForShadowDepth(MaterialRenderProxy, Material, bReflectiveShadowmap, FeatureLevel);
							StaticSubjectMeshElements.push_back(FShadowStaticMeshElement(MaterialRenderProxy, Material, &StaticMesh, bTwoSided));
						}
					}
				}
			}
		}
		else
		{
			// Add the primitive to the subject primitive list.
			DynamicSubjectPrimitives.push_back(PrimitiveSceneInfo);
		}
	}
}

bool FProjectedShadowInfo::HasSubjectPrims() const
{
	return DynamicSubjectPrimitives.size() > 0
		|| StaticSubjectMeshElements.size() > 0;
// 		|| EmissiveOnlyPrimitives.Num() > 0
// 		|| EmissiveOnlyMeshElements.Num() > 0
// 		|| GIBlockingMeshElements.Num() > 0
// 		|| GIBlockingPrimitives.Num() > 0;

}

int32 GCachedShadowsCastFromMovablePrimitives = 1;
static int32 GShadowLightViewConvexHullCull = 1;
void BuildLightViewFrustumConvexHull(const FVector& LightOrigin, const FConvexVolume& Frustum, FConvexVolume& ConvexHull)
{
	// This function assumes that there are 5 planes, which is the case with an infinite projection matrix
	// If this isn't the case, we should really know about it, so assert.
	const int32 EdgeCount = 12;
	const int32 PlaneCount = 5;
	assert(Frustum.Planes.size() == PlaneCount);

	enum EFrustumPlanes
	{
		FLeft,
		FRight,
		FTop,
		FBottom,
		FFar
	};

	const EFrustumPlanes Edges[EdgeCount][2] =
	{
		{ FFar  , FLeft },{ FFar  , FRight },
	{ FFar  , FTop },{ FFar  , FBottom },
	{ FLeft , FTop },{ FLeft , FBottom },
	{ FRight, FTop },{ FRight, FBottom }
	};

	float Distance[PlaneCount];
	bool  Visible[PlaneCount];

	for (int32 PlaneIndex = 0; PlaneIndex < PlaneCount; ++PlaneIndex)
	{
		const FPlane& Plane = Frustum.Planes[PlaneIndex];
		float Dist = Plane.PlaneDot(LightOrigin);
		bool bVisible = Dist < 0.0f;

		Distance[PlaneIndex] = Dist;
		Visible[PlaneIndex] = bVisible;

		if (bVisible)
		{
			ConvexHull.Planes.push_back(Plane);
		}
	}

	for (int32 EdgeIndex = 0; EdgeIndex < EdgeCount; ++EdgeIndex)
	{
		EFrustumPlanes I1 = Edges[EdgeIndex][0];
		EFrustumPlanes I2 = Edges[EdgeIndex][1];

		// Silhouette edge
		if (Visible[I1] != Visible[I2])
		{
			// Add plane that passes through edge and light origin
			FPlane Plane = Frustum.Planes[I1] * Distance[I2] - Frustum.Planes[I2] * Distance[I1];
			if (Visible[I2])
			{
				Plane = Plane.Flip();
			}
			ConvexHull.Planes.push_back(Plane);
		}
	}

	ConvexHull.Init();
}

void BuildLightViewFrustumConvexHulls(const FVector& LightOrigin, const std::vector<FViewInfo>& Views, FLightViewFrustumConvexHulls& ConvexHulls)
{
	if (GShadowLightViewConvexHullCull == 0)
	{
		return;
	}


	ConvexHulls.reserve(Views.size());
	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ++ViewIndex)
	{
		// for now only support perspective projection as ortho camera shadows are broken anyway
		FViewInfo const& View = Views[ViewIndex];
		if (View.IsPerspectiveProjection())
		{
			FConvexVolume ConvexHull;
			BuildLightViewFrustumConvexHull(LightOrigin, View.ViewFrustum, ConvexHull);
			ConvexHulls.push_back(ConvexHull);
		}
	}
}

bool IntersectsConvexHulls(FLightViewFrustumConvexHulls const& ConvexHulls, FBoxSphereBounds const& Bounds)
{
	if (ConvexHulls.size() == 0)
	{
		return true;
	}

	for (uint32 Index = 0; Index < ConvexHulls.size(); ++Index)
	{
		FConvexVolume const& Hull = ConvexHulls[Index];
		if (Hull.IntersectBox(Bounds.Origin, Bounds.BoxExtent))
		{
			return true;
		}
	}

	return false;
}

void FSceneRenderer::CreateWholeSceneProjectedShadow(FLightSceneInfo* LightSceneInfo, uint32& InOutNumPointShadowCachesUpdatedThisFrame, uint32& InOutNumSpotShadowCachesUpdatedThisFrame)
{
	FVisibleLightInfo& VisibleLightInfo = VisibleLightInfos[LightSceneInfo->Id];

	std::vector<FWholeSceneProjectedShadowInitializer> ProjectedShadowInitializers;
	if (LightSceneInfo->Proxy->GetWholeSceneProjectedShadowInitializer(ViewFamily, ProjectedShadowInitializers))
	{
		FSceneRenderTargets& SceneContext_ConstantsOnly = FSceneRenderTargets::Get();

		assert(ProjectedShadowInitializers.size() > 0);

		// Shadow resolution constants.
		const uint32 ShadowBorder = ProjectedShadowInitializers[0].bOnePassPointLightShadow ? 0 : SHADOW_BORDER;
		const uint32 EffectiveDoubleShadowBorder = ShadowBorder * 2;
		const uint32 MinShadowResolution = FMath::Max<int32>(0, 32/*CVarMinShadowResolution.GetValueOnRenderThread()*/);
		const int32 MaxShadowResolutionSetting = /*GetCachedScalabilityCVars().MaxShadowResolution*/2048;
		const FIntPoint ShadowBufferResolution = SceneContext_ConstantsOnly.GetShadowDepthTextureResolution();
		const uint32 MaxShadowResolution = FMath::Min(MaxShadowResolutionSetting, ShadowBufferResolution.X) - EffectiveDoubleShadowBorder;
		const uint32 MaxShadowResolutionY = FMath::Min(MaxShadowResolutionSetting, ShadowBufferResolution.Y) - EffectiveDoubleShadowBorder;
		const uint32 ShadowFadeResolution = FMath::Max<int32>(0, 64/*CVarShadowFadeResolution.GetValueOnRenderThread()*/);

		// Compute the maximum resolution required for the shadow by any view. Also keep track of the unclamped resolution for fading.
		float MaxDesiredResolution = 0;
		std::vector<float> FadeAlphas;
		float MaxFadeAlpha = 0;
		bool bStaticSceneOnly = false;
		bool bAnyViewIsSceneCapture = false;

		for (uint32 ViewIndex = 0, ViewCount = Views.size(); ViewIndex < ViewCount; ++ViewIndex)
		{
			const FViewInfo& View = Views[ViewIndex];

			const float ScreenRadius = LightSceneInfo->Proxy->GetEffectiveScreenRadius(View.ShadowViewMatrices);

			// Determine the amount of shadow buffer resolution needed for this view.
			float UnclampedResolution = 1.0f;

			switch (LightSceneInfo->Proxy->GetLightType())
			{
			case LightType_Point:
				UnclampedResolution = ScreenRadius * 1.27324f;// CVarShadowTexelsPerPixelPointlight.GetValueOnRenderThread();
				break;
			case LightType_Spot:
				UnclampedResolution = ScreenRadius * 2.0f * 1.27324f;// CVarShadowTexelsPerPixelSpotlight.GetValueOnRenderThread();
				break;
			case LightType_Rect:
				UnclampedResolution = ScreenRadius * 1.27324f;;// CVarShadowTexelsPerPixelRectlight.GetValueOnRenderThread();
				break;
			default:
				// directional lights are not handled here
				assert(false);//, TEXT("Unexpected LightType %d appears in CreateWholeSceneProjectedShadow %s"), (int32)LightSceneInfo->Proxy->GetLightType(), *LightSceneInfo->Proxy->GetComponentName().ToString());
			}

			// Compute FadeAlpha before ShadowResolutionScale contribution (artists want to modify the softness of the shadow, not change the fade ranges)
			const float FadeAlpha = CalculateShadowFadeAlpha(UnclampedResolution, ShadowFadeResolution, MinShadowResolution);
			MaxFadeAlpha = FMath::Max(MaxFadeAlpha, FadeAlpha);
			FadeAlphas.push_back(FadeAlpha);

			const float ShadowResolutionScale = 1.0f;//LightSceneInfo->Proxy->GetShadowResolutionScale();

			float ClampedResolution = UnclampedResolution;

			if (ShadowResolutionScale > 1.0f)
			{
				// Apply ShadowResolutionScale before the MaxShadowResolution clamp if raising the resolution
				ClampedResolution *= ShadowResolutionScale;
			}

			ClampedResolution = FMath::Min<float>(ClampedResolution, MaxShadowResolution);

			if (ShadowResolutionScale <= 1.0f)
			{
				// Apply ShadowResolutionScale after the MaxShadowResolution clamp if lowering the resolution
				// Artists want to modify the softness of the shadow with ShadowResolutionScale
				ClampedResolution *= ShadowResolutionScale;
			}

			MaxDesiredResolution = FMath::Max(
				MaxDesiredResolution,
				FMath::Max<float>(
					ClampedResolution,
					FMath::Min<float>(MinShadowResolution, ShadowBufferResolution.X - EffectiveDoubleShadowBorder)
					)
			);

			bStaticSceneOnly = bStaticSceneOnly || View.bStaticSceneOnly;
			//bAnyViewIsSceneCapture = bAnyViewIsSceneCapture || View.bIsSceneCapture;
		}

		if (MaxFadeAlpha > 1.0f / 256.0f)
		{
			for (uint32 ShadowIndex = 0, ShadowCount = ProjectedShadowInitializers.size(); ShadowIndex < ShadowCount; ShadowIndex++)
			{
				FWholeSceneProjectedShadowInitializer& ProjectedShadowInitializer = ProjectedShadowInitializers[ShadowIndex];

				// Round down to the nearest power of two so that resolution changes are always doubling or halving the resolution, which increases filtering stability
				// Use the max resolution if the desired resolution is larger than that
				// FMath::CeilLogTwo(MaxDesiredResolution + 1.0f) instead of FMath::CeilLogTwo(MaxDesiredResolution) because FMath::CeilLogTwo takes
				// an uint32 as argument and this causes MaxDesiredResolution get truncated. For example, if MaxDesiredResolution is 256.1f,
				// FMath::CeilLogTwo returns 8 but the next line of code expects a 9 to work correctly
				int32 RoundedDesiredResolution = FMath::Max<int32>((1 << (FMath::CeilLogTwo(MaxDesiredResolution + 1.0f) - 1)) - ShadowBorder * 2, 1);
				int32 SizeX = MaxDesiredResolution >= MaxShadowResolution ? MaxShadowResolution : RoundedDesiredResolution;
				int32 SizeY = MaxDesiredResolution >= MaxShadowResolutionY ? MaxShadowResolutionY : RoundedDesiredResolution;

				if (ProjectedShadowInitializer.bOnePassPointLightShadow)
				{
					// Round to a resolution that is supported for one pass point light shadows
					SizeX = SizeY = SceneContext_ConstantsOnly.GetCubeShadowDepthZResolution(SceneContext_ConstantsOnly.GetCubeShadowDepthZIndex(MaxDesiredResolution));
				}

				int32 NumShadowMaps = 1;
				EShadowDepthCacheMode CacheMode[2] = { SDCM_Uncached, SDCM_Uncached };

				//if (!bAnyViewIsSceneCapture && !ProjectedShadowInitializer.bRayTracedDistanceField)
				{
					FIntPoint ShadowMapSize(SizeX + ShadowBorder * 2, SizeY + ShadowBorder * 2);

					ComputeWholeSceneShadowCacheModes(
						LightSceneInfo,
						ProjectedShadowInitializer.bOnePassPointLightShadow,
						/*ViewFamily.CurrentRealTime*/0,
						MaxDesiredResolution,
						Scene,
						// Below are in-out or out parameters. They can change
						ProjectedShadowInitializer,
						ShadowMapSize,
						InOutNumPointShadowCachesUpdatedThisFrame, 
						InOutNumSpotShadowCachesUpdatedThisFrame,
						NumShadowMaps,
						CacheMode);

					SizeX = ShadowMapSize.X - ShadowBorder * 2;
					SizeY = ShadowMapSize.Y - ShadowBorder * 2;
				}

				for (int32 CacheModeIndex = 0; CacheModeIndex < NumShadowMaps; CacheModeIndex++)
				{
					// Create the projected shadow info.
					FProjectedShadowInfo* ProjectedShadowInfo = new FProjectedShadowInfo;

					ProjectedShadowInfo->SetupWholeSceneProjection(
						LightSceneInfo,
						NULL,
						ProjectedShadowInitializer,
						SizeX,
						SizeY,
						ShadowBorder,
						false	// no RSM
					);

					ProjectedShadowInfo->CacheMode = CacheMode[CacheModeIndex];
					ProjectedShadowInfo->FadeAlphas = FadeAlphas;

					VisibleLightInfo.MemStackProjectedShadows.push_back(ProjectedShadowInfo);

					if (ProjectedShadowInitializer.bOnePassPointLightShadow)
					{
						const static FVector CubeDirections[6] =
						{
							FVector(-1, 0, 0),
							FVector(1, 0, 0),
							FVector(0, -1, 0),
							FVector(0, 1, 0),
							FVector(0, 0, -1),
							FVector(0, 0, 1)
						};

						const static FVector UpVectors[6] =
						{
							FVector(0, 1, 0),
							FVector(0, 1, 0),
							FVector(0, 0, -1),
							FVector(0, 0, 1),
							FVector(0, 1, 0),
							FVector(0, 1, 0)
						};

						const FLightSceneProxy& LightProxy = *(ProjectedShadowInfo->GetLightSceneInfo().Proxy);

						const FMatrix FaceProjection = FPerspectiveMatrix(PI / 4.0f, 1, 1, 1, LightProxy.GetRadius());
						const FVector LightPosition = LightProxy.GetPosition();

						ProjectedShadowInfo->OnePassShadowViewProjectionMatrices.clear();
						ProjectedShadowInfo->OnePassShadowFrustums.clear();
						ProjectedShadowInfo->OnePassShadowFrustums.resize(6);
						const FMatrix ScaleMatrix = FScaleMatrix(FVector(1, -1, 1));

						// fill in the caster frustum with the far plane from every face
						ProjectedShadowInfo->CasterFrustum.Planes.clear();
						for (int32 FaceIndex = 0; FaceIndex < 6; FaceIndex++)
						{
							// Create a view projection matrix for each cube face
							const FMatrix ShadowViewProjectionMatrix = FLookAtMatrix(LightPosition, LightPosition + CubeDirections[FaceIndex], UpVectors[FaceIndex]) * ScaleMatrix * FaceProjection;
							ProjectedShadowInfo->OnePassShadowViewProjectionMatrices.push_back(ShadowViewProjectionMatrix);
							// Create a convex volume out of the frustum so it can be used for object culling
							GetViewFrustumBounds(ProjectedShadowInfo->OnePassShadowFrustums[FaceIndex], ShadowViewProjectionMatrix, false);

							// Check we have a valid frustum
							if (ProjectedShadowInfo->OnePassShadowFrustums[FaceIndex].Planes.size() > 0)
							{
								// We are assuming here that the last plane is the far plane
								// we need to incorporate PreShadowTranslation (so it can be disincorporated later)
								FPlane Src = ProjectedShadowInfo->OnePassShadowFrustums[FaceIndex].Planes.back();
								// add world space preview translation
								Src.W += (FVector(Src) | ProjectedShadowInfo->PreShadowTranslation);
								ProjectedShadowInfo->CasterFrustum.Planes.push_back(Src);
							}
						}
						ProjectedShadowInfo->CasterFrustum.Init();
					}
					
					// Ray traced shadows use the GPU managed distance field object buffers, no CPU culling should be used
					if (!ProjectedShadowInfo->bRayTracedDistanceField)
					{
						// Build light-view convex hulls for shadow caster culling
						FLightViewFrustumConvexHulls LightViewFrustumConvexHulls;
						if (CacheMode[CacheModeIndex] != SDCM_StaticPrimitivesOnly)
						{
							FVector const& LightOrigin = LightSceneInfo->Proxy->GetOrigin();
							BuildLightViewFrustumConvexHulls(LightOrigin, Views, LightViewFrustumConvexHulls);
						}

						bool bCastCachedShadowFromMovablePrimitives = true;//GCachedShadowsCastFromMovablePrimitives || LightSceneInfo->Proxy->GetForceCachedShadowsForMovablePrimitives();
						if (CacheMode[CacheModeIndex] != SDCM_StaticPrimitivesOnly
							&& (CacheMode[CacheModeIndex] != SDCM_MovablePrimitivesOnly || bCastCachedShadowFromMovablePrimitives))
						{
							// Add all the shadow casting primitives affected by the light to the shadow's subject primitive list.
							for (FLightPrimitiveInteraction* Interaction = LightSceneInfo->DynamicInteractionOftenMovingPrimitiveList;
								Interaction;
								Interaction = Interaction->GetNextPrimitive())
							{
								if (Interaction->HasShadow()
									// If the primitive only wants to cast a self shadow don't include it in whole scene shadows.
									&& !Interaction->CastsSelfShadowOnly()
									&& (!bStaticSceneOnly || Interaction->GetPrimitiveSceneInfo()->Proxy->HasStaticLighting()))
								{
									FBoxSphereBounds const& Bounds = Interaction->GetPrimitiveSceneInfo()->Proxy->GetBounds();
									if (IntersectsConvexHulls(LightViewFrustumConvexHulls, Bounds))
									{
										ProjectedShadowInfo->AddSubjectPrimitive(Interaction->GetPrimitiveSceneInfo(), &Views,  false);
									}
								}
							}
						}

						if (CacheMode[CacheModeIndex] != SDCM_MovablePrimitivesOnly)
						{
							// Add all the shadow casting primitives affected by the light to the shadow's subject primitive list.
							for (FLightPrimitiveInteraction* Interaction = LightSceneInfo->DynamicInteractionStaticPrimitiveList;
								Interaction;
								Interaction = Interaction->GetNextPrimitive())
							{
								if (Interaction->HasShadow()
									// If the primitive only wants to cast a self shadow don't include it in whole scene shadows.
									&& !Interaction->CastsSelfShadowOnly()
									&& (!bStaticSceneOnly || Interaction->GetPrimitiveSceneInfo()->Proxy->HasStaticLighting()))
								{
									FBoxSphereBounds const& Bounds = Interaction->GetPrimitiveSceneInfo()->Proxy->GetBounds();
									if (IntersectsConvexHulls(LightViewFrustumConvexHulls, Bounds))
									{
										ProjectedShadowInfo->AddSubjectPrimitive(Interaction->GetPrimitiveSceneInfo(), &Views,  false);
									}
								}
							}
						}
					}
					
					bool bRenderShadow = true;

					if (CacheMode[CacheModeIndex] == SDCM_StaticPrimitivesOnly)
					{
						const bool bHasStaticPrimitives = ProjectedShadowInfo->HasSubjectPrims();
						bRenderShadow = bHasStaticPrimitives;
						FCachedShadowMapData& CachedShadowMapData = Scene->CachedShadowMaps.at(ProjectedShadowInfo->GetLightSceneInfo().Id);
						CachedShadowMapData.bCachedShadowMapHasPrimitives = bHasStaticPrimitives;
					}

					if (bRenderShadow)
					{
						VisibleLightInfo.AllProjectedShadows.push_back(ProjectedShadowInfo);
					}

				}
			}
		}
	}
}

void FSceneRenderer::InitProjectedShadowVisibility()
{

}

void FSceneRenderer::AllocateShadowDepthTargets()
{
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();

	// Sort visible shadows based on their allocation needs
	// 2d shadowmaps for this frame only that can be atlased across lights
	std::vector<FProjectedShadowInfo*> Shadows;
	// 2d shadowmaps that will persist across frames, can't be atlased
	std::vector<FProjectedShadowInfo*> CachedSpotlightShadows;
	std::vector<FProjectedShadowInfo*> TranslucentShadows;
	// 2d shadowmaps that persist across frames
	std::vector<FProjectedShadowInfo*> CachedPreShadows;
	std::vector<FProjectedShadowInfo*> RSMShadows;
	// Cubemaps, can't be atlased
	std::vector<FProjectedShadowInfo*> WholeScenePointShadows;

	for (auto LightIt = Scene->Lights.begin(); LightIt != Scene->Lights.end(); ++LightIt)
	{
		const FLightSceneInfoCompact& LightSceneInfoCompact = *LightIt;
		FLightSceneInfo* LightSceneInfo = LightSceneInfoCompact.LightSceneInfo;
		FVisibleLightInfo& VisibleLightInfo = VisibleLightInfos[LightSceneInfo->Id];

		// All cascades for a light need to be in the same texture
		std::vector<FProjectedShadowInfo*> WholeSceneDirectionalShadows;

		for (uint32 ShadowIndex = 0; ShadowIndex < VisibleLightInfo.AllProjectedShadows.size(); ++ShadowIndex)
		{
			FProjectedShadowInfo* ProjectedShadowInfo = VisibleLightInfo.AllProjectedShadows[ShadowIndex];

			bool bNeedsProjection = ProjectedShadowInfo->CacheMode != SDCM_StaticPrimitivesOnly
				// Mobile rendering only projects opaque per object shadows.
				&& (/*FeatureLevel >= ERHIFeatureLevel::SM4 || */ProjectedShadowInfo->bPerObjectOpaqueShadow);

			if (bNeedsProjection)
			{
				if (ProjectedShadowInfo->bReflectiveShadowmap)
				{
					VisibleLightInfo.RSMsToProject.push_back(ProjectedShadowInfo);
				}
				else if (ProjectedShadowInfo->bCapsuleShadow)
				{
					VisibleLightInfo.CapsuleShadowsToProject.push_back(ProjectedShadowInfo);
				}
				else
				{
					VisibleLightInfo.ShadowsToProject.push_back(ProjectedShadowInfo);
				}
			}

			const bool bNeedsShadowmapSetup = !ProjectedShadowInfo->bCapsuleShadow && !ProjectedShadowInfo->bRayTracedDistanceField;

			if (bNeedsShadowmapSetup)
			{
				if (ProjectedShadowInfo->bReflectiveShadowmap)
				{
					assert(ProjectedShadowInfo->bWholeSceneShadow);
					RSMShadows.push_back(ProjectedShadowInfo);
				}
				else if (ProjectedShadowInfo->bPreShadow && ProjectedShadowInfo->bAllocatedInPreshadowCache)
				{
					CachedPreShadows.push_back(ProjectedShadowInfo);
				}
				else if (ProjectedShadowInfo->bDirectionalLight && ProjectedShadowInfo->bWholeSceneShadow)
				{
					WholeSceneDirectionalShadows.push_back(ProjectedShadowInfo);
				}
				else if (ProjectedShadowInfo->bOnePassPointLightShadow)
				{
					WholeScenePointShadows.push_back(ProjectedShadowInfo);
				}
				else if (ProjectedShadowInfo->bTranslucentShadow)
				{
					TranslucentShadows.push_back(ProjectedShadowInfo);
				}
				else if (ProjectedShadowInfo->CacheMode == SDCM_StaticPrimitivesOnly)
				{
					assert(ProjectedShadowInfo->bWholeSceneShadow);
					CachedSpotlightShadows.push_back(ProjectedShadowInfo);
				}
				else
				{
					Shadows.push_back(ProjectedShadowInfo);
				}
			}
		}

		//VisibleLightInfo.ShadowsToProject.Sort(FCompareFProjectedShadowInfoBySplitIndex());
		//VisibleLightInfo.RSMsToProject.Sort(FCompareFProjectedShadowInfoBySplitIndex());
	}

	AllocateOnePassPointLightDepthTargets(WholeScenePointShadows);
	//AllocateRSMDepthTargets(RSMShadows);
	//AllocateCachedSpotlightShadowDepthTargets(CachedSpotlightShadows);
	AllocatePerObjectShadowDepthTargets(Shadows);
	//AllocateTranslucentShadowDepthTargets(TranslucentShadows);
}


void FSceneRenderer::AllocatePerObjectShadowDepthTargets(std::vector<FProjectedShadowInfo*>& Shadows)
{

}

void FSceneRenderer::AllocateCSMDepthTargets(const std::vector<FProjectedShadowInfo*>& WholeSceneDirectionalShadows)
{

}

void FSceneRenderer::AllocateOnePassPointLightDepthTargets(const std::vector<FProjectedShadowInfo*>& WholeScenePointShadows)
{
	for (uint32 ShadowIndex = 0; ShadowIndex < WholeScenePointShadows.size(); ShadowIndex++)
	{
		FProjectedShadowInfo* ProjectedShadowInfo = WholeScenePointShadows[ShadowIndex];
		assert(ProjectedShadowInfo->BorderSize == 0);

		if (ProjectedShadowInfo->CacheMode == SDCM_MovablePrimitivesOnly && !ProjectedShadowInfo->HasSubjectPrims())
		{
			FCachedShadowMapData& CachedShadowMapData = Scene->CachedShadowMaps.at(ProjectedShadowInfo->GetLightSceneInfo().Id);
			ProjectedShadowInfo->X = ProjectedShadowInfo->Y = 0;
			ProjectedShadowInfo->bAllocated = true;
			// Skip the shadow depth pass since there are no movable primitives to composite, project from the cached shadowmap directly which contains static primitive depths
			assert(CachedShadowMapData.ShadowMap.IsValid());
			ProjectedShadowInfo->RenderTargets.DepthTarget = CachedShadowMapData.ShadowMap.DepthTarget.Get();
		}
		else
		{
			SortedShadowsForShadowDepthPass.ShadowMapCubemaps.push_back(FSortedShadowMapAtlas());
			FSortedShadowMapAtlas& ShadowMapCubemap = SortedShadowsForShadowDepthPass.ShadowMapCubemaps.back();

			PooledRenderTargetDesc Desc(PooledRenderTargetDesc::CreateCubemapDesc(ProjectedShadowInfo->ResolutionX, PF_ShadowDepth, FClearValueBinding::DepthOne, TexCreate_None, TexCreate_DepthStencilTargetable | TexCreate_NoFastClear, false));
			//Desc.Flags |= GFastVRamConfig.ShadowPointLight;
			GRenderTargetPool.FindFreeElement(Desc, ShadowMapCubemap.RenderTargets.DepthTarget, TEXT("CubeShadowDepthZ"));

			if (ProjectedShadowInfo->CacheMode == SDCM_StaticPrimitivesOnly)
			{
				FCachedShadowMapData& CachedShadowMapData = Scene->CachedShadowMaps.at(ProjectedShadowInfo->GetLightSceneInfo().Id);
				CachedShadowMapData.ShadowMap.DepthTarget = ShadowMapCubemap.RenderTargets.DepthTarget;
			}

			ProjectedShadowInfo->X = ProjectedShadowInfo->Y = 0;
			ProjectedShadowInfo->bAllocated = true;
			ProjectedShadowInfo->RenderTargets.DepthTarget = ShadowMapCubemap.RenderTargets.DepthTarget.Get();

			ProjectedShadowInfo->SetupShadowDepthView(this);
			ShadowMapCubemap.Shadows.push_back(ProjectedShadowInfo);
		}
	}
}

void FSceneRenderer::GatherShadowDynamicMeshElements()
{

}

void FSceneRenderer::InitDynamicShadows()
{
	for (auto LightIt = Scene->Lights.begin();LightIt!= Scene->Lights.end();++LightIt )
	{
		const FLightSceneInfoCompact& LightSceneInfoCompact = *LightIt;
		FLightSceneInfo* LightSceneInfo = LightSceneInfoCompact.LightSceneInfo;

		uint32 NumPointShadowCachesUpdatedThisFrame = 0;
		uint32 NumSpotShadowCachesUpdatedThisFrame = 0;

		const bool bAllowStaticLighting = true;
		const bool bPointLightShadow = LightSceneInfoCompact.LightType == LightType_Point || LightSceneInfoCompact.LightType == LightType_Rect;
		//只有动态光才可能有全场景阴影
		if (!LightSceneInfo->Proxy->HasStaticShadowing())
		{
			CreateWholeSceneProjectedShadow(LightSceneInfo, NumPointShadowCachesUpdatedThisFrame, NumSpotShadowCachesUpdatedThisFrame);
		}
		/*
		if ((!LightSceneInfo->Proxy->HasStaticLighting() && LightSceneInfoCompact.bCastDynamicShadow))
		{
			AddViewDependentWholeSceneShadowsForView(ViewDependentWholeSceneShadows, ViewDependentWholeSceneShadowsThatNeedCulling, VisibleLightInfo, *LightSceneInfo);

			// Look for individual primitives with a dynamic shadow.
			for (FLightPrimitiveInteraction* Interaction = LightSceneInfo->DynamicInteractionOftenMovingPrimitiveList;
				Interaction;
				Interaction = Interaction->GetNextPrimitive()
				)
			{
				SetupInteractionShadows(RHICmdList, Interaction, VisibleLightInfo, bStaticSceneOnly, ViewDependentWholeSceneShadows, PreShadows);
			}

			for (FLightPrimitiveInteraction* Interaction = LightSceneInfo->DynamicInteractionStaticPrimitiveList;
				Interaction;
				Interaction = Interaction->GetNextPrimitive()
				)
			{
				SetupInteractionShadows(RHICmdList, Interaction, VisibleLightInfo, bStaticSceneOnly, ViewDependentWholeSceneShadows, PreShadows);
			}
		}
		*/
	}
	InitProjectedShadowVisibility();

	//UpdatePreshadowCache(FSceneRenderTargets::Get(RHICmdList));

	// Gathers the list of primitives used to draw various shadow types
	//GatherShadowPrimitives(PreShadows, ViewDependentWholeSceneShadowsThatNeedCulling, bStaticSceneOnly);

	AllocateShadowDepthTargets();

	// Generate mesh element arrays from shadow primitive arrays
	GatherShadowDynamicMeshElements();
}

