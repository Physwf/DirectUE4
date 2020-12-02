#pragma once

#include "fbxsdk.h"
#include "UnrealMath.h"
#include "Transform.h"

#include <vector>
#include <string>
#include <set>
#include <map>

typedef uint16 FBoneIndexType;

void FillFbxArray(FbxNode* pNode, std::vector<FbxNode*>& pOutMeshArray);

FbxAMatrix ComputeTotalMatrix(FbxScene* pScence, FbxNode* pNode, bool bTransformVertexToAbsolute, bool bBakePivotInVertex);

struct FbxMaterial
{
	FbxSurfaceMaterial* fbxMaterial;

	std::string GetName() const { return fbxMaterial ? fbxMaterial->GetName() : "None"; }
};
void SetJointPostConversionMatrix(FbxAMatrix ConversionMatrix);
const FbxAMatrix &GetJointPostConversionMatrix();
FVector ConvertPos(FbxVector4 pPos);
FVector ConvertDir(FbxVector4 Vec);
float ConvertDist(FbxDouble Distance);
FQuat ConvertRotToQuat(FbxQuaternion Quaternion);
FVector ConvertScale(FbxDouble3 V);

class UStaticMesh;
class USkeletalMesh;

struct FMeshWedge
{
	uint32			iVertex;			// Vertex index.
	Vector2			UVs[4];				// UVs.
	FColor			Color;				// Vertex color.
};
struct FMeshFace
{
	// Textured Vertex indices.
	uint32		iWedge[3];
	// Source Material (= texture plus unique flags) index.
	uint16		MeshMaterialIndex;
	FVector	TangentX[3];
	FVector	TangentY[3];
	FVector	TangentZ[3];
	// 32-bit flag for smoothing groups.
	uint32   SmoothingGroups;
};
// A bone: an orientation, and a position, all relative to their parent.
struct VJointPos
{
	FTransform	Transform;

	// For collision testing / debug drawing...
	float       Length;
	float       XSize;
	float       YSize;
	float       ZSize;
};
// Textured triangle.
struct VTriangle
{
	// Point to three vertices in the vertex list.
	uint32   WedgeIndex[3];
	// Materials can be anything.
	uint8    MatIndex;
	// Second material from exporter (unused)
	uint8    AuxMatIndex;
	// 32-bit flag for smoothing groups.
	uint32   SmoothingGroups;

	FVector	TangentX[3];
	FVector	TangentY[3];
	FVector	TangentZ[3];


	VTriangle& operator=(const VTriangle& Other)
	{
		this->AuxMatIndex = Other.AuxMatIndex;
		this->MatIndex = Other.MatIndex;
		this->SmoothingGroups = Other.SmoothingGroups;
		this->WedgeIndex[0] = Other.WedgeIndex[0];
		this->WedgeIndex[1] = Other.WedgeIndex[1];
		this->WedgeIndex[2] = Other.WedgeIndex[2];
		this->TangentX[0] = Other.TangentX[0];
		this->TangentX[1] = Other.TangentX[1];
		this->TangentX[2] = Other.TangentX[2];

		this->TangentY[0] = Other.TangentY[0];
		this->TangentY[1] = Other.TangentY[1];
		this->TangentY[2] = Other.TangentY[2];

		this->TangentZ[0] = Other.TangentZ[0];
		this->TangentZ[1] = Other.TangentZ[1];
		this->TangentZ[2] = Other.TangentZ[2];

		return *this;
	}
};

struct FVertInfluence
{
	float Weight;
	uint32 VertIndex;
	FBoneIndexType BoneIndex;
};

// Raw data material.
struct VMaterial
{
	/** The actual material created on import or found among existing materials */
	//TWeakObjectPtr<UMaterialInterface> Material;
	/** The material name found by the importer */
	std::string MaterialImportName;
};
// Raw data bone.
struct VBone
{
	std::string		Name;     //
						  //@ todo FBX - Flags unused?
	uint32		Flags;        // reserved / 0x02 = bone where skin is to be attached...	
	int32 		NumChildren;  // children  // only needed in animation ?
	int32       ParentIndex;  // 0/NULL if this is the root bone.  
	VJointPos	BonePos;      // reference position
};
// Raw data bone influence.
struct VRawBoneInfluence // just weight, vertex, and Bone, sorted later....
{
	float Weight;
	int32   VertexIndex;
	int32   BoneIndex;
};
// Vertex with texturing info, akin to Hoppe's 'Wedge' concept - import only.
struct VVertex
{
	uint32	VertexIndex; // Index to a vertex.
	Vector2 UVs[8];        // Scaled to BYTES, rather...-> Done in digestion phase, on-disk size doesn't matter here.
	FColor	Color;		 // Vertex colors
	uint8    MatIndex;    // At runtime, this one will be implied by the face that's pointing to us.
	uint8    Reserved;    // Top secret.

