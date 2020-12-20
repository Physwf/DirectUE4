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

	void RegisterComponent(class UActorComponent* InComponent);
	void UnregisterComponent(class UActorComponent* InComponent);
private:
	std::vector<Camera*> mCameras;
	std::vector<AActor*> mAllActors;
	std::vector<UActorComponent*> ActorComponents;

	class FPrecomputedVolumetricLightmap*			PrecomputedVolumetricLightmap;
public:
	FScene* Scene;
	class UMapBuildDataRegistry* MapBuildData;
};

extern UWorld GWorld;
