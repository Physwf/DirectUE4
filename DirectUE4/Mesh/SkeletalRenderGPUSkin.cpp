#include "SkeletalRenderGPUSkin.h"
#include "MeshComponent.h"
#include "SceneView.h"
#include "SkeletalMeshRenderData.h"
#include "MeshComponent.h"
#include "UnrealTemplates.h"

FSkeletalMeshObjectGPUSkin::FSkeletalMeshObjectGPUSkin(USkinnedMeshComponent* InMeshComponent, FSkeletalMeshRenderData* InSkelMeshRenderData)
	: FSkeletalMeshObject(InMeshComponent, InSkelMeshRenderData)
// 	, DynamicData(NULL)
// 	, bNeedsUpdateDeferred(false)
// 	, bMorphNeedsUpdateDeferred(false)
// 	, bMorphResourcesInitialized(false)
// 	, LastBoneTransformRevisionNumber(0)
{
	LODs.clear();
	LODs.reserve(SkeletalMeshRenderData->LODRenderData.size());
	for (uint32 LODIndex = 0; LODIndex < SkeletalMeshRenderData->LODRenderData.size(); LODIndex++)
	{
		LODs.push_back(FSkeletalMeshObjectLOD(SkeletalMeshRenderData, LODIndex));
	}

	InitResources(InMeshComponent);
}

FSkeletalMeshObjectGPUSkin::~FSkeletalMeshObjectGPUSkin()
{

}

void FSkeletalMeshObjectGPUSkin::InitResources(USkinnedMeshComponent* InMeshComponent)
{
	for (uint32 LODIndex = 0; LODIndex < LODs.size(); LODIndex++)
	{
		FSkeletalMeshObjectLOD& SkelLOD = LODs[LODIndex];
		const FSkelMeshObjectLODInfo& MeshLODInfo = LODInfo[LODIndex];

		FSkelMeshComponentLODInfo* CompLODInfo = nullptr;
		if (IsValidIndex(InMeshComponent->LODInfo,LODIndex))
		{
			CompLODInfo = &InMeshComponent->LODInfo[LODIndex];
		}

		SkelLOD.InitResources(MeshLODInfo, CompLODInfo);
	}
}

void FSkeletalMeshObjectGPUSkin::ReleaseResources()
{
	for (uint32 LODIndex = 0; LODIndex < LODs.size(); LODIndex++)
	{
		FSkeletalMeshObjectLOD& SkelLOD = LODs[LODIndex];
		SkelLOD.ReleaseResources();
	}
}

const FVertexFactory* FSkeletalMeshObjectGPUSkin::GetSkinVertexFactory(const FSceneView* View, int32 LODIndex, int32 ChunkIdx) const
{
	assert(IsValidIndex(LODs,LODIndex));
	//assert(DynamicData);

	const FSkelMeshObjectLODInfo& MeshLODInfo = LODInfo[LODIndex];
	const FSkeletalMeshObjectLOD& LOD = LODs[LODIndex];

	// If the GPU skinning cache was used, return the passthrough vertex factory
// 	if (SkinCacheEntry && FGPUSkinCache::IsEntryValid(SkinCacheEntry, ChunkIdx))
// 	{
// 		return LOD.GPUSkinVertexFactories.PassthroughVertexFactories[ChunkIdx].Get();
// 	}


	return LOD.GPUSkinVertexFactories.VertexFactories[ChunkIdx].get();
}

int32 FSkeletalMeshObjectGPUSkin::GetLOD() const
{
// 	if (DynamicData)
// 	{
// 		return DynamicData->LODIndex;
// 	}
// 	else
	{
		return 0;
	}
}

