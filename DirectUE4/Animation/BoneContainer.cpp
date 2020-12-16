#include "BoneContainer.h"
#include "SkeletalMesh.h"
#include "Skeleton.h"

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

int32 FBoneContainer::GetParentBoneIndex(const int32 BoneIndex) const
{
	assert(IsValid());
	assert(BoneIndex != INDEX_NONE);
	return RefSkeleton->GetParentIndex(BoneIndex);
}

FCompactPoseBoneIndex FBoneContainer::GetParentBoneIndex(const FCompactPoseBoneIndex& BoneIndex) const
{
	assert(IsValid());
	assert(BoneIndex != INDEX_NONE);
	return CompactPoseParentBones[BoneIndex.GetInt()];
}

void FBoneContainer::CacheRequiredAnimCurveUids(const FCurveEvaluationOption& CurveEvalOption)
{

}
struct FBoneContainerScratchArea 
{
	static FBoneContainerScratchArea& Get()
	{
		static FBoneContainerScratchArea Intance;
		return Intance;
	}
	std::vector<int32> MeshIndexToCompactPoseIndex;
};

void FBoneContainer::Initialize(const FCurveEvaluationOption& CurveEvalOption)
{
	RefSkeleton = nullptr;
	void* AssetObj = Asset;
	USkeletalMesh* AssetSkeletalMeshObj = (USkeletalMesh*)(AssetObj);
	USkeleton* AssetSkeletonObj = nullptr;

	if (AssetSkeletalMeshObj)
	{
		RefSkeleton = &AssetSkeletalMeshObj->RefSkeleton;
		AssetSkeletonObj = AssetSkeletalMeshObj->Skeleton;
	}
	else
	{
		AssetSkeletonObj = (USkeleton*)(AssetObj);
		if (AssetSkeletonObj)
		{
			RefSkeleton = &AssetSkeletonObj->GetReferenceSkeleton();
		}
	}

	assert(AssetSkeletalMeshObj || AssetSkeletonObj);
	assert(RefSkeleton);

	AssetSkeleton = AssetSkeletonObj;
	AssetSkeletalMesh = AssetSkeletalMeshObj;

	const int32 MaxBones = AssetSkeletonObj ? FMath::Max<int32>(RefSkeleton->GetNum(), AssetSkeletonObj->GetReferenceSkeleton().GetNum()) : RefSkeleton->GetNum();

	BoneSwitchArray.resize(MaxBones,false);
	const int32 NumRequiredBones = BoneIndicesArray.size();
	for (int32 Index = 0; Index < NumRequiredBones; Index++)
	{
		const FBoneIndexType BoneIndex = BoneIndicesArray[Index];
		assert(BoneIndex < MaxBones);
		BoneSwitchArray[BoneIndex] = true;
	}

	// Clear remapping table
	SkeletonToPoseBoneIndexArray.clear();
		
	if (AssetSkeletalMeshObj)
	{
		RemapFromSkelMesh(*AssetSkeletalMeshObj, *AssetSkeletonObj);
	}
	// But we also support a Skeleton's RefPose.
	else
	{
		// Right now we only support a single Skeleton. Skeleton hierarchy coming soon!
		RemapFromSkeleton(*AssetSkeletonObj);
	};

	//Set up compact pose data
	int32 NumReqBones = BoneIndicesArray.size();
	CompactPoseParentBones.clear();
	CompactPoseParentBones.reserve(NumReqBones);

	CompactPoseRefPoseBones.clear();
	CompactPoseRefPoseBones.reserve(NumReqBones);
	CompactPoseRefPoseBones.resize(NumReqBones);

	CompactPoseToSkeletonIndex.clear();
	CompactPoseToSkeletonIndex.reserve(NumReqBones);
	CompactPoseToSkeletonIndex.resize(NumReqBones);

	SkeletonToCompactPose.clear();
	SkeletonToCompactPose.reserve(SkeletonToPoseBoneIndexArray.size());

	VirtualBoneCompactPoseData.clear();
	VirtualBoneCompactPoseData.reserve(RefSkeleton->GetVirtualBoneRefData().size());

	const std::vector<FTransform>& RefPoseArray = RefSkeleton->GetRefBonePose();
	std::vector<int32>& MeshIndexToCompactPoseIndex = FBoneContainerScratchArea::Get().MeshIndexToCompactPoseIndex;
	MeshIndexToCompactPoseIndex.clear();
	MeshIndexToCompactPoseIndex.reserve(PoseToSkeletonBoneIndexArray.size());
	MeshIndexToCompactPoseIndex.resize(PoseToSkeletonBoneIndexArray.size());

	for (int32& Item : MeshIndexToCompactPoseIndex)
	{
		Item = -1;
	}

	for (int32 CompactBoneIndex = 0; CompactBoneIndex < NumReqBones; ++CompactBoneIndex)
	{
		FBoneIndexType MeshPoseIndex = BoneIndicesArray[CompactBoneIndex];
		MeshIndexToCompactPoseIndex[MeshPoseIndex] = CompactBoneIndex;

		//Parent Bone
		const int32 ParentIndex = GetParentBoneIndex(MeshPoseIndex);
		const int32 CompactParentIndex = ParentIndex == INDEX_NONE ? INDEX_NONE : MeshIndexToCompactPoseIndex[ParentIndex];

		CompactPoseParentBones.push_back(FCompactPoseBoneIndex(CompactParentIndex));
	}

	//Ref Pose
	for (int32 CompactBoneIndex = 0; CompactBoneIndex < NumReqBones; ++CompactBoneIndex)
	{
		FBoneIndexType MeshPoseIndex = BoneIndicesArray[CompactBoneIndex];
		CompactPoseRefPoseBones[CompactBoneIndex] = RefPoseArray[MeshPoseIndex];
	}

	for (int32 CompactBoneIndex = 0; CompactBoneIndex < NumReqBones; ++CompactBoneIndex)
	{
		FBoneIndexType MeshPoseIndex = BoneIndicesArray[CompactBoneIndex];
		CompactPoseToSkeletonIndex[CompactBoneIndex] = PoseToSkeletonBoneIndexArray[MeshPoseIndex];
	}


	for (uint32 SkeletonBoneIndex = 0; SkeletonBoneIndex < SkeletonToPoseBoneIndexArray.size(); ++SkeletonBoneIndex)
	{
		int32 PoseBoneIndex = SkeletonToPoseBoneIndexArray[SkeletonBoneIndex];
		int32 CompactIndex = (PoseBoneIndex != INDEX_NONE) ? MeshIndexToCompactPoseIndex[PoseBoneIndex] : INDEX_NONE;
		SkeletonToCompactPose.push_back(FCompactPoseBoneIndex(CompactIndex));
	}

	for (const FVirtualBoneRefData& VBRefBone : RefSkeleton->GetVirtualBoneRefData())
	{
		const int32 VBInd = MeshIndexToCompactPoseIndex[VBRefBone.VBRefSkelIndex];
		const int32 SourceInd = MeshIndexToCompactPoseIndex[VBRefBone.SourceRefSkelIndex];
		const int32 TargetInd = MeshIndexToCompactPoseIndex[VBRefBone.TargetRefSkelIndex];

		if ((VBInd != INDEX_NONE) && (SourceInd != INDEX_NONE) && (TargetInd != INDEX_NONE))
		{
			VirtualBoneCompactPoseData.push_back(FVirtualBoneCompactPoseData(FCompactPoseBoneIndex(VBInd), FCompactPoseBoneIndex(SourceInd), FCompactPoseBoneIndex(TargetInd)));
		}
	}
	// cache required curve UID list according to new bone sets
	CacheRequiredAnimCurveUids(CurveEvalOption);

	// Reset retargeting cached data look up table.
	RetargetSourceCachedDataLUT.clear();
}

