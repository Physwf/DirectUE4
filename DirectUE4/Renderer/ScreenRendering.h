#pragma once

#include "GlobalShader.h"
#include "ShaderParameters.h"
#include "SceneView.h"


struct FScreenVertex
{
	Vector2 Position;
	Vector2 UV;
};

/**
* A pixel shader for rendering a textured screen element.
*/
class FScreenPS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FScreenPS, Global, );
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return true; }

	FScreenPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		InTexture.Bind(Initializer.ParameterMap, ("InTexture"), SPF_Mandatory);
		InTextureSampler.Bind(Initializer.ParameterMap, ("InTextureSampler"));
	}
	FScreenPS() {}

// 	void SetParameters(const FTexture* Texture)
// 	{
// 		SetTextureParameter(GetPixelShader(), InTexture, InTextureSampler, Texture);
// 	}

	void SetParameters( ID3D11SamplerState* SamplerStateRHI, ID3D11ShaderResourceView* TextureRHI)
	{
		SetTextureParameter(GetPixelShader(), InTexture, InTextureSampler, SamplerStateRHI, TextureRHI);
	}


private:
	FShaderResourceParameter InTexture;
	FShaderResourceParameter InTextureSampler;
};

/**
* A vertex shader for rendering a textured screen element.
*/
class FScreenVS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FScreenVS, Global, );
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return true; }

	FScreenVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
	}
	FScreenVS() {}


	void SetParameters(FUniformBuffer* const ViewUniformBuffer)
	{
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(GetVertexShader(), ViewUniformBuffer);
	}

};

template<bool bUsingVertexLayers = false>
class TScreenVSForGS : public FScreenVS
{
	DECLARE_EXPORTED_SHADER_TYPE(TScreenVSForGS, Global, );
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		//return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4) && (!bUsingVertexLayers || RHISupportsVertexShaderLayer(Parameters.Platform));
		return true;
	}

	TScreenVSForGS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FScreenVS(Initializer)
	{
	}
	TScreenVSForGS() {}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FScreenVS::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(("USING_LAYERS"), (uint32)(bUsingVertexLayers ? 1 : 0));
		if (!bUsingVertexLayers)
		{
			//OutEnvironment.CompilerFlags.Add(CFLAG_VertexToGeometryShader);
		}
	}
};
