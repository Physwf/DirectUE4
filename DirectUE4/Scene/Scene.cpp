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

void FScene::AddPrimitiveSceneInfo(FPrimitiveSceneInfo* PrimitiveSceneInfo)
{
	Primitives.push_back(PrimitiveSceneInfo);
	PrimitiveSceneProxies.push_back(PrimitiveSceneInfo->Proxy);
	//PrimitiveBounds.AddUninitialized();
	//PrimitiveFlagsCompact.AddUninitialized();
	//PrimitiveVisibilityIds.AddUninitialized();
	//PrimitiveOcclusionFlags.AddUninitialized();
	//PrimitiveComponentIds.AddUninitialized();
	//PrimitiveOcclusionBounds.AddUninitialized();

	PrimitiveSceneInfo->AddToScene(true, true);
}

void FScene::UpdatePrimitiveTransform(FPrimitiveSceneProxy* PrimitiveSceneProxy, const FBoxSphereBounds& WorldBounds, const FBoxSphereBounds& LocalBounds, const FMatrix& LocalToWorld, const FVector& OwnerPosition)
{
	PrimitiveSceneProxy->SetTransform(LocalToWorld, WorldBounds, LocalBounds, OwnerPosition);
}

