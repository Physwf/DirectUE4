#include "MeshComponent.h"
#include "StaticMeshResources.h"
#include "StaticMesh.h"
#include "World.h"
#include "MapBuildDataRegistry.h"

UMeshComponent::UMeshComponent(AActor* InOwner)
	: UPrimitiveComponent(InOwner)
{

}

UMeshComponent::~UMeshComponent()
{

}

UMaterial* UStaticMeshComponent::GetMaterial(int32 MaterialIndex) const
{
	return GetStaticMesh() ? GetStaticMesh()->GetMaterial(MaterialIndex) : nullptr;
}

FPrimitiveSceneProxy* UStaticMeshComponent::CreateSceneProxy()
{
	return new FStaticMeshSceneProxy(this,false);
}

const class FMeshMapBuildData* UStaticMeshComponent::GetMeshMapBuildData() const
{
	UMapBuildDataRegistry* MapBuildData = GetOwner()->GetWorld()->MapBuildData;
	return MapBuildData->GetMeshBuildData(0);
}

