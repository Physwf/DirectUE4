#include "Actor.h"

class UStaticMeshComponent;

class StaticMeshActor : public AActor
{
public:
	StaticMeshActor(class UWorld* InOwner, const char* ResourcePath);

	virtual void PostLoad() override;

	virtual void Tick(float fDeltaTime) override;

private:
	UStaticMeshComponent* MeshComponent;
};