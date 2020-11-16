#pragma once

#include "Actor.h"

class SkeletalMesh;

class SkeletalMeshActor : public Actor
{
public:
	SkeletalMeshActor(const char* ResourcePath);
	virtual ~SkeletalMeshActor();

	virtual void PostLoad() override;

	virtual void Tick(float fDeltaTime) override;
private:
	SkeletalMesh * Mesh;
};