#include "DeferredShading.h"
#include "Scene.h"

float GLightMaxDrawDistanceScale = 1.0f;
float GMinScreenRadiusForLights = 0.03f;
float GMinScreenRadiusForDepthPrepass = 0.03f;

template<bool UseCustomCulling, bool bAlsoUseSphereTest>
static int32 FrustumCull(const FScene* Scene, FViewInfo& View)
{
	const uint32 NumPrimiteves = View.PrimitiveVisibilityMap.size();

	FVector ViewOriginForDistanceCulling = View.ViewMatrices.GetViewOrigin();
	float FadeRadius = 0;// GDisableLODFade ? 0.0f : GDistanceFadeMaxTravel;

	for (uint32 PrimitiveIndex = 0; PrimitiveIndex < NumPrimiteves; PrimitiveIndex++)
	{
		int32 Index = PrimitiveIndex /** NumBitsPerDWORD + BitSubIndex*/;
		const FPrimitiveBounds& Bounds = Scene->PrimitiveBounds[Index];
		float DistanceSquared = (Bounds.BoxSphereBounds.Origin - ViewOriginForDistanceCulling).SizeSquared();
		int32 VisibilityId = INDEX_NONE;

		float MaxDrawDistance = Bounds.MaxCullDistance < FLT_MAX ? Bounds.MaxCullDistance /** MaxDrawDistanceScale*/ : FLT_MAX;
		float MinDrawDistanceSq = Bounds.MinDrawDistanceSq;

		if (DistanceSquared > FMath::Square(MaxDrawDistance + FadeRadius) ||
			(DistanceSquared < MinDrawDistanceSq) ||
			//(UseCustomCulling && !View.CustomVisibilityQuery->IsVisible(VisibilityId, FBoxSphereBounds(Bounds.BoxSphereBounds.Origin, Bounds.BoxSphereBounds.BoxExtent, Bounds.BoxSphereBounds.SphereRadius))) ||
			(bAlsoUseSphereTest && View.ViewFrustum.IntersectSphere(Bounds.BoxSphereBounds.Origin, Bounds.BoxSphereBounds.SphereRadius) == false) ||
			View.ViewFrustum.IntersectBox(Bounds.BoxSphereBounds.Origin, Bounds.BoxSphereBounds.BoxExtent) == false
			)
		{

		}
		else
		{
			if (DistanceSquared > FMath::Square(MaxDrawDistance))
			{
				//FadingBits |= Mask;
			}
			else
			{
				View.PrimitiveVisibilityMap[Index] = true;
			}
		}
	}
	return 0;
}

float Halton(int32 Index, int32 Base)
{
	float Result = 0.0f;
	float InvBase = 1.0f / Base;
	float Fraction = InvBase;
	while (Index > 0)
	{
		Result += (Index % Base) * Fraction;
		Index /= Base;
		Fraction *= InvBase;
	}
	return Result;
}

