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
};