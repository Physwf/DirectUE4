#include "StaticMesh.h"
#include "log.h"
#include "D3D11RHI.h"
#include <vector>
#include <string>
#include <algorithm>
#include <assert.h>
#include "MeshDescriptionOperations.h"
#include "Scene.h"
#include "World.h"
#include "FBXImporter.h"
#include "PrimitiveUniformBufferParameters.h"

void FStaticMeshLODResources::InitResources()
{
	VertexBuffers.PositionVertexBufferRHI = CreateVertexBuffer(false, VertexBuffers.PositionVertexBuffer.size() * sizeof(Vector), VertexBuffers.PositionVertexBuffer.data());
	VertexBuffers.TangentsVertexBufferRHI = CreateVertexBuffer(false, VertexBuffers.TangentsVertexBuffer.size() * sizeof(Vector4), VertexBuffers.TangentsVertexBuffer.data());
	VertexBuffers.TexCoordVertexBufferRHI = CreateVertexBuffer(false, VertexBuffers.TexCoordVertexBuffer.size() * sizeof(Vector2), VertexBuffers.TexCoordVertexBuffer.data());


	IndexBuffer = CreateIndexBuffer(Indices.data(), sizeof(unsigned int) * Indices.size());
}

void FStaticMeshLODResources::ReleaseResources()
{
	if (IndexBuffer)
	{
		IndexBuffer->Release();
	}
}

void FStaticMeshVertexFactories::InitResources(const FStaticMeshLODResources& LodResources, const StaticMesh* Parent)
{
	FLocalVertexFactory::FDataType Data;
	Data.PositionComponent = FVertexStreamComponent(LodResources.VertexBuffers.PositionVertexBufferRHI.Get(), 0, sizeof(Vector), DXGI_FORMAT_R32G32B32_FLOAT);
	Data.TangentBasisComponents[0] = FVertexStreamComponent(LodResources.VertexBuffers.TangentsVertexBufferRHI.Get(), 0, sizeof(Vector), DXGI_FORMAT_R32G32B32A32_FLOAT);
	Data.TangentBasisComponents[1] = FVertexStreamComponent(LodResources.VertexBuffers.TangentsVertexBufferRHI.Get(), 12, sizeof(Vector), DXGI_FORMAT_R32G32B32A32_FLOAT);
	Data.TextureCoordinates = FVertexStreamComponent(LodResources.VertexBuffers.TexCoordVertexBufferRHI.Get(), 0, sizeof(Vector2), DXGI_FORMAT_R32G32_FLOAT);
	Data.LightMapCoordinateComponent = FVertexStreamComponent(LodResources.VertexBuffers.TexCoordVertexBufferRHI.Get(), 8, sizeof(Vector2), DXGI_FORMAT_R32G32_FLOAT);
	VertexFactory.SetData(Data);
	VertexFactory.InitResource();
}

void FStaticMeshVertexFactories::ReleaseResources()
{

}

void FStaticMeshRenderData::InitResources(const StaticMesh* Owner)
{
	for (uint32 LODIndex = 0; LODIndex < LODResources.size(); ++LODIndex)
	{
		LODResources[LODIndex]->InitResources();
		LODVertexFactories[LODIndex]->InitResources(*LODResources[LODIndex], Owner);
	}
}

void FStaticMeshRenderData::ReleaseResources()
{
	for (uint32 LODIndex = 0; LODIndex < LODResources.size(); ++LODIndex)
	{
		LODResources[LODIndex]->ReleaseResources();
		LODVertexFactories[LODIndex]->ReleaseResources();
	}
}

void FStaticMeshRenderData::AllocateLODResources(int32 NumLODs)
{
	while ((int32)LODResources.size() < NumLODs)
	{
		LODResources.push_back(new FStaticMeshLODResources) ;
		LODVertexFactories.push_back(new FStaticMeshVertexFactories());
	}
}

void FStaticMeshRenderData::Cache(StaticMesh* Owner/*, const FStaticMeshLODSettings& LODSettings*/)
{
	FBXImporter Importer;
	Importer.BuildStaticMesh(*this, Owner);
}

StaticMesh::StaticMesh()
{
	RegisterMeshAttributes(MD);
}


void StaticMesh::InitResources()
{
	RenderData->InitResources(this);

	//FPrimitiveUniformShaderParameters PU = GetPrimitiveUniformShaderParameters(
		//GetWorldMatrix(),
		//Position,
		//Bounds,
		//LocalBounds,
		///*bReceivesDecals*/false,
		///*HasDistanceFieldRepresentation()*/false,
		///*HasDynamicIndirectShadowCasterRepresentation()*/false,
		///*UseSingleSampleShadowFromStationaryLights()*/false,
		///*Scene->HasPrecomputedVolumetricLightmap_RenderThread()*/false,
		///*UseEditorDepthTest()*/false,
		///*GetLightingChannelMask()*/0,
		///*LpvBiasMultiplier*/ 1.f
	//);;
	//PrimitiveUniformBuffer = CreateConstantBuffer(false,sizeof(PU),&PU);
}

