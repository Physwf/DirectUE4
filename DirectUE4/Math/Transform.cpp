#include "Transform.h"

const FTransform FTransform::Identity(FQuat(0.f, 0.f, 0.f, 1.f), Vector(0.f), Vector(1.f));

FTransform FTransform::GetRelativeTransform(const FTransform& Other) const
{
	// A * B(-1) = VQS(B)(-1) (VQS (A))
	// 
	// Scale = S(A)/S(B)
	// Rotation = Q(B)(-1) * Q(A)
	// Translation = 1/S(B) *[Q(B)(-1)*(T(A)-T(B))*Q(B)]
	// where A = this, B = Other
	FTransform Result;

	if (AnyHasNegativeScale(Scale3D, Other.GetScale3D()))
	{
		// @note, if you have 0 scale with negative, you're going to lose rotation as it can't convert back to quat
		GetRelativeTransformUsingMatrixWithScale(&Result, this, &Other);
	}
	else
	{
		Vector SafeRecipScale3D = GetSafeScaleReciprocal(Other.Scale3D, SMALL_NUMBER);
		Result.Scale3D = Scale3D * SafeRecipScale3D;

		if (Other.Rotation.IsNormalized() == false)
		{
			return FTransform::Identity;
		}

		FQuat Inverse = Other.Rotation.Inverse();
		Result.Rotation = Inverse * Rotation;

		Result.Translation = (Inverse*(Translation - Other.Translation))*(SafeRecipScale3D);


	}

	return Result;
}

void FTransform::GetRelativeTransformUsingMatrixWithScale(FTransform* OutTransform, const FTransform* Base, const FTransform* Relative)
{
	// the goal of using M is to get the correct orientation
	// but for translation, we still need scale
	Matrix AM = Base->ToMatrixWithScale();
	Matrix BM = Relative->ToMatrixWithScale();
	// get combined scale
	Vector SafeRecipScale3D = GetSafeScaleReciprocal(Relative->Scale3D, SMALL_NUMBER);
	Vector DesiredScale3D = Base->Scale3D*SafeRecipScale3D;
	ConstructTransformFromMatrixWithDesiredScale(AM, BM.Inverse(), DesiredScale3D, *OutTransform);
}

