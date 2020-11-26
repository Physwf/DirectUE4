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

void FLightPrimitiveInteraction::Create(FLightSceneInfo* LightSceneInfo, FPrimitiveSceneInfo* PrimitiveSceneInfo)
{
	bool bDynamic = true;
	bool bRelevant = false;
	bool bIsLightMapped = true;
	bool bShadowMapped = false;

	// Determine the light's relevance to the primitive.
	assert(PrimitiveSceneInfo->Proxy && LightSceneInfo->Proxy);
	PrimitiveSceneInfo->Proxy->GetLightRelevance(LightSceneInfo->Proxy, bDynamic, bRelevant, bIsLightMapped, bShadowMapped);

	if (bRelevant && bDynamic
		// Don't let lights with static shadowing or static lighting affect primitives that should use static lighting, but don't have valid settings (lightmap res 0, etc)
		// This prevents those components with invalid lightmap settings from causing lighting to remain unbuilt after a build
		&& !(LightSceneInfo->Proxy->HasStaticShadowing() && PrimitiveSceneInfo->Proxy->HasStaticLighting() && !PrimitiveSceneInfo->Proxy->HasValidSettingsForStaticLighting()))
	{
		const bool bTranslucentObjectShadow = LightSceneInfo->Proxy->CastsTranslucentShadows() && PrimitiveSceneInfo->Proxy->CastsVolumetricTranslucentShadow();
		const bool bInsetObjectShadow =
			// Currently only supporting inset shadows on directional lights, but could be made to work with any whole scene shadows
			LightSceneInfo->Proxy->GetLightType() == LightType_Directional
			&& PrimitiveSceneInfo->Proxy->CastsInsetShadow();

		// Movable directional lights determine shadow relevance dynamically based on the view and CSM settings. Interactions are only required for per-object cases.
		if (LightSceneInfo->Proxy->GetLightType() != LightType_Directional || LightSceneInfo->Proxy->HasStaticShadowing() || bTranslucentObjectShadow || bInsetObjectShadow)
		{
			// Create the light interaction.
			FLightPrimitiveInteraction* Interaction = new FLightPrimitiveInteraction(LightSceneInfo, PrimitiveSceneInfo, bDynamic, bIsLightMapped, bShadowMapped, bTranslucentObjectShadow, bInsetObjectShadow);
		} //-V773
	}
}

void FLightPrimitiveInteraction::Destroy(FLightPrimitiveInteraction* LightPrimitiveInteraction)
{
	delete LightPrimitiveInteraction;
}

FLightPrimitiveInteraction::FLightPrimitiveInteraction(
	FLightSceneInfo* InLightSceneInfo, 
	FPrimitiveSceneInfo* InPrimitiveSceneInfo, 
	bool bInIsDynamic,
	bool bInLightMapped, 
	bool bInIsShadowMapped, 
	bool bInHasTranslucentObjectShadow, 
	bool bInHasInsetObjectShadow) :
	LightSceneInfo(InLightSceneInfo),
	PrimitiveSceneInfo(InPrimitiveSceneInfo),
	LightId(InLightSceneInfo->Id),
	bLightMapped(bInLightMapped),
	bIsDynamic(bInIsDynamic),
	bIsShadowMapped(bInIsShadowMapped),
	bUncachedStaticLighting(false),
	bHasTranslucentObjectShadow(bInHasTranslucentObjectShadow),
	bHasInsetObjectShadow(bInHasInsetObjectShadow),
	bSelfShadowOnly(false),
	bES2DynamicPointLight(false)
{
	// Determine whether this light-primitive interaction produces a shadow.
	if (PrimitiveSceneInfo->Proxy->HasStaticLighting())
	{
		const bool bHasStaticShadow =
			LightSceneInfo->Proxy->HasStaticShadowing() &&
			LightSceneInfo->Proxy->CastsStaticShadow() &&
			PrimitiveSceneInfo->Proxy->CastsStaticShadow();
		const bool bHasDynamicShadow =
			!LightSceneInfo->Proxy->HasStaticLighting() &&
			LightSceneInfo->Proxy->CastsDynamicShadow() &&
			PrimitiveSceneInfo->Proxy->CastsDynamicShadow();
		bCastShadow = bHasStaticShadow || bHasDynamicShadow;
	}
	else
	{
		bCastShadow = LightSceneInfo->Proxy->CastsDynamicShadow() && PrimitiveSceneInfo->Proxy->CastsDynamicShadow();
	}

	bSelfShadowOnly = PrimitiveSceneInfo->Proxy->CastsSelfShadowOnly();

	if (bIsDynamic)
	{
		// Add the interaction to the light's interaction list.
		PrevPrimitiveLink = PrimitiveSceneInfo->Proxy->IsMeshShapeOftenMoving() ? &LightSceneInfo->DynamicInteractionOftenMovingPrimitiveList : &LightSceneInfo->DynamicInteractionStaticPrimitiveList;
	}

	//FlushCachedShadowMapData();

	NextPrimitive = *PrevPrimitiveLink;
	if (*PrevPrimitiveLink)
	{
		(*PrevPrimitiveLink)->PrevPrimitiveLink = &NextPrimitive;
	}
	*PrevPrimitiveLink = this;

	// Add the interaction to the primitives' interaction list.
	PrevLightLink = &PrimitiveSceneInfo->LightList;
	NextLight = *PrevLightLink;
	if (*PrevLightLink)
	{
		(*PrevLightLink)->PrevLightLink = &NextLight;
	}
	*PrevLightLink = this;
}


