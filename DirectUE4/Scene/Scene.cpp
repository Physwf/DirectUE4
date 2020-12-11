#include "Scene.h"
#include "log.h"
#include "D3D11RHI.h"
#include "RenderTargets.h"
#include "AtmosphereRendering.h"
#include "GlobalShader.h"
#include "LightSceneInfo.h"
#include "LightComponent.h"
#include "SceneManagement.h"
#include "PrimitiveSceneInfo.h"


void FIndirectLightingCache::UpdateCache(FScene* Scene, FSceneRenderer& Renderer, bool bAllowUnbuiltPreview)
{
	const uint32 PrimitiveCount = Scene->Primitives.size();

	for (uint32 PrimitiveIndex = 0; PrimitiveIndex < PrimitiveCount; ++PrimitiveIndex)
	{
		FPrimitiveSceneInfo* PrimitiveSceneInfo = Scene->Primitives[PrimitiveIndex];
		const bool bPrecomputedLightingBufferWasDirty = PrimitiveSceneInfo->NeedsPrecomputedLightingBufferUpdate();

		//UpdateCachePrimitive(AttachmentGroups, PrimitiveSceneInfo, false, true, OutBlocksToUpdate, OutTransitionsOverTimeToUpdate);

		// If it was already dirty, then the primitive is already in one of the view dirty primitive list at this point.
		// This also ensures that a primitive does not get added twice to the list, which could create an array reallocation.
		if (bPrecomputedLightingBufferWasDirty)
		{
			PrimitiveSceneInfo->MarkPrecomputedLightingBufferDirty();

			// Check if it is visible otherwise, it will be updated next time it is visible.
			for (uint32 ViewIndex = 0; ViewIndex < Renderer.Views.size(); ViewIndex++)
			{
				FViewInfo& View = Renderer.Views[ViewIndex];

				//if (View.PrimitiveVisibilityMap[PrimitiveIndex])
				{
					// Since the update can be executed on a threaded job (see GILCUpdatePrimTaskEnabled), no reallocation must happen here.
					//checkSlow(View.DirtyPrecomputedLightingBufferPrimitives.Num() < View.DirtyPrecomputedLightingBufferPrimitives.Max());
					View.DirtyPrecomputedLightingBufferPrimitives.push_back(PrimitiveSceneInfo);
					break; // We only need to add it in one of the view list.
				}
			}
		}
	}
}

FScene::FScene(UWorld* InWorld)
: World(InWorld)
, SkyLight(NULL)
, SunLight(NULL)
, AtmosphericFog(NULL)
, SceneFrameNumber(0)
{
}

void FScene::AddPrimitive(UPrimitiveComponent* Primitive)
{
	FPrimitiveSceneProxy* PrimitiveSceneProxy = Primitive->CreateSceneProxy();
	Primitive->SceneProxy = PrimitiveSceneProxy;

	FPrimitiveSceneInfo* PrimitiveSceneInfo = new FPrimitiveSceneInfo(Primitive, this);
	PrimitiveSceneProxy->PrimitiveSceneInfo = PrimitiveSceneInfo;

	FMatrix RenderMatrix = Primitive->GetRenderMatrix();

	FVector AttachmentRootPosition(0);

	AActor* AttachmentRoot = Primitive->GetAttachmentRootActor();
	if (AttachmentRoot)
	{
		AttachmentRootPosition = AttachmentRoot->GetActorLocation();
	}

	PrimitiveSceneProxy->SetTransform(RenderMatrix, Primitive->Bounds, Primitive->CalcBounds(FTransform::Identity),  AttachmentRootPosition);
	
	AddPrimitiveSceneInfo_RenderThread(PrimitiveSceneInfo);
}

void FScene::RemovePrimitive(UPrimitiveComponent* Primitive)
{

}

void FScene::UpdatePrimitiveTransform(UPrimitiveComponent* Primitive)
{
	FVector AttachmentRootPosition(0);

	AActor* Actor = Primitive->GetAttachmentRootActor();
	if (Actor != NULL)
	{
		AttachmentRootPosition = Actor->GetActorLocation();
	}

	UpdatePrimitiveTransform_RenderThread(Primitive->SceneProxy, Primitive->Bounds, Primitive->CalcBounds(FTransform::Identity), Primitive->GetRenderMatrix(), AttachmentRootPosition);
}

void FScene::AddLight(class ULightComponent* Light)
{
	FLightSceneProxy* Proxy = Light->CreateSceneProxy();
	if (Proxy)
	{
		Light->SceneProxy = Proxy;
		Proxy->SetTransform(Light->GetComponentTransform().ToMatrixNoScale(), Light->GetLightPosition());
		Proxy->LightSceneInfo = new FLightSceneInfo(Proxy);

		AddLightSceneInfo(Proxy->LightSceneInfo);
	}

}

