#pragma once

#include "SkeletalMeshModel.h"
#include "SkeletalMeshRenderData.h"
#include "ReferenceSkeleton.h"
#include "PrimitiveComponent.h"
#include "UnrealTemplates.h"

#include <memory>

class Skeleton;

struct FSkeletalMeshLODInfo
{
};

struct FSkeletalMaterial
{
	FSkeletalMaterial() : MaterialInterface(NULL)
	{

	}

	FSkeletalMaterial(class UMaterial* InMaterialInterface, std::string InMaterialSlotName = "")
		: MaterialInterface(InMaterialInterface)
		, MaterialSlotName(InMaterialSlotName)
	{

	}

	class UMaterial* MaterialInterface;

	/*This name should be use by the gameplay to avoid error if the skeletal mesh Materials array topology change*/
	std::string						MaterialSlotName;
	/** Data used for texture streaming relative to each UV channels. */
	//FMeshUVChannelInfo			UVChannelData;
};

class USkeletalMesh
{
public:
	USkeletalMesh(class AActor* InOwner);

	SkeletalMeshModel* GetImportedModel() const { return ImportedModel.get(); }

	FSkeletalMeshRenderData* GetResourceForRendering() const { return RenderdData.get(); }

	USkeleton* Skeleton;

	FReferenceSkeleton RefSkeleton;

	static void CalculateRequiredBones(FSkeletalMeshLODModel& LODModel, const struct FReferenceSkeleton& RefSkeleton, const std::map<FBoneIndexType, FBoneIndexType>* BonesToRemove);

	void PostLoad();

	virtual void InitResources();
	virtual void ReleaseResources();
	void AllocateResourceForRendering();

	void CalculateInvRefMatrices();

	FMatrix GetRefPoseMatrix(int32 BoneIndex) const;

	FSkeletalMeshLODInfo& AddLODInfo();
	void AddLODInfo(const FSkeletalMeshLODInfo& NewLODInfo) { LODInfo.push_back(NewLODInfo); }
	void RemoveLODInfo(int32 Index);
	void ResetLODInfo();
	std::vector<FSkeletalMeshLODInfo>& GetLODInfoArray() { return LODInfo; }
	FSkeletalMeshLODInfo* GetLODInfo(int32 Index) { return IsValidIndex(LODInfo,Index) ? &LODInfo[Index] : nullptr; }
	const FSkeletalMeshLODInfo* GetLODInfo(int32 Index) const { return IsValidIndex(LODInfo,Index) ? &LODInfo[Index] : nullptr; }
	bool IsValidLODIndex(int32 Index) const { return IsValidIndex(LODInfo, Index); }
	uint32 GetLODNum() const { return LODInfo.size(); }

	std::vector<FMatrix> RefBasesInvMatrix;

	std::vector<FSkeletalMaterial> Materials;
private:
	std::shared_ptr<SkeletalMeshModel> ImportedModel;

	std::unique_ptr<FSkeletalMeshRenderData> RenderdData;

	std::vector<struct FSkeletalMeshLODInfo> LODInfo;

	void CacheDerivedData();

	std::vector<FMatrix> CachedComposedRefPoseMatrices;

};