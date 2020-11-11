#pragma once

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
	std::vector<FVirtualBone> VirtualBones;

public:
	const std::vector<FVirtualBone>& GetVirtualBones() const { return VirtualBones; }
};