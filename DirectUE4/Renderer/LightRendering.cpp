#include "LightRendering.h"
#include "Scene.h"
#include "RenderTargets.h"
#include "DeferredShading.h"
#include "GPUProfiler.h"

FDeferredLightUniformStruct DeferredLightUniforms;

void StencilingGeometry::DrawSphere()
{

}


void StencilingGeometry::DrawVectorSphere()
{

}

void StencilingGeometry::DrawCone()
{

}

StencilingGeometry::TStencilSphereVertexBuffer<18, 12, Vector4> 		StencilingGeometry::GStencilSphereVertexBuffer;
StencilingGeometry::TStencilSphereVertexBuffer<18, 12, FVector> 		StencilingGeometry::GStencilSphereVectorBuffer;
StencilingGeometry::TStencilSphereIndexBuffer<18, 12>					StencilingGeometry::GStencilSphereIndexBuffer;
StencilingGeometry::TStencilSphereVertexBuffer<4, 4, Vector4> 			StencilingGeometry::GLowPolyStencilSphereVertexBuffer;
StencilingGeometry::TStencilSphereIndexBuffer<4, 4> 					StencilingGeometry::GLowPolyStencilSphereIndexBuffer;
StencilingGeometry::FStencilConeVertexBuffer							StencilingGeometry::GStencilConeVertexBuffer;
StencilingGeometry::FStencilConeIndexBuffer								StencilingGeometry::GStencilConeIndexBuffer;

void InitStencilingGeometryResources()
{
	StencilingGeometry::GStencilSphereVertexBuffer.InitRHI();
	StencilingGeometry::GStencilSphereVectorBuffer.InitRHI();
	StencilingGeometry::GStencilSphereIndexBuffer.InitRHI();
	StencilingGeometry::GLowPolyStencilSphereVertexBuffer.InitRHI();
	StencilingGeometry::GLowPolyStencilSphereIndexBuffer.InitRHI();
	StencilingGeometry::GStencilConeVertexBuffer.InitRHI();
	StencilingGeometry::GStencilConeIndexBuffer.InitRHI();
}


float GetLightFadeFactor(const FSceneView& View, const FLightSceneProxy* Proxy)
{
	// Distance fade
	FSphere Bounds = Proxy->GetBoundingSphere();

	const float DistanceSquared = (Bounds.Center - View.ViewMatrices.GetViewOrigin()).SizeSquared();
	extern float GMinScreenRadiusForLights;
	float SizeFade = FMath::Square(FMath::Min(0.0002f, GMinScreenRadiusForLights / Bounds.W) * View.LODDistanceFactor) * DistanceSquared;
	SizeFade = FMath::Clamp(6.0f - 6.0f * SizeFade, 0.0f, 1.0f);

	extern float GLightMaxDrawDistanceScale;
	float MaxDist = Proxy->GetMaxDrawDistance() * GLightMaxDrawDistanceScale;
	float Range = Proxy->GetFadeRange();
	float DistanceFade = MaxDist ? (MaxDist - FMath::Sqrt(DistanceSquared)) / Range : 1.0f;
	DistanceFade = FMath::Clamp(DistanceFade, 0.0f, 1.0f);
	return SizeFade * DistanceFade;
}

void FSceneRenderer::RenderLight(const FLightSceneInfo* LightSceneInfo, struct PooledRenderTarget* ScreenShadowMaskTexture, bool bRenderOverlap, bool bIssueDrawEvent)
{

}

