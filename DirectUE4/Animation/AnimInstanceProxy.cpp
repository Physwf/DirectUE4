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

	std::vector<FAnimTickRecord>& UngroupedActivePlayers = UngroupedActivePlayerArrays[GetSyncGroupWriteIndex()];
	UngroupedActivePlayers.clear();

	std::vector<FAnimGroupInstance>& SyncGroups = SyncGroupArrays[GetSyncGroupWriteIndex()];
	for (uint32 GroupIndex = 0; GroupIndex < SyncGroups.size(); ++GroupIndex)
	{
		SyncGroups[GroupIndex].Reset();
	}

	ComponentTransform = SkeletalMeshComponent->GetComponentTransform();
	ComponentRelativeTransform = SkeletalMeshComponent->GetRelativeTransform();
	ActorTransform = SkeletalMeshComponent->GetOwner() ? SkeletalMeshComponent->GetOwner()->GetActorTransform() : FTransform::Identity;
}

void FAnimInstanceProxy::RecalcRequiredBones(USkeletalMeshComponent* Component, void* Asset)
{
	RequiredBones.InitializeTo(Component->RequiredBones, FCurveEvaluationOption(/*Component->GetAllowedAnimCurveEvaluate()*/true, /*&Component->GetDisallowedAnimCurvesEvaluation()*/nullptr, Component->PredictedLODLevel), Asset);
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
	std::vector<FAnimGroupInstance>& SyncGroups = SyncGroupArrays[GetSyncGroupWriteIndex()];
	const std::vector<FAnimGroupInstance>& PreviousSyncGroups = SyncGroupArrays[GetSyncGroupReadIndex()];
	std::vector<FAnimTickRecord>& UngroupedActivePlayers = UngroupedActivePlayerArrays[GetSyncGroupWriteIndex()];


	for (uint32 TickIndex = 0; TickIndex < UngroupedActivePlayers.size(); ++TickIndex)
	{
		FAnimTickRecord& AssetPlayerToTick = UngroupedActivePlayers[TickIndex];
		const std::vector<std::string>* UniqueNames = AssetPlayerToTick.SourceAsset->GetUniqueMarkerNames();
		const std::vector<std::string>& ValidMarkers = UniqueNames ? *UniqueNames : FMarkerTickContext::DefaultMarkerNames;

		const bool bOnlyOneAnimationInGroup = true;
		FAnimAssetTickContext TickContext(DeltaSeconds, RootMotionMode, bOnlyOneAnimationInGroup, ValidMarkers);
		{
			AssetPlayerToTick.SourceAsset->TickAssetPlayer(AssetPlayerToTick, NotifyQueue, TickContext);
		}
// 		if (RootMotionMode == ERootMotionMode::RootMotionFromEverything && TickContext.RootMotionMovementParams.bHasRootMotion)
// 		{
// 			ExtractedRootMotion.AccumulateWithBlend(TickContext.RootMotionMovementParams, AssetPlayerToTick.GetRootMotionWeight());
// 		}
	}
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

FAnimTickRecord& FAnimInstanceProxy::CreateUninitializedTickRecord(int32 GroupIndex, FAnimGroupInstance*& OutSyncGroupPtr)
{
	// Find or create the sync group if there is one
	OutSyncGroupPtr = NULL;
	if (GroupIndex >= 0)
	{
		std::vector<FAnimGroupInstance>& SyncGroups = SyncGroupArrays[GetSyncGroupWriteIndex()];
		while ((int32)SyncGroups.size() <= GroupIndex)
		{
			SyncGroups.push_back(FAnimGroupInstance());
		}
		OutSyncGroupPtr = &(SyncGroups[GroupIndex]);
	}

	// Create the record
	std::vector<FAnimTickRecord>& Players = (OutSyncGroupPtr != NULL) ? OutSyncGroupPtr->ActivePlayers : UngroupedActivePlayerArrays[GetSyncGroupWriteIndex()];
	Players.push_back(FAnimTickRecord());
	FAnimTickRecord* TickRecord = &Players.back();
	return *TickRecord;
}

void FAnimInstanceProxy::MakeSequenceTickRecord(FAnimTickRecord& TickRecord, UAnimSequenceBase* Sequence, bool bLooping, float PlayRate, float FinalBlendWeight, float& CurrentTime, FMarkerTickRecord& MarkerTickRecord) const
{
	TickRecord.SourceAsset = Sequence;
	TickRecord.TimeAccumulator = &CurrentTime;
	TickRecord.MarkerTickRecord = &MarkerTickRecord;
	TickRecord.PlayRateMultiplier = PlayRate;
	TickRecord.EffectiveBlendWeight = FinalBlendWeight;
	TickRecord.bLooping = bLooping;
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
