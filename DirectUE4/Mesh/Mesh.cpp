#include "Mesh.h"
#include "World.h"
#include "Scene.h"

extern World GWorld;

MeshPrimitive::MeshPrimitive()
{

}

void MeshPrimitive::Register()
{
	Scene->AddPrimitive(this);
}

void MeshPrimitive::UnRegister()
{
	Scene->RemovePrimitive(this);
}

void MeshPrimitive::AddToScene(/*FRHICommandListImmediate& RHICmdList, bool bUpdateStaticDrawLists, bool bAddToStaticDrawLists = true*/)
{
	AddStaticMeshes();
}

void MeshPrimitive::RemoveFromScene(/*bool bUpdateStaticDrawLists*/)
{
	RemoveStaticMeshes();
}

void MeshPrimitive::AddStaticMeshes(/*FRHICommandListImmediate& RHICmdList, bool bUpdateStaticDrawLists = true*/)
{
	DrawStaticElements();
	for (uint32 MeshIndex = 0; MeshIndex < StaticMeshes.size(); MeshIndex++)
	{
		FStaticMesh& Mesh = *StaticMeshes[MeshIndex];

		Mesh.AddToDrawLists(Scene);
	}
}

void MeshPrimitive::RemoveStaticMeshes()
{

}

