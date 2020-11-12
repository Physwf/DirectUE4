#include "SkeletalMeshRenderData.h"
#include "SkeletalMeshModel.h"
#include "SkeletalMesh.h"

struct ESkeletalMeshVertexFlags
{
	enum
	{
		None = 0x0,
		UseFullPrecisionUVs = 0x1,
		HasVertexColors = 0x2,
		UseHighPrecisionTangentBasis = 0x4
	};
};


void SkeletalMeshLODRenderData::InitResources()
{
	StaticVertexBuffers.PositionVertexBufferRHI = CreateVertexBuffer(false, StaticVertexBuffers.PositionVertexBuffer.size() * sizeof(Vector), StaticVertexBuffers.PositionVertexBuffer.data());
	StaticVertexBuffers.StaticMeshVertexBufferRHI = CreateVertexBuffer(false, StaticVertexBuffers.StaticMeshVertexBuffer.size() * sizeof(float), StaticVertexBuffers.StaticMeshVertexBuffer.data());
}

void SkeletalMeshLODRenderData::BuildFromLODModel(const SkeletalMeshLODModel* ImportedModel,uint32 BuildFlags)
{
	bool bUseFullPrecisionUVs = (BuildFlags & ESkeletalMeshVertexFlags::UseFullPrecisionUVs) != 0;
	bool bUseHighPrecisionTangentBasis = (BuildFlags & ESkeletalMeshVertexFlags::UseHighPrecisionTangentBasis) != 0;
	bool bHasVertexColors = (BuildFlags & ESkeletalMeshVertexFlags::HasVertexColors) != 0;

	// Copy required info from source sections
	RenderSections.clear();
	for (uint32 SectionIndex = 0; SectionIndex < ImportedModel->Sections.size(); SectionIndex++)
	{
		const SkeletalMeshSection& ModelSection = ImportedModel->Sections[SectionIndex];

		SkeletalMeshRenderSection NewRenderSection;
		NewRenderSection.MaterialIndex = ModelSection.MaterialIndex;
		NewRenderSection.BaseIndex = ModelSection.BaseIndex;
		NewRenderSection.NumTriangles = ModelSection.NumTriangles;
		//NewRenderSection.bRecomputeTangent = ModelSection.bRecomputeTangent;
		//NewRenderSection.bCastShadow = ModelSection.bCastShadow;
		NewRenderSection.BaseVertexIndex = ModelSection.BaseVertexIndex;
		//NewRenderSection.ClothMappingData = ModelSection.ClothMappingData;
		NewRenderSection.BoneMap = ModelSection.BoneMap;
		NewRenderSection.NumVertices = ModelSection.NumVertices;
		NewRenderSection.MaxBoneInfluences = ModelSection.MaxBoneInfluences;
		//NewRenderSection.CorrespondClothAssetIndex = ModelSection.CorrespondClothAssetIndex;
		//NewRenderSection.ClothingData = ModelSection.ClothingData;
		//NewRenderSection.DuplicatedVerticesBuffer.Init(ModelSection.NumVertices, ModelSection.OverlappingVertices);
		//NewRenderSection.bDisabled = ModelSection.bDisabled;
		RenderSections.push_back(NewRenderSection);
	}

	std::vector<SoftSkinVertex> Vertices;
	ImportedModel->GetVertices(Vertices);

	// match UV and tangent precision for mesh vertex buffer to setting from parent mesh
	//StaticVertexBuffers.StaticMeshVertexBuffer.SetUseFullPrecisionUVs(bUseFullPrecisionUVs);
	//StaticVertexBuffers.StaticMeshVertexBuffer.SetUseHighPrecisionTangentBasis(bUseHighPrecisionTangentBasis);

	// init vertex buffer with the vertex array
	StaticVertexBuffers.PositionVertexBuffer.resize(Vertices.size());
	size_t StaticMeshVertexBufferStride = 3 * sizeof(Vector) / sizeof(float) + ImportedModel->NumTexCoords * sizeof(Vector2) / sizeof(float);
	StaticVertexBuffers.StaticMeshVertexBuffer.resize(Vertices.size() * StaticMeshVertexBufferStride);

	for (uint32 i = 0; i < Vertices.size(); i++)
	{
		StaticVertexBuffers.PositionVertexBuffer[i] = Vertices[i].Position;
		Vector* TangentStart = (Vector*)(StaticVertexBuffers.StaticMeshVertexBuffer.data() + i * StaticMeshVertexBufferStride);
		Vector2* TexCoordsStart = (Vector2*)(StaticVertexBuffers.StaticMeshVertexBuffer.data() + i * StaticMeshVertexBufferStride + 3 * sizeof(Vector) / sizeof(float));
		TangentStart[0] = Vertices[i].TangentX;
		TangentStart[1] = Vertices[i].TangentY;
		TangentStart[2] = Vertices[i].TangentZ;
		//StaticVertexBuffers.StaticMeshVertexBuffer[i].SetVertexTangents(i, Vertices[i].TangentX, Vertices[i].TangentY, Vertices[i].TangentZ);
		for (uint32 j = 0; j < ImportedModel->NumTexCoords; j++)
		{
			//StaticVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, j, Vertices[i].UVs[j]);
			TexCoordsStart[j] = Vertices[i].UVs[j];
		}
	}

	// Init skin weight buffer
	//SkinWeightVertexBuffer.SetNeedsCPUAccess(true);
	SkinWeightVertexBuffer.SetHasExtraBoneInfluences(ImportedModel->DoSectionsNeedExtraBoneInfluences());
	SkinWeightVertexBuffer.Init(Vertices);

	// Init the color buffer if this mesh has vertex colors.
	if (bHasVertexColors && Vertices.size() > 0 /*&& StaticVertexBuffers.ColorVertexBuffer.GetAllocatedSize() == 0*/)
	{
		//StaticVertexBuffers.ColorVertexBuffer.InitFromColorArray(&Vertices[0].Color, Vertices.Num(), sizeof(FSoftSkinVertex));
		StaticVertexBuffers.ColorVertexBuffer.resize(Vertices.size());
		for (uint32 i = 0; i < Vertices.size(); i++)
		{
			StaticVertexBuffers.ColorVertexBuffer[i] = Vertices[i].Color;
		}
	}

// 	if (ImportedModel->HasClothData())
// 	{
// 		TArray<FMeshToMeshVertData> MappingData;
// 		TArray<uint64> ClothIndexMapping;
// 		ImportedModel->GetClothMappingData(MappingData, ClothIndexMapping);
// 		ClothVertexBuffer.Init(MappingData, ClothIndexMapping);
// 	}

	//const uint8 DataTypeSize = (ImportedModel->NumVertices < MAX_uint16) ? sizeof(uint16) : sizeof(uint32);

	//MultiSizeIndexContainer.RebuildIndexBuffer(DataTypeSize, ImportedModel->IndexBuffer);

	IndexBuffer.resize(ImportedModel->IndexBuffer.size());
	memcpy(IndexBuffer.data(), ImportedModel->IndexBuffer.data(), ImportedModel->IndexBuffer.size() * sizeof(uint32));

	//TArray<uint32> BuiltAdjacencyIndices;
	//IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	//MeshUtilities.BuildSkeletalAdjacencyIndexBuffer(Vertices, ImportedModel->NumTexCoords, ImportedModel->IndexBuffer, BuiltAdjacencyIndices);
	//AdjacencyMultiSizeIndexContainer.RebuildIndexBuffer(DataTypeSize, BuiltAdjacencyIndices);

	// MorphTargetVertexInfoBuffers are created in InitResources

	ActiveBoneIndices = ImportedModel->ActiveBoneIndices;
	RequiredBones = ImportedModel->RequiredBones;
}

