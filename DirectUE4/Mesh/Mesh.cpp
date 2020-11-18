#include "Mesh.h"
#include "World.h"
#include "Scene.h"

extern World GWorld;

MeshPrimitive::MeshPrimitive()
{

}

void MeshPrimitive::Register(FScene* Scene)
{
	Scene->AddPrimitive(this);
}

void MeshPrimitive::UnRegister(FScene* Scene)
{
	Scene->RemovePrimitive(this);
}

void MeshPrimitive::AddToScene(FScene* Scene/*FRHICommandListImmediate& RHICmdList, bool bUpdateStaticDrawLists, bool bAddToStaticDrawLists = true*/)
{
	AddStaticMeshes(Scene);
}

void MeshPrimitive::RemoveFromScene(FScene* Scene/*bool bUpdateStaticDrawLists*/)
{
	RemoveStaticMeshes(Scene);
}

void MeshPrimitive::AddStaticMeshes(FScene* Scene/*FRHICommandListImmediate& RHICmdList, bool bUpdateStaticDrawLists = true*/)
{
	DrawStaticElements();
	for (uint32 MeshIndex = 0; MeshIndex < StaticMeshes.size(); MeshIndex++)
	{
		FStaticMesh& Mesh = *StaticMeshes[MeshIndex];

		Mesh.AddToDrawLists(Scene);
	}
}

void MeshPrimitive::RemoveStaticMeshes(FScene* Scene)
{

}

FPrimitiveUniformShaderParameters PrimitiveUniformShaderParameters;
