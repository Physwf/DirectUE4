#include "World.h"
#include "Scene.h"
#include "StaticMesh.h"
#include "SkeletalMesh.h"

void World::InitWorld()
{
	mScene = new Scene();
	extern Scene* GScene;
	GScene = mScene;

	//Mesh* m1 = SpawnActor<Mesh>("Primitives/Sphere.fbx");
	FBXImporter Importer;
	Importer.ImportSkeletalMesh("./Mannequin/SK_Mannequin.FBX");
	//Mesh* m1 = SpawnActor<Mesh>("shaderBallNoCrease/shaderBall.fbx");
	//Mesh* m1 = SpawnActor<Mesh>("k526efluton4-House_15/247_House 15_fbx.fbx");
	//m1->SetPosition(20.0f, -100.0f, 480.0f);
	//m1->SetRotation(-3.14f / 2.0f, 0, 0);

	mDirLight = SpawnActor<DirectionalLight>();
	mDirLight->Color = Vector(1.0f, 1.0f, 1.0f);
	mDirLight->Direction = Vector(1.0f, 1.0f, 1.0f);
	mDirLight->Intencity = 1000.f;
	mDirLight->LightSourceAngle = 0.5357f;
	mDirLight->LightSourceSoftAngle = 0.0f;
	mDirLight->Direction.Normalize();

	Camera* C = SpawnActor<Camera>();
	C->SetPosition(Vector(-400, 0,  0));
	C->LookAt(Vector(0, 0, 0));
	C->SetFOV(90.f);
	mCameras.push_back(C);
}

void World::Tick(float fDeltaSeconds)
{
	for (Actor* actor : mAllActors)
	{
		actor->Tick(fDeltaSeconds);
	}
}

void World::DestroyActor(Actor* InActor)
{
	InActor->UnRegister();
	auto it = std::find(mAllActors.begin(), mAllActors.end(), InActor);
	if (it != mAllActors.end())
	{
		mAllActors.erase(it);
	}
}

World GWorld;