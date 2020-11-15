#include "StaticMeshActor.h"
#include "FBXImporter.h"
#include "StaticMesh.h"

StaticMeshActor::StaticMeshActor(const char* ResourcePath)
{
	FBXImporter Importer;
	Mesh = Importer.ImportStaticMesh(ResourcePath);
	Mesh->Register();
}

void StaticMeshActor::Tick(float fDeltaTime)
{

}

