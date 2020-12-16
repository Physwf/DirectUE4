#include "AnimationAsset.h"

void UAnimationAsset::SetSkeleton(USkeleton* NewSkeleton)
{
	if (NewSkeleton && NewSkeleton != Skeleton)
	{
		Skeleton = NewSkeleton;
	}
}

const std::vector<std::string> FMarkerTickContext::DefaultMarkerNames;
