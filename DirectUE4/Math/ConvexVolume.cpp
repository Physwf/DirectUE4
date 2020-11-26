#include "ConvexVolume.h"

void FConvexVolume::Init(void)
{

}

void GetViewFrustumBounds(FConvexVolume& OutResult, const FMatrix& ViewProjectionMatrix, bool UseNearPlane)
{
	GetViewFrustumBounds(OutResult, ViewProjectionMatrix, FPlane(), false, UseNearPlane);
}

void GetViewFrustumBounds(FConvexVolume& OutResult, const FMatrix& ViewProjectionMatrix, const FPlane& InFarPlane, bool bOverrideFarPlane, bool UseNearPlane)
{
	OutResult.Planes.clear();
	FPlane	Temp;

	// Near clipping plane.
	if (UseNearPlane && ViewProjectionMatrix.GetFrustumNearPlane(Temp))
	{
		OutResult.Planes.push_back(Temp);
	}

	// Left clipping plane.
	if (ViewProjectionMatrix.GetFrustumLeftPlane(Temp))
	{
		OutResult.Planes.push_back(Temp);
	}

	// Right clipping plane.
	if (ViewProjectionMatrix.GetFrustumRightPlane(Temp))
	{
		OutResult.Planes.push_back(Temp);
	}

	// Top clipping plane.
	if (ViewProjectionMatrix.GetFrustumTopPlane(Temp))
	{
		OutResult.Planes.push_back(Temp);
	}

	// Bottom clipping plane.
	if (ViewProjectionMatrix.GetFrustumBottomPlane(Temp))
	{
		OutResult.Planes.push_back(Temp);
	}

	// Far clipping plane.
	if (bOverrideFarPlane)
	{
		OutResult.Planes.push_back(InFarPlane);
	}
	else if (ViewProjectionMatrix.GetFrustumFarPlane(Temp))
	{
		OutResult.Planes.push_back(Temp);
	}

	OutResult.Init();
}