void FSceneRenderer::RenderLights()
{
	SCOPED_DRAW_EVENT_FORMAT(RenderLights, TEXT("Lights"));

	std::vector<FSortedLightSceneInfo> SortedLights;
	SortedLights.reserve(Scene->Lights.size());

	// Build a list of visible lights.
	for (auto LightIt = Scene->Lights.begin(); LightIt != Scene->Lights.end(); ++LightIt)
	{
		const FLightSceneInfoCompact& LightSceneInfoCompact = *LightIt;
		const FLightSceneInfo* const LightSceneInfo = LightSceneInfoCompact.LightSceneInfo;

		for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
		{
			//if (LightSceneInfo->ShouldRenderLight(Views[ViewIndex]))
			{
				SortedLights.emplace_back(FSortedLightSceneInfo(LightSceneInfo));
				FSortedLightSceneInfo* SortedLightInfo = &SortedLights.back();

				// Check for shadows and light functions.
				SortedLightInfo->SortKey.Fields.LightType = LightSceneInfoCompact.LightType;
				SortedLightInfo->SortKey.Fields.bTextureProfile = false;//ViewFamily.EngineShowFlags.TexturedLightProfiles && LightSceneInfo->Proxy->GetIESTextureResource();
				SortedLightInfo->SortKey.Fields.bShadowed = true;// bDynamicShadows && CheckForProjectedShadows(LightSceneInfo);
				SortedLightInfo->SortKey.Fields.bLightFunction = false;// ViewFamily.EngineShowFlags.LightFunctions && CheckForLightFunction(LightSceneInfo);
				SortedLightInfo->SortKey.Fields.bUsesLightingChannels = false;// Views[ViewIndex].bUsesLightingChannels && LightSceneInfo->Proxy->GetLightingChannelMask() != GetDefaultLightingChannelMask();
				break;
			}
		}
	}

	// Sort non-shadowed, non-light function lights first to avoid render target switches.
	struct FCompareFSortedLightSceneInfo
	{
		inline bool operator()(const FSortedLightSceneInfo& A, const FSortedLightSceneInfo& B) const
		{
			return A.SortKey.Packed < B.SortKey.Packed;
		}
	};
	std::sort(SortedLights.begin(), SortedLights.end(), FCompareFSortedLightSceneInfo());

	{
		FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();

		uint32 AttenuationLightStart = SortedLights.size();
		uint32 SupportedByTiledDeferredLightEnd = SortedLights.size();
		bool bAnyUnsupportedByTiledDeferred = false;

		// Iterate over all lights to be rendered and build ranges for tiled deferred and unshadowed lights
		for (uint32 LightIndex = 0; LightIndex < SortedLights.size(); LightIndex++)
		{
			const FSortedLightSceneInfo& SortedLightInfo = SortedLights[LightIndex];
			bool bDrawShadows = SortedLightInfo.SortKey.Fields.bShadowed;
			bool bDrawLightFunction = SortedLightInfo.SortKey.Fields.bLightFunction;
			bool bTextureLightProfile = SortedLightInfo.SortKey.Fields.bTextureProfile;
			bool bLightingChannels = SortedLightInfo.SortKey.Fields.bUsesLightingChannels;

			if (bTextureLightProfile && SupportedByTiledDeferredLightEnd == SortedLights.size())
			{
				// Mark the first index to not support tiled deferred due to texture light profile
				SupportedByTiledDeferredLightEnd = LightIndex;
			}

			if (bDrawShadows || bDrawLightFunction || bLightingChannels)
			{
				AttenuationLightStart = LightIndex;

				if (SupportedByTiledDeferredLightEnd == SortedLights.size())
				{
					// Mark the first index to not support tiled deferred due to shadowing
					SupportedByTiledDeferredLightEnd = LightIndex;
				}
				break;
			}

			if (LightIndex < SupportedByTiledDeferredLightEnd)
			{
				// Directional lights currently not supported by tiled deferred
				bAnyUnsupportedByTiledDeferred = bAnyUnsupportedByTiledDeferred
					|| (SortedLightInfo.SortKey.Fields.LightType != LightType_Point && SortedLightInfo.SortKey.Fields.LightType != LightType_Spot);
			}
		}

		int32 StandardDeferredStart = 0;

		{
			SCOPED_DRAW_EVENT(NonShadowedLights);

			SceneContext.BeginRenderingSceneColor(false,false,true);

			for (uint32 LightIndex = StandardDeferredStart; LightIndex < AttenuationLightStart; LightIndex++)
			{
				const FSortedLightSceneInfo& SortedLightInfo = SortedLights[LightIndex];
				const FLightSceneInfo* const LightSceneInfo = SortedLightInfo.LightSceneInfo;

				// Render the light to the scene color buffer, using a 1x1 white texture as input 
				RenderLight(LightSceneInfo, NULL, false, false);
			}
		}


		{
			SCOPED_DRAW_EVENT(ShadowedLights);

			ComPtr<PooledRenderTarget> ScreenShadowMaskTexture;

			for (uint32 LightIndex = AttenuationLightStart; LightIndex < SortedLights.size(); LightIndex++)
			{
				const FSortedLightSceneInfo& SortedLightInfo = SortedLights[LightIndex];
				const FLightSceneInfo& LightSceneInfo = *SortedLightInfo.LightSceneInfo;
				bool bDrawShadows = SortedLightInfo.SortKey.Fields.bShadowed;
				bool bDrawLightFunction = SortedLightInfo.SortKey.Fields.bLightFunction;
				bool bDrawPreviewIndicator = false;// ViewFamily.EngineShowFlags.PreviewShadowsIndicator && !LightSceneInfo.IsPrecomputedLightingValid() && LightSceneInfo.Proxy->HasStaticShadowing();
				bool bInjectedTranslucentVolume = false;
				bool bUsedShadowMaskTexture = false;

				if ((bDrawShadows || bDrawLightFunction || bDrawPreviewIndicator) && !ScreenShadowMaskTexture.Get())
				{
					SceneContext.AllocateScreenShadowMask(ScreenShadowMaskTexture);
				}

				if (bDrawShadows)
				{
					// Clear light attenuation for local lights with a quad covering their extents
					const bool bClearLightScreenExtentsOnly = SortedLightInfo.SortKey.Fields.LightType != LightType_Directional;
					// All shadows render with min blending
					bool bClearToWhite = !bClearLightScreenExtentsOnly;

					SetRenderTarget(ScreenShadowMaskTexture->TargetableTexture.get(), SceneContext.GetSceneDepthSurface().get(), bClearToWhite ,false, true);

					if (bClearLightScreenExtentsOnly)
					{
						SCOPED_DRAW_EVENT(ClearQuad);

						for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
						{
							const FViewInfo& View = Views[ViewIndex];
							FIntRect ScissorRect;

							if (!LightSceneInfo.Proxy->GetScissorRect(ScissorRect, View, View.ViewRect))
							{
								ScissorRect = View.ViewRect;
							}

							if (ScissorRect.Min.X < ScissorRect.Max.X && ScissorRect.Min.Y < ScissorRect.Max.Y)
							{
								RHISetViewport(ScissorRect.Min.X, ScissorRect.Min.Y, 0.0f, ScissorRect.Max.X, ScissorRect.Max.Y, 1.0f);
								DrawClearQuad(true, FLinearColor(1, 1, 1, 1), false, 0, false, 0);
							}
							else
							{
								LightSceneInfo.Proxy->GetScissorRect(ScissorRect, View, View.ViewRect);
							}
						}
					}

					RenderShadowProjections(&LightSceneInfo, ScreenShadowMaskTexture.Get(), bInjectedTranslucentVolume);

					bUsedShadowMaskTexture = true;
				}

				if (bUsedShadowMaskTexture)
				{
					CopyToResolveTarget(ScreenShadowMaskTexture->TargetableTexture.get(), ScreenShadowMaskTexture->ShaderResourceTexture.get(), FResolveParams(FResolveRect()));
				}

				SceneContext.BeginRenderingSceneColor(false, false, true);

				// Render the light to the scene color buffer, conditionally using the attenuation buffer or a 1x1 white texture as input 
				//if (bDirectLighting)
				{
					RenderLight(&LightSceneInfo, ScreenShadowMaskTexture.Get(), false, true);
				}
			}
		}
	}
}

