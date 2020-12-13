#include "GPUSkinVertexFactory.h"
#include "Material.h"
#include "UnrealTemplates.h"

#define IMPLEMENT_GPUSKINNING_VERTEX_FACTORY_TYPE_INTERNAL(FactoryClass, ShaderFilename,bUsedWithMaterials,bSupportsStaticLighting,bSupportsDynamicLighting,bPrecisePrevWorldPos,bSupportsPositionOnly) \
	template <bool bExtraBoneInfluencesT> FVertexFactoryType FactoryClass<bExtraBoneInfluencesT>::StaticType( \
	bExtraBoneInfluencesT ? #FactoryClass "true" : #FactoryClass "false", \
	(ShaderFilename), \
	bUsedWithMaterials, \
	bSupportsStaticLighting, \
	bSupportsDynamicLighting, \
	bPrecisePrevWorldPos, \
	bSupportsPositionOnly, \
	Construct##FactoryClass##ShaderParameters<bExtraBoneInfluencesT>, \
	FactoryClass<bExtraBoneInfluencesT>::ShouldCompilePermutation, \
	FactoryClass<bExtraBoneInfluencesT>::ModifyCompilationEnvironment, \
	FactoryClass<bExtraBoneInfluencesT>::SupportsTessellationShaders \
	); \
	template <bool bExtraBoneInfluencesT> inline FVertexFactoryType* FactoryClass<bExtraBoneInfluencesT>::GetType() const { return &StaticType; }


#define IMPLEMENT_GPUSKINNING_VERTEX_FACTORY_TYPE(FactoryClass, ShaderFilename,bUsedWithMaterials,bSupportsStaticLighting,bSupportsDynamicLighting,bPrecisePrevWorldPos,bSupportsPositionOnly) \
	template <bool bExtraBoneInfluencesT> FVertexFactoryShaderParameters* Construct##FactoryClass##ShaderParameters(EShaderFrequency ShaderFrequency) { return FactoryClass<bExtraBoneInfluencesT>::ConstructShaderParameters(ShaderFrequency); } \
	IMPLEMENT_GPUSKINNING_VERTEX_FACTORY_TYPE_INTERNAL(FactoryClass, ShaderFilename,bUsedWithMaterials,bSupportsStaticLighting,bSupportsDynamicLighting,bPrecisePrevWorldPos,bSupportsPositionOnly) \
	template class FactoryClass<false>;	\
	template class FactoryClass<true>;

uint32 FSharedPoolPolicyData::GetPoolBucketIndex(uint32 Size)
{
	unsigned long Lower = 0;
	unsigned long Upper = NumPoolBucketSizes;
	unsigned long Middle;

	do
	{
		Middle = (Upper + Lower) >> 1;
		if (Size <= BucketSizes[Middle - 1])
		{
			Upper = Middle;
		}
		else
		{
			Lower = Middle;
		}
	} while (Upper - Lower > 1);

	assert(Size <= BucketSizes[Lower]);
	assert((Lower == 0) || (Size > BucketSizes[Lower - 1]));

	return Lower;
}

uint32 FSharedPoolPolicyData::GetPoolBucketSize(uint32 Bucket)
{
	assert(Bucket < NumPoolBucketSizes);
	return BucketSizes[Bucket];
}

uint32 FSharedPoolPolicyData::BucketSizes[NumPoolBucketSizes] = {
	16, 48, 96, 192, 384, 768, 1536,
	3072, 4608, 6144, 7680, 9216, 12288,
	65536, 131072, 262144, 1048576 // these 4 numbers are added for large cloth simulation vertices, supports up to 65,536 verts
};

