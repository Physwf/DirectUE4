#include "MaterialShader.h"

FMaterialShader::FMaterialShader(const FMaterialShaderType::CompiledShaderInitializerType& Initializer)
	: FShader(Initializer)
{
	MaterialUniformBuffer.Bind(Initializer.ParameterMap, ("Material"));

// 	for (int32 CollectionIndex = 0; CollectionIndex < Initializer.UniformExpressionSet.ParameterCollections.Num(); CollectionIndex++)
// 	{
// 		FShaderUniformBufferParameter CollectionParameter;
// 		CollectionParameter.Bind(Initializer.ParameterMap, *FString::Printf(TEXT("MaterialCollection%u"), CollectionIndex));
// 		ParameterCollectionUniformBuffers.Add(CollectionParameter);
// 	}

	SceneTextureParameters.Bind(Initializer);

// 	InstanceCount.Bind(Initializer.ParameterMap, TEXT("InstanceCount"));
// 	InstanceOffset.Bind(Initializer.ParameterMap, TEXT("InstanceOffset"));
// 	VertexOffset.Bind(Initializer.ParameterMap, TEXT("VertexOffset"));
}



