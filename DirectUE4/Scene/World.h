#pragma once

#include <vector>
#include "Light.h"

class Actor;
class Camera;
class FScene;

class World
{
public:
	void InitWorld();
	void Tick(float fDeltaSeconds);

	template<typename T, typename... ArgTypes>
	T* SpawnActor(ArgTypes... Args)
	{
		T* NewActor = new T(std::forward<ArgTypes>(Args)...);
		mAllActors.push_back(NewActor);
		NewActor->WorldPrivite = this;
		NewActor->PostLoad();
		return NewActor;
	}
	void DestroyActor(Actor* InActor);
	const std::vector<Camera*> GetCameras() const { return mCameras; }

	void SendAllEndOfFrameUpdates();

private:
	std::vector<Camera*> mCameras;
	DirectionalLight* mDirLight;
	std::vector<Actor*> mAllActors;
public:
	FScene* Scene;
};

extern World GWorld;
