#include "MeshComponent.h"
#include "StaticMeshResources.h"

UMeshComponent::UMeshComponent(AActor* InOwner)
	: UPrimitiveComponent(InOwner)
{

}

UMeshComponent::~UMeshComponent()
{

}

FPrimitiveSceneProxy* UStaticMeshComponent::CreateSceneProxy()
{
	return new FStaticMeshSceneProxy(this,false);
}

