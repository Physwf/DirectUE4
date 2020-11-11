#pragma once

#include "UnrealMath.h"
#include "Transform.h"

#include <vector>
#include <string>
#include <map>

typedef uint16 FBoneIndexType;

// This contains Reference-skeleton related info
// Bone transform is saved as FTransform array
struct FMeshBoneInfo
{
	// Bone's name.
	std::string Name;
	// 0/NULL if this is the root bone. 
	int32 ParentIndex;
	// Name used for export (this should be exact as FName may mess with case) 
	std::string ExportName;
	FMeshBoneInfo() : ParentIndex(-1) {}
	FMeshBoneInfo(const std::string& InName, const std::string& InExportName, int32 InParentIndex)
		: Name(InName)
		, ParentIndex(InParentIndex)
		, ExportName(InExportName)
	{}
	FMeshBoneInfo(const FMeshBoneInfo& Other)
		: Name(Other.Name)
		, ParentIndex(Other.ParentIndex)
		, ExportName(Other.ExportName)
	{}
	bool operator==(const FMeshBoneInfo& B) const
	{
		return(Name == B.Name);
	}
};
// Cached Virtual Bone data from USkeleton
struct FVirtualBoneRefData
{
	int32 VBRefSkelIndex;
	int32 SourceRefSkelIndex;
	int32 TargetRefSkelIndex;

	FVirtualBoneRefData(int32 InVBRefSkelIndex, int32 InSourceRefSkelIndex, int32 InTargetRefSkelIndex)
		: VBRefSkelIndex(InVBRefSkelIndex)
		, SourceRefSkelIndex(InSourceRefSkelIndex)
		, TargetRefSkelIndex(InTargetRefSkelIndex)
	{
	}
};

class USkeleton;

struct FReferenceSkeleton;
// Allow modifications to a reference skeleton while guaranteeing that virtual bones remain valid.
struct FReferenceSkeletonModifier
{
private:
	FReferenceSkeleton & RefSkeleton;
	const USkeleton* Skeleton;
public:
	FReferenceSkeletonModifier(FReferenceSkeleton& InRefSkel, const USkeleton* InSkeleton) : RefSkeleton(InRefSkel), Skeleton(InSkeleton) {}
	~FReferenceSkeletonModifier();

	// Update the reference pose transform of the specified bone
	void UpdateRefPoseTransform(const int32 BoneIndex, const FTransform& BonePose);

	// Add a new bone. BoneName must not already exist! ParentIndex must be valid.
	void Add(const FMeshBoneInfo& BoneInfo, const FTransform& BonePose);

	/** Find Bone Index from BoneName. Precache as much as possible in speed critical sections! */
	int32 FindBoneIndex(const std::string& BoneName) const;

	/** Accessor to private data. Const so it can't be changed recklessly. */
	const std::vector<FMeshBoneInfo> & GetRefBoneInfo() const;

	const FReferenceSkeleton& GetReferenceSkeleton() const { return RefSkeleton; }

};


/** Reference Skeleton **/
struct FReferenceSkeleton
{
	FReferenceSkeleton(bool bInOnlyOneRootAllowed = true)
		:bOnlyOneRootAllowed(bInOnlyOneRootAllowed)
	{}

private:
	//RAW BONES: Bones that exist in the original asset
	/** Reference bone related info to be serialized **/
	std::vector<FMeshBoneInfo>	RawRefBoneInfo;
	/** Reference bone transform **/
	std::vector<FTransform>		RawRefBonePose;

	//FINAL BONES: Bones for this skeleton including user added virtual bones
	/** Reference bone related info to be serialized **/
	std::vector<FMeshBoneInfo>	FinalRefBoneInfo;
	/** Reference bone transform **/
	std::vector<FTransform>		FinalRefBonePose;

	/** TMap to look up bone index from bone name. */
	std::map<std::string, int32>		RawNameToIndexMap;
	std::map<std::string, int32>		FinalNameToIndexMap;

	// cached data to allow virtual bones to be built into poses
	std::vector<FBoneIndexType>  RequiredVirtualBones;
	std::vector<FVirtualBoneRefData> UsedVirtualBoneData;

	/** Whether this skeleton is limited to one root or not
	*	Multi root is not supported in general Skeleton/SkeletalMesh
	*	But there are other components that can use this and support multi root - i.e. ControlRig
	*	This sturct is used in draw code, the long term plan may be detach this from draw code
	*	and use interface struct
	*/
	bool bOnlyOneRootAllowed;

