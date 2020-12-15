#include "AnimInstanceProxy.h"
#include "World.h"
#include "MeshComponent.h"
#include "Actor.h"
#include "AnimNodeBase.h"
#include "SkeletalMesh.h"

void FAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	USkeletalMeshComponent* SkelMeshComp = InAnimInstance->GetSkelMeshComponent();
	UWorld* World = SkelMeshComp ? SkelMeshComp->GetWorld() : nullptr;

	CurrentDeltaSeconds = DeltaSeconds;
	CurrentTimeDilation = 1.0f;// World ? World->GetWorldSettings()->GetEffectiveTimeDilation() : 1.0f;
	//RootMotionMode = InAnimInstance->RootMotionMode;
	//bShouldExtractRootMotion = InAnimInstance->ShouldExtractRootMotion();

	InitializeObjects(InAnimInstance);

	if (SkelMeshComp)
	{
		SkelMeshCompLocalToWorld = SkelMeshComp->GetComponentTransform();
		if (const AActor* Owner = SkelMeshComp->GetOwner())
		{
			SkelMeshCompOwnerTransform = Owner->GetTransform();
		}
	}

	ComponentTransform = SkeletalMeshComponent->GetComponentTransform();
	ComponentRelativeTransform = SkeletalMeshComponent->GetRelativeTransform();
	ActorTransform = SkeletalMeshComponent->GetOwner() ? SkeletalMeshComponent->GetOwner()->GetActorTransform() : FTransform::Identity;
}

void FAnimInstanceProxy::RecalcRequiredBones(USkeletalMeshComponent* Component, void* Asset)
{
	//RequiredBones.InitializeTo(Component->RequiredBones, FCurveEvaluationOption(Component->GetAllowedAnimCurveEvaluate(), &Component->GetDisallowedAnimCurvesEvaluation(), Component->PredictedLODLevel), *Asset);

}

void FAnimInstanceProxy::UpdateAnimation()
{
	CacheBones();

	// update native update
	{
		Update(CurrentDeltaSeconds);
	}

	UpdateAnimationNode(CurrentDeltaSeconds);
	TickAssetPlayerInstances(CurrentDeltaSeconds);
}

void FAnimInstanceProxy::TickAssetPlayerInstances(float DeltaSeconds)
{

}

void FAnimInstanceProxy::EvaluateAnimation(FPoseContext& Output)
{
	CacheBones();

	// Evaluate native code if implemented, otherwise evaluate the node graph
	if (!Evaluate(Output))
	{
		EvaluateAnimationNode(Output);
	}
}

void FAnimInstanceProxy::EvaluateAnimationNode(FPoseContext& Output)
{
	if (RootNode != NULL)
	{
		//EvaluationCounter.Increment();
		RootNode->Evaluate_AnyThread(Output);
	}
	else
	{
		Output.ResetToRefPose();
	}
}

void FAnimInstanceProxy::Initialize(UAnimInstance* InAnimInstance)
{
	AnimInstanceObject = InAnimInstance;

	InitializeObjects(InAnimInstance);

	RootNode = nullptr;

	if (const USkeletalMeshComponent* SkelMeshComp = InAnimInstance->GetOwningComponent())
	{
		ComponentTransform = SkelMeshComp->GetComponentTransform();
		ComponentRelativeTransform = SkeletalMeshComponent->GetRelativeTransform();

		const AActor* OwningActor = SkeletalMeshComponent->GetOwner();
		ActorTransform = OwningActor ? OwningActor->GetActorTransform() : FTransform::Identity;
	}
	else
	{
		ComponentTransform = FTransform::Identity;
		ComponentRelativeTransform = FTransform::Identity;
		ActorTransform = FTransform::Identity;
	}
}

void FAnimInstanceProxy::Uninitialize(UAnimInstance* InAnimInstance)
{
	//MontageEvaluationData.Reset();
	//SubInstanceInputNode = nullptr;
}

void FAnimInstanceProxy::CacheBones()
{

}

void FAnimInstanceProxy::UpdateAnimationNode(float DeltaSeconds)
{

}

void FAnimInstanceProxy::PostUpdate(UAnimInstance* InAnimInstance) const
{

}

void FAnimInstanceProxy::InitializeObjects(UAnimInstance* InAnimInstance)
{
	SkeletalMeshComponent = InAnimInstance->GetSkelMeshComponent();
	if (SkeletalMeshComponent->SkeletalMesh != nullptr)
	{
		Skeleton = SkeletalMeshComponent->SkeletalMesh->Skeleton;
	}
	else
	{
		Skeleton = nullptr;
	}
}

void FAnimInstanceProxy::ClearObjects()
{
	SkeletalMeshComponent = nullptr;
	Skeleton = nullptr;
}
