#include "StaticMeshActor.h"
#include "FBXImporter.h"
#include "StaticMesh.h"
#include "World.h"
#include "Scene.h"

StaticMeshActor::StaticMeshActor(class UWorld* InOwner, const char* ResourcePath)
	:Actor(InOwner)
{
	FBXImporter Importer;
	Mesh = Importer.ImportStaticMesh(this, ResourcePath);
	Proxy = Mesh;
}

void StaticMeshActor::PostLoad()
{
	Mesh->Register();
}

void StaticMeshActor::Tick(float fDeltaTime)
{

}

void StaticMeshActor::SendRenderTransform_Concurrent()
{
	GetWorld()->Scene->UpdatePrimitiveTransform(this);
}

