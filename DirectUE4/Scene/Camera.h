#pragma once

#include "Actor.h"
#include "SceneView.h"

class Viewport;

class Camera : public Actor
{
public:
	Camera();
	~Camera() {};

	void SetFOV(float InFOV) { FOV = InFOV; }
	void LookAt(Vector Target);
	void SetLen(float fNear, float fFar);

	SceneView* CalcSceneView(SceneViewFamily& ViewFamily, Viewport& VP);

	virtual void Tick(float fDeltaSeconds) override;
	virtual void Register() override {};
	virtual void UnRegister() override {};
public:
	Vector FaceDir;
	Vector Up;

	float Near;
	float Far;

	float FOV;

	/** The coordinates for the upper left corner of the master viewport subregion allocated to this player. 0-1 */
	Vector2 Origin;
	/** The size of the master viewport subregion allocated to this player. 0-1 */
	Vector2 Size;
};