#include "Actor.h"

Actor::Actor() : Position { 0, 0, 0 }, Rotation{ 0, 0, 0 }
{
}

Actor::~Actor()
{
}

void Actor::Register()
{

}

void Actor::SetPosition(Vector InPosition)
{
	Position = InPosition;
}

void Actor::SetRotation(Rotator InRotation)
{
	Rotation = InRotation;
}

Matrix Actor::GetWorldMatrix()
{
	Matrix R = Matrix(
		Plane(0, 0, 1, 0),
		Plane(1, 0, 0, 0),
		Plane(0, 1, 0, 0),
		Plane(0, 0, 0, 1)) *Matrix::DXFormRotation(Rotation);
	Matrix T = Matrix::DXFromTranslation(Position);
	R.Transpose();
	T.Transpose();
	return  T * R;
}
