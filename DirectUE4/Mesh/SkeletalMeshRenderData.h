#pragma once

#include <vector>

class SkeletalMeshLODModel;

struct SkeletalMeshRenderSection
{
	uint16 MaterialIndex;
	uint32 BaseIndex;
	uint32 NumTriangles;

	uint32 BaseVertexIndex;

	uint32 NumVertices;

	int32 MaxBoneInfluences;
};

class SkeletalMeshLODRenderData
{
public:
	std::vector<SkeletalMeshRenderSection> RenderSections;

	void InitResources();

	void BuildFromLODModel(const SkeletalMeshLODModel* LODModel);


};

class SkeletalMeshRenderData
{
public:
	std::vector<SkeletalMeshLODRenderData*> LODRenderData;

	void InitResources();

};