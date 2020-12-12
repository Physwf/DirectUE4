#pragma once

#include "UnrealMath.h"
#include <vector>
#include <map>

typedef uint16 FBoneIndexType;

/** Max number of bone influences that a single skinned vert can have. */
#define MAX_TOTAL_INFLUENCES		8

/** Max number of bone influences that a single skinned vert can have per vertex stream. */
#define MAX_INFLUENCES_PER_STREAM	4

struct FSoftSkinVertex
{
	FVector Position;
	FVector TangentX;//Tangent
	FVector TangentY;//Binomal
	Vector4 TangentZ;//Normal

	Vector2 UVs[4];
	FColor Color;

	uint8		InfluenceBones[MAX_TOTAL_INFLUENCES];
	uint8		InfluenceWeights[MAX_TOTAL_INFLUENCES];
};

struct FSkeletalMeshSection
{
	uint16 MaterialIndex;
	uint32 BaseIndex;
	uint32 NumTriangles;

	uint32 BaseVertexIndex;

	bool bCastShadow;

	std::vector<FSoftSkinVertex> SoftVertices;

	//TArray<FMeshToMeshVertData> ClothMappingData;

	std::vector<FBoneIndexType> BoneMap;

	int32 NumVertices;
	int32 MaxBoneInfluences;

	int32 MaxBoneInflunces;

	std::map<int32, std::vector<int32>> OverlappingVertices;

	FSkeletalMeshSection()
		: MaterialIndex(0)
		, BaseIndex(0)
		, NumTriangles(0)
		//, bSelected(false)
		//, bRecomputeTangent(false)
		, bCastShadow(true)
		//, bLegacyClothingSection_DEPRECATED(false)
		//, CorrespondClothSectionIndex_DEPRECATED(-1)
		, BaseVertexIndex(0)
		, NumVertices(0)
		, MaxBoneInfluences(4)
		//, CorrespondClothAssetIndex(INDEX_NONE)
		//, bDisabled(false)
		//, GenerateUpToLodIndex(-1)
	{}

	void CalcMaxBoneInfluences();

	inline bool HasExtraBoneInfluences() const
	{
		return MaxBoneInfluences > MAX_INFLUENCES_PER_STREAM;
	}
};

class FSkeletalMeshLODModel
{
public:
	std::vector<FSkeletalMeshSection> Sections;
	uint32 NumVertices;
	uint32 NumTexCoords;

	std::vector<uint32> IndexBuffer;//all Sections;

	std::vector<FBoneIndexType> ActiveBoneIndices;
	std::vector<FBoneIndexType> RequiredBones;
	std::vector<int32>			MeshToImportVertexMap;
	int32						MaxImportVertex;

	void GetVertices(std::vector<FSoftSkinVertex>& Vertices) const;

	bool DoSectionsNeedExtraBoneInfluences() const;
};


class SkeletalMeshModel
{
public:
	std::vector<FSkeletalMeshLODModel*> LODModels;


};