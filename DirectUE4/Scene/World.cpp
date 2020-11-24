#include "World.h"
#include "Scene.h"
#include "StaticMeshActor.h"
#include "SkeletalMeshActor.h"

void UWorld::InitWorld()
{
	Scene = new FScene();

	StaticMeshActor* m1 = SpawnActor<StaticMeshActor>("Primitives/Sphere.fbx");
	//SkeletalMeshActor* m2 = SpawnActor<SkeletalMeshActor>("./Mannequin/SK_Mannequin.FBX");
	//Mesh* m1 = SpawnActor<Mesh>("shaderBallNoCrease/shaderBall.fbx");
	//Mesh* m1 = SpawnActor<Mesh>("k526efluton4-House_15/247_House 15_fbx.fbx");
	//m1->SetPosition(20.0f, -100.0f, 480.0f);
	//m1->SetRotation(-3.14f / 2.0f, 0, 0);

	Camera* C = SpawnActor<Camera>();
	C->SetPosition(FVector(-400, 0,  0));
	C->LookAt(FVector(0, 0, 0));
	C->SetFOV(90.f);
	mCameras.push_back(C);
}

void UWorld::Tick(float fDeltaSeconds)
{
	for (Actor* actor : mAllActors)
	{
		actor->Tick(fDeltaSeconds);
	}
}

void UWorld::DestroyActor(Actor* InActor)
{
	auto it = std::find(mAllActors.begin(), mAllActors.end(), InActor);
	if (it != mAllActors.end())
	{
		mAllActors.erase(it);
	}
}

void UWorld::SendAllEndOfFrameUpdates()
{
	for (Actor* actor : mAllActors)
	{
		actor->DoDeferredRenderUpdates_Concurrent();
	}
}

UWorld GWorld;