void FSceneRenderer::PreVisibilityFrameSetup()
{
	// Setup motion blur parameters (also check for camera movement thresholds)
	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
	{
		FViewInfo& View = Views[ViewIndex];
		FSceneViewState* ViewState = View.ViewState;

		// Cache the projection matrix b		
		// Cache the projection matrix before AA is applied
		View.ViewMatrices.SaveProjectionNoAAMatrix();

		if (View.AntiAliasingMethod == AAM_TemporalAA && ViewState)
		{
			// Subpixel jitter for temporal AA
			int32 TemporalAASamples = 8;// CVarTemporalAASamples.GetValueOnRenderThread();

			if (TemporalAASamples > 1 && View.bAllowTemporalJitter)
			{
				float SampleX, SampleY;

				if (TemporalAASamples == 2)
				{
				}
				else if (TemporalAASamples == 3)
				{
				}
				else if (TemporalAASamples == 4)
				{
				}
				else if (TemporalAASamples == 5)
				{
				}
				else if (View.PrimaryScreenPercentageMethod == EPrimaryScreenPercentageMethod::TemporalUpscale)
				{
				}
				else
				{
					ViewState->OnFrameRenderingSetup(TemporalAASamples, ViewFamily);
					uint32 Index = ViewState->GetCurrentTemporalAASampleIndex();

					float u1 = Halton(Index + 1, 2);
					float u2 = Halton(Index + 1, 3);

					//static auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.TemporalAAFilterSize"));
					float FilterSize = 1.0f;// CVar->GetFloat();

					// Scale distribution to set non-unit variance
					// Variance = Sigma^2
					float Sigma = 0.47f * FilterSize;

					// Window to [-0.5, 0.5] output
					// Without windowing we could generate samples far away on the infinite tails.
					float OutWindow = 0.5f;
					float InWindow = FMath::Exp(-0.5 * FMath::Square(OutWindow / Sigma));

					// Box-Muller transform
					float Theta = 2.0f * PI * u2;
					float r = Sigma * FMath::Sqrt(-2.0f * FMath::Loge((1.0f - u1) * InWindow + u1));

					SampleX = r * FMath::Cos(Theta);
					SampleY = r * FMath::Sin(Theta);
				}

				View.TemporalJitterPixels.X = SampleX;
				View.TemporalJitterPixels.Y = SampleY;

				View.ViewMatrices.HackAddTemporalAAProjectionJitter(Vector2(SampleX * 2.0f / View.ViewRect.Width(), SampleY * -2.0f / View.ViewRect.Height()));
			}
		}
	}
}

template<class T, int TAmplifyFactor = 1>
struct FRelevancePrimSet
{
	enum
	{
		MaxInputPrims = 127, //like 128, but we leave space for NumPrims
		MaxOutputPrims = MaxInputPrims * TAmplifyFactor
	};
	int32 NumPrims;

	T Prims[MaxOutputPrims];

	inline FRelevancePrimSet()
		: NumPrims(0)
	{
		//FMemory::Memzero(Prims, sizeof(T) * GetMaxOutputPrim());
	}
	inline void AddPrim(T Prim)
	{
		assert(NumPrims < MaxOutputPrims);
		Prims[NumPrims++] = Prim;
	}
	inline bool IsFull() const
	{
		return NumPrims >= MaxOutputPrims;
	}
	template<class TARRAY>
	inline void AppendTo(TARRAY& DestArray)
	{
		DestArray.Append(Prims, NumPrims);
	}
};
struct FRelevancePacket
{
	const FScene* Scene;
	const FViewInfo& View;
	const uint8 ViewBit;
	FPrimitiveViewMasks& OutHasDynamicMeshElementsMasks;
	FPrimitiveViewMasks& OutHasDynamicEditorMeshElementsMasks;
	/*uint8* __restrict MarkMasks;*/

	FRelevancePrimSet<int32> Input;
	FRelevancePrimSet<int32> RelevantStaticPrimitives;
	FRelevancePrimSet<int32> NotDrawRelevant;
	FRelevancePrimSet<FPrimitiveSceneInfo*> VisibleDynamicPrimitives;

	FRelevancePacket(
		const FScene* InScene,
		const FViewInfo& InView,
		uint8 InViewBit,
		FPrimitiveViewMasks& InOutHasDynamicMeshElementsMasks,
		FPrimitiveViewMasks& InOutHasDynamicEditorMeshElementsMasks,
		/*uint8* InMarkMasks,*/
		FPrimitiveViewMasks& InOutHasViewCustomDataMasks)
		: Scene(InScene)
		, View(InView)
		, ViewBit(InViewBit)
		, OutHasDynamicMeshElementsMasks(InOutHasDynamicMeshElementsMasks)
		, OutHasDynamicEditorMeshElementsMasks(InOutHasDynamicEditorMeshElementsMasks)
		/*, MarkMasks(InMarkMasks)*/
	{

	}
	void AnyThreadTask()
	{
		ComputeRelevance();
		MarkRelevant();
	}