void FScene::UpdateLightTransform(ULightComponent* Light)
{
	if (Light->SceneProxy)
	{
		FUpdateLightTransformParameters Parameters;
		Parameters.LightToWorld = Light->GetComponentTransform().ToMatrixNoScale();
		Parameters.Position = Light->GetLightPosition();
		UpdateLightTransform_RenderThread(Light->SceneProxy->GetLightSceneInfo(), Parameters);
	}
}

void FScene::RemoveLight(class ULightComponent* Light)
{

}

void FScene::AddAtmosphericFog(UAtmosphericFogComponent* FogComponent)
{
	FAtmosphericFogSceneInfo* FogSceneInfo = new FAtmosphericFogSceneInfo(FogComponent, this);
	if (AtmosphericFog)
	{
		delete AtmosphericFog;
		AtmosphericFog = NULL;
	}
	AtmosphericFog = FogSceneInfo;
}

void FScene::RemoveAtmosphericFog(UAtmosphericFogComponent* FogComponent)
{
	if (AtmosphericFog && AtmosphericFog->Component == FogComponent)
	{
		delete AtmosphericFog;
		AtmosphericFog = NULL;
	}
}

void FScene::SetSkyLight(FSkyLightSceneProxy* LightProxy)
{
	SkyLightStack.push_back(LightProxy);
	SkyLight = LightProxy;
}

void FScene::DisableSkyLight(FSkyLightSceneProxy* LightProxy)
{
	auto it = std::find(SkyLightStack.begin(), SkyLightStack.end(), LightProxy);
	if(it != SkyLightStack.end()) SkyLightStack.erase(it);

	if (SkyLightStack.size() > 0)
	{
		// Use the most recently enabled skylight
		SkyLight = SkyLightStack.back();
	}
	else
	{
		SkyLight = NULL;
	}
}

void FScene::AddLightSceneInfo(FLightSceneInfo* LightSceneInfo)
{
	LightSceneInfo->Id = Lights.size();
	LightSceneInfo->Scene = this;
	Lights.push_back(FLightSceneInfoCompact(LightSceneInfo));

	if (LightSceneInfo->Proxy->GetLightType() == LightType_Directional && !LightSceneInfo->Proxy->HasStaticLighting())//非静态方向光
	{
		if (!SimpleDirectionalLight)
		{
			SimpleDirectionalLight = LightSceneInfo;
		}
	}

	LightSceneInfo->AddToScene();
}

void FScene::AddPrimitiveSceneInfo_RenderThread(FPrimitiveSceneInfo* PrimitiveSceneInfo)
{
	Primitives.push_back(PrimitiveSceneInfo);
	PrimitiveSceneProxies.push_back(PrimitiveSceneInfo->Proxy);
	PrimitiveBounds.push_back(FPrimitiveBounds());
	//PrimitiveFlagsCompact.AddUninitialized();
	//PrimitiveVisibilityIds.AddUninitialized();
	//PrimitiveOcclusionFlags.AddUninitialized();
	//PrimitiveComponentIds.AddUninitialized();
	//PrimitiveOcclusionBounds.AddUninitialized();

	const int32 SourceIndex = PrimitiveSceneProxies.size() - 1;
	PrimitiveSceneInfo->PackedIndex = SourceIndex;

	PrimitiveSceneInfo->AddToScene(true, true);
}

void FScene::UpdateLightTransform_RenderThread(FLightSceneInfo* LightSceneInfo, const struct FUpdateLightTransformParameters& Parameters)
{
	if (LightSceneInfo)
	{
		// Don't remove directional lights when their transform changes as nothing in RemoveFromScene() depends on their transform
		if (!(LightSceneInfo->Proxy->GetLightType() == LightType_Directional))
		{
			// Remove the light from the scene.
			LightSceneInfo->RemoveFromScene();
		}

		// Update the light's transform and position.
		LightSceneInfo->Proxy->SetTransform(Parameters.LightToWorld, Parameters.Position);

		// Also update the LightSceneInfoCompact
		if (LightSceneInfo->Id != INDEX_NONE)
		{
			LightSceneInfo->Scene->Lights[LightSceneInfo->Id].Init(LightSceneInfo);

			// Don't re-add directional lights when their transform changes as nothing in AddToScene() depends on their transform
			if (!(LightSceneInfo->Proxy->GetLightType() == LightType_Directional))
			{
				// Add the light to the scene at its new location.
				LightSceneInfo->AddToScene();
			}
		}
	}
}


void FScene::UpdatePrimitiveTransform_RenderThread(FPrimitiveSceneProxy* PrimitiveSceneProxy, const FBoxSphereBounds& WorldBounds, const FBoxSphereBounds& LocalBounds, const FMatrix& LocalToWorld, const FVector& OwnerPosition)
{
	PrimitiveSceneProxy->SetTransform(LocalToWorld, WorldBounds, LocalBounds, OwnerPosition);
}

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
