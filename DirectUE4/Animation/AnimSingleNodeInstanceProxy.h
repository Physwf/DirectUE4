#pragma once

#include "AnimInstanceProxy.h"
#include "AnimTypes.h"
#include "AnimationAsset.h"
#include "AnimNodeBase.h"

struct FAnimNode_SingleNode : public FAnimNode_Base
{
	friend struct FAnimSingleNodeInstanceProxy;
	//FPoseLink SourcePose;

	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
private:
	FAnimSingleNodeInstanceProxy * Proxy;
};

struct FAnimSingleNodeInstanceProxy : public FAnimInstanceProxy
{
	friend struct FAnimNode_SingleNode;

	FAnimSingleNodeInstanceProxy(UAnimInstance* InAnimInstance)
		: FAnimInstanceProxy(InAnimInstance)
		, CurrentAsset(nullptr)
		//, BlendSpaceInput(0.0f, 0.0f, 0.0f)
		, CurrentTime(0.0f)
		, PlayRate(1.f)
		, bLooping(true)
		, bPlaying(true)
		, bReverse(false)
	{
		SingleNode.Proxy = this;
	}

	virtual void SetAnimationAsset(UAnimationAsset* NewAsset, USkeletalMeshComponent* MeshComponent, bool bIsLooping, float InPlayRate);

	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual bool Evaluate(FPoseContext& Output) override;
	virtual void UpdateAnimationNode(float DeltaSeconds) override;
	virtual void PostUpdate(UAnimInstance* InAnimInstance) const override;
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual void InitializeObjects(UAnimInstance* InAnimInstance) override;
	virtual void ClearObjects() override;

	void SetPlaying(bool bIsPlaying)
	{
		bPlaying = bIsPlaying;
	}
	void SetLooping(bool bIsLooping)
	{
		bLooping = bIsLooping;
	}
private:
	UAnimationAsset * CurrentAsset;
	FAnimNode_SingleNode SingleNode;

	float CurrentTime;

	FMarkerTickRecord MarkerTickRecord;

	float PlayRate;
	bool bLooping;
	bool bPlaying;
	bool bReverse;
};