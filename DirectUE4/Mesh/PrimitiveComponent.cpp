#include "PrimitiveComponent.h"
#include "World.h"
#include "Scene.h"

extern UWorld GWorld;

UPrimitiveComponent::UPrimitiveComponent(Actor* InOwner)
	: USceneComponent(InOwner)
{

}

void UPrimitiveComponent::Register()
{
	GetWorld()->Scene->AddPrimitive(this);
}

void UPrimitiveComponent::Unregister()
{
	GetWorld()->Scene->RemovePrimitive(this);
}

void UPrimitiveComponent::InitResources()
{
	UniformBuffer.InitDynamicRHI();
}

void UPrimitiveComponent::ReleaseResources()
{
	UniformBuffer.ReleaseDynamicRHI();
}

void UPrimitiveComponent::ConditionalUpdateUniformBuffer()
{
	if (bNeedsUniformBufferUpdate)
	{
		UpdateUniformBuffer();
	}
}

void UPrimitiveComponent::UpdateUniformBuffer()
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

void UPrimitiveComponent::AddToScene(FScene* Scene/*FRHICommandListImmediate& RHICmdList, bool bUpdateStaticDrawLists, bool bAddToStaticDrawLists = true*/)
{
	AddStaticMeshes(Scene);
}

void UPrimitiveComponent::RemoveFromScene(FScene* Scene/*bool bUpdateStaticDrawLists*/)
{
	RemoveStaticMeshes(Scene);
}

void UPrimitiveComponent::AddStaticMeshes(FScene* Scene/*FRHICommandListImmediate& RHICmdList, bool bUpdateStaticDrawLists = true*/)
{
	DrawStaticElements();
	for (uint32 MeshIndex = 0; MeshIndex < StaticMeshes.size(); MeshIndex++)
	{
		FStaticMesh& Mesh = *StaticMeshes[MeshIndex];

		Mesh.AddToDrawLists(Scene);
	}
}

void UPrimitiveComponent::RemoveStaticMeshes(FScene* Scene)
{

}

void UPrimitiveComponent::SetTransform(const FMatrix& InLocalToWorld, const FBoxSphereBounds& InBounds, const FBoxSphereBounds& InLocalBounds, FVector InActorPosition)
{
	LocalToWorld = InLocalToWorld;
	bIsLocalToWorldDeterminantNegative = LocalToWorld.Determinant() < 0.0f;

	// Update the cached bounds.
	Bounds = InBounds;
	LocalBounds = InLocalBounds;
	ActorPosition = InActorPosition;

	bNeedsUniformBufferUpdate = true;
}

