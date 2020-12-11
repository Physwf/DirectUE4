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

	uint8 bRegistered : 1;

	/** If the render state is currently created for this component */
	uint8 bRenderStateCreated : 1;

private:
	uint8 bRenderStateDirty : 1;
	/** Is this component's transform in need of sending to the renderer? */
	uint8 bRenderTransformDirty : 1;

	/** Is this component's dynamic data in need of sending to the renderer? */
	uint8 bRenderDynamicDataDirty : 1;

public:
	void Register();
	void Unregister();
	bool IsRegistered() const { return bRegistered; }
	void DoDeferredRenderUpdates_Concurrent();

	void MarkRenderStateDirty();
	/** Indicate that dynamic data for this component needs to be sent at the end of the frame. */
	void MarkRenderDynamicDataDirty();
	/** Marks the transform as dirty - will be sent to the render thread at the end of the frame*/
	void MarkRenderTransformDirty();
	/** If we belong to a world, mark this for a deferred update, otherwise do it now. */
	void MarkForNeededEndOfFrameUpdate();
	/** If we belong to a world, mark this for a deferred update, otherwise do it now. */
	void MarkForNeededEndOfFrameRecreate();

	AActor* GetOwner() const { return Owner; }
	UWorld* GetWorld() { return WorldPrivite; }
protected:


	virtual void CreateRenderState_Concurrent();

	virtual void SendRenderTransform_Concurrent();;

	virtual void SendRenderDynamicData_Concurrent();;

	virtual void DestroyRenderState_Concurrent();


	void RecreateRenderState_Concurrent();
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

	virtual void UpdateBounds();

	AActor* GetAttachmentRootActor() const;

	bool MoveComponent(const FVector& Delta, const FQuat& NewRotation);

	void ConditionalUpdateComponentToWorld();

};

