#include "MeshComponent.h"
#include "StaticMeshResources.h"
#include "StaticMesh.h"

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

