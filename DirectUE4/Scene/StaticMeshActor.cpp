#include "StaticMeshActor.h"
#include "FBXImporter.h"
#include "StaticMesh.h"
#include "World.h"

StaticMeshActor::StaticMeshActor(const char* ResourcePath)
{
	FBXImporter Importer;
	Mesh = Importer.ImportStaticMesh(ResourcePath);
}

void StaticMeshActor::PostLoad()
{
	Mesh->Register(GetWorld()->Scene);
}

void StaticMeshActor::Tick(float fDeltaTime)
{

}