	void ComputeRelevance()
	{
		for (int32 Index = 0; Index < Input.NumPrims; Index++)
		{
			int32 BitIndex = Input.Prims[Index];
			FPrimitiveSceneInfo* PrimitiveSceneInfo = Scene->Primitives[BitIndex];
			FPrimitiveViewRelevance& ViewRelevance = const_cast<FPrimitiveViewRelevance&>(View.PrimitiveViewRelevanceMap[BitIndex]);
			ViewRelevance = PrimitiveSceneInfo->Proxy->GetViewRelevance(&View);

			const bool bStaticRelevance = ViewRelevance.bStaticRelevance;
			const bool bDrawRelevance = ViewRelevance.bDrawRelevance;
			const bool bDynamicRelevance = ViewRelevance.bDynamicRelevance;
			const bool bShadowRelevance = ViewRelevance.bShadowRelevance;
			const bool bEditorRelevance = ViewRelevance.bEditorPrimitiveRelevance;
			const bool bEditorSelectionRelevance = ViewRelevance.bEditorStaticSelectionRelevance;
			const bool bTranslucentRelevance = ViewRelevance.HasTranslucency();


			if (bDynamicRelevance)
			{
				// Keep track of visible dynamic primitives.
				VisibleDynamicPrimitives.AddPrim(PrimitiveSceneInfo);
				OutHasDynamicMeshElementsMasks[BitIndex] |= ViewBit;
			}
		}
	}

	void MarkRelevant()
	{
	}

};
static void ComputeAndMarkRelevanceForViewParallel(
	const FScene* Scene,
	FViewInfo& View,
	uint8 ViewBit,
	FPrimitiveViewMasks& OutHasDynamicMeshElementsMasks,
	FPrimitiveViewMasks& OutHasDynamicEditorMeshElementsMasks,
	FPrimitiveViewMasks& HasViewCustomDataMasks
)
{
	std::vector<FRelevancePacket*> Packets;

	FRelevancePacket* Packet = new FRelevancePacket(
		Scene,
		View,
		ViewBit,
		/*ViewData,*/
		OutHasDynamicMeshElementsMasks,
		OutHasDynamicEditorMeshElementsMasks,
		/*MarkMasks,*/
		/*WillExecuteInParallel ? View.AllocateCustomDataMemStack() : View.GetCustomDataGlobalMemStack(),*/
		HasViewCustomDataMasks);
	Packets.push_back(Packet);

	for (size_t i = 0; i < View.PrimitiveVisibilityMap.size(); ++i)
	{
		Packet->Input.AddPrim(i);
	}
	
	for (FRelevancePacket* Packet : Packets)
	{
		Packet->AnyThreadTask();
	}
}

void FSceneRenderer::ComputeViewVisibility()
{
	uint32 NumPrimitives = Scene->Primitives.size();

	FPrimitiveViewMasks HasDynamicMeshElementsMasks;
	HasDynamicMeshElementsMasks.resize(NumPrimitives);

	FPrimitiveViewMasks HasViewCustomDataMasks;
	HasViewCustomDataMasks.resize(NumPrimitives);

	FPrimitiveViewMasks HasDynamicEditorMeshElementsMasks;

	if (Scene->Lights.size() > 0)
	{
		VisibleLightInfos.resize(Scene->Lights.size());
	}

	uint8 ViewBit = 0x1;
	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ++ViewIndex)
	{
		FViewInfo& View = Views[ViewIndex];

		View.PrimitiveVisibilityMap.resize(Scene->Primitives.size(), false);
		View.DynamicMeshEndIndices.resize(Scene->Primitives.size(), 0);
		View.PrimitiveFadeUniformBuffers.resize(Scene->Primitives.size(),0);
		//View.StaticMeshVisibilityMap.resize(Scene->StaticMeshes.size(), false);


		View.VisibleLightInfos.reserve(Scene->Lights.size());

		for (uint32 LightIndex = 0; LightIndex < Scene->Lights.size(); LightIndex++)
		{
			View.VisibleLightInfos.push_back(FVisibleLightViewInfo());
		}

		View.PrimitiveViewRelevanceMap.clear();
		View.PrimitiveViewRelevanceMap.reserve(Scene->Primitives.size());
		View.PrimitiveViewRelevanceMap.resize(Scene->Primitives.size());

		bool bNeedsFrustumCulling = true;

		FrustumCull<true, true>(Scene, View);

		ComputeAndMarkRelevanceForViewParallel(Scene, View, ViewBit, HasDynamicMeshElementsMasks, HasDynamicEditorMeshElementsMasks, HasViewCustomDataMasks);
	}

	GatherDynamicMeshElements(Views, Scene, ViewFamily, HasDynamicMeshElementsMasks, HasDynamicEditorMeshElementsMasks, HasViewCustomDataMasks, MeshCollector);
}

