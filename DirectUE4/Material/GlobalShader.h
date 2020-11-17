#pragma once

#include "Shader.h"

/**
* FGlobalShader
*
* Global shaders derive from this class to set their default recompile group as a global one
*/
class FGlobalShader : public FShader
{
	//DECLARE_SHADER_TYPE(FGlobalShader, Global);
public: 
	//using FPermutationDomain = FShaderPermutationNone; 
	using ShaderMetaType = FGlobalShaderType; 
	static ShaderMetaType StaticType; 
	static FShader* ConstructSerializedInstance() { return new FGlobalShader(); } 
	static FShader* ConstructCompiledInstance(const ShaderMetaType::CompiledShaderInitializerType& Initializer)  { return new FGlobalShader(Initializer); }
	//virtual uint32 GetTypeSize() const override { return sizeof(*this); }
public:

	FGlobalShader() : FShader() {}

	FGlobalShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	template<typename TViewUniformShaderParameters, typename ShaderRHIParamRef>
	inline void SetParameters(const ShaderRHIParamRef ShaderRHI, const std::shared_ptr<FUniformBuffer> ViewUniformBuffer)
	{
		const auto& ViewUniformBufferParameter = static_cast<const FShaderUniformBufferParameter&>(GetUniformBufferParameter<TViewUniformShaderParameters>());
		CheckShaderIsValid();
		SetUniformBufferParameter(ShaderRHI, ViewUniformBufferParameter, ViewUniformBuffer);
	}

	typedef void(*ModifyCompilationEnvironmentType)( FShaderCompilerEnvironment&);

	// FShader interface.
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {}
	static bool ValidateCompiledResult(const FShaderParameterMap& ParameterMap, std::vector<std::string>& OutErrors) { return true; }
};

/**
* An internal dummy pixel shader to use when the user calls RHISetPixelShader(NULL).
*/
class FNULLPS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FNULLPS, Global, );
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	FNULLPS() { }
	FNULLPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
	}
};
