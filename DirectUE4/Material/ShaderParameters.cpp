#include "ShaderParameters.h"
#include "ShaderCore.h"
#include "Shader.h"

void FShaderParameter::Bind(const FShaderParameterMap& ParameterMap, const char* ParameterName, EShaderParameterFlags Flags /*= SPF_Optional*/)
{
	if (!ParameterMap.FindParameterAllocation(ParameterName, BufferIndex, BaseIndex, NumBytes) && Flags == SPF_Mandatory)
	{
		assert(false);
	}
}

void FShaderResourceParameter::Bind(const FShaderParameterMap& ParameterMap, const char* ParameterName, EShaderParameterFlags Flags)
{
	uint16 UnusedBufferIndex = 0;

	if (!ParameterMap.FindParameterAllocation(ParameterName, UnusedBufferIndex, BaseIndex, NumResources) && Flags == SPF_Mandatory)
	{
		assert(false);
	}
}

void FShaderUniformBufferParameter::Bind(const FShaderParameterMap& ParameterMap, const char* ParameterName, EShaderParameterFlags Flags /*= SPF_Optional*/)
{
	uint16 UnusedBaseIndex = 0;
	uint16 UnusedNumBytes = 0;
	if (!ParameterMap.FindParameterAllocation(ParameterName, BaseIndex, UnusedBaseIndex, UnusedNumBytes))
	{
		bIsBound = false;
		if (Flags == SPF_Mandatory)
		{
			assert(false);
		}
	}
	else
	{
		bIsBound = true;
	}
}

void FShaderUniformBufferParameter::BindSRV(const FShaderParameterMap& ParameterMap, const char* ParameterName, EShaderParameterFlags Flags /*= SPF_Optional*/)
{
	uint16 UnusedBaseIndex = 0;
	uint16 UnusedNumBytes = 0;
	uint16 ResourceIndex = 0;
	if (!ParameterMap.FindParameterAllocation(ParameterName, ResourceIndex, UnusedBaseIndex, UnusedNumBytes))
	{
		if (Flags == SPF_Mandatory)
		{
			assert(false);
		}
	}
	else
	{
		SRVsBaseIndices.insert(std::make_pair(std::string(ParameterName), ResourceIndex));
	}
}

void FShaderUniformBufferParameter::BindSampler(const FShaderParameterMap& ParameterMap, const char* ParameterName, EShaderParameterFlags Flags /*= SPF_Optional*/)
{
	uint16 UnusedBaseIndex = 0;
	uint16 UnusedNumBytes = 0;
	uint16 ResourceIndex = 0;
	if (!ParameterMap.FindParameterAllocation(ParameterName, ResourceIndex, UnusedBaseIndex, UnusedNumBytes))
	{
		if (Flags == SPF_Mandatory)
		{
			assert(false);
		}
	}
	else
	{
		SamplersBaseIndices.insert(std::make_pair(std::string(ParameterName), ResourceIndex));
	}
}

void FShaderUniformBufferParameter::BindUAV(const FShaderParameterMap& ParameterMap, const char* ParameterName, EShaderParameterFlags Flags /*= SPF_Optional*/)
{
	uint16 UnusedBaseIndex = 0;
	uint16 UnusedNumBytes = 0;
	uint16 ResourceIndex = 0;
	if (!ParameterMap.FindParameterAllocation(ParameterName, ResourceIndex, UnusedBaseIndex, UnusedNumBytes))
	{
		if (Flags == SPF_Mandatory)
		{
			assert(false);
		}
	}
	else
	{
		UAVsBaseIndices.insert(std::make_pair(std::string(ParameterName), ResourceIndex));
	}
}

void CacheUniformBufferIncludes(std::map<const char*, struct FCachedUniformBufferDeclaration>& Cache)
{
	for (auto It = Cache.begin(); It != Cache.end(); ++It)
	{
		FCachedUniformBufferDeclaration& BufferDeclaration = It->second;
		assert(BufferDeclaration.Declaration.get() == NULL);

		for (auto StructIt = GetUniformBufferInfoList().begin(); StructIt != GetUniformBufferInfoList().end(); ++StructIt)
		{
			if (It->first == StructIt->ConstantBufferName)
			{
				std::string* NewDeclaration = new std::string();
				CreateUniformBufferShaderDeclaration(StructIt->ConstantBufferName.c_str(), *StructIt, *NewDeclaration);
				assert(NewDeclaration->length());
				BufferDeclaration.Declaration.reset(NewDeclaration);
				break;
			}
		}
	}
}

void CreateUniformBufferShaderDeclaration(const char* Name, const UniformBufferInfo& Info, std::string& OutDeclaration)
{
	char ShaderFileName[128];
	sprintf_s(ShaderFileName, sizeof(ShaderFileName), "%s.hlsl", Name);
	assert(LoadFileToString(OutDeclaration, ShaderFileName));
}
