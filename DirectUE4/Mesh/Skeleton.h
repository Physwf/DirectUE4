#pragma once

#include "ReferenceSkeleton.h"

#include <vector>
#include <string>

class SkeletalMesh;

struct FBoneNode
{

};
struct FReferencePose
{
	std::string	PoseName;

	std::vector<FTransform>	ReferencePose;

	SkeletalMesh* SourceReferenceMesh;
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
	const std::vector<FVirtualBone>& GetVirtualBones() const { return VirtualBones; }

	bool DoesParentChainMatch(int32 StartBoneTreeIndex, const SkeletalMesh* InSkelMesh) const;
	bool IsCompatibleMesh(const SkeletalMesh* InSkelMesh) const;
	bool CreateReferenceSkeletonFromMesh(const SkeletalMesh* InSkeletalMesh, const std::vector<int32>& RequiredRefBones);
	bool MergeBonesToBoneTree(const SkeletalMesh* InSkeletalMesh, const std::vector<int32>& RequiredRefBones);
	bool MergeAllBonesToBoneTree(const SkeletalMesh* InSkelMesh);
	bool RecreateBoneTree(SkeletalMesh* InSkelMesh);
};