#include "Camera.h"
#include "log.h"
#include "Viewport.h"

float					GNearClippingPlane = 10.0f;				/* Near clipping plane */

Camera::Camera(class UWorld* InOwner)
	: AActor(InOwner), Near(1.0f), Far(1000.0f)
{
	Position = { 0,0,0 };
	Up = { 0, 0, 1 };
	FaceDir = { 1,0,0 };
	Origin = { 0,0 };
	Size = { 1.f,1.f };
}

void Camera::LookAt(FVector Target)
{
	FaceDir = Target - Position;
	FaceDir.Normalize();
}

void Camera::SetLen(float fNear, float fFar)
{
	Near = fNear;
	Far = fFar;
}

FSceneView* Camera::CalcSceneView(FSceneViewFamily& ViewFamily, FViewport& VP)
{
	ViewInitOptions InitOptions;
	InitOptions.ViewFamily = &ViewFamily;
	int32 X = FMath::TruncToInt(Origin.X * VP.GetSizeXY().X);
	int32 Y = FMath::TruncToInt(Origin.Y * VP.GetSizeXY().Y);

	X += VP.GetInitialPositionXY().X;
	Y += VP.GetInitialPositionXY().Y;

	uint32 SizeX = FMath::TruncToInt(Size.X * VP.GetSizeXY().X);
	uint32 SizeY = FMath::TruncToInt(Size.Y * VP.GetSizeXY().Y);

	FIntRect UnconstrainedRectangle = FIntRect(X, Y, X + SizeX, Y + SizeY);

	InitOptions.SetViewRectangle(UnconstrainedRectangle);

	InitOptions.ViewOrigin = Position;
	InitOptions.ViewRotationMatrix = FInverseRotationMatrix(Rotation) * FMatrix(
		FPlane(0, 0, 1, 0),
		FPlane(1, 0, 0, 0),
		FPlane(0, 1, 0, 0),
		FPlane(0, 0, 0, 1));

	float AspectRatio = (float)VP.GetSizeXY().X / (float)VP.GetSizeXY().Y;
	InitOptions.ProjectionMatrix = FReversedZPerspectiveMatrix(
		FMath::Max(0.001f, FOV) * (float)PI / 360.0f,
		AspectRatio,
		1.0f,
		GNearClippingPlane);

	InitOptions.ViewActor = this;

	InitOptions.FOV = FOV;
	InitOptions.DesiredFOV = FOV;
	InitOptions.ViewOrigin = Position;
	//InitOptions.ViewRotation = Rotation;

	FSceneView* const View = new /*(std::align_val_t(alignof(FSceneView)))*/FSceneView(InitOptions);
	ViewFamily.Views.push_back(View);

	return View;
}

void Camera::Tick(float fDeltaSeconds)
{

}

