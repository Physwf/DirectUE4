#pragma once

#include "UnrealMath.h"

#include <map>
#include <string>

class FShaderParameterMap;
struct FShaderCompilerEnvironment;


enum EShaderParameterFlags
{
	// no shader error if the parameter is not used
	SPF_Optional,
	// shader error if the parameter is not used
	SPF_Mandatory
};

/** A shader parameter's register binding. e.g. float1/2/3/4, can be an array, UAV */
class FShaderParameter
{
public:
	FShaderParameter()
		: BufferIndex(0)
		, BaseIndex(0)
		, NumBytes(0)
	{}
	void Bind(const FShaderParameterMap& ParameterMap, const char* ParameterName, EShaderParameterFlags Flags = SPF_Optional);
	bool IsBound() const { return NumBytes > 0; }
	inline bool IsInitialized() const
	{
		return true;

	}
	uint32 GetBufferIndex() const { return BufferIndex; }
	uint32 GetBaseIndex() const { return BaseIndex; }
	uint32 GetNumBytes() const { return NumBytes; }
private:
	uint16 BufferIndex;
	uint16 BaseIndex;
	// 0 if the parameter wasn't bound
	uint16 NumBytes;
};
/** A shader resource binding (textures or samplerstates). */
class FShaderResourceParameter
{
public:
	FShaderResourceParameter()
		: BaseIndex(0)
		, NumResources(0)

	{}
	void Bind(const FShaderParameterMap& ParameterMap, const char* ParameterName, EShaderParameterFlags Flags = SPF_Optional);
	bool IsBound() const { return NumResources > 0; }
	inline bool IsInitialized() const
	{
		return true;
	}
	uint32 GetBaseIndex() const { return BaseIndex; }
	uint32 GetNumResources() const { return NumResources; }
private:
	uint16 BaseIndex;
	uint16 NumResources;

};
/** A class that binds either a UAV or SRV of a resource. */
class FRWShaderParameter
{
public:
	void Bind(const FShaderParameterMap& ParameterMap, const char* BaseName)
	{
		SRVParameter.Bind(ParameterMap, BaseName);
		// If the shader wants to bind the parameter as a UAV, the parameter name must start with "RW"
		std::string UAVName = std::string(("RW")) + BaseName;
		UAVParameter.Bind(ParameterMap, UAVName.c_str());
		// Verify that only one of the UAV or SRV parameters is accessed by the shader.
		assert(!(SRVParameter.GetNumResources() && UAVParameter.GetNumResources())/*, TEXT("Shader binds SRV and UAV of the same resource: %s"), BaseName*/);
	}
	bool IsBound() const
	{
		return SRVParameter.IsBound() || UAVParameter.IsBound();
	}
	bool IsUAVBound() const
	{
		return UAVParameter.IsBound();
	}
	uint32 GetUAVIndex() const
	{
		return UAVParameter.GetBaseIndex();
	}
// 	template<typename TShaderRHIRef, typename TRHICmdList>
// 	inline void SetBuffer(TRHICmdList& RHICmdList, TShaderRHIRef Shader, const FRWBuffer& RWBuffer) const;
// 
// 	template<typename TShaderRHIRef, typename TRHICmdList>
// 	inline void SetBuffer(TRHICmdList& RHICmdList, TShaderRHIRef Shader, const FRWBufferStructured& RWBuffer) const;
// 
// 	template<typename TShaderRHIRef, typename TRHICmdList>
// 	inline void SetTexture(TRHICmdList& RHICmdList, TShaderRHIRef Shader, const FTextureRHIParamRef Texture, FUnorderedAccessViewRHIParamRef UAV) const;
// 
// 	template<typename TRHICmdList>
// 	inline void UnsetUAV(TRHICmdList& RHICmdList, FComputeShaderRHIParamRef ComputeShader) const;

private:
	FShaderResourceParameter SRVParameter;
	FShaderResourceParameter UAVParameter;
};

class FShaderUniformBufferParameter
{
public:
	FShaderUniformBufferParameter()
		: SetParametersId(0)
		, BaseIndex(0)
		, bIsBound(false)
	{}
	//static void ModifyCompilationEnvironment(const char* ParameterName, const FUniformBufferStruct& Struct, FShaderCompilerEnvironment& OutEnvironment);
	void Bind(const FShaderParameterMap& ParameterMap, const char* ParameterName, EShaderParameterFlags Flags = SPF_Optional);
	void BindSRV(const FShaderParameterMap& ParameterMap, const char* ParameterName, EShaderParameterFlags Flags = SPF_Optional);
	void BindSampler(const FShaderParameterMap& ParameterMap, const char* ParameterName, EShaderParameterFlags Flags = SPF_Optional);
	void BindUAV(const FShaderParameterMap& ParameterMap, const char* ParameterName, EShaderParameterFlags Flags = SPF_Optional);
	bool IsBound() const { return bIsBound; }
	const std::map<std::string, uint32>& GetSRVs() const { return SRVsBaseIndices; }
	const std::map<std::string, uint32>& GetSamplers() const { return SamplersBaseIndices; }
	const std::map<std::string, uint32>& GetUAVs() const { return UAVsBaseIndices; }
	inline bool IsInitialized() const
	{
		return true;
	}
	inline void SetInitialized()
	{
	}
	uint32 GetBaseIndex() const { return BaseIndex; }
	/** Used to track when a parameter was set, to detect cases where a bound parameter is used for rendering without being set. */
	mutable uint32 SetParametersId;
private:
	uint16 BaseIndex;
	bool bIsBound;
	std::map<std::string, uint32> SRVsBaseIndices;
	std::map<std::string, uint32> SamplersBaseIndices;
	std::map<std::string, uint32> UAVsBaseIndices;
};
/** A shader uniform buffer binding with a specific structure. */
template<typename TBufferStruct>
class TShaderUniformBufferParameter : public FShaderUniformBufferParameter
{
public:
	static void ModifyCompilationEnvironment(const char* ParameterName, FShaderCompilerEnvironment& OutEnvironment)
	{
		//FShaderUniformBufferParameter::ModifyCompilationEnvironment(ParameterName, TBufferStruct::StaticStruct, OutEnvironment);
	}
};

