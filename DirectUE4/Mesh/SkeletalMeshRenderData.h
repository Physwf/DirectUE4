#pragma once

#include "StaticMeshResources.h"
#include "SkinWeightVertexBuffer.h"
#include "MultiSizeIndexContainer.h"

class SkeletalMeshLODModel;
class USkeletalMesh;

struct FSkeletalMeshRenderSection
{
	uint16 MaterialIndex;
	uint32 BaseIndex;
	uint32 NumTriangles;

	uint32 BaseVertexIndex;

	std::vector<FBoneIndexType> BoneMap;

	uint32 NumVertices;

	int32 MaxBoneInfluences;
};

class FSkeletalMeshLODRenderData
{
public:
	std::vector<FSkeletalMeshRenderSection> RenderSections;

	/** static vertices from chunks for skinning on GPU */
	FStaticMeshVertexBuffers	StaticVertexBuffers;

	/** Skin weights for skinning */
	FSkinWeightVertexBuffer		SkinWeightVertexBuffer;

	FMultiSizeIndexContainer	MultiSizeIndexContainer;

	void InitResources();

	void BuildFromLODModel(const SkeletalMeshLODModel* LODModel,uint32 BuildFlags);

	uint32 GetNumVertices() const
	{
		return StaticVertexBuffers.PositionVertexBuffer.size();
	}

	std::vector<FBoneIndexType> ActiveBoneIndices;
	std::vector<FBoneIndexType> RequiredBones;
};

class FSkeletalMeshRenderData
{
public:
	std::vector<FSkeletalMeshLODRenderData*> LODRenderData;

	void InitResources(/*bool bNeedsVertexColors, TArray<UMorphTarget*>& InMorphTargets*/);

	void Cache(USkeletalMesh* Owner);

	int32 GetMaxBonesPerSection() const;
};