template<class VertexFactoryType>
void InitGPUSkinVertexFactoryComponents(typename VertexFactoryType::FDataType* VertexFactoryData,
	const FSkeletalMeshObjectGPUSkin::FVertexFactoryBuffers& VertexBuffers, const VertexFactoryType* VertexFactory)
{
	//typedef TGPUSkinVertexBase BaseVertexType;
	//typedef TSkinWeightInfo<VertexFactoryType::HasExtraBoneInfluences> WeightInfoType;

	//position
	//VertexBuffers.StaticVertexBuffers->PositionVertexBuffer.BindPositionVertexBuffer(VertexFactory, *VertexFactoryData);
	VertexFactoryData->PositionComponent = FVertexStreamComponent(
		VertexBuffers.StaticVertexBuffers->PositionVertexBufferRHI.Get(),
		0,
		sizeof(FVector),
		DXGI_FORMAT_R32G32B32_FLOAT
	);
	// tangents
	//VertexBuffers.StaticVertexBuffers->StaticMeshVertexBuffer.BindTangentVertexBuffer(VertexFactory, *VertexFactoryData);
	//VertexBuffers.StaticVertexBuffers->StaticMeshVertexBuffer.BindTexCoordVertexBuffer(VertexFactory, *VertexFactoryData);
	VertexFactoryData->TangentBasisComponents[0] = FVertexStreamComponent(
		VertexBuffers.StaticVertexBuffers->TangentsVertexBufferRHI.Get(),
		0,
		sizeof(Vector4),
		DXGI_FORMAT_R32G32B32A32_FLOAT
	);
	VertexFactoryData->TangentBasisComponents[1] = FVertexStreamComponent(
		VertexBuffers.StaticVertexBuffers->TangentsVertexBufferRHI.Get(),
		sizeof(Vector4),
		sizeof(Vector4),
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		4
	);
	VertexFactoryData->TextureCoordinates = FVertexStreamComponent(
		VertexBuffers.StaticVertexBuffers->TexCoordVertexBufferRHI.Get(),
		0,
		sizeof(Vector2),
		DXGI_FORMAT_R32G32_FLOAT,
		4
	);
	// bone indices
	VertexFactoryData->BoneIndices = FVertexStreamComponent(
		VertexBuffers.SkinWeightVertexBuffer->WeightVertexBufferRHI.Get(), 
		/*STRUCT_OFFSET(WeightInfoType, InfluenceBones)*/0, 
		/*VertexBuffers.SkinWeightVertexBuffer->GetStride()*/4,
		DXGI_FORMAT_R8G8B8A8_UINT);
	// bone weights
	VertexFactoryData->BoneWeights = FVertexStreamComponent(
		VertexBuffers.SkinWeightVertexBuffer->WeightVertexBufferRHI.Get(),
		/*STRUCT_OFFSET(WeightInfoType, InfluenceWeights)*/4, 
		/*VertexBuffers.SkinWeightVertexBuffer->GetStride()*/4,
		DXGI_FORMAT_R8G8B8A8_UNORM);

// 	if (VertexFactoryType::HasExtraBoneInfluences)
// 	{
// 		// Extra streams for bone indices & weights
// 		VertexFactoryData->ExtraBoneIndices = FVertexStreamComponent(
// 			VertexBuffers.SkinWeightVertexBuffer, STRUCT_OFFSET(WeightInfoType, InfluenceBones) + 4, VertexBuffers.SkinWeightVertexBuffer->GetStride(), VET_UByte4);
// 		VertexFactoryData->ExtraBoneWeights = FVertexStreamComponent(
// 			VertexBuffers.SkinWeightVertexBuffer, STRUCT_OFFSET(WeightInfoType, InfluenceWeights) + 4, VertexBuffers.SkinWeightVertexBuffer->GetStride(), VET_UByte4N);
// 	}

	// Color data may be NULL
// 	if (VertexBuffers.ColorVertexBuffer != NULL &&
// 		VertexBuffers.ColorVertexBuffer->IsInitialized())
// 	{
// 		//Color
// 		VertexBuffers.ColorVertexBuffer->BindColorVertexBuffer(VertexFactory, *VertexFactoryData);
// 	}
// 	else
// 	{
// 		VertexFactoryData->ColorComponentsSRV = nullptr;
// 		VertexFactoryData->ColorIndexMask = 0;
// 	}
}

template <class VertexFactoryType>
class TDynamicUpdateVertexFactoryData
{
public:
	TDynamicUpdateVertexFactoryData(
		VertexFactoryType* InVertexFactory,
		const FSkeletalMeshObjectGPUSkin::FVertexFactoryBuffers& InVertexBuffers)
		: VertexFactory(InVertexFactory)
		, VertexBuffers(InVertexBuffers)
	{}
	VertexFactoryType* VertexFactory;
	const FSkeletalMeshObjectGPUSkin::FVertexFactoryBuffers VertexBuffers;

};

