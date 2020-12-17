#pragma once

#include "Shader.h"
#include "GlobalShader.h"
#include "SceneView.h"
#include "PostProcess/RenderingCompositionGraph.h"
#include "DeferredShading.h"

class FPostprocessContext
{
public:
	FPostprocessContext(FRenderingCompositionGraph& InGraph, const FViewInfo& InView);

	FRenderingCompositionGraph& Graph;
	const FViewInfo& View;

	// 0 if there was no scene color available at constructor call time
	FRenderingCompositePass* SceneColor;
	// never 0
	FRenderingCompositePass* SceneDepth;

	FRenderingCompositeOutputRef FinalOutput;

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
	void SetParameters(const FRenderingCompositePassContext& Context)
	{
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(GetVertexShader(), Context.View.ViewUniformBuffer);
	}

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

class FPostProcessing
{
public:
	bool AllowFullPostProcessing(const FViewInfo& View);
	void Process(const FViewInfo& View, ComPtr<PooledRenderTarget>& VelocityRT);
};

extern FPostProcessing GPostProcessing;