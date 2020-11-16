#include "Actor.h"

class StaticMesh;

class StaticMeshActor : public Actor
{
public:
	StaticMeshActor(const char* ResourcePath);

	virtual void PostLoad() override;

	virtual void Tick(float fDeltaTime) override;

private:
	StaticMesh * Mesh;
};