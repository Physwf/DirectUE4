#include "StaticMeshActor.h"
#include "FBXImporter.h"
#include "World.h"
#include "Scene.h"
#include "MeshComponent.h"
#include "StaticMesh.h"

StaticMeshActor::StaticMeshActor(class UWorld* InOwner, const char* ResourcePath)
	:AActor(InOwner)
{
	MeshComponent = new UStaticMeshComponent(this);

	FBXImporter Importer;
	UStaticMesh* Mesh = Importer.ImportStaticMesh(this, ResourcePath);

	MeshComponent->SetStaticMesh(Mesh);

	RootComponent = MeshComponent;
}

void StaticMeshActor::PostLoad()
{
	MeshComponent->Register();
}

void StaticMeshActor::Tick(float fDeltaTime)
{

}