	/** Removes the specified bone, so long as it has no children. Returns whether we removed the bone or not */
	bool RemoveIndividualBone(int32 BoneIndex, std::vector<int32>& OutBonesRemoved)
	{
		bool bRemoveThisBone = true;

		// Make sure we have no children
		for (int32 CurrBoneIndex = BoneIndex + 1; CurrBoneIndex < GetRawBoneNum(); CurrBoneIndex++)
		{
			if (RawRefBoneInfo[CurrBoneIndex].ParentIndex == BoneIndex)
			{
				bRemoveThisBone = false;
				break;
			}
		}

		if (bRemoveThisBone)
		{
			// Update parent indices of bones further through the array
			for (int32 CurrBoneIndex = BoneIndex + 1; CurrBoneIndex < GetRawBoneNum(); CurrBoneIndex++)
			{
				FMeshBoneInfo& Bone = RawRefBoneInfo[CurrBoneIndex];
				if (Bone.ParentIndex > BoneIndex)
				{
					Bone.ParentIndex -= 1;
				}
			}

			OutBonesRemoved.push_back(BoneIndex);
			// 			RawRefBonePose.RemoveAt(BoneIndex, 1);
			// 			RawRefBoneInfo.RemoveAt(BoneIndex, 1);
			RawRefBonePose.erase(RawRefBonePose.begin() + BoneIndex);
			RawRefBoneInfo.erase(RawRefBoneInfo.begin() + BoneIndex);
		}
		return bRemoveThisBone;
	}

	int32 GetParentIndexInternal(const int32 BoneIndex, const std::vector<FMeshBoneInfo>& BoneInfo) const
	{
		const int32 ParentIndex = BoneInfo[BoneIndex].ParentIndex;

		// Parent must be valid. Either INDEX_NONE for Root, or before children for non root bones.
		assert(!bOnlyOneRootAllowed || (((BoneIndex == 0) && (ParentIndex == -1)) || ((BoneIndex > 0) && ((int32)BoneInfo.size() > ParentIndex) && (ParentIndex < BoneIndex))));

		return ParentIndex;
	}

	void UpdateRefPoseTransform(const int32 BoneIndex, const FTransform& BonePose)
	{
		RawRefBonePose[BoneIndex] = BonePose;
	}

	/** Add a new bone.
	* BoneName must not already exist! ParentIndex must be valid. */
	void Add(const FMeshBoneInfo& BoneInfo, const FTransform& BonePose)
	{
		// Adding a bone that already exists is illegal
		assert(FindRawBoneIndex(BoneInfo.Name) == -1);

		// Make sure our arrays are in sync.
		assert((RawRefBoneInfo.size() == RawRefBonePose.size()) && (RawRefBoneInfo.size() == RawNameToIndexMap.size()));

		RawRefBoneInfo.push_back(BoneInfo);
		const int32 BoneIndex = RawRefBoneInfo.size() - 1;
		RawRefBonePose.push_back(BonePose);
		RawNameToIndexMap.insert(std::make_pair(BoneInfo.Name, BoneIndex));

		// Normalize Quaternion to be safe.
		RawRefBonePose[BoneIndex].NormalizeRotation();

		// Parent must be valid. Either INDEX_NONE for Root, or before children for non root bones.
		assert(!bOnlyOneRootAllowed ||
			(((BoneIndex == 0) && (BoneInfo.ParentIndex == -1))
				|| ((BoneIndex > 0) && (int32)RawRefBoneInfo.size()  > BoneInfo.ParentIndex && (BoneInfo.ParentIndex < BoneIndex))));
	}

	// Help us translate a virtual bone source into a raw bone source (for evaluating virtual bone transform)
	int32 GetRawSourceBoneIndex(const USkeleton* Skeleton, const std::string& SourceBoneName) const;

public:
	void RebuildRefSkeleton(const USkeleton* Skeleton, bool bRebuildNameMap);

	/** Returns number of bones in Skeleton. */
	int32 GetNum() const
	{
		return FinalRefBoneInfo.size();
	}

	/** Returns number of raw bones in Skeleton. These are the original bones of the asset */
	int32 GetRawBoneNum() const
	{
		return RawRefBoneInfo.size();
	}

	const std::vector<FBoneIndexType>& GetRequiredVirtualBones() const { return RequiredVirtualBones; }

	const std::vector<FVirtualBoneRefData>& GetVirtualBoneRefData() const { return UsedVirtualBoneData; }

	/** Accessor to private data. These include the USkeletons virtual bones. Const so it can't be changed recklessly. */
	const std::vector<FMeshBoneInfo> & GetRefBoneInfo() const
	{
		return FinalRefBoneInfo;
	}

	/** Accessor to private data. These include the USkeletons virtual bones. Const so it can't be changed recklessly. */
	const std::vector<FTransform> & GetRefBonePose() const
	{
		return FinalRefBonePose;
	}

	/** Accessor to private data. Raw relates to original asset. Const so it can't be changed recklessly. */
	const std::vector<FMeshBoneInfo> & GetRawRefBoneInfo() const
	{
		return RawRefBoneInfo;
	}

	/** Accessor to private data. Raw relates to original asset. Const so it can't be changed recklessly. */
	const std::vector<FTransform> & GetRawRefBonePose() const
	{
		return RawRefBonePose;
	}

	void Empty(int32 Size = 0)
	{
		RawRefBoneInfo.clear();
		RawRefBonePose.clear();

		FinalRefBoneInfo.clear();
		FinalRefBonePose.clear();

		RawNameToIndexMap.clear();
		FinalNameToIndexMap.clear();
	}

