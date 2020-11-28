#include "Actor.h"

AActor::AActor(UWorld* InOwner)
	: WorldPrivite(InOwner), Position{ 0, 0, 0 }, Rotation{ 0, 0, 0 }
{
}

AActor::~AActor()
{
}

void AActor::SetActorLocation(FVector InPosition)
{
	Position = InPosition;
}

void AActor::SetActorRotation(FRotator InRotation)
{
	Rotation = InRotation;
}

FMatrix AActor::GetWorldMatrix()
{
	FMatrix R = FMatrix(
		FPlane(0, 0, 1, 0),
		FPlane(1, 0, 0, 0),
		FPlane(0, 1, 0, 0),
		FPlane(0, 0, 0, 1)) *FMatrix::DXFormRotation(Rotation);
	FMatrix T = FMatrix::DXFromTranslation(Position);
	//R.Transpose();
	//T.Transpose();
	return  R * T;
}
