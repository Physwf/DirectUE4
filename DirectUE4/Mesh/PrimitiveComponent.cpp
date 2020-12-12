#include "PrimitiveComponent.h"
#include "World.h"
#include "Scene.h"

extern UWorld GWorld;

UPrimitiveComponent::UPrimitiveComponent(AActor* InOwner)
	: USceneComponent(InOwner)
{
	CastShadow = true;
}

FMatrix UPrimitiveComponent::GetRenderMatrix() const
{
	return GetComponentTransform().ToMatrixWithScale();
}

void UPrimitiveComponent::CreateRenderState_Concurrent()
{
	USceneComponent::CreateRenderState_Concurrent();

	UpdateBounds();

	GetWorld()->Scene->AddPrimitive(this);
}

void UPrimitiveComponent::SendRenderTransform_Concurrent()
{
	UpdateBounds();

	GetWorld()->Scene->UpdatePrimitiveTransform(this);

	USceneComponent::SendRenderTransform_Concurrent();
}

void UPrimitiveComponent::DestroyRenderState_Concurrent()
{
	GetWorld()->Scene->RemovePrimitive(this);

	USceneComponent::DestroyRenderState_Concurrent();

}
