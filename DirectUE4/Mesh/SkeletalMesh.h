#pragma once

#include "SkeletalMeshModel.h"
#include "SkeletalMeshRenderData.h"
#include "ReferenceSkeleton.h"
#include "Mesh.h"

#include <memory>

class Skeleton;

class SkeletalMesh : public MeshPrimitive
{
public:
	SkeletalMesh();

	SkeletalMeshModel* GetImportedModel() const { return ImportedModel.get(); }

	SkeletalMeshRenderData* GetResourceForRendering() const { return RenderdData.get(); }

	USkeleton* Skeleton;

	FReferenceSkeleton RefSkeleton;

	static void CalculateRequiredBones(SkeletalMeshLODModel& LODModel, const struct FReferenceSkeleton& RefSkeleton, const std::map<FBoneIndexType, FBoneIndexType>* BonesToRemove);

	void PostLoad();

	virtual void InitResource();
	virtual void ReleaseResource();
	virtual void UpdateTransform() {};
	virtual void GetDynamicMeshElements(const std::vector<const SceneView*>& Views, const SceneViewFamily& ViewFamily, uint32 VisibilityMap/*, FMeshElementCollector& Collector*/) const override {} ;
	virtual FPrimitiveViewRelevance GetViewRelevance(const SceneView* View) const override { return FPrimitiveViewRelevance(); };
	void AllocateResourceForRendering();
private:
	std::shared_ptr<SkeletalMeshModel> ImportedModel;

	std::unique_ptr<SkeletalMeshRenderData> RenderdData;

	void CacheDerivedData();
};