	VVertex()
	{
		memset(this, 0, sizeof(VVertex));
	}
};
// Points: regular Vectors (for now..)
struct VPoint
{
	FVector	Point; // Change into packed integer later IF necessary, for 3x size reduction...
};

/**
* Container and importer for skeletal mesh (FBX file) data
**/
class FSkeletalMeshImportData
{
public:
	std::vector<VMaterial>			Materials;		// Materials
	std::vector<FVector>				Points;			// 3D Points
	std::vector<VVertex>			Wedges;			// Wedges
	std::vector<VTriangle>			Faces;			// Faces
	std::vector<VBone>				RefBonesBinary;	// Reference Skeleton
	std::vector<VRawBoneInfluence>	Influences;		// Influences
	std::vector<int32>				PointToRawMap;	// Mapping from current point index to the original import point index
	uint32	NumTexCoords;						// The number of texture coordinate sets
	uint32	MaxMaterialIndex;					// The max material index found on a triangle
	bool 	bHasVertexColors; 					// If true there are vertex colors in the imported file
	bool	bHasNormals;						// If true there are normals in the imported file
	bool	bHasTangents;						// If true there are tangents in the imported file
	bool	bUseT0AsRefPose;					// If true, then the pose at time=0 will be used instead of the ref pose
	bool	bDiffPose;							// If true, one of the bones has a different pose at time=0 vs the ref pose

	FSkeletalMeshImportData()
		: NumTexCoords(0)
		, MaxMaterialIndex(0)
		, bHasVertexColors(false)
		, bHasNormals(false)
		, bHasTangents(false)
		, bUseT0AsRefPose(false)
		, bDiffPose(false)
	{

	}
	void CopyLODImportData(
		std::vector<FVector>& LODPoints,
		std::vector<FMeshWedge>& LODWedges,
		std::vector<FMeshFace>& LODFaces,
		std::vector<FVertInfluence>& LODInfluences,
		std::vector<int32>& LODPointToRawMap) const;

	static std::string FixupBoneName(const std::string& InBoneName);

	/**
	* Removes all import data
	*/
	void Empty()
	{
		Materials.clear();
		Points.clear();
		Wedges.clear();
		Faces.clear();
		RefBonesBinary.clear();
		Influences.clear();
		PointToRawMap.clear();
	}
};
enum EFBXNormalImportMethod
{
	FBXNIM_ComputeNormals ,
	FBXNIM_ImportNormals ,
	FBXNIM_ImportNormalsAndTangents ,
	FBXNIM_MAX,
};
namespace EFBXNormalGenerationMethod
{
	enum Type
	{
		/** Use the legacy built in method to generate normals (faster in some cases) */
		BuiltIn,
		/** Use MikkTSpace to generate normals and tangents */
		MikkTSpace,
	};
}
struct FOverlappingThresholds
{
public:
	FOverlappingThresholds()
		: ThresholdPosition(THRESH_POINTS_ARE_SAME)
		, ThresholdTangentNormal(THRESH_NORMALS_ARE_SAME)
		, ThresholdUV(THRESH_UVS_ARE_SAME)
	{}

	/** Threshold use to decide if two vertex position are equal. */
	float ThresholdPosition;

	/** Threshold use to decide if two normal, tangents or bi-normals are equal. */
	float ThresholdTangentNormal;

	/** Threshold use to decide if two UVs are equal. */
	float ThresholdUV;
};