template <class VertexFactoryTypeBase, class VertexFactoryType>
static VertexFactoryType* CreateVertexFactory(std::vector<std::unique_ptr<VertexFactoryTypeBase>>& VertexFactories,
	const FSkeletalMeshObjectGPUSkin::FVertexFactoryBuffers& InVertexBuffers
)
{
	VertexFactoryType* VertexFactory = new VertexFactoryType(InVertexBuffers.NumVertices);
	VertexFactories.push_back(std::unique_ptr<VertexFactoryTypeBase>(VertexFactory));

	// Setup the update data for enqueue
	TDynamicUpdateVertexFactoryData<VertexFactoryType> VertexUpdateData(VertexFactory, InVertexBuffers);

	// update vertex factory components and sync it
	//ENQUEUE_RENDER_COMMAND(InitGPUSkinVertexFactory)(
		//[VertexUpdateData](FRHICommandList& CmdList)
		//{
			typename VertexFactoryType::FDataType Data;
			InitGPUSkinVertexFactoryComponents<VertexFactoryType>(&Data, VertexUpdateData.VertexBuffers, VertexUpdateData.VertexFactory);
			VertexUpdateData.VertexFactory->SetData(Data);
		//}
	//);

	// init rendering resource	
	//BeginInitResource(VertexFactory);

	return VertexFactory;
}

void FSkeletalMeshObjectGPUSkin::FVertexFactoryData::InitVertexFactories(const FVertexFactoryBuffers& VertexBuffers, const std::vector<FSkeletalMeshRenderSection>& Sections)
{
	VertexFactories.clear();
	VertexFactories.reserve(Sections.size());
	{
		for (uint32 FactoryIdx = 0; FactoryIdx < Sections.size(); ++FactoryIdx)
		{
			//if (VertexBuffers.SkinWeightVertexBuffer->HasExtraBoneInfluences())
			{
				//TGPUSkinVertexFactory<true>* VertexFactory = CreateVertexFactory< FGPUBaseSkinVertexFactory, TGPUSkinVertexFactory<true> >(VertexFactories, VertexBuffers);
				//CreatePassthroughVertexFactory<TGPUSkinVertexFactory<true>>(InFeatureLevel, PassthroughVertexFactories, VertexFactory);
			}
			//else
			{
				TGPUSkinVertexFactory<false>* VertexFactory = CreateVertexFactory< FGPUBaseSkinVertexFactory, TGPUSkinVertexFactory<false> >(VertexFactories, VertexBuffers);
				//CreatePassthroughVertexFactory<TGPUSkinVertexFactory<false>>(InFeatureLevel, PassthroughVertexFactories, VertexFactory);
			}
		}
	}
}

void FSkeletalMeshObjectGPUSkin::FVertexFactoryData::ReleaseVertexFactories()
{

}

void FSkeletalMeshObjectGPUSkin::FSkeletalMeshObjectLOD::InitResources(const FSkelMeshObjectLODInfo& MeshLODInfo, FSkelMeshComponentLODInfo* CompLODInfo)
{
	assert(SkelMeshRenderData);
	assert(IsValidIndex(SkelMeshRenderData->LODRenderData,LODIndex));

	FSkeletalMeshLODRenderData& LODData = *SkelMeshRenderData->LODRenderData[LODIndex];

	MeshObjectWeightBuffer = &LODData.SkinWeightVertexBuffer;

	FVertexFactoryBuffers VertexBuffers;
	GetVertexBuffers(VertexBuffers, LODData);

	GPUSkinVertexFactories.InitVertexFactories(VertexBuffers, LODData.RenderSections);
}

void FSkeletalMeshObjectGPUSkin::FSkeletalMeshObjectLOD::ReleaseResources()
{
	GPUSkinVertexFactories.ReleaseVertexFactories();

	// Release APEX cloth vertex factory
	//GPUSkinVertexFactories.ReleaseAPEXClothVertexFactories();
}

void FSkeletalMeshObjectGPUSkin::FSkeletalMeshObjectLOD::GetVertexBuffers(FVertexFactoryBuffers& OutVertexBuffers, FSkeletalMeshLODRenderData& LODData)
{
	OutVertexBuffers.StaticVertexBuffers = &LODData.StaticVertexBuffers;
	//OutVertexBuffers.ColorVertexBuffer = MeshObjectColorBuffer;
	OutVertexBuffers.SkinWeightVertexBuffer = MeshObjectWeightBuffer;
	//OutVertexBuffers.MorphVertexBuffer = &MorphVertexBuffer;
	//OutVertexBuffers.APEXClothVertexBuffer = &LODData.ClothVertexBuffer;
	OutVertexBuffers.NumVertices = LODData.GetNumVertices();
}