void FSceneRenderer::PostVisibilityFrameSetup()
{
	Scene->IndirectLightingCache.UpdateCache(Scene, *this, true);


	InitFogConstants();
}


void FSceneRenderer::InitViewsPossiblyAfterPrepass()
{
	InitDynamicShadows();

	UpdatePrimitivePrecomputedLightingBuffers();
}

void FSceneRenderer::GatherDynamicMeshElements(
	std::vector<FViewInfo>& InViews, 
	const FScene* InScene, 
	const FSceneViewFamily& InViewFamily, 
	const FPrimitiveViewMasks& HasDynamicMeshElementsMasks, 
	const FPrimitiveViewMasks& HasDynamicEditorMeshElementsMasks, 
	const FPrimitiveViewMasks& HasViewCustomDataMasks, 
	FMeshElementCollector& Collector)
{
	uint32 NumPrimitives = InScene->Primitives.size();
	assert(HasDynamicMeshElementsMasks.size() == NumPrimitives);

	int32 ViewCount = InViews.size();
	{
		Collector.ClearViewMeshArrays();

		for (int32 ViewIndex = 0; ViewIndex < ViewCount; ViewIndex++)
		{
			Collector.AddViewMeshArrays(&InViews[ViewIndex], &InViews[ViewIndex].DynamicMeshElements /*,&InViews[ViewIndex].SimpleElementCollector*//*, InViewFamily.GetFeatureLevel()*/);
		}

		const bool bIsInstancedStereo = false;// (ViewCount > 0) ? (InViews[0].IsInstancedStereoPass() || InViews[0].bIsMobileMultiViewEnabled) : false;

		for (uint32 PrimitiveIndex = 0; PrimitiveIndex < NumPrimitives; ++PrimitiveIndex)
		{
			const uint8 ViewMask = HasDynamicMeshElementsMasks[PrimitiveIndex];

			if (ViewMask != 0)
			{
				// Don't cull a single eye when drawing a stereo pair
				const uint8 ViewMaskFinal = (bIsInstancedStereo) ? ViewMask | 0x3 : ViewMask;

				FPrimitiveSceneInfo* PrimitiveSceneInfo = InScene->Primitives[PrimitiveIndex];
				Collector.SetPrimitive(PrimitiveSceneInfo->Proxy/*, PrimitiveSceneInfo->DefaultDynamicHitProxyId*/);

				//SetDynamicMeshElementViewCustomData(InViews, HasViewCustomDataMasks, PrimitiveSceneInfo);

				PrimitiveSceneInfo->Proxy->GetDynamicMeshElements(InViewFamily.Views, InViewFamily, ViewMaskFinal, Collector);
			}

			// to support GetDynamicMeshElementRange()
			for (int32 ViewIndex = 0; ViewIndex < ViewCount; ViewIndex++)
			{
				InViews[ViewIndex].DynamicMeshEndIndices[PrimitiveIndex] = Collector.GetMeshBatchCount(ViewIndex);
			}
		}
	}
}

void FSceneRenderer::InitViews()
{
	PreVisibilityFrameSetup();

	ComputeViewVisibility();

	PostVisibilityFrameSetup();

	InitViewsPossiblyAfterPrepass();

	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
	{
		FViewInfo& View = Views[ViewIndex];
		// Initialize the view's RHI resources.
		View.InitRHIResources();
	}

	for (FPrimitiveSceneInfo* Primitive : Scene->Primitives)
	{
		Primitive->ConditionalUpdateUniformBuffer();
	}
}

