#pragma once

#include "PrimitiveUniformBufferParameters.h"
#include "SceneRenderTargetParameters.h"
#include "SceneView.h"
#include "MeshMaterialShader.h"

/** Base Hull shader for drawing policy rendering */
class FBaseHS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FBaseHS, MeshMaterial);
public:

	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
// 		if (!RHISupportsTessellation(Platform))
// 		{
// 			return false;
// 		}

		if (VertexFactoryType && !VertexFactoryType->SupportsTessellationShaders())
		{
			// VF can opt out of tessellation
			return false;
		}

		if (!Material || Material->GetTessellationMode() == MTM_NoTessellation)
		{
			// Material controls use of tessellation
			return false;
		}

		return true;
	}

	FBaseHS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FMeshMaterialShader(Initializer)
	{
		PassUniformBuffer.Bind(Initializer.ParameterMap, FSceneTexturesUniformParameters::GetConstantBufferName().c_str());
	}

	FBaseHS() {}

	void SetParameters(
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FSceneView& View,
		const TUniformBufferPtr<FViewUniformShaderParameters>& ViewUniformBuffer,
		FUniformBuffer* PassUniformBufferValue)
	{
		FMeshMaterialShader::SetParameters(GetHullShader(), MaterialRenderProxy, *MaterialRenderProxy->GetMaterial(), View, ViewUniformBuffer, PassUniformBufferValue);
	}

	void SetMesh(const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatchElement& BatchElement, const FDrawingPolicyRenderState& DrawRenderState)
	{
		FMeshMaterialShader::SetMesh(GetHullShader(), VertexFactory, View, Proxy, BatchElement, DrawRenderState);
	}
};

/** Base Domain shader for drawing policy rendering */
class FBaseDS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FBaseDS, MeshMaterial);
public:

	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
// 		if (!RHISupportsTessellation(Platform))
// 		{
// 			return false;
// 		}

		if (VertexFactoryType && !VertexFactoryType->SupportsTessellationShaders())
		{
			// VF can opt out of tessellation
			return false;
		}

		if (!Material || Material->GetTessellationMode() == MTM_NoTessellation)
		{
			// Material controls use of tessellation
			return false;
		}

		return true;
	}

	FBaseDS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FMeshMaterialShader(Initializer)
	{
		PassUniformBuffer.Bind(Initializer.ParameterMap, FSceneTexturesUniformParameters::GetConstantBufferName().c_str());
	}

	FBaseDS() {}

	void SetParameters(
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FSceneView& View,
		const TUniformBufferPtr<FViewUniformShaderParameters>& ViewUniformBuffer,
		FUniformBuffer* PassUniformBufferValue)
	{
		FMeshMaterialShader::SetParameters(GetDomainShader(), MaterialRenderProxy, *MaterialRenderProxy->GetMaterial(), View, ViewUniformBuffer, PassUniformBufferValue);
	}

	void SetMesh(const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatchElement& BatchElement, const FDrawingPolicyRenderState& DrawRenderState)
	{
		FMeshMaterialShader::SetMesh(GetDomainShader(), VertexFactory, View, Proxy, BatchElement, DrawRenderState);
	}
};