void StaticMesh::ReleaseResources()
{
	RenderData->ReleaseResources();
}

void StaticMesh::UpdateUniformBuffer()
{
#if 0
	if (bTransformDirty)
	{
		FPrimitiveUniformShaderParameters PU = GetPrimitiveUniformShaderParameters(
			GetWorldMatrix(),
			Position,
			//Bounds,
			//LocalBounds,
			/*bReceivesDecals*/false,
			/*HasDistanceFieldRepresentation()*/false,
			/*HasDynamicIndirectShadowCasterRepresentation()*/false,
			/*UseSingleSampleShadowFromStationaryLights()*/false,
			/*Scene->HasPrecomputedVolumetricLightmap_RenderThread()*/false,
			/*UseEditorDepthTest()*/false,
			/*GetLightingChannelMask()*/0,
			/*LpvBiasMultiplier*/ 1.f
		);
		D3D11DeviceContext->UpdateSubresource(PrimitiveUniformBuffer, 0, 0, &PU, 0, 0);
		bTransformDirty = false;
	}
#endif
}

bool StaticMesh::GetMeshElement(int BatchIndex, int SectionIndex, FMeshBatch& OutMeshBatch)
{
	const FStaticMeshLODResources& LOD = *RenderData->LODResources[0];
	const FStaticMeshVertexFactories& VFs = *RenderData->LODVertexFactories[0];
	const StaticMeshSection& Section = LOD.Sections[SectionIndex];

	FMeshBatchElement Element;
	Element.PrimitiveUniformBufferResource = &GetUniformBuffer();
	Element.FirstIndex = Section.FirstIndex;
	Element.NumPrimitives = Section.NumTriangles;
	//Element.MaterialIndex = Section.MaterialIndex;
	Element.IndexBuffer = RenderData->LODResources[0]->IndexBuffer.Get();
	OutMeshBatch.VertexFactory = &VFs.VertexFactory;
	OutMeshBatch.Elements.emplace_back(Element);
	return true;
}

void StaticMesh::DrawStaticElements()
{
	const FStaticMeshLODResources& LODModel = *RenderData->LODResources[0];

	for (size_t SectionIndex = 0; SectionIndex < LODModel.Sections.size();++SectionIndex)
	{
		FMeshBatch MB;
		for (int BatchIndex = 0; BatchIndex < GetNumberBatches(); ++BatchIndex)
		{
			if (GetMeshElement(BatchIndex, SectionIndex, MB))
			{
				StaticMeshes.push_back(new FStaticMesh(this,MB));
			}
		}
	}
}

void StaticMesh::PostLoad()
{
	CacheDerivedData();

	InitResources();
}

void StaticMesh::GetRenderMeshDescription(const MeshDescription& InOriginalMeshDescription, MeshDescription& OutRenderMeshDescription)
{
	OutRenderMeshDescription = InOriginalMeshDescription;

	float ComparisonThreshold = 0.00002f;//BuildSettings->bRemoveDegenerates ? THRESH_POINTS_ARE_SAME : 0.0f;

	MeshDescriptionOperations::CreatePolygonNTB(OutRenderMeshDescription, 0.f);

	TMeshElementArray<MeshVertexInstance>& VertexInstanceArray = OutRenderMeshDescription.VertexInstances();
	std::vector<Vector>& Normals = OutRenderMeshDescription.VertexInstanceAttributes().GetAttributes<Vector>(MeshAttribute::VertexInstance::Normal);
	std::vector<Vector>& Tangents = OutRenderMeshDescription.VertexInstanceAttributes().GetAttributes<Vector>(MeshAttribute::VertexInstance::Tangent);
	std::vector<float>& BinormalSigns = OutRenderMeshDescription.VertexInstanceAttributes().GetAttributes<float>(MeshAttribute::VertexInstance::BinormalSign);

	MeshDescriptionOperations::FindOverlappingCorners(OverlappingCorners, OutRenderMeshDescription, ComparisonThreshold);

	uint32 TangentOptions = MeshDescriptionOperations::ETangentOptions::BlendOverlappingNormals;

	bool bHasAllNormals = true;
	bool bHasAllTangents = true;

	for (const int VertexInstanceID : VertexInstanceArray.GetElementIDs())
	{
		bHasAllNormals &= !Normals[VertexInstanceID].IsNearlyZero();
		bHasAllTangents &= !Tangents[VertexInstanceID].IsNearlyZero();
	}

	if (!bHasAllNormals)
	{

	}

	MeshDescriptionOperations::CreateMikktTangents(OutRenderMeshDescription, (MeshDescriptionOperations::ETangentOptions)TangentOptions);

	
	std::vector<std::vector<Vector2>>& VertexInstanceUVs = OutRenderMeshDescription.VertexInstanceAttributes().GetAttributesSet<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate);
	int32 NumIndices = (int32)VertexInstanceUVs.size();
	//Verify the src light map channel
