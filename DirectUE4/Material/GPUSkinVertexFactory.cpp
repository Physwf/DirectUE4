#include "GPUSkinVertexFactory.h"
#include "Material.h"

FBoneMatricesUniformShaderParameters Bones;

#define IMPLEMENT_GPUSKINNING_VERTEX_FACTORY_TYPE_INTERNAL(FactoryClass, ShaderFilename,bUsedWithMaterials,bSupportsStaticLighting,bSupportsDynamicLighting,bPrecisePrevWorldPos,bSupportsPositionOnly) \
	template <bool bExtraBoneInfluencesT> FVertexFactoryType FactoryClass<bExtraBoneInfluencesT>::StaticType( \
	bExtraBoneInfluencesT ? #FactoryClass "true" : #FactoryClass "false", \
	(ShaderFilename), \
	bUsedWithMaterials, \
	bSupportsStaticLighting, \
	bSupportsDynamicLighting, \
	bPrecisePrevWorldPos, \
	bSupportsPositionOnly, \
	Construct##FactoryClass##ShaderParameters<bExtraBoneInfluencesT>, \
	FactoryClass<bExtraBoneInfluencesT>::ShouldCompilePermutation, \
	FactoryClass<bExtraBoneInfluencesT>::ModifyCompilationEnvironment, \
	FactoryClass<bExtraBoneInfluencesT>::SupportsTessellationShaders \
	); \
	template <bool bExtraBoneInfluencesT> inline FVertexFactoryType* FactoryClass<bExtraBoneInfluencesT>::GetType() const { return &StaticType; }


#define IMPLEMENT_GPUSKINNING_VERTEX_FACTORY_TYPE(FactoryClass, ShaderFilename,bUsedWithMaterials,bSupportsStaticLighting,bSupportsDynamicLighting,bPrecisePrevWorldPos,bSupportsPositionOnly) \
	template <bool bExtraBoneInfluencesT> FVertexFactoryShaderParameters* Construct##FactoryClass##ShaderParameters(EShaderFrequency ShaderFrequency) { return FactoryClass<bExtraBoneInfluencesT>::ConstructShaderParameters(ShaderFrequency); } \
	IMPLEMENT_GPUSKINNING_VERTEX_FACTORY_TYPE_INTERNAL(FactoryClass, ShaderFilename,bUsedWithMaterials,bSupportsStaticLighting,bSupportsDynamicLighting,bPrecisePrevWorldPos,bSupportsPositionOnly) \
	template class FactoryClass<false>;	\
	template class FactoryClass<true>;


template<bool bExtraBoneInfluencesT>
bool TGPUSkinVertexFactory<bExtraBoneInfluencesT>::ShouldCompilePermutation(const class FMaterial* Material, const FShaderType* ShaderType)
{
	return (Material->IsUsedWithSkeletalMesh() || Material->IsSpecialEngineMaterial());
}

template<bool bExtraBoneInfluencesT>
void TGPUSkinVertexFactory<bExtraBoneInfluencesT>::ModifyCompilationEnvironment(const class FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
{
	FVertexFactory::ModifyCompilationEnvironment(Material, OutEnvironment);
	const int32 MaxGPUSkinBones = 256;// GetFeatureLevelMaxNumberOfBones(GetMaxSupportedFeatureLevel(Platform));
	OutEnvironment.SetDefine(("MAX_SHADER_BONES"), MaxGPUSkinBones);
	const uint32 UseExtraBoneInfluences = bExtraBoneInfluencesT;
	OutEnvironment.SetDefine(("GPUSKIN_USE_EXTRA_INFLUENCES"), UseExtraBoneInfluences);
	{
		bool bLimit2BoneInfluences = false;// (CVarGPUSkinLimit2BoneInfluences.GetValueOnAnyThread() != 0);
		OutEnvironment.SetDefine(("GPUSKIN_LIMIT_2BONE_INFLUENCES"), (bLimit2BoneInfluences ? 1 : 0));
	}
}

template<bool bExtraBoneInfluencesT>
void TGPUSkinVertexFactory<bExtraBoneInfluencesT>::ReleaseRHI()
{

}

template<bool bExtraBoneInfluencesT>
void TGPUSkinVertexFactory<bExtraBoneInfluencesT>::InitRHI()
{
	std::vector<D3D11_INPUT_ELEMENT_DESC> Elements;
	AddVertexElements(Data, Elements);

	// create the actual device decls
	InitDeclaration(Elements);
}

template<bool bExtraBoneInfluencesT>
void TGPUSkinVertexFactory<bExtraBoneInfluencesT>::AddVertexElements(FDataType& InData, std::vector<D3D11_INPUT_ELEMENT_DESC>& OutElements)
{
	// position decls
	OutElements.push_back(AccessStreamComponent(InData.PositionComponent, 0));

	// tangent basis vector decls
	OutElements.push_back(AccessStreamComponent(InData.TangentBasisComponents[0], 1));
	OutElements.push_back(AccessStreamComponent(InData.TangentBasisComponents[1], 2));

	const uint8 BaseTexCoordAttribute = 5;
	//for (int32 CoordinateIndex = 0; CoordinateIndex < InData.TextureCoordinates.Num(); CoordinateIndex++)
	{
		OutElements.push_back(AccessStreamComponent(
			InData.TextureCoordinates,
			BaseTexCoordAttribute + 0//CoordinateIndex
		));
	}

	// bone indices decls
	OutElements.push_back(AccessStreamComponent(InData.BoneIndices, 3));

	// bone weights decls
	OutElements.push_back(AccessStreamComponent(InData.BoneWeights, 4));
}


/** Shader parameters for use with TGPUSkinVertexFactory */
class FGPUSkinVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:
	virtual void Bind(const FShaderParameterMap& ParameterMap) override
	{
		PerBoneMotionBlur.Bind(ParameterMap, ("PerBoneMotionBlur"));
		BoneMatrices.Bind(ParameterMap, ("BoneMatrices"));
		PreviousBoneMatrices.Bind(ParameterMap, ("PreviousBoneMatrices"));
	}
	virtual void SetMesh(FShader* Shader, const FVertexFactory* VertexFactory, const FSceneView& View, const FMeshBatchElement& BatchElement, uint32 DataFlags) const override
	{

	}
private:
	FShaderParameter PerBoneMotionBlur;
	FShaderResourceParameter BoneMatrices;
	FShaderResourceParameter PreviousBoneMatrices;
};

template <bool bExtraBoneInfluencesT>
FVertexFactoryShaderParameters* TGPUSkinVertexFactory<bExtraBoneInfluencesT>::ConstructShaderParameters(EShaderFrequency ShaderFrequency)
{
	return (ShaderFrequency == SF_Vertex) ? new FGPUSkinVertexFactoryShaderParameters() : NULL;
}

IMPLEMENT_GPUSKINNING_VERTEX_FACTORY_TYPE(TGPUSkinVertexFactory, "GpuSkinVertexFactory.dusf", true, false, true, false, false);