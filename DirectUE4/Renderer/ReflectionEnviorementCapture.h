#pragma once

#include "SHMath.h"
#include "GlobalShader.h"

extern void ComputeDiffuseIrradiance(FD3D11Texture* LightingSource, int32 LightingSourceMipIndex, FSHVectorRGB3* OutIrradianceEnvironmentMap);

/** Pixel shader used for filtering a mip. */
class FCubeFilterPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCubeFilterPS, Global);
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	FCubeFilterPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		CubeFace.Bind(Initializer.ParameterMap, ("CubeFace"));
		MipIndex.Bind(Initializer.ParameterMap, ("MipIndex"));
		NumMips.Bind(Initializer.ParameterMap, ("NumMips"));
		SourceTexture.Bind(Initializer.ParameterMap, ("SourceTexture"));
		SourceTextureSampler.Bind(Initializer.ParameterMap, ("SourceTextureSampler"));
	}
	FCubeFilterPS() {}

	FShaderParameter CubeFace;
	FShaderParameter MipIndex;
	FShaderParameter NumMips;
	FShaderResourceParameter SourceTexture;
	FShaderResourceParameter SourceTextureSampler;
};

template< uint32 bNormalize >
class TCubeFilterPS : public FCubeFilterPS
{
	DECLARE_SHADER_TYPE(TCubeFilterPS, Global);
public:
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FCubeFilterPS::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(("NORMALIZE"), bNormalize);
	}

	TCubeFilterPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FCubeFilterPS(Initializer)
	{}

	TCubeFilterPS() {}
};