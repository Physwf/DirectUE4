#include "SkeletalMeshActor.h"
#include "FBXImporter.h"
#include "SkeletalMesh.h"
#include "World.h"

SkeletalMeshActor::SkeletalMeshActor(class UWorld* InOwner, const char* ResourcePath)
 : AActor(InOwner)
{
	//FBXImporter Importer;
	//Mesh = Importer.ImportSkeletalMesh(this,ResourcePath);
}

SkeletalMeshActor::~SkeletalMeshActor()
{
	//Mesh->Unregister();
}

void SkeletalMeshActor::PostLoad()
{
	//Mesh->Register();
}

void SkeletalMeshActor::Tick(float fDeltaTime)
{

}

