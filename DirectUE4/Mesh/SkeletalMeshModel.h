#pragma once

#include "UnrealMath.h"
#include <vector>
#include <map>

typedef uint16 FBoneIndexType;

/** Max number of bone influences that a single skinned vert can have. */
#define MAX_TOTAL_INFLUENCES		8

/** Max number of bone influences that a single skinned vert can have per vertex stream. */
#define MAX_INFLUENCES_PER_STREAM	4

struct SoftSkinVertex
{
	Vector Position;
	Vector TangentX;//Tangent
	Vector TangentY;//Binomal
	Vector4 TangentZ;//Normal

	Vector2 UVs[4];
	FColor Color;

	uint8		InfluenceBones[MAX_TOTAL_INFLUENCES];
	uint8		InfluenceWeights[MAX_TOTAL_INFLUENCES];
};

struct SkeletalMeshSection
{
	uint16 MaterialIndex;
	uint32 BaseIndex;
	uint32 NumTriangles;

	uint32 BaseVertexIndex;

	std::vector<SoftSkinVertex> SoftVertices;

	//TArray<FMeshToMeshVertData> ClothMappingData;

	std::vector<FBoneIndexType> BoneMap;

	int32 NumVertices;
	int32 MaxBoneInfluences;

	int32 MaxBoneInflunces;

	std::map<int32, std::vector<int32>> OverlappingVertices;

	void CalcMaxBoneInfluences();

	inline bool HasExtraBoneInfluences() const
	{
		return MaxBoneInfluences > MAX_INFLUENCES_PER_STREAM;
	}
};

class SkeletalMeshLODModel
{
public:
	std::vector<SkeletalMeshSection> Sections;
	uint32 NumVertices;
	uint32 NumTexCoords;

	std::vector<uint32> IndexBuffer;//all Sections;

	std::vector<FBoneIndexType> ActiveBoneIndices;
	std::vector<FBoneIndexType> RequiredBones;
	std::vector<int32>			MeshToImportVertexMap;
	int32						MaxImportVertex;

	void GetVertices(std::vector<SoftSkinVertex>& Vertices) const;

	bool DoSectionsNeedExtraBoneInfluences() const;
};


class SkeletalMeshModel
{
public:
	std::vector<SkeletalMeshLODModel*> LODModels;


};