FVertexBufferAndSRV FBoneBufferPoolPolicy::CreateResource(FSharedPoolPolicyData::CreationArguments Args)
{
	uint32 BufferSize = GetPoolBucketSize(GetPoolBucketIndex(Args));
	// in VisualStudio the copy constructor call on the return argument can be optimized out
	// see https://msdn.microsoft.com/en-us/library/ms364057.aspx#nrvo_cpp05_topic3
	FVertexBufferAndSRV Buffer;
	//FRHIResourceCreateInfo CreateInfo;
	Buffer.VertexBufferRHI = RHICreateVertexBuffer(BufferSize, D3D11_USAGE_DYNAMIC , D3D11_BIND_SHADER_RESOURCE, 0, NULL);
	Buffer.VertexBufferSRV = RHICreateShaderResourceView(Buffer.VertexBufferRHI.Get(), sizeof(Vector4), DXGI_FORMAT_R32G32B32A32_FLOAT);
	return Buffer;
}

FSharedPoolPolicyData::CreationArguments FBoneBufferPoolPolicy::GetCreationArguments(const FVertexBufferAndSRV& Resource)
{
	D3D11_BUFFER_DESC Desc;
	Resource.VertexBufferRHI->GetDesc(&Desc);
	return Desc.ByteWidth;
}

void FBoneBufferPoolPolicy::FreeResource(FVertexBufferAndSRV Resource)
{

}

FBoneBufferPool::~FBoneBufferPool()
{

}

void RHIUpdateBoneBuffer(ID3D11Buffer* InVertexBuffer, uint32 InBufferSize, const std::vector<FMatrix>& InReferenceToLocalMatrices, const std::vector<FBoneIndexType>& InBoneMap)
{
	D3D11_MAPPED_SUBRESOURCE MapedSubresource;
	assert(S_OK == D3D11DeviceContext->Map(InVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MapedSubresource));
	FSkinMatrix3x4* ChunkMatrices = (FSkinMatrix3x4*)MapedSubresource.pData;
	const uint32 NumBones = InBoneMap.size();
	for (uint32 BoneIdx = 0; BoneIdx < NumBones; BoneIdx++)
	{
		const FBoneIndexType RefToLocalIdx = InBoneMap[BoneIdx];
		assert(IsValidIndex(InReferenceToLocalMatrices, RefToLocalIdx)); // otherwise maybe some bad threading on BoneMap, maybe we need to copy that

		FSkinMatrix3x4& BoneMat = ChunkMatrices[BoneIdx];
		const FMatrix& RefToLocal = InReferenceToLocalMatrices[RefToLocalIdx];
		RefToLocal.To3x4MatrixTranspose((float*)BoneMat.M);
	}
	D3D11DeviceContext->Unmap(InVertexBuffer, 0);
}

bool FGPUBaseSkinVertexFactory::FShaderDataType::UpdateBoneData(const std::vector<FMatrix>& ReferenceToLocalMatrices, const std::vector<FBoneIndexType>& BoneMap, uint32 RevisionNumber, bool bPrevious, bool bUseSkinCache)
{
	const uint32 NumBones = BoneMap.size();
	assert(NumBones <= MaxGPUSkinBones);
	FSkinMatrix3x4* ChunkMatrices = nullptr;

	FVertexBufferAndSRV* CurrentBoneBuffer = 0;

	// make sure current revision is up-to-date
	SetCurrentRevisionNumber(RevisionNumber);

	CurrentBoneBuffer = &GetBoneBufferForWriting(bPrevious);

	static FSharedPoolPolicyData PoolPolicy;
	uint32 NumVectors = NumBones * 3;
	assert(NumVectors <= (MaxGPUSkinBones * 3));
	uint32 VectorArraySize = NumVectors * sizeof(Vector4);
	uint32 PooledArraySize = BoneBufferPool.PooledSizeForCreationArguments(VectorArraySize);

	D3D11_BUFFER_DESC Desc;
	if (IsValidRef(*CurrentBoneBuffer))
	{
		CurrentBoneBuffer->VertexBufferRHI->GetDesc(&Desc);
	}
	if (!IsValidRef(*CurrentBoneBuffer) || PooledArraySize != Desc.ByteWidth)
	{
		if (IsValidRef(*CurrentBoneBuffer))
		{
			BoneBufferPool.ReleasePooledResource(*CurrentBoneBuffer);
		}
		*CurrentBoneBuffer = BoneBufferPool.CreatePooledResource(VectorArraySize);
		assert(IsValidRef(*CurrentBoneBuffer));
	}
	if (NumBones)
	{
		if (!bUseSkinCache/* && DeferSkeletalLockAndFillToRHIThread()*/)
		{
			//new (RHICmdList.AllocCommand<FRHICommandUpdateBoneBuffer>()) FRHICommandUpdateBoneBuffer(CurrentBoneBuffer->VertexBufferRHI, VectorArraySize, ReferenceToLocalMatrices, BoneMap);
			RHIUpdateBoneBuffer(CurrentBoneBuffer->VertexBufferRHI.Get(), VectorArraySize, ReferenceToLocalMatrices, BoneMap);
			return true;
		}
		//ChunkMatrices = (FSkinMatrix3x4*)RHILockVertexBuffer(CurrentBoneBuffer->VertexBufferRHI, 0, VectorArraySize, RLM_WriteOnly);
	}
	return false;
}