FLightPrimitiveInteraction::~FLightPrimitiveInteraction()
{
	//FlushCachedShadowMapData();

	// Remove the interaction from the light's interaction list.
	if (NextPrimitive)
	{
		NextPrimitive->PrevPrimitiveLink = PrevPrimitiveLink;
	}
	*PrevPrimitiveLink = NextPrimitive;

	// Remove the interaction from the primitive's interaction list.
	if (NextLight)
	{
		NextLight->PrevLightLink = PrevLightLink;
	}
	*PrevLightLink = NextLight;
}

FStaticMesh::~FStaticMesh()
{

}

void FStaticMesh::AddToDrawLists(/*FRHICommandListImmediate& RHICmdList,*/ FScene* Scene)
{
	FDepthDrawingPolicyFactory::AddStaticMesh(Scene, this);
}


void FStaticMesh::RemoveFromDrawLists()
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
	
	AddPrimitiveSceneInfo(PrimitiveSceneInfo);
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

	UpdatePrimitiveTransform(Primitive->SceneProxy, Primitive->Bounds, Primitive->CalcBounds(FTransform::Identity), Primitive->GetRenderMatrix(), AttachmentRootPosition);
}

void FScene::AddLight(class ULightComponent* Light)
{
	FLightSceneProxy* Proxy = Light->CreateSceneProxy();
	if (Proxy)
	{
		Proxy->SetTransform(Light->GetComponentTransform().ToMatrixNoScale(), Light->GetLightPosition());
		Proxy->LightSceneInfo = new FLightSceneInfo(Proxy);

		AddLightSceneInfo(Proxy->LightSceneInfo);
	}

}

void FScene::RemoveLight(class ULightComponent* Light)
{

}

void FScene::AddLightSceneInfo(FLightSceneInfo* LightSceneInfo)
{
	if (LightSceneInfo->Proxy->GetLightType() == LightType_Directional && !LightSceneInfo->Proxy->HasStaticLighting())//非静态方向光
	{
		if (!SimpleDirectionalLight)
		{
			SimpleDirectionalLight = LightSceneInfo;
		}
	}

	LightSceneInfo->AddToScene();
	
}

void FScene::AddPrimitiveSceneInfo(FPrimitiveSceneInfo* PrimitiveSceneInfo)
{
	Primitives.push_back(PrimitiveSceneInfo);
	PrimitiveSceneInfo->AddToScene(true, true);
}

void FScene::UpdatePrimitiveTransform(FPrimitiveSceneProxy* PrimitiveSceneProxy, const FBoxSphereBounds& WorldBounds, const FBoxSphereBounds& LocalBounds, const FMatrix& LocalToWorld, const FVector& OwnerPosition)
{
	PrimitiveSceneProxy->SetTransform(LocalToWorld, WorldBounds, LocalBounds, OwnerPosition);
}

