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

/** DECLARE_GLOBAL_SHADER and IMPLEMENT_GLOBAL_SHADER setup a global shader class's boiler plate. They are meant to be used like so:
*
* class FMyGlobalShaderPS : public FGlobalShader
* {
*		// Setup the shader's boiler plate.
*		DECLARE_GLOBAL_SHADER(FMyGlobalShaderPS);
*
*		// Setup the shader's permutation domain. If no dimensions, can do FPermutationDomain = FShaderPermutationNone.
*		using FPermutationDomain = TShaderPermutationDomain<DIMENSIONS...>;
*
*		// ...
* };
*
* // Instantiates global shader's global variable that will take care of compilation process of the shader. This needs imperatively to be
* done in a .cpp file regardless of whether FMyGlobalShaderPS is in a header or not.
* IMPLEMENT_GLOBAL_SHADER(FMyGlobalShaderPS, "/Engine/Private/MyShaderFile.usf", "MainPS", SF_Pixel);
*/
#define DECLARE_GLOBAL_SHADER(ShaderClass) \
	public: \
	\
	using ShaderMetaType = FGlobalShaderType; \
	\
	static ShaderMetaType StaticType; \
	\
	static FShader* ConstructSerializedInstance() { return new ShaderClass(); } \
	static FShader* ConstructCompiledInstance(const ShaderMetaType::CompiledShaderInitializerType& Initializer) \
	{ return new ShaderClass(Initializer); } \
	\
	static bool ShouldCompilePermutationImpl(const FGlobalShaderPermutationParameters& Parameters) \
	{ \
		return ShaderClass::ShouldCompilePermutation(Parameters); \
	} \
	\
	static bool ValidateCompiledResultImpl(const FShaderParameterMap& ParameterMap, std::vector<std::string>& OutErrors) \
	{ \
		return ShaderClass::ValidateCompiledResult(ParameterMap, OutErrors); \
	} \
	static void ModifyCompilationEnvironmentImpl( \
		const FGlobalShaderPermutationParameters& Parameters, \
		FShaderCompilerEnvironment& OutEnvironment) \
	{ \
		FPermutationDomain PermutationVector(Parameters.PermutationId); \
		PermutationVector.ModifyCompilationEnvironment(OutEnvironment); \
		ShaderClass::ModifyCompilationEnvironment(Parameters, OutEnvironment); \
	}

#define IMPLEMENT_GLOBAL_SHADER(ShaderClass,SourceFilename,FunctionName,Frequency) \
	ShaderClass::ShaderMetaType ShaderClass::StaticType( \
		(#ShaderClass), \
		(SourceFilename), \
		(FunctionName), \
		Frequency, \
		ShaderClass::FPermutationDomain::PermutationCount, \
		ShaderClass::ConstructSerializedInstance, \
		ShaderClass::ConstructCompiledInstance, \
		ShaderClass::ModifyCompilationEnvironmentImpl, \
		ShaderClass::ShouldCompilePermutationImpl, \
		ShaderClass::ValidateCompiledResultImpl, \
		ShaderClass::GetStreamOutElements \
		)