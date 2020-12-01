#include "StaticMesh.h"
#include "log.h"
#include "D3D11RHI.h"
#include "MeshDescriptionOperations.h"
#include "Scene.h"
#include "World.h"
#include "FBXImporter.h"
#include "Material.h"
#include "MeshComponent.h"
#include "PrimitiveSceneInfo.h"

#include <vector>
#include <string>
#include <algorithm>
#include <assert.h>

void FStaticMeshLODResources::InitResources()
{
	VertexBuffers.PositionVertexBufferRHI = RHICreateVertexBuffer(VertexBuffers.PositionVertexBuffer.size() * sizeof(FVector), D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, VertexBuffers.PositionVertexBuffer.data()); 
	VertexBuffers.TangentsVertexBufferRHI = RHICreateVertexBuffer(VertexBuffers.TangentsVertexBuffer.size() * sizeof(Vector4), D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, VertexBuffers.TangentsVertexBuffer.data());
	VertexBuffers.TexCoordVertexBufferRHI = RHICreateVertexBuffer(VertexBuffers.TexCoordVertexBuffer.size() * sizeof(Vector2), D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, VertexBuffers.TexCoordVertexBuffer.data());

	VertexBuffers.TangentsVertexBufferSRV = RHICreateShaderResourceView(VertexBuffers.TangentsVertexBufferRHI.Get(), sizeof(Vector4) * 2, DXGI_FORMAT_R32G32B32A32_FLOAT);
	VertexBuffers.TexCoordVertexBufferSRV = RHICreateShaderResourceView(VertexBuffers.TexCoordVertexBufferRHI.Get(), sizeof(Vector2) * 2, DXGI_FORMAT_R32G32_FLOAT);

	IndexBuffer = CreateIndexBuffer(Indices.data(), sizeof(unsigned int) * Indices.size());


}

void FStaticMeshLODResources::ReleaseResources()
{
	if (IndexBuffer)
	{
		IndexBuffer->Release();
	}
}

void FStaticMeshVertexFactories::InitResources(const FStaticMeshLODResources& LodResources, const UStaticMesh* Parent)
{
	FLocalVertexFactory::FDataType Data;
	Data.PositionComponent = FVertexStreamComponent(LodResources.VertexBuffers.PositionVertexBufferRHI.Get(), 0, sizeof(FVector), DXGI_FORMAT_R32G32B32_FLOAT);
	Data.TangentBasisComponents[0] = FVertexStreamComponent(LodResources.VertexBuffers.TangentsVertexBufferRHI.Get(), 0, sizeof(FVector), DXGI_FORMAT_R32G32B32A32_FLOAT);
	Data.TangentBasisComponents[1] = FVertexStreamComponent(LodResources.VertexBuffers.TangentsVertexBufferRHI.Get(), 12, sizeof(FVector), DXGI_FORMAT_R32G32B32A32_FLOAT);
	Data.TextureCoordinates = FVertexStreamComponent(LodResources.VertexBuffers.TexCoordVertexBufferRHI.Get(), 0, sizeof(Vector2), DXGI_FORMAT_R32G32_FLOAT);
	Data.LightMapCoordinateComponent = FVertexStreamComponent(LodResources.VertexBuffers.TexCoordVertexBufferRHI.Get(), 8, sizeof(Vector2), DXGI_FORMAT_R32G32_FLOAT);
	Data.TangentsSRV = LodResources.VertexBuffers.TangentsVertexBufferSRV;
	Data.TextureCoordinatesSRV = LodResources.VertexBuffers.TexCoordVertexBufferSRV;
	VertexFactory.SetData(Data);
	VertexFactory.InitResource();
}

void FStaticMeshVertexFactories::ReleaseResources()
{

}

void FStaticMeshRenderData::InitResources(const UStaticMesh* Owner)
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

void FStaticMeshRenderData::Cache(UStaticMesh* Owner/*, const FStaticMeshLODSettings& LODSettings*/)
{
	FBXImporter Importer;
	Importer.BuildStaticMesh(*this, Owner);
}

