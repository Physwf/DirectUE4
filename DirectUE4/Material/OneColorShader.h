#pragma once

#include "ShaderParameters.h"
#include "Shader.h"
#include "GlobalShader.h"

/**
* Vertex shader for rendering a single, constant color.
*/
template<bool bUsingNDCPositions = true, bool bUsingVertexLayers = false>
class TOneColorVS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(TOneColorVS, Global, );

	/** Default constructor. */
	TOneColorVS() {}

public:

	TOneColorVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(("USING_NDC_POSITIONS"), (uint32)(bUsingNDCPositions ? 1 : 0));
		OutEnvironment.SetDefine(("USING_LAYERS"), (uint32)(bUsingVertexLayers ? 1 : 0));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	static const char* GetSourceFilename()
	{
		return ("OneColorShader.dusf");
	}

	static const char* GetFunctionName()
	{
		return ("MainVertexShader");
	}
};

/**
* Pixel shader for rendering a single, constant color.
*/
class FOneColorPS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FOneColorPS, Global, );
public:

	FOneColorPS() { }
	FOneColorPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		//ColorParameter.Bind( Initializer.ParameterMap, TEXT("DrawColorMRT"), SPF_Mandatory);
	}

	virtual void SetColors(const FLinearColor* Colors, int32 NumColors);


	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	/** The parameter to use for setting the draw Color. */
	//FShaderParameter ColorParameter;
};

/**
* Pixel shader for rendering a single, constant color to MRTs.
*/
template<int32 NumOutputs>
class TOneColorPixelShaderMRT : public FOneColorPS
{
	DECLARE_EXPORTED_SHADER_TYPE(TOneColorPixelShaderMRT, Global, );
public:
	TOneColorPixelShaderMRT() { }
	TOneColorPixelShaderMRT(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FOneColorPS(Initializer)
	{
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		if (NumOutputs > 1)
		{
			return true/*IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1)*/;
		}
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FOneColorPS::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(("NUM_OUTPUTS"), NumOutputs);
	}
};
