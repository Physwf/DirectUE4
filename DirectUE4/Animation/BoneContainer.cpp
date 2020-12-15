#include "BoneContainer.h"

FBoneContainer::FBoneContainer()
	: Asset(nullptr)
	, AssetSkeletalMesh(nullptr)
	, AssetSkeleton(nullptr)
	, RefSkeleton(nullptr)
	, bDisableRetargeting(false)
	, bUseRAWData(false)
	, bUseSourceData(false)
{
	BoneIndicesArray.clear();
	BoneSwitchArray.clear();
	SkeletonToPoseBoneIndexArray.clear();
	PoseToSkeletonBoneIndexArray.clear();
}

FBoneContainer::FBoneContainer(const std::vector<FBoneIndexType>& InRequiredBoneIndexArray, const FCurveEvaluationOption& CurveEvalOption, void* InAsset)
	: BoneIndicesArray(InRequiredBoneIndexArray)
	, Asset(InAsset)
	, AssetSkeletalMesh(nullptr)
	, AssetSkeleton(nullptr)
	, RefSkeleton(nullptr)
	, bDisableRetargeting(false)
	, bUseRAWData(false)
	, bUseSourceData(false)
{
	Initialize(CurveEvalOption);
}

void FBoneContainer::InitializeTo(const std::vector<FBoneIndexType>& InRequiredBoneIndexArray, const FCurveEvaluationOption& CurveEvalOption, void* InAsset)
{
	BoneIndicesArray = InRequiredBoneIndexArray;
	Asset = InAsset;

	Initialize(CurveEvalOption);
}

void FBoneContainer::Initialize(const FCurveEvaluationOption& CurveEvalOption)
{

}