FStaticMeshSceneProxy::FStaticMeshSceneProxy(UStaticMeshComponent* InComponent, bool bForceLODsShareStaticLighting)
	: FPrimitiveSceneProxy(InComponent)
	, RenderData(InComponent->GetStaticMesh()->RenderData.get())
{
	// Build the proxy's LOD data.
	bool bAnySectionCastsShadows = false;
	LODs.resize(RenderData->LODResources.size());
	const bool bLODsShareStaticLighting = true/*RenderData->bLODsShareStaticLighting || bForceLODsShareStaticLighting*/;
	for (uint32 LODIndex = 0; LODIndex < RenderData->LODResources.size(); LODIndex++)
	{
		LODs.push_back(new FLODInfo(InComponent, RenderData->LODVertexFactories, LODIndex, bLODsShareStaticLighting));
		FLODInfo* NewLODInfo = LODs.back();

		// Under certain error conditions an LOD's material will be set to 
		// DefaultMaterial. Ensure our material view relevance is set properly.
		const uint32 NumSections = NewLODInfo->Sections.size();
		for (uint32 SectionIndex = 0; SectionIndex < NumSections; ++SectionIndex)
		{
			const FLODInfo::FSectionInfo& SectionInfo = NewLODInfo->Sections[SectionIndex];
			bAnySectionCastsShadows |= RenderData->LODResources[LODIndex]->Sections[SectionIndex].bCastShadow;
			if (SectionInfo.Material == UMaterial::GetDefaultMaterial(MD_Surface))
			{
				//MaterialRelevance |= UMaterial::GetDefaultMaterial(MD_Surface)->GetRelevance(FeatureLevel);
			}
		}
	}
}

FStaticMeshSceneProxy::~FStaticMeshSceneProxy()
{
}

bool FStaticMeshSceneProxy::GetShadowMeshElement(int32 LODIndex, int32 BatchIndex, uint8 InDepthPriorityGroup, FMeshBatch& OutMeshBatch, bool bDitheredLODTransition) const
{
	return false;
}

bool FStaticMeshSceneProxy::GetMeshElement(
	int32 LODIndex, 
	int32 BatchIndex, 
	int32 SectionIndex, 
	uint8 InDepthPriorityGroup, 
	bool bUseSelectedMaterial, 
	bool bUseHoveredMaterial, 
	bool bAllowPreCulledIndices, 
	FMeshBatch& OutMeshBatch) const
{
	const FStaticMeshLODResources& LOD = *RenderData->LODResources[LODIndex];
	const FStaticMeshVertexFactories& VFs = *RenderData->LODVertexFactories[LODIndex];
	const FStaticMeshSection& Section = LOD.Sections[SectionIndex];

	const FLODInfo& ProxyLODInfo = *LODs[LODIndex];
	UMaterial* Material = ProxyLODInfo.Sections[SectionIndex].Material;
	OutMeshBatch.MaterialRenderProxy = Material->GetRenderProxy(false, false);

	FMeshBatchElement Element;
	Element.PrimitiveUniformBufferResource = &GetUniformBuffer();
	Element.FirstIndex = Section.FirstIndex;
	Element.NumPrimitives = Section.NumTriangles;
	//Element.MaterialIndex = Section.MaterialIndex;
	Element.IndexBuffer = RenderData->LODResources[LODIndex]->IndexBuffer.Get();
	OutMeshBatch.VertexFactory = &VFs.VertexFactory;
	OutMeshBatch.Elements[0] = Element;

	OutMeshBatch.LCI = &ProxyLODInfo;
	return true;
}

// void FStaticMeshSceneProxy::GetLightRelevance(const FLightSceneProxy* LightSceneProxy, bool& bDynamic, bool& bRelevant, bool& bLightMapped, bool& bShadowMapped) const
// {
// 	FPrimitiveSceneProxy::GetLightRelevance()
// }

void FStaticMeshSceneProxy::DrawStaticElements(FPrimitiveSceneInfo* PrimitiveSceneInfo)
{
	const FStaticMeshLODResources& LODModel = *RenderData->LODResources[0];
	for (uint32 SectionIndex = 0; SectionIndex < LODModel.Sections.size(); SectionIndex++)
	{
		const int32 NumBatches = GetNumMeshBatches();

		for (int32 BatchIndex = 0; BatchIndex < NumBatches; BatchIndex++)
		{
			FMeshBatch MeshBatch;

			if (GetMeshElement(0, BatchIndex, SectionIndex, /*PrimitiveDPG*/0, false, false, true, MeshBatch))
			{
				PrimitiveSceneInfo->StaticMeshes.push_back(new FStaticMesh(PrimitiveSceneInfo, MeshBatch));
				//PDI->DrawMesh(MeshBatch, FLT_MAX);
			}
		}
	}
}

FStaticMeshSceneProxy::FLODInfo::FLODInfo(const UStaticMeshComponent* InComponent, const std::vector<FStaticMeshVertexFactories*>& InLODVertexFactories, int32 InLODIndex, bool bLODsShareStaticLighting)
	: FLightCacheInterface(nullptr, nullptr)
	//, OverrideColorVertexBuffer(0)
	//, PreCulledIndexBuffer(NULL)
	, bUsesMeshModifyingMaterials(false)
{
	FStaticMeshRenderData* MeshRenderData = InComponent->GetStaticMesh()->RenderData.get();
	FStaticMeshLODResources& LODModel = *MeshRenderData->LODResources[InLODIndex];
	const FStaticMeshVertexFactories& VFs = *InLODVertexFactories[InLODIndex];

	Sections.clear();
	Sections.resize(MeshRenderData->LODResources[InLODIndex]->Sections.size());
	for (uint32 SectionIndex = 0; SectionIndex < LODModel.Sections.size(); SectionIndex++)
	{
		const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];
		FSectionInfo SectionInfo;

		// Determine the material applied to this element of the LOD.
		SectionInfo.Material = InComponent->GetMaterial(Section.MaterialIndex);

		// Store the element info.
		Sections.push_back(SectionInfo);
	}
}

