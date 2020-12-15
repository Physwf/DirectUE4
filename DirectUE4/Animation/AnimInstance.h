#pragma once

#include "AnimTypes.h"
#include "Skeleton.h"
#include "AnimationAsset.h"
#include "AnimCurveTypes.h"
#include "BonePose.h"

struct FAnimInstanceProxy;
class AActor;
class USkeletalMeshComponent;

class UAnimInstance
{
public:
	UAnimInstance(class USkeletalMeshComponent* InOuter) : Outer(InOuter) {}
	~UAnimInstance() {}

	USkeleton* CurrentSkeleton;

	void InitializeAnimation();
	void UpdateAnimation(float DeltaSeconds, bool bNeedsValidRootMotion);
	void PostUpdateAnimation();

	void ParallelUpdateAnimation();

	virtual void NativeUpdateAnimation(float DeltaSeconds);

	inline USkeletalMeshComponent* GetSkelMeshComponent() const { return Outer; }

	virtual FAnimInstanceProxy* CreateAnimInstanceProxy();

	bool ParallelCanEvaluate(const USkeletalMesh* InSkeletalMesh) const;
	void ParallelEvaluateAnimation(bool bForceRefPose, const USkeletalMesh* InSkeletalMesh, std::vector<FTransform>& OutBoneSpaceTransforms, FBlendedHeapCurve& OutCurve, FCompactPose& OutPose);

	void PreEvaluateAnimation();

	void UninitializeAnimation();
	virtual void NativeInitializeAnimation();
	virtual void NativeUninitializeAnimation();
	void RecalcRequiredBones();

	AActor* GetOwningActor() const;
	USkeletalMeshComponent* GetOwningComponent() const;
protected:

	virtual void PreUpdateAnimation(float DeltaSeconds);

	/** Proxy object, nothing should access this from an externally-callable API as it is used as a scratch area on worker threads */
	mutable FAnimInstanceProxy* AnimInstanceProxy;

	template <typename T /*= FAnimInstanceProxy*/>	// @TODO: Cant default parameters to this function on Xbox One until we move off the VS2012 compiler
	inline T& GetProxyOnGameThread()
	{
		if (AnimInstanceProxy == nullptr)
		{
			AnimInstanceProxy = CreateAnimInstanceProxy();
		}
		return *static_cast<T*>(AnimInstanceProxy);
	}

	/** Access the proxy but block if a task is currently in progress as it wouldn't be safe to access it */
	template <typename T/* = FAnimInstanceProxy*/>	// @TODO: Cant default parameters to this function on Xbox One until we move off the VS2012 compiler
	inline const T& GetProxyOnGameThread() const
	{
		if (AnimInstanceProxy == nullptr)
		{
			AnimInstanceProxy = const_cast<UAnimInstance*>(this)->CreateAnimInstanceProxy();
		}
		return *static_cast<const T*>(AnimInstanceProxy);
	}

	/** Access the proxy but block if a task is currently in progress (and we are on the game thread) as it wouldn't be safe to access it */
	template <typename T/* = FAnimInstanceProxy*/>	// @TODO: Cant default parameters to this function on Xbox One until we move off the VS2012 compiler
	inline T& GetProxyOnAnyThread()
	{
		if (AnimInstanceProxy == nullptr)
		{
			AnimInstanceProxy = CreateAnimInstanceProxy();
		}
		return *static_cast<T*>(AnimInstanceProxy);
	}

	/** Access the proxy but block if a task is currently in progress (and we are on the game thread) as it wouldn't be safe to access it */
	template <typename T/* = FAnimInstanceProxy*/>	// @TODO: Cant default parameters to this function on Xbox One until we move off the VS2012 compiler
	inline const T& GetProxyOnAnyThread() const
	{
		if (AnimInstanceProxy == nullptr)
		{
			AnimInstanceProxy = const_cast<UAnimInstance*>(this)->CreateAnimInstanceProxy();
		}
		return *static_cast<const T*>(AnimInstanceProxy);
	}
private:
	class USkeletalMeshComponent* Outer;
};
