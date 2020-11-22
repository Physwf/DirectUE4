#pragma once

#include "Shader.h"

extern const int32 GlobalShaderMapId;


extern TShaderMap<FGlobalShaderType>* GGlobalShaderMap;

/**
* FGlobalShader
*
* Global shaders derive from this class to set their default recompile group as a global one
*/
class FGlobalShader : public FShader
{
	DECLARE_SHADER_TYPE(FGlobalShader, Global);
public:

	FGlobalShader() : FShader() {}

	FGlobalShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	template<typename TViewUniformShaderParameters, typename ShaderRHIParamRef>
	inline void SetParameters(const ShaderRHIParamRef ShaderRHI, FUniformBuffer* const ViewUniformBuffer)
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

extern TShaderMap<FGlobalShaderType>* GetGlobalShaderMap();