#pragma once

#include "UnrealMath.h"
#include "Transform.h"
#include "EngineTypes.h"

class UWorld;
class AActor;


class UActorComponent
{
public:
	UActorComponent(AActor* InOwner);
	virtual ~UActorComponent();
protected:

	virtual void OnRegister();
	virtual void OnUnregister();
	/** Is this component's transform in need of sending to the renderer? */
	uint8 bRenderTransformDirty : 1;

	/** Is this component's dynamic data in need of sending to the renderer? */
	uint8 bRenderDynamicDataDirty : 1;

public:
	void Register();
	void Unregister();

	void DoDeferredRenderUpdates_Concurrent();

	AActor* GetOwner() const { return Owner; }
	UWorld* GetWorld() { return WorldPrivite; }
protected:
	virtual void CreateRenderState_Concurrent();

	virtual void SendRenderTransform_Concurrent() 
	{
		bRenderTransformDirty = false;
	};

	virtual void SendRenderDynamicData_Concurrent() 
	{
		bRenderDynamicDataDirty = false;
	};

	virtual void DestroyRenderState_Concurrent();
private:
	UWorld * WorldPrivite;
	mutable AActor* Owner;
};

class USceneComponent : public UActorComponent
{
public:
	USceneComponent(AActor* InOwner);
	virtual ~USceneComponent();

	FBoxSphereBounds Bounds;
	/** Location of the component relative to its parent */
	FVector RelativeLocation;
	/** Rotation of the component relative to its parent */
	FRotator RelativeRotation;
	FVector RelativeScale3D;

	FVector ComponentVelocity;

	EComponentMobility::Type Mobility;
private:
	/** Current transform of the component, relative to the world */
	FTransform ComponentToWorld;
protected:
	bool InternalSetWorldLocationAndRotation(FVector NewLocation, const FQuat& NewQuat);
public:
	inline const FTransform& GetComponentTransform() const
	{
		return ComponentToWorld;
	}
	FVector GetComponentLocation() const
	{
		return GetComponentTransform().GetLocation();
	}
	FQuat GetComponentQuat() const
	{
		return GetComponentTransform().GetRotation();
	}
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const;

	AActor* GetAttachmentRootActor() const;

	bool MoveComponent(const FVector& Delta, const FQuat& NewRotation);

	void ConditionalUpdateComponentToWorld();

};