// 	if (BuildSettings->SrcLightmapIndex >= NumIndices)
// 	{
// 		BuildSettings->SrcLightmapIndex = 0;
// 	}
	//Verify the destination light map channel
// 	if (BuildSettings->DstLightmapIndex >= NumIndices)
// 	{
// 		//Make sure we do not add illegal UV Channel index
// 		if (BuildSettings->DstLightmapIndex >= MAX_MESH_TEXTURE_COORDS_MD)
// 		{
// 			BuildSettings->DstLightmapIndex = MAX_MESH_TEXTURE_COORDS_MD - 1;
// 		}
// 
// 		//Add some unused UVChannel to the mesh description for the lightmapUVs
// 		VertexInstanceUVs.SetNumIndices(BuildSettings->DstLightmapIndex + 1);
// 		BuildSettings->DstLightmapIndex = NumIndices;
// 	}
	VertexInstanceUVs.resize(2);
	VertexInstanceUVs[1].resize(VertexInstanceUVs[0].size());
	MeshDescriptionOperations::CreateLightMapUVLayout(OutRenderMeshDescription,
		/*BuildSettings->SrcLightmapIndex,*/0,
		/*BuildSettings->DstLightmapIndex,*/1,
		/*BuildSettings->MinLightmapResolution,*/64,
		/*(MeshDescriptionOperations::ELightmapUVVersion)(StaticMesh->LightmapUVVersion),*/(MeshDescriptionOperations::ELightmapUVVersion)4,
		OverlappingCorners);
}

void StaticMesh::CacheDerivedData()
{
	RenderData = std::make_unique<FStaticMeshRenderData>();
	RenderData->Cache(this/*, LODSettings*/);
}

void RegisterMeshAttributes(MeshDescription& MD)
{
	// Add basic vertex attributes
	MD.VertexAttributes().RegisterAttribute<Vector>(MeshAttribute::Vertex::Position, 1, Vector());
	MD.VertexAttributes().RegisterAttribute<float>(MeshAttribute::Vertex::CornerSharpness, 1, 0.0f);

	// Add basic vertex instance attributes
	MD.VertexInstanceAttributes().RegisterAttribute<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate, 1, Vector2());
	MD.VertexInstanceAttributes().RegisterAttribute<Vector>(MeshAttribute::VertexInstance::Normal, 1, Vector());
	MD.VertexInstanceAttributes().RegisterAttribute<Vector>(MeshAttribute::VertexInstance::Tangent, 1, Vector());
	MD.VertexInstanceAttributes().RegisterAttribute<float>(MeshAttribute::VertexInstance::BinormalSign, 1, 0.0f);
	MD.VertexInstanceAttributes().RegisterAttribute<Vector4>(MeshAttribute::VertexInstance::Color, 1, Vector4(1.0f));

	// Add basic edge attributes
	MD.EdgeAttributes().RegisterAttribute<bool>(MeshAttribute::Edge::IsHard, 1, false);
	MD.EdgeAttributes().RegisterAttribute<bool>(MeshAttribute::Edge::IsUVSeam, 1, false);
	MD.EdgeAttributes().RegisterAttribute<float>(MeshAttribute::Edge::CreaseSharpness, 1, 0.0f);

	// Add basic polygon attributes
	MD.PolygonAttributes().RegisterAttribute<Vector>(MeshAttribute::Polygon::Normal, 1, Vector());
	MD.PolygonAttributes().RegisterAttribute<Vector>(MeshAttribute::Polygon::Tangent, 1, Vector());
	MD.PolygonAttributes().RegisterAttribute<Vector>(MeshAttribute::Polygon::Binormal, 1, Vector());
	MD.PolygonAttributes().RegisterAttribute<Vector>(MeshAttribute::Polygon::Center, 1, Vector());

	// Add basic polygon group attributes
	MD.PolygonGroupAttributes().RegisterAttribute<std::string>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName, 1, std::string()); //The unique key to match the mesh material slot
}