void FBoneContainer::RemapFromSkelMesh(USkeletalMesh const & SourceSkeletalMesh, USkeleton& TargetSkeleton)
{
	int32 const SkelMeshLinkupIndex = TargetSkeleton.GetMeshLinkupIndex(&SourceSkeletalMesh);
	assert(SkelMeshLinkupIndex != INDEX_NONE);

	FSkeletonToMeshLinkup const & LinkupTable = TargetSkeleton.LinkupCache[SkelMeshLinkupIndex];

	// Copy LinkupTable arrays for now.
	// @laurent - Long term goal is to trim that down based on LOD, so we can get rid of the BoneIndicesArray and branch cost of testing if PoseBoneIndex is in that required bone index array.
	SkeletonToPoseBoneIndexArray = LinkupTable.SkeletonToMeshTable;
	PoseToSkeletonBoneIndexArray = LinkupTable.MeshToSkeletonTable;
}

void FBoneContainer::RemapFromSkeleton(USkeleton const & SourceSkeleton)
{
	SkeletonToPoseBoneIndexArray.resize(SourceSkeleton.GetRefLocalPoses().size(), INDEX_NONE);
	for (uint32 Index = 0; Index < BoneIndicesArray.size(); Index++)
	{
		int32 const & PoseBoneIndex = BoneIndicesArray[Index];
		SkeletonToPoseBoneIndexArray[PoseBoneIndex] = PoseBoneIndex;
	}

	// Skeleton to Skeleton mapping...
	PoseToSkeletonBoneIndexArray = SkeletonToPoseBoneIndexArray;
}

