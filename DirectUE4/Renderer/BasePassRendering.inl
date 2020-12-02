#pragma once

template<typename VertexParametersType>
void TBasePassVertexShaderPolicyParamType<VertexParametersType>::SetMesh(const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatch& Mesh, const FMeshBatchElement& BatchElement, const FDrawingPolicyRenderState& DrawRenderState)
{
	ID3D11VertexShader* VertexShaderRHI = GetVertexShader();
	FMeshMaterialShader::SetMesh(VertexShaderRHI, VertexFactory, View, Proxy, BatchElement, DrawRenderState);

	const bool bHasPreviousLocalToWorldParameter = PreviousLocalToWorldParameter.IsBound();
	const bool bHasSkipOutputVelocityParameter = SkipOutputVelocityParameter.IsBound();

	float SkipOutputVelocityValue = 1.0f;
	FMatrix PreviousLocalToWorldMatrix;

	if (bHasPreviousLocalToWorldParameter || bHasSkipOutputVelocityParameter)
	{
		if (Proxy)
		{
			// 			bool bHasPreviousLocalToWorldMatrix = false;
			// 			const auto& ViewInfo = (const FViewInfo&)View;
			// 
			// 			if (FVelocityDrawingPolicy::HasVelocityOnBasePass(ViewInfo, Proxy, Proxy->GetPrimitiveSceneInfo(), Mesh,
			// 				bHasPreviousLocalToWorldMatrix, PreviousLocalToWorldMatrix))
			// 			{
			// 				PreviousLocalToWorldMatrix = bHasPreviousLocalToWorldMatrix ? PreviousLocalToWorldMatrix : Proxy->GetLocalToWorld();
			// 
			// 				SkipOutputVelocityValue = 0.0f;
			// 			}
			// 			else
			{
				PreviousLocalToWorldMatrix.SetIdentity();
			}
		}
		else
		{
			PreviousLocalToWorldMatrix.SetIdentity();
		}
	}

	if (bHasPreviousLocalToWorldParameter)
	{
		SetShaderValue(VertexShaderRHI, PreviousLocalToWorldParameter, PreviousLocalToWorldMatrix);
	}

	if (bHasSkipOutputVelocityParameter)
	{
		SetShaderValue(VertexShaderRHI, SkipOutputVelocityParameter, SkipOutputVelocityValue);
	}

}

template<typename VertexParametersType>
void TBasePassVertexShaderPolicyParamType<VertexParametersType>::SetInstancedEyeIndex(const uint32 EyeIndex)
{
	if (InstancedEyeIndexParameter.IsBound())
	{
		SetShaderValue(GetVertexShader(), InstancedEyeIndexParameter, EyeIndex);
	}
}

template<typename PixelParametersType>
void TBasePassPixelShaderPolicyParamType<PixelParametersType>::SetMesh(const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatchElement& BatchElement, const FDrawingPolicyRenderState& DrawRenderState, EBlendMode BlendMode)
{
// 	if (View.GetFeatureLevel() >= ERHIFeatureLevel::SM4)
// 	{
// 		ReflectionParameters.SetMesh(RHICmdList, GetPixelShader(), View, Proxy, View.GetFeatureLevel());
// 	}

	FMeshMaterialShader::SetMesh(GetPixelShader(), VertexFactory, View, Proxy, BatchElement, DrawRenderState);
}