void SkeletalMeshRenderData::InitResources(/*bool bNeedsVertexColors, TArray<UMorphTarget*>& InMorphTargets*/)
{
	for (uint32 LODIndex = 0; LODIndex < LODRenderData.size(); LODIndex++)
	{
		SkeletalMeshLODRenderData& RenderData = *LODRenderData[LODIndex];

		if (RenderData.GetNumVertices() > 0)
		{
			RenderData.InitResources(/*bNeedsVertexColors, LODIndex, InMorphTargets*/);
		}
	}
}

void SkeletalMeshRenderData::Cache(SkeletalMesh* Owner)
{
	SkeletalMeshModel* SkelMeshModel = Owner->GetImportedModel();

	//uint32 VertexBufferBuildFlags = Owner->GetVertexBufferFlags();
	uint32 VertexBufferBuildFlags = 0;
	for (uint32 LODIndex = 0; LODIndex < SkelMeshModel->LODModels.size(); LODIndex++)
	{
		SkeletalMeshLODModel& LODModel = *SkelMeshModel->LODModels[LODIndex];
		SkeletalMeshLODRenderData* LODData = new SkeletalMeshLODRenderData();
		LODRenderData.push_back(LODData);
		LODData->BuildFromLODModel(&LODModel, VertexBufferBuildFlags);
	}
}