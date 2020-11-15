#include "VertexFactory.h"

std::list<FVertexFactoryType*>& FVertexFactoryType::GetTypeList()
{
	static std::list<FVertexFactoryType*> TypeList;
	return TypeList;
}

FVertexFactoryType* FVertexFactoryType::GetVFByName(const std::string& VFName)
{
	for (auto It = GetTypeList().begin(); It != GetTypeList().end(); ++It)
	{
		std::string CurrentVFName = std::string((*It)->GetName());
		if (CurrentVFName == VFName)
		{
			return *It;
		}
	}
	return NULL;
}

void FVertexFactoryType::Initialize(const std::map<std::string, std::vector<const char*>>& ShaderFileToUniformBufferVariables)
{

}

FVertexFactoryType::FVertexFactoryType(
	const char* InName, 
	const char* InShaderFilename, 
	bool bInUsedWithMaterials, 
	bool bInSupportsStaticLighting, 
	bool bInSupportsDynamicLighting, 
	bool bInSupportsPrecisePrevWorldPos, 
	bool bInSupportsPositionOnly, 
	ConstructParametersType InConstructParameters, 
	ShouldCacheType InShouldCache, 
	ModifyCompilationEnvironmentType InModifyCompilationEnvironment, 
	SupportsTessellationShadersType InSupportsTessellationShaders
	) :
	Name(InName),
	ShaderFilename(InShaderFilename),
	TypeName(InName),
	bUsedWithMaterials(bInUsedWithMaterials),
	bSupportsStaticLighting(bInSupportsStaticLighting),
	bSupportsDynamicLighting(bInSupportsDynamicLighting),
	bSupportsPrecisePrevWorldPos(bInSupportsPrecisePrevWorldPos),
	bSupportsPositionOnly(bInSupportsPositionOnly),
	ConstructParameters(InConstructParameters),
	ShouldCacheRef(InShouldCache),
	ModifyCompilationEnvironmentRef(InModifyCompilationEnvironment),
	SupportsTessellationShadersRef(InSupportsTessellationShaders)
{
	GlobalListLink = GetTypeList().insert(GetTypeList().begin(),this);
	HashIndex = NextHashIndex++;
}

FVertexFactoryType::~FVertexFactoryType()
{
	GetTypeList().erase(GlobalListLink);
}

uint32 FVertexFactoryType::NextHashIndex = 2;

void FVertexFactory::SetStreams(ID3D11DeviceContext* Context) const
{
	for (uint32 StreamIndex = 0; StreamIndex < Streams.size(); StreamIndex++)
	{
		const FVertexStream& Stream = Streams[StreamIndex];
		//EVertexStreamUsage::ManualFetch == 4
		if (!(Stream.VertexStreamUsage & 4))
		{
			if (!Stream.VertexBuffer)
			{
				Context->IASetVertexBuffers(StreamIndex,1, nullptr, &Stream.Stride, &Stream.Offset);
			}
			else
			{
// 				if (EnumHasAnyFlags(EVertexStreamUsage::Overridden, Stream.VertexStreamUsage) && !Stream.VertexBuffer->IsInitialized())
// 				{
// 					RHICmdList.SetStreamSource(StreamIndex, nullptr, 0);
// 				}
// 				else
				{
					//checkf(Stream.VertexBuffer->IsInitialized(), TEXT("Vertex buffer was not initialized! Stream %u, Stride %u, Name %s"), StreamIndex, Stream.Stride, *Stream.VertexBuffer->GetFriendlyName());
					//RHICmdList.SetStreamSource(StreamIndex, Stream.VertexBuffer->VertexBufferRHI, Stream.Offset);
					Context->IASetVertexBuffers(StreamIndex, 1, (ID3D11Buffer**)&Stream.VertexBuffer, &Stream.Stride, &Stream.Offset);
				}
			}
		}
	}
}

void FVertexFactory::SetPositionStream(ID3D11DeviceContext* Context) const
{
	for (uint32 StreamIndex = 0; StreamIndex < PositionStream.size(); StreamIndex++)
	{
		const FVertexStream& Stream = PositionStream[StreamIndex];
		//check(Stream.VertexBuffer->IsInitialized());
		//RHICmdList.SetStreamSource(StreamIndex, Stream.VertexBuffer->VertexBufferRHI, Stream.Offset);
		Context->IASetVertexBuffers(StreamIndex, 1, (ID3D11Buffer**)&Stream.VertexBuffer, &Stream.Stride, &Stream.Offset);
	}
}

D3D11_INPUT_ELEMENT_DESC FVertexFactory::AccessStreamComponent(const FVertexStreamComponent& Component, uint8 AttributeIndex)
{
	FVertexStream VertexStream;
	VertexStream.VertexBuffer = Component.VertexBuffer;
	VertexStream.Stride = Component.Stride;
	VertexStream.Offset = Component.StreamOffset;
	VertexStream.VertexStreamUsage = Component.VertexStreamUsage;

	Streams.push_back(VertexStream);

	D3D11_INPUT_ELEMENT_DESC Desc;
	Desc.SemanticName = "ATTRIBUTE";
	Desc.SemanticIndex = AttributeIndex;
	Desc.Format = Component.Type;
	Desc.InputSlot = AttributeIndex;
	Desc.AlignedByteOffset = Component.Offset;
	Desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	Desc.InstanceDataStepRate = 0;
	return Desc;
}

D3D11_INPUT_ELEMENT_DESC FVertexFactory::AccessPositionStreamComponent(const FVertexStreamComponent& Component, uint8 AttributeIndex)
{
	FVertexStream VertexStream;
	VertexStream.VertexBuffer = Component.VertexBuffer;
	VertexStream.Stride = Component.Stride;
	VertexStream.Offset = Component.StreamOffset;
	VertexStream.VertexStreamUsage = Component.VertexStreamUsage;

	PositionStream.push_back(VertexStream);

	D3D11_INPUT_ELEMENT_DESC Desc;
	Desc.SemanticName = "ATTRIBUTE";
	Desc.SemanticIndex = AttributeIndex;
	Desc.Format = Component.Type;
	Desc.InputSlot = AttributeIndex;
	Desc.AlignedByteOffset = Component.Offset;
	Desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	Desc.InstanceDataStepRate = 0;
	return Desc;
}


uint32 FVertexFactory::NextHashIndex = 0;

void FLocalVertexFactory::SetData(const FDataType& InData)
{
	Data = InData;
	ReleaseRHI();
	InitRHI();
}

void FLocalVertexFactory::InitResource()
{
	InitRHI();
}

void FLocalVertexFactory::ReleaseResource()
{
	ReleaseRHI();
}

void FLocalVertexFactory::InitRHI()
{

}

void FLocalVertexFactory::ReleaseRHI()
{

}
