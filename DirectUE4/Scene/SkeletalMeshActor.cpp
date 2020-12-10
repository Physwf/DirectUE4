#include "SkeletalMeshActor.h"
#include "FBXImporter.h"
#include "SkeletalMesh.h"
#include "World.h"
#include "MeshComponent.h"

SkeletalMeshActor::SkeletalMeshActor(class UWorld* InOwner, const char* ResourcePath)
 : AActor(InOwner)
{
	FBXImporter Importer;
	USkeletalMesh* Mesh = Importer.ImportSkeletalMesh(this,ResourcePath);

	MeshComponent = new USkeletalMeshComponent(this);
	MeshComponent->SetSkeletalMesh(Mesh);
	MeshComponent->Mobility = EComponentMobility::Movable;
}

SkeletalMeshActor::~SkeletalMeshActor()
{
	MeshComponent->Unregister();
}

void SkeletalMeshActor::PostLoad()
{
	MeshComponent->Register();
}

void SkeletalMeshActor::Tick(float fDeltaTime)
{

}

