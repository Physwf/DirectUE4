#include "SkeletalMeshActor.h"
#include "FBXImporter.h"
#include "SkeletalMesh.h"
#include "World.h"

SkeletalMeshActor::SkeletalMeshActor(const char* ResourcePath)
{
	FBXImporter Importer;
	Mesh = Importer.ImportSkeletalMesh(ResourcePath);
	Proxy = Mesh;
}

SkeletalMeshActor::~SkeletalMeshActor()
{
	Mesh->UnRegister(GetWorld()->Scene);
}

void SkeletalMeshActor::PostLoad()
{
	Mesh->Register(GetWorld()->Scene);
}

void SkeletalMeshActor::Tick(float fDeltaTime)
{

}

