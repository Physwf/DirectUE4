#pragma once

#include "Shader.h"
#include "GlobalShader.h"
#include "SceneView.h"

class PostProcessContext
{
public:
	PostProcessContext();

};
/** Encapsulates the post processing vertex shader. */
class FPostProcessVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessVS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	/** Default constructor. */
	FPostProcessVS() {}

	/** to have a similar interface as all other shaders */
// 	void SetParameters(const FRenderingCompositePassContext& Context)
// 	{
// 		FGlobalShader::SetParameters<FViewUniformShaderParameters>(GetVertexShader(), Context.View.ViewUniformBuffer);
// 	}

	void SetParameters(FUniformBuffer* const ViewUniformBuffer)
	{
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(GetVertexShader(), ViewUniformBuffer);
	}

public:

	/** Initialization constructor. */
	FPostProcessVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
	}
};



extern PostProcessContext GPostProcessing;