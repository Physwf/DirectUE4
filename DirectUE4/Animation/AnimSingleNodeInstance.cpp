#include "AnimSingleNodeInstance.h"
#include "AnimSingleNodeInstanceProxy.h"
#include "MeshComponent.h"
#include "SkeletalMesh.h"

void UAnimSingleNodeInstance::SetAnimationAsset(UAnimationAsset* NewAsset, bool bInIsLooping /*= true*/, float InPlayRate /*= 1.f*/)
{
	if (NewAsset != CurrentAsset)
	{
		CurrentAsset = NewAsset;
	}

	FAnimSingleNodeInstanceProxy& Proxy = GetProxyOnGameThread<FAnimSingleNodeInstanceProxy>();

	USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	if (MeshComponent)
	{
		if (MeshComponent->SkeletalMesh == nullptr)
		{
			// if it does not have SkeletalMesh, we nullify it
			CurrentAsset = nullptr;
		}
		else if (CurrentAsset != nullptr)
		{
			// if we have an asset, make sure their skeleton matches, otherwise, null it
			if (MeshComponent->SkeletalMesh->Skeleton != CurrentAsset->GetSkeleton())
			{
				// clear asset since we do not have matching skeleton
				CurrentAsset = nullptr;
			}
		}

		// We've changed the animation asset, and the next frame could be wildly different from the frame we're
		// on now. In this case of a single node instance, we reset the clothing on the next update.
		//MeshComponent->ClothTeleportMode = EClothingTeleportMode::TeleportAndReset;
	}

	Proxy.SetAnimationAsset(NewAsset, GetSkelMeshComponent(), bInIsLooping, InPlayRate);
}

void UAnimSingleNodeInstance::SetPlaying(bool bIsPlaying)
{
	FAnimSingleNodeInstanceProxy& Proxy = GetProxyOnGameThread<FAnimSingleNodeInstanceProxy>();
	Proxy.SetPlaying(bIsPlaying);
}

void UAnimSingleNodeInstance::SetLooping(bool bIsLooping)
{
	FAnimSingleNodeInstanceProxy& Proxy = GetProxyOnGameThread<FAnimSingleNodeInstanceProxy>();
	Proxy.SetLooping(bIsLooping);
}

FAnimInstanceProxy* UAnimSingleNodeInstance::CreateAnimInstanceProxy()
{
	return new FAnimSingleNodeInstanceProxy(this);
}

