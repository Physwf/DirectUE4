#pragma once

#include "Actor.h"
#include "SceneView.h"
#include "SceneTypes.h"

class FViewport;

class Camera : public AActor
{
public:
	Camera(class UWorld* InOwner);
	~Camera() {};

	void SetFOV(float InFOV) { FOV = InFOV; }
	void LookAt(FVector Target);
	void SetLen(float fNear, float fFar);

	FSceneView* CalcSceneView(FSceneViewFamily& ViewFamily, FViewport& VP);

	virtual void PostLoad() override {}
	virtual void Tick(float fDeltaSeconds) override;
public:
	FVector FaceDir;
	FVector Up;

	float Near;
	float Far;

	float FOV;

	/** The coordinates for the upper left corner of the master viewport subregion allocated to this player. 0-1 */
	Vector2 Origin;
	/** The size of the master viewport subregion allocated to this player. 0-1 */
	Vector2 Size;

	FSceneViewStateReference ViewState;
};