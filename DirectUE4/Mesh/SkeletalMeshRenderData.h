#pragma once

#include "StaticMeshResources.h"
#include "SkinWeightVertexBuffer.h"

class SkeletalMeshLODModel;
class USkeletalMesh;

struct SkeletalMeshRenderSection
{
	uint16 MaterialIndex;
	uint32 BaseIndex;
	uint32 NumTriangles;

	uint32 BaseVertexIndex;

	std::vector<FBoneIndexType> BoneMap;

	uint32 NumVertices;

	int32 MaxBoneInfluences;
};

class SkeletalMeshLODRenderData
{
public:
	std::vector<SkeletalMeshRenderSection> RenderSections;

	/** static vertices from chunks for skinning on GPU */
	FStaticMeshVertexBuffers	StaticVertexBuffers;

	/** Skin weights for skinning */
	FSkinWeightVertexBuffer		SkinWeightVertexBuffer;

	std::vector<uint32> IndexBuffer;

	void InitResources();

	void BuildFromLODModel(const SkeletalMeshLODModel* LODModel,uint32 BuildFlags);

	uint32 GetNumVertices() const
	{
		return StaticVertexBuffers.PositionVertexBuffer.size();
	}

	std::vector<FBoneIndexType> ActiveBoneIndices;
	std::vector<FBoneIndexType> RequiredBones;
};

class SkeletalMeshRenderData
{
public:
	std::vector<SkeletalMeshLODRenderData*> LODRenderData;

	void InitResources(/*bool bNeedsVertexColors, TArray<UMorphTarget*>& InMorphTargets*/);

	void Cache(USkeletalMesh* Owner);
};