struct FBXImportOptions
{
	// General options
// 	bool bCanShowDialog;
	bool bImportScene;
// 	bool bImportMaterials;
// 	bool bInvertNormalMap;
// 	bool bImportTextures;
// 	bool bImportLOD;
// 	bool bUsedAsFullName;
// 	bool bConvertScene;
// 	bool bForceFrontXAxis;
// 	bool bConvertSceneUnit;
// 	bool bRemoveNameSpace;
// 	FVector ImportTranslation;
// 	FRotator ImportRotation;
// 	float ImportUniformScale;
	EFBXNormalImportMethod NormalImportMethod;
	EFBXNormalGenerationMethod::Type NormalGenerationMethod;
	bool bTransformVertexToAbsolute;
 	bool bBakePivotInVertex;
// 	EFBXImportType ImportType;
// 	// Static Mesh options
// 	bool bCombineToSingle;
// 	EVertexColorImportOption::Type VertexColorImportOption;
// 	FColor VertexOverrideColor;
// 	bool bRemoveDegenerates;
// 	bool bBuildAdjacencyBuffer;
// 	bool bBuildReversedIndexBuffer;
// 	bool bGenerateLightmapUVs;
// 	bool bOneConvexHullPerUCX;
// 	bool bAutoGenerateCollision;
// 	FName StaticMeshLODGroup;
// 	bool bImportStaticMeshLODs;
// 	bool bAutoComputeLodDistances;
// 	TArray<float> LodDistances;
// 	int32 MinimumLodNumber;
// 	int32 LodNumber;
// 	// Material import options
// 	class UMaterialInterface *BaseMaterial;
// 	FString BaseColorName;
// 	FString BaseDiffuseTextureName;
// 	FString BaseEmissiveColorName;
// 	FString BaseNormalTextureName;
// 	FString BaseEmmisiveTextureName;
// 	FString BaseSpecularTextureName;
// 	EMaterialSearchLocation MaterialSearchLocation;
// 	// Skeletal Mesh options
// 	bool bImportMorph;
// 	bool bImportAnimations;
// 	bool bUpdateSkeletonReferencePose;
// 	bool bResample;
// 	bool bImportRigidMesh;
// 	bool bUseT0AsRefPose;
	bool bPreserveSmoothingGroups;
	FOverlappingThresholds OverlappingThresholds;
// 	bool bImportMeshesInBoneHierarchy;
// 	bool bCreatePhysicsAsset;
// 	UPhysicsAsset *PhysicsAsset;
// 	bool bImportSkeletalMeshLODs;
// 	// Animation option
// 	USkeleton* SkeletonForAnimation;
// 	EFBXAnimationLengthImportType AnimationLengthImportType;
// 	struct FIntPoint AnimationRange;
// 	FString AnimationName;
// 	bool	bPreserveLocalTransform;
// 	bool	bDeleteExistingMorphTargetCurves;
// 	bool	bImportCustomAttribute;
// 	bool	bImportBoneTracks;
// 	bool	bSetMaterialDriveParameterOnCustomAttribute;
// 	bool	bRemoveRedundantKeys;
// 	bool	bDoNotImportCurveWithZero;
// 	TArray<FString> MaterialCurveSuffixes;

	bool ShouldImportNormals()
	{
		return NormalImportMethod == FBXNIM_ImportNormals || NormalImportMethod == FBXNIM_ImportNormalsAndTangents;
	}

	bool ShouldImportTangents()
	{
		return NormalImportMethod == FBXNIM_ImportNormalsAndTangents;
	}
};

struct MeshBuildOptions
{
	MeshBuildOptions()
		: bRemoveDegenerateTriangles(false)
		, bComputeNormals(true)
		, bComputeTangents(true)
		, bUseMikkTSpace(false)
	{
	}

	bool bRemoveDegenerateTriangles;
	bool bComputeNormals;
	bool bComputeTangents;
	bool bUseMikkTSpace;
	FOverlappingThresholds OverlappingThresholds;
};
class SkeletalMeshLODModel;
struct FReferenceSkeleton;

// /**
// * Skinned model data needed to generate skinned mesh chunks for reprocessing.
// */
// struct FSkinnedModelData
// {
// 	/** Vertices of the model. */
// 	std::vector<SoftSkinVertex> Vertices;
// 	/** Indices of the model. */
// 	std::vector<uint32> Indices;
// 	/** Contents of the model's RawPointIndices bulk data. */
// 	std::vector<uint32> RawPointIndices;
// 	/** Map of vertex index to the original import index. */
// 	std::vector<int32> MeshToImportVertexMap;
// 	/** Per-section information. */
// 	std::vector<FSkelMeshSection> Sections;
// 	/** Per-section bone maps. */
// 	std::vector<std::vector<FBoneIndexType>*> BoneMaps;
// 	/** The number of valid texture coordinates. */
// 	int32 NumTexCoords;
// };
#define MAX_TOTAL_INFLUENCES 8
typedef FVector FPackedNormal;
struct FSoftSkinBuildVertex
{
	FVector			Position;
	FPackedNormal	TangentX,	// Tangent, U-direction
		TangentY,	// Binormal, V-direction
		TangentZ;	// Normal
	Vector2			UVs[4]; // UVs
	FColor			Color;		// VertexColor
	FBoneIndexType	InfluenceBones[MAX_TOTAL_INFLUENCES];
	uint8			InfluenceWeights[MAX_TOTAL_INFLUENCES];
	uint32 PointWedgeIdx;
};
// this is used for a sub-quadratic routine to find "equal" verts
struct FSkeletalMeshVertIndexAndZ
{
	int32 Index;
	float Z;
};
/**
* A chunk of skinned mesh vertices used as intermediate data to build a renderable
* skinned mesh.
*/
struct FSkinnedMeshChunk
{
	/** The material index with which this chunk should be rendered. */
	int32 MaterialIndex;
	/** The original section index for which this chunk was generated. */
	int32 OriginalSectionIndex;
	/** The vertices associated with this chunk. */
	std::vector<FSoftSkinBuildVertex> Vertices;
	/** The indices of the triangles in this chunk. */
	std::vector<uint32> Indices;
	/** If not empty, contains a map from bones referenced in this chunk to the skeleton. */
	std::vector<FBoneIndexType> BoneMap;
};

