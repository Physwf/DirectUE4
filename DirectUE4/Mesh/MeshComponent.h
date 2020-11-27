#pragma once

#include "PrimitiveComponent.h"

class AActor;
class UMaterial;

class UMeshComponent : public UPrimitiveComponent
{
public:
	UMeshComponent(AActor* InOwner);
	virtual ~UMeshComponent();

};

class UStaticMeshComponent : public UMeshComponent
{
public:
	UStaticMeshComponent(AActor* InOwner) : UMeshComponent(InOwner) {}
	virtual ~UStaticMeshComponent() { }

	bool SetStaticMesh(class UStaticMesh* NewMesh)
	{
		StaticMesh = NewMesh;
		return true;
	}
	UStaticMesh* GetStaticMesh() const
	{
		return StaticMesh;
	}
	virtual UMaterial* GetMaterial(int32 MaterialIndex) const override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy();
private:
	class UStaticMesh* StaticMesh;
};