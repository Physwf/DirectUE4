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

void MeshPrimitive::InitResources()
{
	UniformBuffer.InitDynamicRHI();
}

void MeshPrimitive::ReleaseResources()
{
	UniformBuffer.ReleaseDynamicRHI();
}

void MeshPrimitive::ConditionalUpdateUniformBuffer()
{
	if (bNeedsUniformBufferUpdate)
	{
		UpdateUniformBuffer();
	}
}

void MeshPrimitive::UpdateUniformBuffer()
{
	const FPrimitiveUniformShaderParameters PrimitiveUniformShaderParameters =
		GetPrimitiveUniformShaderParameters(
			LocalToWorld,
			ActorPosition,
			Bounds,
			LocalBounds,
			false/*bReceivesDecals*/,
			false/*HasDistanceFieldRepresentation()*/,
			false/*HasDynamicIndirectShadowCasterRepresentation()*/,
			false/*UseSingleSampleShadowFromStationaryLights()*/,
			false/*Scene->HasPrecomputedVolumetricLightmap_RenderThread()*/,
			false/*UseEditorDepthTest()*/,
			1/*GetLightingChannelMask()*/,
			1.0f/*LpvBiasMultiplier*/);
	UniformBuffer.SetContents(PrimitiveUniformShaderParameters);
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

void MeshPrimitive::SetTransform(const FMatrix& InLocalToWorld, const FBoxSphereBounds& InBounds, const FBoxSphereBounds& InLocalBounds, FVector InActorPosition)
{
	LocalToWorld = InLocalToWorld;
	bIsLocalToWorldDeterminantNegative = LocalToWorld.Determinant() < 0.0f;

	// Update the cached bounds.
	Bounds = InBounds;
	LocalBounds = InLocalBounds;
	ActorPosition = InActorPosition;

	bNeedsUniformBufferUpdate = true;
}

