#include "Actor.h"

class StaticMesh;

class StaticMeshActor : public Actor
{
public:
	StaticMeshActor(class UWorld* InOwner, const char* ResourcePath);

	virtual void PostLoad() override;

	virtual void Tick(float fDeltaTime) override;

	virtual void SendRenderTransform_Concurrent() override;

private:
	StaticMesh * Mesh;
};