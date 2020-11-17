#include "ShaderCore.h"

bool FShaderParameterMap::FindParameterAllocation(const char* ParameterName, uint16& OutBufferIndex, uint16& OutBaseIndex, uint16& OutSize) const
{
	const FParameterAllocation& Allocation = ParameterMap.at(ParameterName);

	OutBufferIndex = Allocation.BufferIndex;
	OutBaseIndex = Allocation.BaseIndex;
	OutSize = Allocation.Size;
	if (Allocation.bBound)
	{
		// Can detect copy-paste errors in binding parameters.  Need to fix all the false positives before enabling.
		//UE_LOG(LogShaders, Warning, TEXT("Parameter %s was bound multiple times. Code error?"), ParameterName);
	}
	Allocation.bBound = true;
	return true;
}

bool FShaderParameterMap::ContainsParameterAllocation(const char* ParameterName) const
{
	return ParameterMap.find(ParameterName) != ParameterMap.end();
}

void FShaderParameterMap::AddParameterAllocation(const char* ParameterName, uint16 BufferIndex, uint16 BaseIndex, uint16 Size)
{
	FParameterAllocation Allocation;
	Allocation.BufferIndex = BufferIndex;
	Allocation.BaseIndex = BaseIndex;
	Allocation.Size = Size;
	ParameterMap.insert(std::make_pair(std::string(ParameterName) , Allocation));
}

void FShaderParameterMap::RemoveParameterAllocation(const char* ParameterName)
{
	ParameterMap.erase(std::string(ParameterName));
}

void FShaderParameterMap::VerifyBindingsAreComplete(const char* ShaderTypeName, class FVertexFactoryType* InVertexFactoryType) const
{

}
