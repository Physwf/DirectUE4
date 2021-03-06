#include "World.h"
#include "Scene.h"
#include "StaticMeshActor.h"
#include "SkeletalMeshActor.h"
#include "PointLightActor.h"
#include "DirectionalLightActor.h"
#include "MapBuildDataRegistry.h"
#include "LightMap.h"
#include "ShadowMap.h"
#include "MeshComponent.h"
#include "AtmosphereFog.h"
#include "SkyLight.h"
#include "PrecomputedVolumetricLightmap.h"

class FloorActor : public StaticMeshActor
{
public:
	FloorActor(class UWorld* InOwner, const char* ResourcePath)
		: StaticMeshActor(InOwner, ResourcePath)
	{
		MeshComponent->Mobility = EComponentMobility::Static;
	}
};

class SphereActor : public StaticMeshActor
{
public:
	SphereActor(class UWorld* InOwner, const char* ResourcePath)
		:StaticMeshActor(InOwner, ResourcePath)
	{
		MeshComponent->Mobility = EComponentMobility::Movable;
	}
};

void UWorld::InitWorld()
{
	Scene = new FScene(this);

	PrecomputedVolumetricLightmap = new FPrecomputedVolumetricLightmap();

	AAtmosphericFog* Atmosphere = SpawnActor<AAtmosphericFog>();
	ASkylight* Skylight = SpawnActor<ASkylight>();

	USkyLightComponent::UpdateSkyCaptureContents(this);

	MapBuildData = new UMapBuildDataRegistry();
	FMeshMapBuildData& MeshBuildData = MapBuildData->AllocateMeshBuildData(0, false);
	MeshBuildData.LightMap = FLightMap2D::AllocateLightMap();
	MeshBuildData.ShadowMap = FShadowMap2D::AllocateShadowMap();

	MapBuildData->AllocateLevelPrecomputedVolumetricLightmapBuildData();
	PrecomputedVolumetricLightmap->AddToScene(Scene, MapBuildData);


	StaticMeshActor* Floor = SpawnActor<FloorActor>("Primitives/Floor.fbx");
	Floor->SetActorLocation(FVector(0, 0, -100));
	StaticMeshActor* Sphere = SpawnActor<SphereActor>("Primitives/Sphere.fbx");
	Sphere->SetActorLocation(FVector(0,0,100));
	//SkeletalMeshActor* m2 = SpawnActor<SkeletalMeshActor>("./Mannequin/SK_Mannequin.FBX");
	//Mesh* m1 = SpawnActor<Mesh>("shaderBallNoCrease/shaderBall.fbx");
	//Mesh* m1 = SpawnActor<Mesh>("k526efluton4-House_15/247_House 15_fbx.fbx");

	SkeletalMeshActor* Mannequin = SpawnActor<SkeletalMeshActor>("Mannequin/SK_Mannequin.FBX","Mannequin/ThirdPersonWalk.FBX");
	Mannequin->SetActorRotation(FRotator(0, 90.f, 0));
	Mannequin->SetActorLocation(FVector(0.f, -100.f, -45.f));

	PointLightActor* l1 = SpawnActor<PointLightActor>();
	l1->SetActorLocation(FVector(0, 0, 500));

	Camera* C = SpawnActor<Camera>();
	C->SetActorLocation(FVector(-400, 0,  0));
	C->LookAt(FVector(0, 0, 0));
	C->SetFOV(90.f);
	mCameras.push_back(C);
}

void UWorld::Tick(float fDeltaSeconds)
{
	for (AActor* actor : mAllActors)
	{
		actor->Tick(fDeltaSeconds);
	}
}

void UWorld::DestroyActor(AActor* InActor)
{
	auto it = std::find(mAllActors.begin(), mAllActors.end(), InActor);
	if (it != mAllActors.end())
	{
		mAllActors.erase(it);
	}
}

void UWorld::SendAllEndOfFrameUpdates()
{
	for (UActorComponent* Component : ActorComponents)
	{
		Component->DoDeferredRenderUpdates_Concurrent();
	}
}

void UWorld::RegisterComponent(class UActorComponent* InComponent)
{
	ActorComponents.push_back(InComponent);
}

void UWorld::UnregisterComponent(class UActorComponent* InComponent)
{
	auto It = std::find(ActorComponents.begin(), ActorComponents.end(), InComponent);
	if (It != ActorComponents.end()) ActorComponents.erase(It);
}

UWorld GWorld;