namespace SkeletalMeshTools
{
	inline bool SkeletalMesh_UVsEqual(const FMeshWedge& V1, const FMeshWedge& V2, const FOverlappingThresholds& OverlappingThresholds, const int32 UVIndex = 0)
	{
		const Vector2& UV1 = V1.UVs[UVIndex];
		const Vector2& UV2 = V2.UVs[UVIndex];

		if (FMath::Abs(UV1.X - UV2.X) > OverlappingThresholds.ThresholdUV)
			return 0;

		if (FMath::Abs(UV1.Y - UV2.Y) > OverlappingThresholds.ThresholdUV)
			return 0;

		return 1;
	}

	/** @return true if V1 and V2 are equal */
	bool AreSkelMeshVerticesEqual(const FSoftSkinBuildVertex& V1, const FSoftSkinBuildVertex& V2, const FOverlappingThresholds& OverlappingThresholds);

	/**
	* Creates chunks and populates the vertex and index arrays inside each chunk
	*
	* @param Faces						List of raw faces
	* @param RawVertices				List of raw created, unordered, unwelded vertices
	* @param RawVertIndexAndZ			List of indices into the RawVertices array and each raw vertex Z position.  This is used for fast lookup of overlapping vertices
	* @param OverlappingThresholds		The thresholds to use to compute overlap vertex instance
	* @param OutChunks					Created array of chunks
	*/
	void BuildSkeletalMeshChunks(const std::vector<FMeshFace>& Faces, const std::vector<FSoftSkinBuildVertex>& RawVertices, std::vector<FSkeletalMeshVertIndexAndZ>& RawVertIndexAndZ, const FOverlappingThresholds &OverlappingThresholds, std::vector<FSkinnedMeshChunk*>& OutChunks, bool& bOutTooManyVerts);

	/**
	* Splits chunks to satisfy the requested maximum number of bones per chunk
	* @param Chunks			Chunks to split. Upon return contains the results of splitting chunks.
	* @param MaxBonesPerChunk	The maximum number of bones a chunk may reference.
	*/
	void ChunkSkinnedVertices(std::vector<FSkinnedMeshChunk*>& Chunks, int32 MaxBonesPerChunk);

	//void CalcBoneVertInfos(USkeletalMesh* SkeletalMesh, std::vector<FBoneVertInfo>& Infos, bool bOnlyDominant);
};

class FStaticMeshRenderData;
struct FStaticMeshLODResources;
struct StaticMeshBuildVertex;
class MeshDescription;

class FBXImporter
{
public:
	UStaticMesh* ImportStaticMesh(class AActor* InOwner, const char* filename);
	USkeletalMesh* ImportSkeletalMesh(class AActor* InOwner, const char* filename);

	bool FillSkeletalMeshImportData(std::vector<FbxNode*>& NodeArray, std::vector<FbxShape*> *FbxShapeArray, FSkeletalMeshImportData* OutData);
	bool ImportBone(std::vector<FbxNode*>& NodeArray, FSkeletalMeshImportData &ImportData, std::vector<FbxNode*> &OutSortedLinks, bool& bUseTime0AsRefPose, FbxNode *SkeletalMeshNode);
	bool FillSkelMeshImporterFromFbx(FSkeletalMeshImportData& ImportData, FbxMesh*& Mesh, FbxSkin* Skin, FbxShape* Shape, std::vector<FbxNode*> &SortedLinks, const std::vector<FbxSurfaceMaterial*>& FbxMaterials, FbxNode *RootNode);
	bool FillSkeletalMeshImportPoints(FSkeletalMeshImportData* OutData, FbxNode* RootNode, FbxNode* Node, FbxShape* FbxShape);
	void RecursiveBuildSkeleton(FbxNode* Link, std::vector<FbxNode*>& OutSortedLinks);
	bool RetrievePoseFromBindPose(const std::vector<FbxNode*>& NodeArray, FbxArray<FbxPose*> & PoseArray) const;
	void BuildSkeletonSystem(std::vector<FbxCluster*>& ClusterArray, std::vector<FbxNode*>& OutSortedLinks);
	FbxNode* GetRootSkeleton(FbxNode* Link);
	void SkinControlPointsToPose(FSkeletalMeshImportData &ImportData, FbxMesh* Mesh, FbxShape* Shape, bool bUseT0);
	FbxAMatrix ComputeTotalMatrix(FbxNode* Node);
	FbxAMatrix ComputeSkeletalMeshTotalMatrix(FbxNode* Node, FbxNode *RootSkeletalNode);
	bool IsOddNegativeScale(FbxAMatrix& TotalMatrix);

