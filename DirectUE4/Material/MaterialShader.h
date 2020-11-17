#pragma once

#include "Shader.h"
#include "SceneView.h"
#include "SceneRenderTargetParameters.h"
#include "Material.h"

/** Base class of all shaders that need material parameters. */
class FMaterialShader : public FShader
{
public:
	static std::string UniformBufferLayoutName;

	FMaterialShader()
	{
	}

	FMaterialShader(const FMaterialShaderType::CompiledShaderInitializerType& Initializer);

	typedef void(*ModifyCompilationEnvironmentType)(const FMaterial*, FShaderCompilerEnvironment&);

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
	}

	static bool ValidateCompiledResult(const std::vector<FMaterial*>& Materials, const FShaderParameterMap& ParameterMap, std::vector<std::string>& OutError)
	{
		return true;
	}

	//FUniformBufferRHIParamRef GetParameterCollectionBuffer(const FGuid& Id, const FSceneInterface* SceneInterface) const;

	template<typename ShaderRHIParamRef>
	inline void SetViewParameters(const ShaderRHIParamRef ShaderRHI, const FSceneView& View, const TUniformBufferPtr<FViewUniformShaderParameters>& ViewUniformBuffer)
	{
		const auto& ViewUniformBufferParameter = GetUniformBufferParameter<FViewUniformShaderParameters>();
		CheckShaderIsValid();
		SetUniformBufferParameter(ShaderRHI, ViewUniformBufferParameter, ViewUniformBuffer);

// 		if (View.bShouldBindInstancedViewUB && View.Family->Views.Num() > 0)
// 		{
// 			// When drawing the left eye in a stereo scene, copy the right eye view values into the instanced view uniform buffer.
// 			const EStereoscopicPass StereoPassIndex = (View.StereoPass != eSSP_FULL) ? eSSP_RIGHT_EYE : eSSP_FULL;
// 
// 			const FSceneView& InstancedView = View.Family->GetStereoEyeView(StereoPassIndex);
// 			const auto& InstancedViewUniformBufferParameter = GetUniformBufferParameter<FInstancedViewUniformShaderParameters>();
// 			SetUniformBufferParameter(RHICmdList, ShaderRHI, InstancedViewUniformBufferParameter, InstancedView.ViewUniformBuffer);
// 		}
	}

	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{ }

	/** Sets pixel parameters that are material specific but not FMeshBatch specific. */
	template< typename ShaderRHIParamRef >
	void SetParametersInner(
		const ShaderRHIParamRef ShaderRHI,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FMaterial& Material,
		const FSceneView& View);

	/** Sets pixel parameters that are material specific but not FMeshBatch specific. */
	template< typename ShaderRHIParamRef >
	void SetParameters(
		const ShaderRHIParamRef ShaderRHI,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FMaterial& Material,
		const FSceneView& View,
		const TUniformBufferPtr<FViewUniformShaderParameters>& ViewUniformBuffer,
		ESceneTextureSetupMode SceneTextureSetupMode);

	// FShader interface.
	//virtual uint32 GetAllocatedSize() const override;

// 	void SetInstanceParameters(uint32 InVertexOffset, uint32 InInstanceOffset, uint32 InInstanceCount) const
// 	{
// 		bool const bZeroInstanceOffset = IsVulkanPlatform(GMaxRHIShaderPlatform) || IsVulkanMobilePlatform(GMaxRHIShaderPlatform);
// 		SetShaderValue(RHICmdList, GetVertexShader(), VertexOffset, bZeroInstanceOffset ? 0 : InVertexOffset);
// 		SetShaderValue(RHICmdList, GetVertexShader(), InstanceOffset, bZeroInstanceOffset ? 0 : InInstanceOffset);
// 		SetShaderValue(RHICmdList, GetVertexShader(), InstanceCount, InInstanceCount);
// 	}

protected:

	FSceneTextureShaderParameters SceneTextureParameters;

private:

	FShaderUniformBufferParameter MaterialUniformBuffer;
	std::vector<FShaderUniformBufferParameter> ParameterCollectionUniformBuffers;

// 	FShaderParameter InstanceCount;
// 	FShaderParameter InstanceOffset;
// 	FShaderParameter VertexOffset;

	/** If true, cached uniform expressions are allowed. */
	static int32 bAllowCachedUniformExpressions;
};

template< typename ShaderRHIParamRef >
void FMaterialShader::SetParametersInner(
	const ShaderRHIParamRef ShaderRHI,
	const FMaterialRenderProxy* MaterialRenderProxy, 
	const FMaterial& Material,
	const FSceneView& View
)
{

}


template< typename ShaderRHIParamRef >
void FMaterialShader::SetParameters(
	const ShaderRHIParamRef ShaderRHI,
	const FMaterialRenderProxy* MaterialRenderProxy, 
	const FMaterial& Material,
	const FSceneView& View,
	const TUniformBufferPtr<FViewUniformShaderParameters>& ViewUniformBuffer,
	ESceneTextureSetupMode SceneTextureSetupMode)
{

}

/** A macro to implement material shaders. */
#define IMPLEMENT_MATERIAL_SHADER_TYPE(TemplatePrefix,ShaderClass,SourceFilename,FunctionName,Frequency) \
	IMPLEMENT_SHADER_TYPE( \
		TemplatePrefix, \
		ShaderClass, \
		SourceFilename, \
		FunctionName, \
		Frequency \
		);