FLightInteraction FStaticMeshSceneProxy::FLODInfo::GetInteraction(const FLightSceneProxy* LightSceneProxy) const
{
	// ask base class
	ELightInteractionType LightInteraction = GetStaticInteraction(LightSceneProxy, IrrelevantLights);

	if (LightInteraction != LIT_MAX)
	{
		return FLightInteraction(LightInteraction);
	}

	// Use dynamic lighting if the light doesn't have static lighting.
	return FLightInteraction::Dynamic();
}

UStaticMesh::UStaticMesh(class AActor* InOwner)
{
	RegisterMeshAttributes(MD);

	Material = UMaterial::GetDefaultMaterial(MD_Surface);
}

void UStaticMesh::InitResources()
{
	RenderData->InitResources(this);
}

void UStaticMesh::ReleaseResources()
{
	RenderData->ReleaseResources();
}

void UStaticMesh::PostLoad()
{
	CacheDerivedData();

	InitResources();
}

void UStaticMesh::GetRenderMeshDescription(const MeshDescription& InOriginalMeshDescription, MeshDescription& OutRenderMeshDescription)
{
	OutRenderMeshDescription = InOriginalMeshDescription;

	float ComparisonThreshold = 0.00002f;//BuildSettings->bRemoveDegenerates ? THRESH_POINTS_ARE_SAME : 0.0f;

	MeshDescriptionOperations::CreatePolygonNTB(OutRenderMeshDescription, 0.f);

	TMeshElementArray<MeshVertexInstance>& VertexInstanceArray = OutRenderMeshDescription.VertexInstances();
	std::vector<FVector>& Normals = OutRenderMeshDescription.VertexInstanceAttributes().GetAttributes<FVector>(MeshAttribute::VertexInstance::Normal);
	std::vector<FVector>& Tangents = OutRenderMeshDescription.VertexInstanceAttributes().GetAttributes<FVector>(MeshAttribute::VertexInstance::Tangent);
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

UMaterial* UStaticMesh::GetMaterial(int32 MaterialIndex) const
{
	return Material;
}

void UStaticMesh::CacheDerivedData()
{
	RenderData = std::make_unique<FStaticMeshRenderData>();
	RenderData->Cache(this/*, LODSettings*/);
}

void RegisterMeshAttributes(MeshDescription& MD)
{
	// Add basic vertex attributes
	MD.VertexAttributes().RegisterAttribute<FVector>(MeshAttribute::Vertex::Position, 1, FVector());
	MD.VertexAttributes().RegisterAttribute<float>(MeshAttribute::Vertex::CornerSharpness, 1, 0.0f);

	// Add basic vertex instance attributes
	MD.VertexInstanceAttributes().RegisterAttribute<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate, 1, Vector2());
	MD.VertexInstanceAttributes().RegisterAttribute<FVector>(MeshAttribute::VertexInstance::Normal, 1, FVector());
	MD.VertexInstanceAttributes().RegisterAttribute<FVector>(MeshAttribute::VertexInstance::Tangent, 1, FVector());
	MD.VertexInstanceAttributes().RegisterAttribute<float>(MeshAttribute::VertexInstance::BinormalSign, 1, 0.0f);
	MD.VertexInstanceAttributes().RegisterAttribute<Vector4>(MeshAttribute::VertexInstance::Color, 1, Vector4(1.0f));

	// Add basic edge attributes
	MD.EdgeAttributes().RegisterAttribute<bool>(MeshAttribute::Edge::IsHard, 1, false);
	MD.EdgeAttributes().RegisterAttribute<bool>(MeshAttribute::Edge::IsUVSeam, 1, false);
	MD.EdgeAttributes().RegisterAttribute<float>(MeshAttribute::Edge::CreaseSharpness, 1, 0.0f);

	// Add basic polygon attributes
	MD.PolygonAttributes().RegisterAttribute<FVector>(MeshAttribute::Polygon::Normal, 1, FVector());
	MD.PolygonAttributes().RegisterAttribute<FVector>(MeshAttribute::Polygon::Tangent, 1, FVector());
	MD.PolygonAttributes().RegisterAttribute<FVector>(MeshAttribute::Polygon::Binormal, 1, FVector());
	MD.PolygonAttributes().RegisterAttribute<FVector>(MeshAttribute::Polygon::Center, 1, FVector());

	// Add basic polygon group attributes
	MD.PolygonGroupAttributes().RegisterAttribute<std::string>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName, 1, std::string()); //The unique key to match the mesh material slot
}
