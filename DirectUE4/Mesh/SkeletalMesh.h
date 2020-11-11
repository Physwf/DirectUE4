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

	Skeleton* mSkeleton;

	FReferenceSkeleton RefSkeleton;
private:
	std::shared_ptr<SkeletalMeshModel> ImportedModel;

	std::unique_ptr<SkeletalMeshRenderData> RenderdData;

};