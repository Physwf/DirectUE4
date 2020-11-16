#include "Camera.h"
#include "log.h"
#include "Viewport.h"

float					GNearClippingPlane = 10.0f;				/* Near clipping plane */

Camera::Camera():Near(1.0f), Far(1000.0f)
{
	Position = { 0,0,0 };
	Up = { 0, 0, 1 };
	FaceDir = { 1,0,0 };
	Origin = { 0,0 };
	Size = { 1.f,1.f };
}

void Camera::LookAt(Vector Target)
{
	FaceDir = Target - Position;
	FaceDir.Normalize();
}

void Camera::SetLen(float fNear, float fFar)
{
	Near = fNear;
	Far = fFar;
}

SceneView* Camera::CalcSceneView(SceneViewFamily& ViewFamily, Viewport& VP)
{
	ViewInitOptions InitOptions;

	int32 X = FMath::TruncToInt(Origin.X * VP.GetSizeXY().X);
	int32 Y = FMath::TruncToInt(Origin.Y * VP.GetSizeXY().Y);

	X += VP.GetInitialPositionXY().X;
	Y += VP.GetInitialPositionXY().Y;

	uint32 SizeX = FMath::TruncToInt(Size.X * VP.GetSizeXY().X);
	uint32 SizeY = FMath::TruncToInt(Size.Y * VP.GetSizeXY().Y);

	IntRect UnconstrainedRectangle = IntRect(X, Y, X + SizeX, Y + SizeY);

	InitOptions.SetViewRectangle(UnconstrainedRectangle);

	InitOptions.ViewOrigin = Position;
	InitOptions.ViewRotationMatrix = InverseRotationMatrix(Rotation) * Matrix(
		Plane(0, 0, 1, 0),
		Plane(1, 0, 0, 0),
		Plane(0, 1, 0, 0),
		Plane(0, 0, 0, 1));

	float AspectRatio = (float)VP.GetSizeXY().X / (float)VP.GetSizeXY().Y;
	InitOptions.ProjectionMatrix = ReversedZPerspectiveMatrix(
		FMath::Max(0.001f, FOV) * (float)PI / 360.0f,
		AspectRatio,
		1.0f,
		GNearClippingPlane);

	InitOptions.ViewActor = this;

	InitOptions.FOV = FOV;
	InitOptions.DesiredFOV = FOV;
	InitOptions.ViewOrigin = Position;
	//InitOptions.ViewRotation = Rotation;

	SceneView* const View = new SceneView(InitOptions);
	ViewFamily.Views.push_back(View);

	return View;
}

void Camera::Tick(float fDeltaSeconds)
{

}

