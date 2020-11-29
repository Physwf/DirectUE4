#include "PrimitiveComponent.h"
#include "World.h"
#include "Scene.h"

extern UWorld GWorld;

UPrimitiveComponent::UPrimitiveComponent(AActor* InOwner)
	: USceneComponent(InOwner)
{

}

void UPrimitiveComponent::Register()
{
	GetWorld()->Scene->AddPrimitive(this);
	OnRegister();
}

void UPrimitiveComponent::Unregister()
{
	GetWorld()->Scene->RemovePrimitive(this);
	OnUnregister();
}

FMatrix UPrimitiveComponent::GetRenderMatrix() const
{
	return GetComponentTransform().ToMatrixWithScale();
}

void UPrimitiveComponent::SendRenderTransform_Concurrent()
{
	//UpdateBounds();

	GetWorld()->Scene->UpdatePrimitiveTransform(this);
}
