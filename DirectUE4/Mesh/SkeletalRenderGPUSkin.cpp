#include "SkeletalRenderGPUSkin.h"
#include "MeshComponent.h"
#include "SceneView.h"
#include "SkeletalMeshRenderData.h"
#include "MeshComponent.h"
#include "UnrealTemplates.h"
#include "SkeletalMeshTypes.h"
#include "SkeletalRender.h"
#include "Scene.h"

void FDynamicSkelMeshObjectDataGPUSkin::Clear()
{
	ReferenceToLocal.clear();
}

FDynamicSkelMeshObjectDataGPUSkin* FDynamicSkelMeshObjectDataGPUSkin::AllocDynamicSkelMeshObjectDataGPUSkin()
{
	return new FDynamicSkelMeshObjectDataGPUSkin;
}

void FDynamicSkelMeshObjectDataGPUSkin::FreeDynamicSkelMeshObjectDataGPUSkin(FDynamicSkelMeshObjectDataGPUSkin* Who)
{
	delete Who;
}

void FDynamicSkelMeshObjectDataGPUSkin::InitDynamicSkelMeshObjectDataGPUSkin(
	USkinnedMeshComponent* InMeshComponent, 
	FSkeletalMeshRenderData* InSkeletalMeshRenderData, 
	int32 InLODIndex, 
	/*const TArray<FActiveMorphTarget>& InActiveMorphTargets,*/ 
	/*const TArray<float>& InMorphTargetWeights,*/ 
	bool bUpdatePreviousBoneTransform)
{
	LODIndex = InLODIndex;

	FSkeletalMeshSceneProxy* SkeletalMeshProxy = (FSkeletalMeshSceneProxy*)InMeshComponent->SceneProxy;
	const std::vector<FBoneIndexType>* ExtraRequiredBoneIndices = /*SkeletalMeshProxy ? &SkeletalMeshProxy->GetSortedShadowBoneIndices() : */nullptr;

	// update ReferenceToLocal
	UpdateRefToLocalMatrices(ReferenceToLocal, InMeshComponent, InSkeletalMeshRenderData, LODIndex, ExtraRequiredBoneIndices);
	if (bUpdatePreviousBoneTransform)
	{
		UpdatePreviousRefToLocalMatrices(PreviousReferenceToLocal, InMeshComponent, InSkeletalMeshRenderData, LODIndex, ExtraRequiredBoneIndices);
	}
	else
	{
		// otherwise, clear it, it will use previous buffer
		PreviousReferenceToLocal.clear();
	}
}

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

void FSkeletalMeshObjectGPUSkin::Update(int32 LODIndex, USkinnedMeshComponent* InMeshComponent, /*const TArray<FActiveMorphTarget>& ActiveMorphTargets, const TArray<float>& MorphTargetWeights,*/ bool bUpdatePreviousBoneTransform)
{
	FDynamicSkelMeshObjectDataGPUSkin* NewDynamicData = FDynamicSkelMeshObjectDataGPUSkin::AllocDynamicSkelMeshObjectDataGPUSkin();
	NewDynamicData->InitDynamicSkelMeshObjectDataGPUSkin(InMeshComponent, SkeletalMeshRenderData, LODIndex, /*ActiveMorphTargets, MorphTargetWeights,*/ bUpdatePreviousBoneTransform);
	extern uint32 GFrameNumber;
	uint32 FrameNumberToPrepare = GFrameNumber + 1;
	uint32 RevisionNumber = 0;

	FGPUSkinCache* GPUSkinCache = nullptr;
	if (InMeshComponent && InMeshComponent->SceneProxy)
	{
		// We allow caching of per-frame, per-scene data
		FrameNumberToPrepare = InMeshComponent->SceneProxy->GetScene().GetFrameNumber() + 1;
		GPUSkinCache = InMeshComponent->SceneProxy->GetScene().GetGPUSkinCache();
		RevisionNumber = InMeshComponent->GetBoneTransformRevisionNumber();
	}

	UpdateDynamicData_RenderThread(GPUSkinCache, NewDynamicData, nullptr, FrameNumberToPrepare, RevisionNumber);
}

void FSkeletalMeshObjectGPUSkin::UpdateDynamicData_RenderThread(FGPUSkinCache* GPUSkinCache, FDynamicSkelMeshObjectDataGPUSkin* InDynamicData, FScene* Scene, uint32 FrameNumberToPrepare, uint32 RevisionNumber)
{

	DynamicData = InDynamicData;
	LastBoneTransformRevisionNumber = RevisionNumber;

	bool bMorphNeedsUpdate = false;
	ProcessUpdatedDynamicData(GPUSkinCache, FrameNumberToPrepare, RevisionNumber, bMorphNeedsUpdate);
}

void FSkeletalMeshObjectGPUSkin::ProcessUpdatedDynamicData(FGPUSkinCache* GPUSkinCache, uint32 FrameNumberToPrepare, uint32 RevisionNumber, bool bMorphNeedsUpdate)
{
	FSkeletalMeshObjectLOD& LOD = LODs[DynamicData->LODIndex];

	const FSkeletalMeshLODRenderData& LODData = *SkeletalMeshRenderData->LODRenderData[DynamicData->LODIndex];
	const std::vector<FSkeletalMeshRenderSection>& Sections = GetRenderSections(DynamicData->LODIndex);

	FVertexFactoryData& VertexFactoryData = LOD.GPUSkinVertexFactories;

	bool bDataPresent = false;

	bDataPresent = VertexFactoryData.VertexFactories.size() > 0;

	if (bDataPresent)
	{
		for (uint32 SectionIdx = 0; SectionIdx < Sections.size(); SectionIdx++)
		{
			const FSkeletalMeshRenderSection& Section = Sections[SectionIdx];

			bool bUseSkinCache = false;

			FGPUBaseSkinVertexFactory* VertexFactory;

			VertexFactory = VertexFactoryData.VertexFactories[SectionIdx].get();

			FGPUBaseSkinVertexFactory::FShaderDataType& ShaderData = VertexFactory->GetShaderData();

			if (DynamicData->PreviousReferenceToLocal.size() > 0)
			{
				std::vector<FMatrix>& PreviousReferenceToLocalMatrices = DynamicData->PreviousReferenceToLocal;
				ShaderData.UpdateBoneData(PreviousReferenceToLocalMatrices, Section.BoneMap, RevisionNumber, true, bUseSkinCache);
			}

			// Create a uniform buffer from the bone transforms.
			std::vector<FMatrix>& ReferenceToLocalMatrices = DynamicData->ReferenceToLocal;
			bool bNeedFence = ShaderData.UpdateBoneData(ReferenceToLocalMatrices, Section.BoneMap, RevisionNumber, false, bUseSkinCache);
		}

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
		sizeof(FVector),
		DXGI_FORMAT_R32G32B32_FLOAT,
		4
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
		/*VertexBuffers.SkinWeightVertexBuffer->GetStride()*/8,
		DXGI_FORMAT_R8G8B8A8_UINT);
	// bone weights
	VertexFactoryData->BoneWeights = FVertexStreamComponent(
		VertexBuffers.SkinWeightVertexBuffer->WeightVertexBufferRHI.Get(),
		/*STRUCT_OFFSET(WeightInfoType, InfluenceWeights)*/4, 
		/*VertexBuffers.SkinWeightVertexBuffer->GetStride()*/8,
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
