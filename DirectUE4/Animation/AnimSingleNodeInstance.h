#pragma once

#include "AnimInstance.h"

class  UAnimSingleNodeInstance : public UAnimInstance
{
public:
	UAnimSingleNodeInstance(class USkeletalMeshComponent* InOuter) : UAnimInstance(InOuter) {}
	~UAnimSingleNodeInstance() {}

	virtual void SetAnimationAsset(UAnimationAsset* NewAsset, bool bIsLooping = true, float InPlayRate = 1.f);
	void SetPlaying(bool bIsPlaying);
	void SetLooping(bool bIsLooping);

	class UAnimationAsset* CurrentAsset;
protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
};

