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

}

FPrimitiveSceneInfo::FPrimitiveSceneInfo(UPrimitiveComponent* InComponent, FScene* InScene)
	: Proxy(InComponent->SceneProxy),
	Scene(InScene)
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

void FPrimitiveSceneInfo::UpdateUniformBuffer()
{
	bNeedsUniformBufferUpdate = false;
	Proxy->UpdateUniformBuffer();
}
