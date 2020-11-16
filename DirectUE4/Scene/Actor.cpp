#include "Actor.h"

Actor::Actor() : Position { 0, 0, 0 }, Rotation{ 0, 0, 0 }
{
}

Actor::~Actor()
{
}

void Actor::SetPosition(FVector InPosition)
{
	Position = InPosition;
}

void Actor::SetRotation(FRotator InRotation)
{
	Rotation = InRotation;
}

FMatrix Actor::GetWorldMatrix()
{
	FMatrix R = FMatrix(
		Plane(0, 0, 1, 0),
		Plane(1, 0, 0, 0),
		Plane(0, 1, 0, 0),
		Plane(0, 0, 0, 1)) *FMatrix::DXFormRotation(Rotation);
	FMatrix T = FMatrix::DXFromTranslation(Position);
	R.Transpose();
	T.Transpose();
	return  T * R;
}
