#include "PrimitiveSceneInfo.h"
#include "PrimitiveComponent.h"
#include "PrimitiveSceneProxy.h"
#include "Scene.h"

FPrimitiveFlagsCompact::FPrimitiveFlagsCompact(const FPrimitiveSceneProxy* Proxy)
{

}

FPrimitiveSceneInfoCompact::FPrimitiveSceneInfoCompact(FPrimitiveSceneInfo* InPrimitiveSceneInfo)
	: PrimitiveFlagsCompact(InPrimitiveSceneInfo->Proxy)
{
	PrimitiveSceneInfo = InPrimitiveSceneInfo;
	Proxy = PrimitiveSceneInfo->Proxy;
	Bounds = PrimitiveSceneInfo->Proxy->GetBounds();
	//MinDrawDistance = PrimitiveSceneInfo->Proxy->GetMinDrawDistance();
	//MaxDrawDistance = PrimitiveSceneInfo->Proxy->GetMaxDrawDistance();

	//VisibilityId = PrimitiveSceneInfo->Proxy->GetVisibilityId();
}

FPrimitiveSceneInfo::FPrimitiveSceneInfo(UPrimitiveComponent* InComponent, FScene* InScene)
	: Proxy(InComponent->SceneProxy)
	,Scene(InScene)
	,LightList(NULL)

{

}

FPrimitiveSceneInfo::~FPrimitiveSceneInfo()
{

}

void FPrimitiveSceneInfo::AddToScene(bool bUpdateStaticDrawLists, bool bAddToStaticDrawLists /*= true*/)
{
	if (bUpdateStaticDrawLists)
	{
		AddStaticMeshes(bAddToStaticDrawLists);
	}

	FPrimitiveSceneInfoCompact CompactPrimitiveSceneInfo(this);

	Scene->PrimitiveOctree.push_back(CompactPrimitiveSceneInfo);

	for (auto LightIt = Scene->Lights.begin(); LightIt != Scene->Lights.end();++LightIt)
	{
		const FLightSceneInfoCompact& LightSceneInfoCompact = *LightIt;
		if (LightSceneInfoCompact.AffectsPrimitive(CompactPrimitiveSceneInfo.Bounds, CompactPrimitiveSceneInfo.Proxy))
		{
			FLightPrimitiveInteraction::Create(LightSceneInfoCompact.LightSceneInfo, this);
		}
	}
}

void FPrimitiveSceneInfo::RemoveFromScene(bool bUpdateStaticDrawLists)
{

}

void FPrimitiveSceneInfo::AddStaticMeshes(bool bAddToStaticDrawLists /*= true*/)
{
	Proxy->DrawStaticElements(this);

	for (uint32 MeshIndex = 0; MeshIndex < StaticMeshes.size(); MeshIndex++)
	{
		FStaticMesh& Mesh = *StaticMeshes[MeshIndex];

		if (bAddToStaticDrawLists)
		{
			// By this point, the index buffer render resource must be initialized
			// Add the static mesh to the appropriate draw lists.
			Mesh.AddToDrawLists(Scene);
		}
	}
}

void FPrimitiveSceneInfo::RemoveStaticMeshes()
{

}

void FPrimitiveSceneInfo::UpdateStaticMeshes(bool bReAddToDrawLists /*= true*/)
{
	bNeedsStaticMeshUpdate = !bReAddToDrawLists;

// 	// Remove the primitive's static meshes from the draw lists they're currently in, and re-add them to the appropriate draw lists.
// 	for (int32 MeshIndex = 0; MeshIndex < StaticMeshes.Num(); MeshIndex++)
// 	{
// 		StaticMeshes[MeshIndex].RemoveFromDrawLists();
// 		if (bReAddToDrawLists)
// 		{
// 			StaticMeshes[MeshIndex].AddToDrawLists(Scene);
// 		}
// 	}
}

void FPrimitiveSceneInfo::UpdateUniformBuffer()
{
	bNeedsUniformBufferUpdate = false;
	Proxy->UpdateUniformBuffer();
}

void FPrimitiveSceneInfo::UpdatePrecomputedLightingBuffer()
{

}
