#pragma once

#include "ReferenceSkeleton.h"
#include "AnimTypes.h"
#include "UnrealTemplates.h"

#include <vector>
#include <string>

class USkeletalMesh;

namespace EBoneTranslationRetargetingMode
{
	enum Type
	{
		/** Use translation from animation data. */
		Animation,
		/** Use fixed translation from Skeleton. */
		Skeleton,
		/** Use Translation from animation, but scale length by Skeleton's proportions. */
		AnimationScaled,
		/** Use Translation from animation, but also play the difference from retargeting pose as an additive. */
		AnimationRelative,
		/** Apply delta orientation and scale from ref pose */
		OrientAndScale,
	};
}

struct FBoneNode
{
	EBoneTranslationRetargetingMode::Type TranslationRetargetingMode;

	FBoneNode()
		: TranslationRetargetingMode(EBoneTranslationRetargetingMode::Animation)
	{
	}
};
struct FReferencePose
{
	std::string	PoseName;

	std::vector<FTransform>	ReferencePose;

	USkeletalMesh* SourceReferenceMesh;
};

struct FNameMapping
{
	std::string NodeName;
	std::string BoneName;

	FNameMapping()
	{
	}

	FNameMapping(const std::string& InNodeName)
		: NodeName(InNodeName)
	{
	}
	FNameMapping(const std::string& InNodeName, const std::string& InBoneName)
		: NodeName(InNodeName)
		, BoneName(InBoneName)
	{
	}
};

struct FVirtualBone
{

public:
	std::string SourceBoneName;

	std::string TargetBoneName;

	std::string VirtualBoneName;

	FVirtualBone() {}

	FVirtualBone(const std::string& InSource, const std::string& InTarget)
		: SourceBoneName(InSource)
		, TargetBoneName(InTarget)
	{
		VirtualBoneName = SourceBoneName + "_" + TargetBoneName;
	}
};

class USkeleton
{
	std::vector<struct FBoneNode> BoneTree;
	std::vector<FVirtualBone> VirtualBones;
	FReferenceSkeleton ReferenceSkeleton;

public:
	typedef SmartName::UID_Type AnimCurveUID;

	static const std::string AnimCurveMappingName;

	static const std::string AnimTrackCurveMappingName;
public:
	std::map<std::string, FReferencePose> AnimRetargetSources;

public:
	const std::vector<FVirtualBone>& GetVirtualBones() const { return VirtualBones; }

	const FReferenceSkeleton& GetReferenceSkeleton() const
	{
		return ReferenceSkeleton;
	}
	const std::vector<FTransform>& GetRefLocalPoses(std::string RetargetSource = "") const
	{
		if (RetargetSource != "")
		{
			auto FoundRetargetSourceIt = AnimRetargetSources.find(RetargetSource);
			if (FoundRetargetSourceIt != AnimRetargetSources.end())
			{
				return FoundRetargetSourceIt->second.ReferencePose;
			}
		}

		return ReferenceSkeleton.GetRefBonePose();
	}
	SmartName::UID_Type GetUIDByName(const std::string& ContainerName, const std::string& Name) const;

	EBoneTranslationRetargetingMode::Type GetBoneTranslationRetargetingMode(const int32 BoneTreeIdx) const
	{
		if (IsValidIndex(BoneTree,BoneTreeIdx))
		{
			return BoneTree[BoneTreeIdx].TranslationRetargetingMode;
		}
		return EBoneTranslationRetargetingMode::Animation;
	}

	bool DoesParentChainMatch(int32 StartBoneTreeIndex, const USkeletalMesh* InSkelMesh) const;
	bool IsCompatibleMesh(const USkeletalMesh* InSkelMesh) const;
	bool CreateReferenceSkeletonFromMesh(const USkeletalMesh* InSkeletalMesh, const std::vector<int32>& RequiredRefBones);
	bool MergeBonesToBoneTree(const USkeletalMesh* InSkeletalMesh, const std::vector<int32>& RequiredRefBones);
	bool MergeAllBonesToBoneTree(const USkeletalMesh* InSkelMesh);
	bool RecreateBoneTree(USkeletalMesh* InSkelMesh);
};