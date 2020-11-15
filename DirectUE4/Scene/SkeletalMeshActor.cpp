#include "SkeletalMeshActor.h"
#include "FBXImporter.h"
#include "SkeletalMesh.h"

SkeletalMeshActor::SkeletalMeshActor(const char* ResourcePath)
{
	FBXImporter Importer;
	Mesh = Importer.ImportSkeletalMesh(ResourcePath);
	Mesh->Register();
}

SkeletalMeshActor::~SkeletalMeshActor()
{
	Mesh->UnRegister();
}

void SkeletalMeshActor::Tick(float fDeltaTime)
{

}

