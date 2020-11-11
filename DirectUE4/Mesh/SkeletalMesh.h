#pragma once

#include "SkeletalMeshModel.h"
#include "SkeletalMeshRenderData.h"
#include "ReferenceSkeleton.h"

#include <memory>

class Skeleton;

class SkeletalMesh
{
public:
	SkeletalMesh();

	SkeletalMeshModel* GetImportedModel() const { return ImportedModel.get(); }

	SkeletalMeshRenderData* GetResourceForRendering() const { return RenderdData.get(); }

	USkeleton* Skeleton;

	FReferenceSkeleton RefSkeleton;

	static void CalculateRequiredBones(SkeletalMeshLODModel& LODModel, const struct FReferenceSkeleton& RefSkeleton, const std::map<FBoneIndexType, FBoneIndexType>* BonesToRemove);
private:
	std::shared_ptr<SkeletalMeshModel> ImportedModel;

	std::unique_ptr<SkeletalMeshRenderData> RenderdData;

};