	/** Find Bone Index from BoneName. Precache as much as possible in speed critical sections! */
	int32 FindBoneIndex(const std::string& BoneName) const
	{
		assert(FinalRefBoneInfo.size() == FinalNameToIndexMap.size());
		int32 BoneIndex = -1;
		if (BoneName != std::string(""))
		{
			auto it = FinalNameToIndexMap.find(BoneName);
			if (it != FinalNameToIndexMap.end())
			{
				BoneIndex = it->second;
			}
		}
		return BoneIndex;
	}

	/** Find Bone Index from BoneName. Precache as much as possible in speed critical sections! */
	int32 FindRawBoneIndex(const std::string& BoneName) const
	{
		assert(RawRefBoneInfo.size() == RawNameToIndexMap.size());
		int32 BoneIndex = -1;
		if (BoneName != std::string(""))
		{
			auto it = RawNameToIndexMap.find(BoneName);
			if (it != RawNameToIndexMap.end())
			{
				BoneIndex = it->second;
			}
		}
		return BoneIndex;
	}

	std::string GetBoneName(const int32 BoneIndex) const
	{
		return FinalRefBoneInfo[BoneIndex].Name;
	}

	int32 GetParentIndex(const int32 BoneIndex) const
	{
		return GetParentIndexInternal(BoneIndex, FinalRefBoneInfo);
	}

	int32 GetRawParentIndex(const int32 BoneIndex) const
	{
		return GetParentIndexInternal(BoneIndex, RawRefBoneInfo);
	}

	bool IsValidIndex(int32 Index) const
	{
		return (Index >= 0 && (int32)FinalRefBoneInfo.size() > (Index));
	}

	bool IsValidRawIndex(int32 Index) const
	{
		return (Index >= 0 && (int32)RawRefBoneInfo.size() > (Index));
	}

	/**
	* Returns # of Depth from BoneIndex to ParentBoneIndex
	* This will return 0 if BoneIndex == ParentBoneIndex;
	* This will return -1 if BoneIndex isn't child of ParentBoneIndex
	*/
	int32 GetDepthBetweenBones(const int32 BoneIndex, const int32 ParentBoneIndex) const
	{
		if (BoneIndex >= ParentBoneIndex)
		{
			int32 CurBoneIndex = BoneIndex;
			int32 Depth = 0;

			do
			{
				// if same return;
				if (CurBoneIndex == ParentBoneIndex)
				{
					return Depth;
				}

				CurBoneIndex = FinalRefBoneInfo[CurBoneIndex].ParentIndex;
				++Depth;

			} while (CurBoneIndex != -1);
		}

		return -1;
	}

	bool BoneIsChildOf(const int32 ChildBoneIndex, const int32 ParentBoneIndex) const
	{
		// Bones are in strictly increasing order.
		// So child must have an index greater than his parent.
		if (ChildBoneIndex > ParentBoneIndex)
		{
			int32 BoneIndex = GetParentIndex(ChildBoneIndex);
			do
			{
				if (BoneIndex == ParentBoneIndex)
				{
					return true;
				}
				BoneIndex = GetParentIndex(BoneIndex);

			} while (BoneIndex != -1);
		}

		return false;
	}

	//void RemoveDuplicateBones(const UObject* Requester, std::vector<FBoneIndexType> & DuplicateBones);


	/** Removes the supplied bones from the skeleton, unless they have children that aren't also going to be removed */
	// 	std::vector<int32> RemoveBonesByName(USkeleton* Skeleton, const std::vector<std::string>& BonesToRemove)
	// 	{
	// 		std::vector<int32> BonesRemoved;
	// 
	// 		const int32 NumBones = GetRawBoneNum();
	// 		for (int32 BoneIndex = NumBones - 1; BoneIndex >= 0; BoneIndex--)
	// 		{
	// 			FMeshBoneInfo& Bone = RawRefBoneInfo[BoneIndex];
	// 
	// 			if (BonesToRemove.Contains(Bone.Name))
	// 			{
	// 				RemoveIndividualBone(BoneIndex, BonesRemoved);
	// 			}
	// 		}
	// 
	// 		const bool bRebuildNameMap = true;
	// 		RebuildRefSkeleton(Skeleton, bRebuildNameMap);
	// 		return BonesRemoved;
	// 	}

	void RebuildNameToIndexMap();

	/** Ensure parent exists in the given input sorted array. Insert parent to the array. The result should be sorted. */
	void EnsureParentsExist(std::vector<FBoneIndexType>& InOutBoneSortedArray) const;

	/** Ensure parent exists in the given input array. Insert parent to the array. The result should be sorted. */
	void EnsureParentsExistAndSort(std::vector<FBoneIndexType>& InOutBoneUnsortedArray) const;

	size_t GetDataSize() const;

	// very slow search function for all children
	int32 GetDirectChildBones(int32 ParentBoneIndex, std::vector<int32> & Children) const;

	friend FReferenceSkeletonModifier;
};
