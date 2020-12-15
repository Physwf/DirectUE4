#include "SkeletalMeshActor.h"
#include "FBXImporter.h"
#include "SkeletalMesh.h"
#include "World.h"
#include "MeshComponent.h"
#include "AnimSequence.h"

SkeletalMeshActor::SkeletalMeshActor(class UWorld* InOwner, const char* ResourcePath)
 : AActor(InOwner)
{
	FBXImporter Importer;
	USkeletalMesh* Mesh = Importer.ImportSkeletalMesh(this,ResourcePath);

	MeshComponent = new USkeletalMeshComponent(this);
	MeshComponent->SetSkeletalMesh(Mesh);
	MeshComponent->Mobility = EComponentMobility::Movable;

	RootComponent = MeshComponent;
}

SkeletalMeshActor::SkeletalMeshActor(class UWorld* InOwner, const char* ResourcePath, const char* AnimationPath)
	:SkeletalMeshActor(InOwner, ResourcePath)
{
	FBXImporter Importer;
	UAnimSequence* AnimSequence = Importer.ImportFbxAnimation(MeshComponent->SkeletalMesh->Skeleton, AnimationPath,"Idle",false);
	MeshComponent->PlayAnimation(AnimSequence,true);
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

