#pragma once

#include <vector>

class AActor;
class Camera;
class FScene;
class UActorComponent;

class UWorld
{
public:
	void InitWorld();
	void Tick(float fDeltaSeconds);

	template<typename T, typename... ArgTypes>
	T* SpawnActor(ArgTypes... Args)
	{
		T* NewActor = new T(this, std::forward<ArgTypes>(Args)...);
		mAllActors.push_back(NewActor);
		NewActor->PostLoad();
		return NewActor;
	}
	void DestroyActor(AActor* InActor);
	const std::vector<Camera*> GetCameras() const { return mCameras; }

	void SendAllEndOfFrameUpdates();

private:
	std::vector<Camera*> mCameras;
	std::vector<AActor*> mAllActors;
	std::vector<UActorComponent*> ActorComponents;
public:
	FScene* Scene;
};

extern UWorld GWorld;
