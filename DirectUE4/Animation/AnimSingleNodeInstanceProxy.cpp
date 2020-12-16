#include "AnimSingleNodeInstanceProxy.h"
#include "AnimationRuntime.h"
#include "AnimSingleNodeInstance.h"

void FAnimNode_SingleNode::Evaluate_AnyThread(FPoseContext& Output)
{
	const bool bCanProcessAdditiveAnimationsLocal = false;

	if (Proxy->CurrentAsset != NULL /*&& !Proxy->CurrentAsset->HasAnyFlags(RF_BeginDestroyed)*/)
	{
// 		if (UBlendSpaceBase* BlendSpace = Cast<UBlendSpaceBase>(Proxy->CurrentAsset))
// 		{
// 			Proxy->InternalBlendSpaceEvaluatePose(BlendSpace, Proxy->BlendSampleData, Output);
// 		}
		/*else */if (UAnimSequence* Sequence = (UAnimSequence*)(Proxy->CurrentAsset))
		{
			if (Sequence->IsValidAdditive())
			{
				FAnimExtractContext ExtractionContext(Proxy->CurrentTime, Sequence->bEnableRootMotion);

				if (bCanProcessAdditiveAnimationsLocal)
				{
					Sequence->GetAdditiveBasePose(Output.Pose, Output.Curve, ExtractionContext);
				}
				else
				{
					Output.ResetToRefPose();
				}

				FCompactPose AdditivePose;
				FBlendedCurve AdditiveCurve;
				AdditivePose.SetBoneContainer(&Output.Pose.GetBoneContainer());
				AdditiveCurve.InitFrom(Output.Curve);
				Sequence->GetAnimationPose(AdditivePose, AdditiveCurve, ExtractionContext);

				FAnimationRuntime::AccumulateAdditivePose(Output.Pose, AdditivePose, Output.Curve, AdditiveCurve, 1.f, Sequence->AdditiveAnimType);
				Output.Pose.NormalizeRotations();
			}
			else
			{
				// if SkeletalMesh isn't there, we'll need to use skeleton
				Sequence->GetAnimationPose(Output.Pose, Output.Curve, FAnimExtractContext(Proxy->CurrentTime, Sequence->bEnableRootMotion));
			}
		}
// 		else if (UAnimComposite* Composite = Cast<UAnimComposite>(Proxy->CurrentAsset))
// 		{
// 
// 		}
// 		else if (UAnimMontage* Montage = Cast<UAnimMontage>(Proxy->CurrentAsset))
// 		{
// 
// 		}
		else
		{
			// pose asset is handled by preview instance : pose blend node
			// and you can't drag pose asset to level to create single node instance. 
			Output.ResetToRefPose();
		}
	}
} 

void FAnimNode_SingleNode::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	float NewPlayRate = Proxy->PlayRate;
	UAnimSequence* PreviewBasePose = NULL;

	if (Proxy->bPlaying == false)
	{
		// we still have to tick animation when bPlaying is false because 
		NewPlayRate = 0.f;
	}
	if (Proxy->CurrentAsset != NULL)
	{
		FAnimGroupInstance* SyncGroup;
		if (UAnimSequence* Sequence = (UAnimSequence*)(Proxy->CurrentAsset))
		{
			FAnimTickRecord& TickRecord = Proxy->CreateUninitializedTickRecord(INDEX_NONE, /*out*/ SyncGroup);
			Proxy->MakeSequenceTickRecord(TickRecord, Sequence, Proxy->bLooping, NewPlayRate, 1.f, /*inout*/ Proxy->CurrentTime, Proxy->MarkerTickRecord);
			// if it's not looping, just set play to be false when reached to end
			if (!Proxy->bLooping)
			{
				const float CombinedPlayRate = NewPlayRate * Sequence->RateScale;
				if ((CombinedPlayRate < 0.f && Proxy->CurrentTime <= 0.f) || (CombinedPlayRate > 0.f && Proxy->CurrentTime >= Sequence->SequenceLength))
				{
					Proxy->SetPlaying(false);
				}
			}
		}
	}
}

void FAnimSingleNodeInstanceProxy::SetAnimationAsset(UAnimationAsset* NewAsset, USkeletalMeshComponent* MeshComponent, bool bIsLooping, float InPlayRate)
{
	bLooping = bIsLooping;
	PlayRate = InPlayRate;
	CurrentTime = 0.f;
}

void FAnimSingleNodeInstanceProxy::Initialize(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::Initialize(InAnimInstance);

	CurrentAsset = NULL;
	CurrentTime = 0.f;

	FAnimationInitializeContext InitContext(this);
	SingleNode.Initialize_AnyThread(InitContext);
}

bool FAnimSingleNodeInstanceProxy::Evaluate(FPoseContext& Output)
{
	SingleNode.Evaluate_AnyThread(Output);

	return true;
}

void FAnimSingleNodeInstanceProxy::UpdateAnimationNode(float DeltaSeconds)
{
	FAnimationUpdateContext UpdateContext(this, DeltaSeconds);
	SingleNode.Update_AnyThread(UpdateContext);
}

void FAnimSingleNodeInstanceProxy::PostUpdate(UAnimInstance* InAnimInstance) const
{
	FAnimInstanceProxy::PostUpdate(InAnimInstance);
}

void FAnimSingleNodeInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
}

void FAnimSingleNodeInstanceProxy::InitializeObjects(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::InitializeObjects(InAnimInstance);

	UAnimSingleNodeInstance* AnimSingleNodeInstance = (UAnimSingleNodeInstance*)(InAnimInstance);
	CurrentAsset = AnimSingleNodeInstance->CurrentAsset;
}

void FAnimSingleNodeInstanceProxy::ClearObjects()
{

}

