#pragma once

#include "SkeletalMeshModel.h"
#include "SkeletalMeshRenderData.h"
#include "ReferenceSkeleton.h"
#include "PrimitiveComponent.h"

#include <memory>

class Skeleton;

class USkeletalMesh
{
public:
	USkeletalMesh(class AActor* InOwner);

	SkeletalMeshModel* GetImportedModel() const { return ImportedModel.get(); }

	FSkeletalMeshRenderData* GetResourceForRendering() const { return RenderdData.get(); }

	USkeleton* Skeleton;

	FReferenceSkeleton RefSkeleton;

	static void CalculateRequiredBones(SkeletalMeshLODModel& LODModel, const struct FReferenceSkeleton& RefSkeleton, const std::map<FBoneIndexType, FBoneIndexType>* BonesToRemove);

	void PostLoad();

	virtual void InitResources();
	virtual void ReleaseResources();
	void AllocateResourceForRendering();
private:
	std::shared_ptr<SkeletalMeshModel> ImportedModel;

	std::unique_ptr<FSkeletalMeshRenderData> RenderdData;

	void CacheDerivedData();
};