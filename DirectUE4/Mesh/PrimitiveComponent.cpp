#include "PrimitiveComponent.h"
#include "World.h"
#include "Scene.h"

extern UWorld GWorld;

UPrimitiveComponent::UPrimitiveComponent(AActor* InOwner)
	: USceneComponent(InOwner)
{

}

FMatrix UPrimitiveComponent::GetRenderMatrix() const
{
	return GetComponentTransform().ToMatrixWithScale();
}

void UPrimitiveComponent::CreateRenderState_Concurrent()
{
	GetWorld()->Scene->AddPrimitive(this);
	OnRegister();
}

void UPrimitiveComponent::SendRenderTransform_Concurrent()
{
	//UpdateBounds();

	GetWorld()->Scene->UpdatePrimitiveTransform(this);
}

void UPrimitiveComponent::DestroyRenderState_Concurrent()
{
	OnUnregister();
	GetWorld()->Scene->RemovePrimitive(this);
}