uint32 FGPUBaseSkinVertexFactory::FShaderDataType::MaxGPUSkinBones = 0;

int32 FGPUBaseSkinVertexFactory::GetMaxGPUSkinBones()
{
	return GHardwareMaxGPUSkinBones;
}

FBoneBufferPool FGPUBaseSkinVertexFactory::BoneBufferPool;

template<bool bExtraBoneInfluencesT>
bool TGPUSkinVertexFactory<bExtraBoneInfluencesT>::ShouldCompilePermutation(const class FMaterial* Material, const FShaderType* ShaderType)
{
	return (Material->IsUsedWithSkeletalMesh() || Material->IsSpecialEngineMaterial());
}

template<bool bExtraBoneInfluencesT>
void TGPUSkinVertexFactory<bExtraBoneInfluencesT>::ModifyCompilationEnvironment(const class FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
{
	FVertexFactory::ModifyCompilationEnvironment(Material, OutEnvironment);
	const int32 MaxGPUSkinBones = 256;// GetFeatureLevelMaxNumberOfBones(GetMaxSupportedFeatureLevel(Platform));
	OutEnvironment.SetDefine(("MAX_SHADER_BONES"), MaxGPUSkinBones);
	const uint32 UseExtraBoneInfluences = bExtraBoneInfluencesT;
	OutEnvironment.SetDefine(("GPUSKIN_USE_EXTRA_INFLUENCES"), UseExtraBoneInfluences);
	{
		bool bLimit2BoneInfluences = false;// (CVarGPUSkinLimit2BoneInfluences.GetValueOnAnyThread() != 0);
		OutEnvironment.SetDefine(("GPUSKIN_LIMIT_2BONE_INFLUENCES"), (bLimit2BoneInfluences ? 1 : 0));
	}
}

template<bool bExtraBoneInfluencesT>
void TGPUSkinVertexFactory<bExtraBoneInfluencesT>::ReleaseRHI()
{

}

template<bool bExtraBoneInfluencesT>
void TGPUSkinVertexFactory<bExtraBoneInfluencesT>::InitRHI()
{
	std::vector<D3D11_INPUT_ELEMENT_DESC> Elements;
	AddVertexElements(Data, Elements);

	// create the actual device decls
	InitDeclaration(Elements);
}

template<bool bExtraBoneInfluencesT>
void TGPUSkinVertexFactory<bExtraBoneInfluencesT>::AddVertexElements(FDataType& InData, std::vector<D3D11_INPUT_ELEMENT_DESC>& OutElements)
{
	// position decls
	OutElements.push_back(AccessStreamComponent(InData.PositionComponent, 0));

	// tangent basis vector decls
	OutElements.push_back(AccessStreamComponent(InData.TangentBasisComponents[0], 1));
	OutElements.push_back(AccessStreamComponent(InData.TangentBasisComponents[1], 2));


	// bone indices decls
	OutElements.push_back(AccessStreamComponent(InData.BoneIndices, 3));

	// bone weights decls
	OutElements.push_back(AccessStreamComponent(InData.BoneWeights, 4));

	const uint8 BaseTexCoordAttribute = 5;
	//for (int32 CoordinateIndex = 0; CoordinateIndex < InData.TextureCoordinates.Num(); CoordinateIndex++)
	{
		OutElements.push_back(AccessStreamComponent(
			InData.TextureCoordinates,
			BaseTexCoordAttribute + 0//CoordinateIndex
		));
	}

	FVertexStreamComponent NullColorComponent(GNullColorVertexBuffer.Get(), 0, 0, sizeof(FColor), DXGI_FORMAT_R8G8B8A8_UNORM,4);
	OutElements.push_back(AccessStreamComponent(NullColorComponent, 13));


}


/** Shader parameters for use with TGPUSkinVertexFactory */
class FGPUSkinVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:
	virtual void Bind(const FShaderParameterMap& ParameterMap) override
	{
		PerBoneMotionBlur.Bind(ParameterMap, ("PerBoneMotionBlur"));
		BoneMatrices.Bind(ParameterMap, ("BoneMatrices"));
		PreviousBoneMatrices.Bind(ParameterMap, ("PreviousBoneMatrices"));
	}
	virtual void SetMesh(FShader* Shader, const FVertexFactory* VertexFactory, const FSceneView& View, const FMeshBatchElement& BatchElement, uint32 DataFlags) const override
	{
		ID3D11VertexShader* ShaderRHI = Shader->GetVertexShader();

		if (ShaderRHI)
		{
			const FGPUBaseSkinVertexFactory::FShaderDataType& ShaderData = ((const FGPUBaseSkinVertexFactory*)VertexFactory)->GetShaderData();

			bool bLocalPerBoneMotionBlur = false;

			if (BoneMatrices.IsBound())
			{
				ID3D11ShaderResourceView* CurrentData = ShaderData.GetBoneBufferForReading(false).VertexBufferSRV.Get();

				SetShaderSRV(ShaderRHI, BoneMatrices.GetBaseIndex(), CurrentData);
				//RHICmdList.SetShaderResourceViewParameter(ShaderRHI, BoneMatrices.GetBaseIndex(), CurrentData);
			}
			if (PreviousBoneMatrices.IsBound())
			{
				// todo: Maybe a check for PreviousData!=CurrentData would save some performance (when objects don't have velocty yet) but removing the bool also might save performance
				bLocalPerBoneMotionBlur = true;

				ID3D11ShaderResourceView* PreviousData = ShaderData.GetBoneBufferForReading(true).VertexBufferSRV.Get();
				SetShaderSRV(ShaderRHI, PreviousBoneMatrices.GetBaseIndex(), PreviousData);
				//RHICmdList.SetShaderResourceViewParameter(ShaderRHI, PreviousBoneMatrices.GetBaseIndex(), PreviousData);
			}
		}
	}
private:
	FShaderParameter PerBoneMotionBlur;
	FShaderResourceParameter BoneMatrices;
	FShaderResourceParameter PreviousBoneMatrices;
};

template <bool bExtraBoneInfluencesT>
FVertexFactoryShaderParameters* TGPUSkinVertexFactory<bExtraBoneInfluencesT>::ConstructShaderParameters(EShaderFrequency ShaderFrequency)
{
	return (ShaderFrequency == SF_Vertex) ? new FGPUSkinVertexFactoryShaderParameters() : NULL;
}

IMPLEMENT_GPUSKINNING_VERTEX_FACTORY_TYPE(TGPUSkinVertexFactory, "GpuSkinVertexFactory.dusf", true, false, true, false, false);

