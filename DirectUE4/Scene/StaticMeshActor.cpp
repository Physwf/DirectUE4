#include "StaticMeshActor.h"
#include "FBXImporter.h"
#include "StaticMesh.h"
#include "World.h"
#include "Scene.h"

StaticMeshActor::StaticMeshActor(const char* ResourcePath)
{
	FBXImporter Importer;
	Mesh = Importer.ImportStaticMesh(ResourcePath);
	Proxy = Mesh;
}

void StaticMeshActor::PostLoad()
{
	Mesh->Register(GetWorld()->Scene);
}

void StaticMeshActor::Tick(float fDeltaTime)
{

}

void StaticMeshActor::SendRenderTransform_Concurrent()
{
	GetWorld()->Scene->UpdatePrimitiveTransform(this);
}