	bool BuildSkeletalMesh(SkeletalMeshLODModel& LODModel, const FReferenceSkeleton& RefSkeleton, const std::vector<FVertInfluence>& Influences, const std::vector<FMeshWedge>& Wedges, const std::vector<FMeshFace>& Faces, const std::vector<FVector>& Points, const std::vector<int32>& PointToOriginalMap, const MeshBuildOptions& BuildOptions = MeshBuildOptions(), std::vector<std::string> * OutWarningMessages = NULL, std::vector<std::string> * OutWarningNames = NULL);
	void BuildSkeletalModelFromChunks(SkeletalMeshLODModel& LODModel, const FReferenceSkeleton& RefSkeleton, std::vector<FSkinnedMeshChunk*>& Chunks, const std::vector<int32>& PointToOriginalMap);

	bool BuildStaticMesh(FStaticMeshRenderData& OutRenderData, UStaticMesh* Mesh/*, const FStaticMeshLODGroup& LODGroup */);
	void BuildVertexBuffer(const MeshDescription& MD2, FStaticMeshLODResources& StaticMeshLOD, std::vector<std::vector<uint32> >& OutPerSectionIndices, std::vector<StaticMeshBuildVertex>& StaticMeshBuildVertices, const std::multimap<int32, int32>& OverlappingCorners, float VertexComparisonThreshold, std::vector<int32>& RemapVerts);
public:
	FbxScene* fbxScene;
	FbxManager* SDKManager;
	FBXImportOptions* ImportOptions;
	FbxGeometryConverter* GeometryConverter;
};

namespace ETangentOptions
{
	enum Type
	{
		None = 0,
		BlendOverlappingNormals = 0x1,
		IgnoreDegenerateTriangles = 0x2,
		UseMikkTSpace = 0x4,
	};
};

enum class ELightmapUVVersion : int32
{
	BitByBit = 0,
	Segments = 1,
	SmallChartPacking = 2,
	ScaleChartsOrderingFix = 3,
	ChartJoiningLFix = 4,
	Latest = ChartJoiningLFix
};

/**
*	Contains the vertices that are most dominated by that bone. Vertices are in Bone space.
*	Not used at runtime, but useful for fitting physics assets etc.
*/
struct FBoneVertInfo
{
	// Invariant: Arrays should be same length!
	std::vector<FVector>	Positions;
	std::vector<FVector>	Normals;
};

/**
* Container to hold overlapping corners. For a vertex, lists all the overlapping vertices
*/
struct FOverlappingCorners
{
	FOverlappingCorners() : bFinishedAdding(false)
	{
	}

	/* Resets, pre-allocates memory, marks all indices as not overlapping in preperation for calls to Add() */
	void Init(int32 NumIndices);

	/* Add overlapping indices pair */
	void Add(int32 Key, int32 Value);

	/* Sorts arrays, converts sets to arrays for sorting and to allow simple iterating code, prevents additional adding */
	void FinishAdding();

	/* Estimate memory allocated */
	uint32 GetAllocatedSize(void) const;

	/**
	* @return array of sorted overlapping indices including input 'Key', empty array for indices that have no overlaps.
	*/
	const std::vector<int32>& FindIfOverlapping(int32 Key) const
	{
		assert(bFinishedAdding);
		int32 ContainerIndex = IndexBelongsTo[Key];
		return (ContainerIndex != -1) ? Arrays[ContainerIndex] : EmptyArray;
	}

private:
	std::vector<int32> IndexBelongsTo;
	std::vector< std::vector<int32> > Arrays;
	std::vector< std::set<int32> > Sets;
	std::vector<int32> EmptyArray;
	bool bFinishedAdding;
};