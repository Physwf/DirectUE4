#include "ShaderCore.h"

bool FShaderParameterMap::FindParameterAllocation(const char* ParameterName, uint16& OutBufferIndex, uint16& OutBaseIndex, uint16& OutSize) const
{
	return false;
}

bool FShaderParameterMap::ContainsParameterAllocation(const char* ParameterName) const
{
	return false;
}

void FShaderParameterMap::AddParameterAllocation(const char* ParameterName, uint16 BufferIndex, uint16 BaseIndex, uint16 Size)
{
}

void FShaderParameterMap::RemoveParameterAllocation(const char* ParameterName)
{

}

void FShaderParameterMap::VerifyBindingsAreComplete(const char* ShaderTypeName, class FVertexFactoryType* InVertexFactoryType) const
{

}
