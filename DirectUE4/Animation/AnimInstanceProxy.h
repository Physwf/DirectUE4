#pragma once

#include "AnimTypes.h"
#include "BoneContainer.h"
#include "Skeleton.h"
#include "AnimationAsset.h"
#include "BonePose.h"
#include "AnimInstance.h"

struct FAnimNode_Base;
struct FPoseContext;

struct FAnimInstanceProxy
{
	FAnimInstanceProxy(UAnimInstance* Instance) 
		: AnimInstanceObject(Instance)
		, Skeleton(nullptr)
		, SkeletalMeshComponent(nullptr)
		, CurrentDeltaSeconds(0.0f)
		, CurrentTimeDilation(1.0f)
		, SyncGroupWriteIndex(0)
		, RootNode(nullptr)
	{

	}

	
	void RecalcRequiredBones(USkeletalMeshComponent* Component, void* Asset);

	void UpdateAnimation();
	void TickAssetPlayerInstances(float DeltaSeconds);

	const FBoneContainer& GetRequiredBones() const
	{
		return RequiredBones;
	}

	void EvaluateAnimation(FPoseContext& Output);
	void EvaluateAnimationNode(FPoseContext& Output);

	FAnimTickRecord& CreateUninitializedTickRecord(int32 GroupIndex, FAnimGroupInstance*& OutSyncGroupPtr);
	void MakeSequenceTickRecord(FAnimTickRecord& TickRecord, UAnimSequenceBase* Sequence, bool bLooping, float PlayRate, float FinalBlendWeight, float& CurrentTime, FMarkerTickRecord& MarkerTickRecord) const;
private:
	FTransform ComponentTransform;
	FTransform ComponentRelativeTransform;
	FTransform ActorTransform;
	FTransform SkelMeshCompLocalToWorld;
	FTransform SkelMeshCompOwnerTransform;

	mutable UAnimInstance* AnimInstanceObject;
	float CurrentDeltaSeconds;
	float CurrentTimeDilation;
	FBoneContainer RequiredBones;
	FAnimNode_Base* RootNode;

	USkeleton* Skeleton;
	USkeletalMeshComponent* SkeletalMeshComponent;

	std::vector<FAnimTickRecord> UngroupedActivePlayerArrays[2];
	std::vector<FAnimGroupInstance> SyncGroupArrays[2];
	int32 SyncGroupWriteIndex;

	/** Animation Notifies that has been triggered in the latest tick **/
	FAnimNotifyQueue NotifyQueue;

	// Root motion mode duplicated from the anim instance
	ERootMotionMode::Type RootMotionMode;


	friend class UAnimInstance;
	friend class UAnimSingleNodeInstance;
	friend class USkeletalMeshComponent;
	friend struct FAnimationBaseContext;
protected:
	virtual void Initialize(UAnimInstance* InAnimInstance);
	virtual void Uninitialize(UAnimInstance* InAnimInstance);
	virtual void CacheBones();
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds);
	virtual void Update(float DeltaSeconds) {}
	virtual void UpdateAnimationNode(float DeltaSeconds);
	virtual bool Evaluate(FPoseContext& Output) { return false; }
	virtual void PostUpdate(UAnimInstance* InAnimInstance) const;
	virtual void InitializeObjects(UAnimInstance* InAnimInstance);
	virtual void ClearObjects();

	int32 GetSyncGroupReadIndex() const
	{
		return 1 - SyncGroupWriteIndex;
	}

	// Gets the sync group we should be writing to
	int32 GetSyncGroupWriteIndex() const
	{
		return SyncGroupWriteIndex;
	}
};