#pragma once

#include "MaterialShader.h"
#include "VertexFactory.h"

struct FMeshBatchElement;
struct FDrawingPolicyRenderState;

/** Base class of all shaders that need material and vertex factory parameters. */
class FMeshMaterialShader : public FMaterialShader
{
public:
	FMeshMaterialShader() {}

	FMeshMaterialShader(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer)
		: FMaterialShader(Initializer)
		, VertexFactoryParameters(Initializer.VertexFactoryType, Initializer.ParameterMap, (EShaderFrequency)Initializer.Frequency)
	{
		//NonInstancedDitherLODFactorParameter.Bind(Initializer.ParameterMap, TEXT("NonInstancedDitherLODFactor"));
	}

	static bool ValidateCompiledResult(const std::vector<FMaterial*>& Materials, const FVertexFactoryType* VertexFactoryType, const FShaderParameterMap& ParameterMap, std::vector<std::string>& OutError)
	{
		return true;
	}

	inline void ValidateAfterBind()
	{
		//assert(PassUniformBuffer.IsInitialized(), TEXT("FMeshMaterialShader must bind a pass uniform buffer, even if it is just FSceneTexturesUniformParameters: %s"), GetType()->GetName());
	}

	template< typename ShaderRHIParamRef >
	void SetPassUniformBuffer(
		const ShaderRHIParamRef ShaderRHI,
		std::shared_ptr<FUniformBuffer> PassUniformBufferValue)
	{
		SetUniformBufferParameter(ShaderRHI, PassUniformBuffer, PassUniformBufferValue);
	}

	template< typename ShaderRHIParamRef >
	void SetParameters(
		const ShaderRHIParamRef ShaderRHI,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FMaterial& Material,
		const FSceneView& View,
		const TUniformBufferPtr<FViewUniformShaderParameters>& ViewUniformBuffer,
		FUniformBuffer* PassUniformBufferValue)
	{
		SetUniformBufferParameter(ShaderRHI, PassUniformBuffer, PassUniformBufferValue);

		//assert(!(PassUniformBuffer.IsBound() && SceneTextureParameters.IsBound()) || SceneTextureParameters.IsSameUniformParameter(PassUniformBuffer), TEXT("If the pass uniform buffer is bound, it should contain SceneTexturesStruct: %s"), GetType()->GetName());

		SetViewParameters(ShaderRHI, View, ViewUniformBuffer);
		FMaterialShader::SetParametersInner(ShaderRHI, MaterialRenderProxy, Material, View);
	}

	template< typename ShaderRHIParamRef >
	void SetMesh(
		const ShaderRHIParamRef ShaderRHI,
		const FVertexFactory* VertexFactory,
		const FSceneView& View,
		//const FPrimitiveSceneProxy* Proxy,
		const FMeshBatchElement& BatchElement,
		const FDrawingPolicyRenderState& DrawRenderState,
		uint32 DataFlags = 0
	);

	/**
	* Retrieves the fade uniform buffer parameter from a FSceneViewState for the primitive
	* This code was moved from SetMesh() to work around the template first-use vs first-seen differences between MSVC and others
	*/
	//std::shared_ptr<FUniformBuffer> GetPrimitiveFadeUniformBufferParameter(const FSceneView& View, const FPrimitiveSceneProxy* Proxy);

	// FShader interface.
	virtual const FVertexFactoryParameterRef* GetVertexFactoryParameterRef() const override { return &VertexFactoryParameters; }
	//virtual uint32 GetAllocatedSize() const override;

protected:
	FShaderUniformBufferParameter PassUniformBuffer;

private:
	FVertexFactoryParameterRef VertexFactoryParameters;
	FShaderParameter NonInstancedDitherLODFactorParameter;
};

template< typename ShaderRHIParamRef >
void FMeshMaterialShader::SetMesh(
	const ShaderRHIParamRef ShaderRHI,
	const FVertexFactory* VertexFactory,
	const FSceneView& View,
	/*const FPrimitiveSceneProxy* Proxy, */
	const FMeshBatchElement& BatchElement,
	const FDrawingPolicyRenderState& DrawRenderState,
	uint32 DataFlags /*= 0 */)
{
	VertexFactoryParameters.SetMesh(this, VertexFactory, View, BatchElement,0);

	if (BatchElement.PrimitiveUniformBuffer)
	{
		SetUniformBufferParameter(ShaderRHI, GetUniformBufferParameter<FPrimitiveUniformShaderParameters>(), BatchElement.PrimitiveUniformBuffer);
	}
}

