#include "AnimInstance.h"
#include "MeshComponent.h"
#include "AnimInstanceProxy.h"
#include "SkeletalMesh.h"
#include "AnimNodeBase.h"

void UAnimInstance::InitializeAnimation()
{
	UninitializeAnimation();

	USkeletalMeshComponent* OwnerComponent = GetSkelMeshComponent();
	if (OwnerComponent->SkeletalMesh != NULL)
	{
		CurrentSkeleton = OwnerComponent->SkeletalMesh->Skeleton;
	}
	else
	{
		CurrentSkeleton = NULL;
	}

	RecalcRequiredBones();

	GetProxyOnGameThread<FAnimInstanceProxy>().Initialize(this);

	NativeInitializeAnimation();

	//GetProxyOnGameThread<FAnimInstanceProxy>().InitializeRootNode();
}

void UAnimInstance::UpdateAnimation(float DeltaSeconds, bool bNeedsValidRootMotion)
{
	FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();

	PreUpdateAnimation(DeltaSeconds);

	{
		NativeUpdateAnimation(DeltaSeconds);
	}

	//if (bNeedsValidRootMotion || NeedsImmediateUpdate(DeltaSeconds))
	{
		// cant use parallel update, so just do the work here
		Proxy.UpdateAnimation();
		PostUpdateAnimation();
	}
}

void UAnimInstance::PostUpdateAnimation()
{

}

void UAnimInstance::ParallelUpdateAnimation()
{
	GetProxyOnAnyThread<FAnimInstanceProxy>().UpdateAnimation();
}

void UAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
}

FAnimInstanceProxy* UAnimInstance::CreateAnimInstanceProxy()
{
	return new FAnimInstanceProxy(this);
}

bool UAnimInstance::ParallelCanEvaluate(const USkeletalMesh* InSkeletalMesh) const
{
	const FAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FAnimInstanceProxy>();
	return Proxy.GetRequiredBones().IsValid() && (Proxy.GetRequiredBones().GetAsset() == InSkeletalMesh);
}

void UAnimInstance::ParallelEvaluateAnimation(bool bForceRefPose, const USkeletalMesh* InSkeletalMesh, std::vector<FTransform>& OutBoneSpaceTransforms, FBlendedHeapCurve& OutCurve, FCompactPose& OutPose)
{
	FAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FAnimInstanceProxy>();
	OutPose.SetBoneContainer(&Proxy.GetRequiredBones());
	OutPose.ResetToRefPose();

	if (!bForceRefPose)
	{
		// Create an evaluation context
		FPoseContext EvaluationContext(&Proxy);
		EvaluationContext.ResetToRefPose();

		// Run the anim blueprint
		Proxy.EvaluateAnimation(EvaluationContext);
		// Move the curves
		OutCurve.CopyFrom(EvaluationContext.Curve);
		OutPose.CopyBonesFrom(EvaluationContext.Pose);
	}
	else
	{
		OutPose.ResetToRefPose();
	}
}

void UAnimInstance::PreEvaluateAnimation()
{

}

void UAnimInstance::UninitializeAnimation()
{
	NativeUninitializeAnimation();

	GetProxyOnGameThread<FAnimInstanceProxy>().Uninitialize(this);
}

void UAnimInstance::NativeInitializeAnimation()
{

}

void UAnimInstance::NativeUninitializeAnimation()
{

}

void UAnimInstance::RecalcRequiredBones()
{
	USkeletalMeshComponent* SkelMeshComp = GetSkelMeshComponent();
	assert(SkelMeshComp);

	if (SkelMeshComp->SkeletalMesh && SkelMeshComp->SkeletalMesh->Skeleton)
	{
		GetProxyOnGameThread<FAnimInstanceProxy>().RecalcRequiredBones(SkelMeshComp, SkelMeshComp->SkeletalMesh);
	}
	else if (CurrentSkeleton != NULL)
	{
		GetProxyOnGameThread<FAnimInstanceProxy>().RecalcRequiredBones(SkelMeshComp, CurrentSkeleton);
	}
}

AActor* UAnimInstance::GetOwningActor() const
{
	USkeletalMeshComponent* OwnerComponent = GetSkelMeshComponent();
	return OwnerComponent->GetOwner();
}

USkeletalMeshComponent* UAnimInstance::GetOwningComponent() const
{
	return GetSkelMeshComponent();
}

void UAnimInstance::PreUpdateAnimation(float DeltaSeconds)
{
	GetProxyOnGameThread<FAnimInstanceProxy>().PreUpdate(this, DeltaSeconds);
}

