#pragma once

#include "D3D11RHI.h"
#include "UnrealMath.h"

class FForwardLocalLightData
{
public:
	Vector4 LightPositionAndInvRadius;
	Vector4 LightColorAndFalloffExponent;
	Vector4 LightDirectionAndShadowMapChannelMask;
	Vector4 SpotAnglesAndSourceRadiusPacked;
	Vector4 LightTangentAndSoftSourceRadius;
};

struct alignas(16) FSharedBasePassUniformParameters
{
	FSharedBasePassUniformParameters()
	{
		ConstructUniformBufferInfo(*this);
	}

	struct ConstantStruct
	{
		FForwardLightData::ConstantStruct Forward;
		FForwardLightData::ConstantStruct ForwardISR;
		FReflectionUniformParameters::ConstantStruct Reflection;
		FFogUniformParameters::ConstantStruct Fog;
	} Constants;

	ID3D11ShaderResourceView* SSProfilesTexture;

	static std::string GetConstantBufferName()
	{
		return "BasePass";
	}

#define ADD_RES(StructName, MemberName) List.insert(std::make_pair(std::string(#StructName) + "_" + std::string(#MemberName),StructName.MemberName))
	static std::map<std::string, ID3D11ShaderResourceView*> GetSRVs(const FSharedBasePassUniformParameters& BasePass)
	{
		std::map<std::string, ID3D11ShaderResourceView*> List;
		ADD_RES(BasePass, SSProfilesTexture);
		return List;
	}
	static std::map<std::string, ID3D11SamplerState*> GetSamplers(const FSharedBasePassUniformParameters& BasePass)
	{
		std::map<std::string, ID3D11SamplerState*> List;
		return List;
	}
	static std::map<std::string, ID3D11UnorderedAccessView*> GetUAVs(const FSharedBasePassUniformParameters& BasePass)
	{
		std::map<std::string, ID3D11UnorderedAccessView*> List;
		return List;
	}
#undef ADD_RES
};

struct alignas(16) FOpaqueBasePassUniformParameters
{
	FSharedBasePassUniformParameters Shared;
	// Forward shading 
	ID3D11ShaderResourceView* ForwardScreenSpaceShadowMaskTexture;
	ID3D11SamplerState* ForwardScreenSpaceShadowMaskTextureSampler;
	ID3D11ShaderResourceView* IndirectOcclusionTexture;
	ID3D11SamplerState* IndirectOcclusionTextureSampler;
	ID3D11ShaderResourceView* ResolvedSceneDepthTexture;
	// DBuffer decals
	ID3D11ShaderResourceView* DBufferATexture;
	ID3D11SamplerState* DBufferATextureSampler;
	ID3D11ShaderResourceView* DBufferBTexture;
	ID3D11SamplerState* DBufferBTextureSampler;
	ID3D11ShaderResourceView* DBufferCTexture;
	ID3D11SamplerState* DBufferCTextureSampler;
	ID3D11ShaderResourceView* DBufferRenderMask;//Texture2D<uint>
	// Misc
	ID3D11ShaderResourceView* EyeAdaptation;

	static std::string GetConstantBufferName()
	{
		return "OpaqueBasePass";
	}

#define ADD_RES(StructName, MemberName) List.insert(std::make_pair(std::string(#StructName) + "_" + std::string(#MemberName),StructName.MemberName))
	static std::map<std::string, ID3D11ShaderResourceView*> GetSRVs(const FOpaqueBasePassUniformParameters& OpaqueBasePass)
	{
		std::map<std::string, ID3D11ShaderResourceView*> List;
		ADD_RES(OpaqueBasePass, ForwardScreenSpaceShadowMaskTexture);
		ADD_RES(OpaqueBasePass, IndirectOcclusionTexture);
		ADD_RES(OpaqueBasePass, ResolvedSceneDepthTexture);
		ADD_RES(OpaqueBasePass, DBufferATexture);
		ADD_RES(OpaqueBasePass, DBufferBTexture);
		ADD_RES(OpaqueBasePass, DBufferCTexture);
		ADD_RES(OpaqueBasePass, DBufferRenderMask);
		ADD_RES(OpaqueBasePass, EyeAdaptation);
		return List;
	}
	static std::map<std::string, ID3D11SamplerState*> GetSamplers(const FOpaqueBasePassUniformParameters& BasePass)
	{
		std::map<std::string, ID3D11SamplerState*> List;
		ADD_RES(OpaqueBasePass, ForwardScreenSpaceShadowMaskTextureSampler);
		ADD_RES(OpaqueBasePass, IndirectOcclusionTextureSampler);
		ADD_RES(OpaqueBasePass, DBufferATextureSampler);
		ADD_RES(OpaqueBasePass, DBufferBTextureSampler);
		ADD_RES(OpaqueBasePass, DBufferCTextureSampler);
		return List;
	}
	static std::map<std::string, ID3D11UnorderedAccessView*> GetUAVs(const FOpaqueBasePassUniformParameters& BasePass)
	{
		std::map<std::string, ID3D11UnorderedAccessView*> List;
		return List;
	}
#undef ADD_RES
};

/** Parameters for computing forward lighting. */
class FForwardLightingParameters
{
public:

	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(("LOCAL_LIGHT_DATA_STRIDE"), FMath::DivideAndRoundUp<int32>(sizeof(FForwardLocalLightData), sizeof(Vector4)));
		extern int32 NumCulledLightsGridStride;
		OutEnvironment.SetDefine(("NUM_CULLED_LIGHTS_GRID_STRIDE"), NumCulledLightsGridStride);
		extern int32 NumCulledGridPrimitiveTypes;
		OutEnvironment.SetDefine(("NUM_CULLED_GRID_PRIMITIVE_TYPES"), NumCulledGridPrimitiveTypes);
	}
};
extern void SetupSharedBasePassParameters(
	const FViewInfo& View,
	FSceneRenderTargets& SceneRenderTargets,
	FSharedBasePassUniformParameters& BasePassParameters);

extern void CreateOpaqueBasePassUniformBuffer(
	const FViewInfo& View,
	PooledRenderTarget* ForwardScreenSpaceShadowMask,
	TUniformBufferPtr<FOpaqueBasePassUniformParameters>& BasePassUniformBuffer);

inline void BindBasePassUniformBuffer(const FShaderParameterMap& ParameterMap, FShaderUniformBufferParameter& BasePassUniformBuffer)
{
	//TArray<const FUniformBufferStruct*> NestedStructs;
	//FOpaqueBasePassUniformParameters::StaticStruct.GetNestedStructs(NestedStructs);
	//FTranslucentBasePassUniformParameters::StaticStruct.GetNestedStructs(NestedStructs);

// 	for (int32 StructIndex = 0; StructIndex < NestedStructs.Num(); StructIndex++)
// 	{
// 		const char* StructVariableName = NestedStructs[StructIndex]->GetShaderVariableName();
// 		assert(!ParameterMap.ContainsParameterAllocation(StructVariableName)/*, TEXT("%s found bound in the base pass.  Base Pass uniform buffer nested structs should not be bound separately"), StructVariableName*/);
// 	}

	const bool bNeedsOpaqueBasePass = ParameterMap.ContainsParameterAllocation(FOpaqueBasePassUniformParameters::GetConstantBufferName().c_str());
	//const bool bNeedsTransparentBasePass = ParameterMap.ContainsParameterAllocation(FTranslucentBasePassUniformParameters::StaticStruct.GetShaderVariableName());

	//assert(!(bNeedsOpaqueBasePass && bNeedsTransparentBasePass));

	BasePassUniformBuffer.Bind(ParameterMap, FOpaqueBasePassUniformParameters::GetConstantBufferName().c_str());

// 	if (!BasePassUniformBuffer.IsBound())
// 	{
// 		BasePassUniformBuffer.Bind(ParameterMap, FTranslucentBasePassUniformParameters::StaticStruct.GetShaderVariableName());
// 	}
}

/**
* The base shader type for vertex shaders that render the emissive color, and light-mapped/ambient lighting of a mesh.
* The base type is shared between the versions with and without atmospheric fog.
*/
template<typename VertexParametersType>
class TBasePassVertexShaderPolicyParamType : public FMeshMaterialShader, public VertexParametersType
{
protected:

	TBasePassVertexShaderPolicyParamType() {}
	TBasePassVertexShaderPolicyParamType(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) :
		FMeshMaterialShader(Initializer)
	{
		VertexParametersType::Bind(Initializer.ParameterMap);
		BindBasePassUniformBuffer(Initializer.ParameterMap, PassUniformBuffer);
		ReflectionCaptureBuffer.Bind(Initializer.ParameterMap, ("ReflectionCapture"));
// 		const bool bOutputsVelocityToGBuffer = FVelocityRendering::OutputsToGBuffer();
// 		if (bOutputsVelocityToGBuffer)
// 		{
// 			PreviousLocalToWorldParameter.Bind(Initializer.ParameterMap, ("PreviousLocalToWorld"));
// 			//@todo-rco: Move to pixel shader
// 			SkipOutputVelocityParameter.Bind(Initializer.ParameterMap, ("SkipOutputVelocity"));
// 		}

		InstancedEyeIndexParameter.Bind(Initializer.ParameterMap, ("InstancedEyeIndex"));
		IsInstancedStereoParameter.Bind(Initializer.ParameterMap, ("bIsInstancedStereo"));
	}

public:

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Material, OutEnvironment);
		FForwardLightingParameters::ModifyCompilationEnvironment(OutEnvironment);
	}

	void SetParameters(
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FVertexFactory* VertexFactory,
		const FMaterial& InMaterialResource,
		const FViewInfo& View,
		const FDrawingPolicyRenderState& DrawRenderState,
		bool bIsInstancedStereo
	)
	{
		ID3D11VertexShader* const ShaderRHI = GetVertexShader();

		FMeshMaterialShader::SetParameters(ShaderRHI, MaterialRenderProxy, InMaterialResource, View, DrawRenderState.GetViewUniformBuffer(), DrawRenderState.GetPassUniformBuffer());

		SetUniformBufferParameter(ShaderRHI, ReflectionCaptureBuffer, View.ReflectionCaptureUniformBuffer);

		if (IsInstancedStereoParameter.IsBound())
		{
			SetShaderValue(ShaderRHI, IsInstancedStereoParameter, bIsInstancedStereo);
		}

		if (InstancedEyeIndexParameter.IsBound())
		{
			SetShaderValue(ShaderRHI, InstancedEyeIndexParameter, 0);
		}
	}

	void SetMesh(const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatch& Mesh, const FMeshBatchElement& BatchElement, const FDrawingPolicyRenderState& DrawRenderState);

	void SetInstancedEyeIndex(const uint32 EyeIndex);

private:

	FShaderUniformBufferParameter ReflectionCaptureBuffer;
	// When outputting from base pass, the previous transform
	FShaderParameter PreviousLocalToWorldParameter;
	FShaderParameter SkipOutputVelocityParameter;
	FShaderParameter InstancedEyeIndexParameter;
	FShaderParameter IsInstancedStereoParameter;
};




/**
* The base shader type for vertex shaders that render the emissive color, and light-mapped/ambient lighting of a mesh.
* The base type is shared between the versions with and without atmospheric fog.
*/

template<typename LightMapPolicyType>
class TBasePassVertexShaderBaseType : public TBasePassVertexShaderPolicyParamType<typename LightMapPolicyType::VertexParametersType>
{
	typedef TBasePassVertexShaderPolicyParamType<typename LightMapPolicyType::VertexParametersType> Super;

protected:

	TBasePassVertexShaderBaseType(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) : Super(Initializer) {}

	TBasePassVertexShaderBaseType() {}

public:

	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		return LightMapPolicyType::ShouldCompilePermutation(Material, VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		LightMapPolicyType::ModifyCompilationEnvironment(Material, OutEnvironment);
		Super::ModifyCompilationEnvironment(Material, OutEnvironment);
	}
};

template<typename LightMapPolicyType, bool bEnableAtmosphericFog>
class TBasePassVS : public TBasePassVertexShaderBaseType<LightMapPolicyType>
{
	DECLARE_SHADER_TYPE(TBasePassVS, MeshMaterial);
	typedef TBasePassVertexShaderBaseType<LightMapPolicyType> Super;

protected:

	TBasePassVS() {}
	TBasePassVS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) :
		Super(Initializer)
	{
	}

public:
	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		static const auto SupportAtmosphericFog = true;// IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.SupportAtmosphericFog"));
		static const auto SupportAllShaderPermutations = false;// IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.SupportAllShaderPermutations"));
		const bool bForceAllPermutations = SupportAllShaderPermutations /*&& SupportAllShaderPermutations->GetValueOnAnyThread() != 0*/;

		const bool bProjectAllowsAtmosphericFog = /*!SupportAtmosphericFog || SupportAtmosphericFog->GetValueOnAnyThread() != 0*/SupportAtmosphericFog || bForceAllPermutations;

		bool bShouldCache = Super::ShouldCompilePermutation(Material, VertexFactoryType);
		bShouldCache &= (bEnableAtmosphericFog && bProjectAllowsAtmosphericFog && IsTranslucentBlendMode(Material->GetBlendMode())) || !bEnableAtmosphericFog;

		return bShouldCache && true/*(IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4))*/;
	}

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		Super::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		// @todo MetalMRT: Remove this hack and implement proper atmospheric-fog solution for Metal MRT...
		OutEnvironment.SetDefine(("BASEPASS_ATMOSPHERIC_FOG"), /*(Platform != SP_METAL_MRT && Platform != SP_METAL_MRT_MAC)*/true ? bEnableAtmosphericFog : 0);
	}
};

/** Parameters needed for reflections, shared by multiple shaders. */
class FBasePassReflectionParameters
{
public:

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		//PlanarReflectionParameters.Bind(ParameterMap);
		SingleCubemapArrayIndex.Bind(ParameterMap, ("SingleCubemapArrayIndex"));
		SingleCaptureOffsetAndAverageBrightness.Bind(ParameterMap, ("SingleCaptureOffsetAndAverageBrightness"));
		SingleCapturePositionAndRadius.Bind(ParameterMap, ("SingleCapturePositionAndRadius"));
		SingleCaptureBrightness.Bind(ParameterMap, ("SingleCaptureBrightness"));
	}

	void SetMesh(ID3D11PixelShader* PixelShaderRH, const FSceneView& View, const FPrimitiveSceneProxy* Proxy);

private:

	//FPlanarReflectionParameters PlanarReflectionParameters;

	FShaderParameter SingleCubemapArrayIndex;
	FShaderParameter SingleCaptureOffsetAndAverageBrightness;
	FShaderParameter SingleCapturePositionAndRadius;
	FShaderParameter SingleCaptureBrightness;
};

/**
* The base type for pixel shaders that render the emissive color, and light-mapped/ambient lighting of a mesh.
* The base type is shared between the versions with and without sky light.
*/
template<typename PixelParametersType>
class TBasePassPixelShaderPolicyParamType : public FMeshMaterialShader, public PixelParametersType
{
public:

	// static bool ShouldCompilePermutation(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);

// 		const bool bOutputVelocity = FVelocityRendering::OutputsToGBuffer();
// 		if (bOutputVelocity)
// 		{
// 			const int32 VelocityIndex = 4; // As defined in BasePassPixelShader.usf
// 			OutEnvironment.SetRenderTargetOutputFormat(VelocityIndex, PF_G16R16);
// 		}

		FForwardLightingParameters::ModifyCompilationEnvironment(Platform, OutEnvironment);
	}

	static bool ValidateCompiledResult(const std::vector<FMaterial*>& Materials, const FVertexFactoryType* VertexFactoryType, const FShaderParameterMap& ParameterMap, std::vector<std::string>& OutError)
	{
		if (ParameterMap.ContainsParameterAllocation(FSceneTexturesUniformParameters::GetConstantBufferName().c_str()))
		{
			OutError.push_back(("Base pass shaders cannot read from the SceneTexturesStruct."));
			return false;
		}

		return true;
	}

	/** Initialization constructor. */
	TBasePassPixelShaderPolicyParamType(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) :
		FMeshMaterialShader(Initializer)
	{
		PixelParametersType::Bind(Initializer.ParameterMap);
		BindBasePassUniformBuffer(Initializer.ParameterMap, PassUniformBuffer);
		//ReflectionParameters.Bind(Initializer.ParameterMap);
		//ReflectionCaptureBuffer.Bind(Initializer.ParameterMap, ("ReflectionCapture"));

		// These parameters should only be used nested in the base pass uniform buffer
		assert(!Initializer.ParameterMap.ContainsParameterAllocation(FFogUniformParameters::GetConstantBufferName().c_str()));
		assert(!Initializer.ParameterMap.ContainsParameterAllocation(FReflectionUniformParameters::GetConstantBufferName().c_str()));
	}
	TBasePassPixelShaderPolicyParamType() {}

	void SetParameters(
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FMaterial& MaterialResource,
		const FViewInfo* View,
		const FDrawingPolicyRenderState& DrawRenderState,
		EBlendMode BlendMode)
	{
		ID3D11PixelShader* const ShaderRHI = GetPixelShader();

		FMeshMaterialShader::SetParameters(ShaderRHI, MaterialRenderProxy, MaterialResource, *View, DrawRenderState.GetViewUniformBuffer(), DrawRenderState.GetPassUniformBuffer());

		//SetUniformBufferParameter(ShaderRHI, ReflectionCaptureBuffer, View->ReflectionCaptureUniformBuffer);
	}

	void SetMesh(const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatchElement& BatchElement, const FDrawingPolicyRenderState& DrawRenderState, EBlendMode BlendMode);

private:
	//FBasePassReflectionParameters ReflectionParameters;
	//FShaderUniformBufferParameter ReflectionCaptureBuffer;
};



/**
* The base type for pixel shaders that render the emissive color, and light-mapped/ambient lighting of a mesh.
* The base type is shared between the versions with and without sky light.
*/
template<typename LightMapPolicyType>
class TBasePassPixelShaderBaseType : public TBasePassPixelShaderPolicyParamType<typename LightMapPolicyType::PixelParametersType>
{
	typedef TBasePassPixelShaderPolicyParamType<typename LightMapPolicyType::PixelParametersType> Super;

public:

	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		return LightMapPolicyType::ShouldCompilePermutation(Material, VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		LightMapPolicyType::ModifyCompilationEnvironment(Material, OutEnvironment);
		Super::ModifyCompilationEnvironment(Material, OutEnvironment);
	}

	/** Initialization constructor. */
	TBasePassPixelShaderBaseType(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) : Super(Initializer) {}

	TBasePassPixelShaderBaseType() {}
};

/** The concrete base pass pixel shader type. */
template<typename LightMapPolicyType, bool bEnableSkyLight>
class TBasePassPS : public TBasePassPixelShaderBaseType<LightMapPolicyType>
{
	DECLARE_SHADER_TYPE(TBasePassPS, MeshMaterial);
public:

	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		// Only compile skylight version for lit materials, and if the project allows them.
		static const auto SupportStationarySkylight = true; //IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.SupportStationarySkylight"));
		static const auto SupportAllShaderPermutations = false;// IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.SupportAllShaderPermutations"));

		const bool bTranslucent = IsTranslucentBlendMode(Material->GetBlendMode());
		const bool bForceAllPermutations = SupportAllShaderPermutations /*&& SupportAllShaderPermutations->GetValueOnAnyThread() != 0*/;
		const bool bProjectSupportsStationarySkylight = SupportStationarySkylight/* !SupportStationarySkylight || SupportStationarySkylight->GetValueOnAnyThread() != 0 || bForceAllPermutations*/;

		const bool bCacheShaders = !bEnableSkyLight
			//translucent materials need to compile skylight support to support MOVABLE skylights also.
			|| bTranslucent
			// Some lightmap policies (eg Simple Forward) always require skylight support
			|| LightMapPolicyType::RequiresSkylight()
			|| (bProjectSupportsStationarySkylight && (Material->GetShadingModel() != MSM_Unlit));
		return bCacheShaders
			&& true/*(IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4))*/
			&& TBasePassPixelShaderBaseType<LightMapPolicyType>::ShouldCompilePermutation(Platform, Material, VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		// For deferred decals, the shader class used is FDeferredDecalPS. the TBasePassPS is only used in the material editor and will read wrong values.
		OutEnvironment.SetDefine(("SCENE_TEXTURES_DISABLED"), Material->GetMaterialDomain() != MD_Surface);

		OutEnvironment.SetDefine(("ENABLE_SKY_LIGHT"), bEnableSkyLight);
		TBasePassPixelShaderBaseType<LightMapPolicyType>::ModifyCompilationEnvironment(Material, OutEnvironment);
	}

	/** Initialization constructor. */
	TBasePassPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		TBasePassPixelShaderBaseType<LightMapPolicyType>(Initializer)
	{}

	/** Default constructor. */
	TBasePassPS() {}
};

/**
* Get shader templates allowing to redirect between compatible shaders.
*/

template <typename LightMapPolicyType>
void GetBasePassShaders(
	const FMaterial& Material,
	FVertexFactoryType* VertexFactoryType,
	LightMapPolicyType LightMapPolicy,
	bool bNeedsHSDS,
	bool bEnableAtmosphericFog,
	bool bEnableSkyLight,
	FBaseHS*& HullShader,
	FBaseDS*& DomainShader,
	TBasePassVertexShaderPolicyParamType<typename LightMapPolicyType::VertexParametersType>*& VertexShader,
	TBasePassPixelShaderPolicyParamType<typename LightMapPolicyType::PixelParametersType>*& PixelShader
)
{
	if (bNeedsHSDS)
	{
		//DomainShader = Material.GetShader<TBasePassDS<LightMapPolicyType > >(VertexFactoryType);

		// Metal requires matching permutations, but no other platform should worry about this complication.
		//if (bEnableAtmosphericFog && DomainShader && IsMetalPlatform(EShaderPlatform(DomainShader->GetTarget().Platform)))
		{
			//HullShader = Material.GetShader<TBasePassHS<LightMapPolicyType, true > >(VertexFactoryType);
		}
		//else
		{
			//HullShader = Material.GetShader<TBasePassHS<LightMapPolicyType, false > >(VertexFactoryType);
		}
	}

	if (bEnableAtmosphericFog)
	{
		VertexShader = Material.GetShader<TBasePassVS<LightMapPolicyType, true> >(VertexFactoryType);
	}
	else
	{
		VertexShader = Material.GetShader<TBasePassVS<LightMapPolicyType, false> >(VertexFactoryType);
	}
	if (bEnableSkyLight)
	{
		PixelShader = Material.GetShader<TBasePassPS<LightMapPolicyType, true> >(VertexFactoryType);
	}
	else
	{
		PixelShader = Material.GetShader<TBasePassPS<LightMapPolicyType, false> >(VertexFactoryType);
	}
}

template <>
void GetBasePassShaders<FUniformLightMapPolicy>(
	const FMaterial& Material,
	FVertexFactoryType* VertexFactoryType,
	FUniformLightMapPolicy LightMapPolicy,
	bool bNeedsHSDS,
	bool bEnableAtmosphericFog,
	bool bEnableSkyLight,
	FBaseHS*& HullShader,
	FBaseDS*& DomainShader,
	TBasePassVertexShaderPolicyParamType<FUniformLightMapPolicyShaderParametersType>*& VertexShader,
	TBasePassPixelShaderPolicyParamType<FUniformLightMapPolicyShaderParametersType>*& PixelShader
	);

class FBasePassDrawingPolicy : public FMeshDrawingPolicy
{
public:
	FBasePassDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		const FMaterial& InMaterialResource,
		const FMeshDrawingPolicyOverrideSettings& InOverrideSettings,
		/*EDebugViewShaderMode InDebugViewShaderMode,*/
		bool bInEnableReceiveDecalOutput) : FMeshDrawingPolicy(InVertexFactory, InMaterialRenderProxy, InMaterialResource, InOverrideSettings/*, InDebugViewShaderMode*/), bEnableReceiveDecalOutput(bInEnableReceiveDecalOutput)
	{}

	void ApplyDitheredLODTransitionState(FDrawingPolicyRenderState& DrawRenderState, const FViewInfo& ViewInfo, const FStaticMesh& Mesh, const bool InAllowStencilDither);

protected:

	/** Whether or not outputing the receive decal boolean */
	uint32 bEnableReceiveDecalOutput : 1;
};

/**
* Draws the emissive color and the light-map of a mesh.
*/
template<typename LightMapPolicyType>
class TBasePassDrawingPolicy : public FBasePassDrawingPolicy
{
public:

	/** The data the drawing policy uses for each mesh element. */
	class ElementDataType
	{
	public:

		/** The element's light-map data. */
		typename LightMapPolicyType::ElementDataType LightMapElementData;

		/** Default constructor. */
		ElementDataType()
		{}

		/** Initialization constructor. */
		ElementDataType(const typename LightMapPolicyType::ElementDataType& InLightMapElementData)
			: LightMapElementData(InLightMapElementData)
		{}
	};

	/** Initialization constructor. */
	TBasePassDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		const FMaterial& InMaterialResource,
		LightMapPolicyType InLightMapPolicy,
		EBlendMode InBlendMode,
		bool bInEnableSkyLight,
		bool bInEnableAtmosphericFog,
		const FMeshDrawingPolicyOverrideSettings& InOverrideSettings,
		/*EDebugViewShaderMode InDebugViewShaderMode = DVSM_None,*/
		bool bInEnableReceiveDecalOutput = false
	) :
		FBasePassDrawingPolicy(InVertexFactory, InMaterialRenderProxy, InMaterialResource, InOverrideSettings, /*InDebugViewShaderMode,*/ bInEnableReceiveDecalOutput),
		LightMapPolicy(InLightMapPolicy),
		BlendMode(InBlendMode),
		bEnableSkyLight(bInEnableSkyLight),
		bEnableAtmosphericFog(bInEnableAtmosphericFog)
	{
		HullShader = NULL;
		DomainShader = NULL;

		const EMaterialTessellationMode MaterialTessellationMode = InMaterialResource.GetTessellationMode();

		const bool bNeedsHSDS = true;// RHISupportsTessellation(GShaderPlatformForFeatureLevel[InFeatureLevel])
			&& InVertexFactory->GetType()->SupportsTessellationShaders()
			&& MaterialTessellationMode != MTM_NoTessellation;

		GetBasePassShaders<LightMapPolicyType>(
			InMaterialResource,
			VertexFactory->GetType(),
			InLightMapPolicy,
			bNeedsHSDS,
			bEnableAtmosphericFog,
			bEnableSkyLight,
			HullShader,
			DomainShader,
			VertexShader,
			PixelShader
			);

		BaseVertexShader = VertexShader;
	}

	// FMeshDrawingPolicy interface.

// 	FDrawingPolicyMatchResult Matches(const TBasePassDrawingPolicy& Other, bool bForReals = false) const
// 	{
// 		DRAWING_POLICY_MATCH_BEGIN
// 			DRAWING_POLICY_MATCH(FMeshDrawingPolicy::Matches(Other, bForReals)) &&
// 			DRAWING_POLICY_MATCH(VertexShader == Other.VertexShader) &&
// 			DRAWING_POLICY_MATCH(PixelShader == Other.PixelShader) &&
// 			DRAWING_POLICY_MATCH(HullShader == Other.HullShader) &&
// 			DRAWING_POLICY_MATCH(DomainShader == Other.DomainShader) &&
// 			DRAWING_POLICY_MATCH(bEnableSkyLight == Other.bEnableSkyLight) &&
// 			DRAWING_POLICY_MATCH(LightMapPolicy == Other.LightMapPolicy) &&
// 			DRAWING_POLICY_MATCH(bEnableReceiveDecalOutput == Other.bEnableReceiveDecalOutput) &&
// 			DRAWING_POLICY_MATCH(UseDebugViewPS() == Other.UseDebugViewPS());
// 		DRAWING_POLICY_MATCH_END
// 	}

	void SetupPipelineState(FDrawingPolicyRenderState& DrawRenderState, const FSceneView& View) const
	{
		{
			switch (BlendMode)
			{
			default:
			case BLEND_Opaque:
				// Opaque materials are rendered together in the base pass, where the blend state is set at a higher level
				break;
			case BLEND_Masked:
				// Masked materials are rendered together in the base pass, where the blend state is set at a higher level
				break;
			case BLEND_Translucent:
				// Note: alpha channel used by separate translucency, storing how much of the background should be added when doing the final composite
				// The Alpha channel is also used by non-separate translucency when rendering to scene captures, which store the final opacity
				DrawRenderState.SetBlendState(TStaticBlendState<FALSE,TRUE, D3D11_COLOR_WRITE_ENABLE_ALL, D3D11_BLEND_OP_ADD, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA , D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_INV_SRC_ALPHA>::GetRHI());
				break;
			case BLEND_Additive:
				// Add to the existing scene color
				// Note: alpha channel used by separate translucency, storing how much of the background should be added when doing the final composite
				// The Alpha channel is also used by non-separate translucency when rendering to scene captures, which store the final opacity
				DrawRenderState.SetBlendState(TStaticBlendState<FALSE, TRUE, D3D11_COLOR_WRITE_ENABLE_ALL, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_INV_SRC_ALPHA>::GetRHI());
				break;
			case BLEND_Modulate:
				// Modulate with the existing scene color, preserve destination alpha.
				DrawRenderState.SetBlendState(TStaticBlendState<FALSE, TRUE, D3D11_COLOR_WRITE_ENABLE_ALL, D3D11_BLEND_OP_ADD, D3D11_BLEND_DEST_COLOR, D3D11_BLEND_ZERO>::GetRHI());
				break;
			case BLEND_AlphaComposite:
				// Blend with existing scene color. New color is already pre-multiplied by alpha.
				DrawRenderState.SetBlendState(TStaticBlendState<FALSE, TRUE, D3D11_COLOR_WRITE_ENABLE_ALL, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_INV_SRC_ALPHA>::GetRHI());
				break;
			};
		}
	}

	void SetSharedState(const FDrawingPolicyRenderState& DrawRenderState, const FViewInfo* View, const ContextDataType PolicyContext) const
	{
		// If the current debug view shader modes are allowed, different VS/DS/HS must be used (with only SV_POSITION as PS interpolant).
// 		if (View->Family->UseDebugViewVSDSHS())
// 		{
// 			FDebugViewMode::SetParametersVSHSDS(RHICmdList, MaterialRenderProxy, MaterialResource, *View, VertexFactory, HullShader && DomainShader, DrawRenderState);
// 		}
// 		else
		{
			assert(VertexFactory && VertexFactory->IsInitialized());
			VertexFactory->SetStreams(D3D11DeviceContext);

			VertexShader->SetParameters(MaterialRenderProxy, VertexFactory, *MaterialResource, *View, DrawRenderState, PolicyContext.bIsInstancedStereo);

			if (HullShader)
			{
				HullShader->SetParameters(MaterialRenderProxy, *View, DrawRenderState.GetViewUniformBuffer(), DrawRenderState.GetPassUniformBuffer());
			}

			if (DomainShader)
			{
				DomainShader->SetParameters(MaterialRenderProxy, *View, DrawRenderState.GetViewUniformBuffer(), DrawRenderState.GetPassUniformBuffer());
			}
		}

// 		if (UseDebugViewPS())
// 		{
// 			FDebugViewMode::GetPSInterface(View->ShaderMap, MaterialResource, GetDebugViewShaderMode())->SetParameters(RHICmdList, VertexShader, PixelShader, MaterialRenderProxy, *MaterialResource, *View, DrawRenderState);
// 		}
// 		else
		{
			PixelShader->SetParameters(MaterialRenderProxy, *MaterialResource, View, DrawRenderState, BlendMode);
		}
	}

	void SetInstancedEyeIndex(const uint32 EyeIndex) const
	{
		VertexShader->SetInstancedEyeIndex(EyeIndex);
	}

	/**
	* Create bound shader state using the vertex decl from the mesh draw policy
	* as well as the shaders needed to draw the mesh
	* @param DynamicStride - optional stride for dynamic vertex data
	* @return new bound shader state object
	*/
	FBoundShaderStateInput GetBoundShaderStateInput() const
	{
		FBoundShaderStateInput BoundShaderStateInput(
			FMeshDrawingPolicy::GetVertexDeclaration(),
			VertexShader->GetVertexShader(),
			/*GETSAFERHISHADER_HULL(HullShader)*/NULL,
			/*GETSAFERHISHADER_DOMAIN(DomainShader)*/NULL,
			PixelShader->GetPixelShader(),
			NULL
		);

// 		if (UseDebugViewPS())
// 		{
// 			FDebugViewMode::PatchBoundShaderState(BoundShaderStateInput, MaterialResource, VertexFactory, InFeatureLevel, GetDebugViewShaderMode());
// 		}
		return BoundShaderStateInput;
	}

	void SetMeshRenderState(
		const FViewInfo& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMeshBatch& Mesh,
		int32 BatchElementIndex,
		FDrawingPolicyRenderState& DrawRenderState,
		const ElementDataType& ElementData,
		const ContextDataType PolicyContext
	) const
	{
		const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];


		{
			// Set the light-map policy's mesh-specific settings.
			LightMapPolicy.SetMesh(
				View,
				PrimitiveSceneProxy,
				VertexShader,
				/*!UseDebugViewPS() ?*/ PixelShader/* : nullptr*/,
				VertexShader,
				PixelShader,
				VertexFactory,
				MaterialRenderProxy,
				ElementData.LightMapElementData);

			VertexShader->SetMesh(VertexFactory, View, PrimitiveSceneProxy, Mesh, BatchElement, DrawRenderState);

// 			if (HullShader && DomainShader)
// 			{
// 				HullShader->SetMesh(VertexFactory, View, PrimitiveSceneProxy, BatchElement, DrawRenderState);
// 				DomainShader->SetMesh(VertexFactory, View, PrimitiveSceneProxy, BatchElement, DrawRenderState);
// 			}
		}


		{
			PixelShader->SetMesh(VertexFactory, View, PrimitiveSceneProxy, BatchElement, DrawRenderState, BlendMode);
		}
	}

// 	friend int32 CompareDrawingPolicy(const TBasePassDrawingPolicy& A, const TBasePassDrawingPolicy& B)
// 	{
// 		COMPAREDRAWINGPOLICYMEMBERS(VertexShader);
// 		COMPAREDRAWINGPOLICYMEMBERS(PixelShader);
// 		COMPAREDRAWINGPOLICYMEMBERS(HullShader);
// 		COMPAREDRAWINGPOLICYMEMBERS(DomainShader);
// 		COMPAREDRAWINGPOLICYMEMBERS(VertexFactory);
// 		COMPAREDRAWINGPOLICYMEMBERS(MaterialRenderProxy);
// 		COMPAREDRAWINGPOLICYMEMBERS(bEnableSkyLight);
// 		COMPAREDRAWINGPOLICYMEMBERS(bEnableReceiveDecalOutput);
// 
// 		return CompareDrawingPolicy(A.LightMapPolicy, B.LightMapPolicy);
// 	}

protected:

	// Here we don't store the most derived type of shaders, for instance TBasePassVertexShaderBaseType<LightMapPolicyType>.
	// This is to allow any shader using the same parameters to be used, and is required to allow FUniformLightMapPolicy to use shaders derived from TUniformLightMapPolicy.
	TBasePassVertexShaderPolicyParamType<typename LightMapPolicyType::VertexParametersType>* VertexShader;
	FBaseHS* HullShader; // Does not depend on LightMapPolicyType
	FBaseDS* DomainShader; // Does not depend on LightMapPolicyType
	TBasePassPixelShaderPolicyParamType<typename LightMapPolicyType::PixelParametersType>* PixelShader;

	LightMapPolicyType LightMapPolicy;
	EBlendMode BlendMode;

	uint32 bEnableSkyLight : 1;

	/** Whether or not this policy enables atmospheric fog */
	uint32 bEnableAtmosphericFog : 1;
};

/**
* A drawing policy factory for the base pass drawing policy.
*/
class FBasePassOpaqueDrawingPolicyFactory
{
public:

	enum { bAllowSimpleElements = true };
	struct ContextType
	{
		ContextType()
		{}
	};

	static void AddStaticMesh(FScene* Scene, FStaticMesh* StaticMesh);
	static bool DrawDynamicMesh(
		const FViewInfo& View,
		ContextType DrawingContext,
		const FMeshBatch& Mesh,
		bool bPreFog,
		const FDrawingPolicyRenderState& DrawRenderState,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		/*FHitProxyId HitProxyId,*/
		const bool bIsInstancedStereo = false
	);
};

/** The parameters used to process a base pass mesh. */
class FProcessBasePassMeshParameters
{
public:

	const FMeshBatch& Mesh;
	const uint64 BatchElementMask;
	const FMaterial* Material;
	const FPrimitiveSceneProxy* PrimitiveSceneProxy;
	EBlendMode BlendMode;
	EMaterialShadingModel ShadingModel;
	const bool bAllowFog;
	const bool bIsInstancedStereo;

	/** Initialization constructor. */
	FProcessBasePassMeshParameters(
		const FMeshBatch& InMesh,
		const FMaterial* InMaterial,
		const FPrimitiveSceneProxy* InPrimitiveSceneProxy,
		bool InbAllowFog,
		const bool InbIsInstancedStereo = false
	) :
		Mesh(InMesh),
		BatchElementMask(Mesh.Elements.Num() == 1 ? 1 : (1 << Mesh.Elements.Num()) - 1), // 1 bit set for each mesh element
		Material(InMaterial),
		PrimitiveSceneProxy(InPrimitiveSceneProxy),
		BlendMode(InMaterial->GetBlendMode()),
		ShadingModel(InMaterial->GetShadingModel()),
		bAllowFog(InbAllowFog),
		bIsInstancedStereo(InbIsInstancedStereo)
	{
	}

	/** Initialization constructor. */
	FProcessBasePassMeshParameters(
		const FMeshBatch& InMesh,
		const uint64& InBatchElementMask,
		const FMaterial* InMaterial,
		const FPrimitiveSceneProxy* InPrimitiveSceneProxy,
		bool InbAllowFog,
		bool InbIsInstancedStereo = false
	) :
		Mesh(InMesh),
		BatchElementMask(InBatchElementMask),
		Material(InMaterial),
		PrimitiveSceneProxy(InPrimitiveSceneProxy),
		BlendMode(InMaterial->GetBlendMode()),
		ShadingModel(InMaterial->GetShadingModel()),
		bAllowFog(InbAllowFog),
		bIsInstancedStereo(InbIsInstancedStereo)
	{
	}
};

/** Processes a base pass mesh using an unknown light map policy, and unknown fog density policy. */
template<typename ProcessActionType>
void ProcessBasePassMesh(
	const FProcessBasePassMeshParameters& Parameters,
	ProcessActionType&& Action
)
{
	// Check for a cached light-map.
	const bool bIsLitMaterial = (Parameters.ShadingModel != MSM_Unlit);
	//static const auto AllowStaticLightingVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.AllowStaticLighting"));
	const bool bAllowStaticLighting = true;// (!AllowStaticLightingVar || AllowStaticLightingVar->GetValueOnRenderThread() != 0);


	const FLightMapInteraction LightMapInteraction = (bAllowStaticLighting && Parameters.Mesh.LCI && bIsLitMaterial)
		? Parameters.Mesh.LCI->GetLightMapInteraction()
		: FLightMapInteraction();

	// force LQ lightmaps based on system settings
	const bool bPlatformAllowsHighQualityLightMaps = true;//AllowHighQualityLightmaps(Parameters.FeatureLevel);
	const bool bAllowHighQualityLightMaps = bPlatformAllowsHighQualityLightMaps && LightMapInteraction.AllowsHighQualityLightmaps();

// 	if (IsSimpleForwardShadingEnabled(GetFeatureLevelShaderPlatform(Parameters.FeatureLevel)))
// 	{
// 		// Only compiling simple lighting shaders for HQ lightmaps to save on permutations
// 		check(bPlatformAllowsHighQualityLightMaps);
// 		ProcessBasePassMeshForSimpleForwardShading(RHICmdList, Parameters, Action, LightMapInteraction, bIsLitMaterial, bAllowStaticLighting);
// 	}
// 	// Render self-shadowing only for >= SM4 and fallback to non-shadowed for lesser shader models
// 	else if (bIsLitMaterial && Action.UseTranslucentSelfShadowing() && Parameters.FeatureLevel >= ERHIFeatureLevel::SM4)
// 	{
// 		if (bIsLitMaterial
// 			&& bAllowStaticLighting
// 			&& Action.UseVolumetricLightmap()
// 			&& Parameters.PrimitiveSceneProxy)
// 		{
// 			Action.template Process<FSelfShadowedVolumetricLightmapPolicy>(RHICmdList, Parameters, FSelfShadowedVolumetricLightmapPolicy(), FSelfShadowedTranslucencyPolicy::ElementDataType(Action.GetTranslucentSelfShadow()));
// 		}
// 		else if (IsIndirectLightingCacheAllowed(Parameters.FeatureLevel)
// 			&& Action.AllowIndirectLightingCache()
// 			&& Parameters.PrimitiveSceneProxy)
// 		{
// 			// Apply cached point indirect lighting as well as self shadowing if needed
// 			Action.template Process<FSelfShadowedCachedPointIndirectLightingPolicy>(RHICmdList, Parameters, FSelfShadowedCachedPointIndirectLightingPolicy(), FSelfShadowedTranslucencyPolicy::ElementDataType(Action.GetTranslucentSelfShadow()));
// 		}
// 		else
// 		{
// 			Action.template Process<FSelfShadowedTranslucencyPolicy>(RHICmdList, Parameters, FSelfShadowedTranslucencyPolicy(), FSelfShadowedTranslucencyPolicy::ElementDataType(Action.GetTranslucentSelfShadow()));
// 		}
// 	}
// 	else
	{
		//static const auto CVarSupportLowQualityLightmap = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.SupportLowQualityLightmaps"));
		const bool bAllowLowQualityLightMaps = true;// (!CVarSupportLowQualityLightmap) || (CVarSupportLowQualityLightmap->GetValueOnAnyThread() != 0);

		switch (LightMapInteraction.GetType())
		{
		case LMIT_Texture:
			if (bAllowHighQualityLightMaps)
			{
				const FShadowMapInteraction ShadowMapInteraction = (bAllowStaticLighting && Parameters.Mesh.LCI && bIsLitMaterial)
					? Parameters.Mesh.LCI->GetShadowMapInteraction()
					: FShadowMapInteraction();

				if (ShadowMapInteraction.GetType() == SMIT_Texture)
				{
					Action.template Process< FUniformLightMapPolicy >(Parameters, FUniformLightMapPolicy(LMP_DISTANCE_FIELD_SHADOWS_AND_HQ_LIGHTMAP), Parameters.Mesh.LCI);
				}
				else
				{
					Action.template Process< FUniformLightMapPolicy >(Parameters, FUniformLightMapPolicy(LMP_HQ_LIGHTMAP), Parameters.Mesh.LCI);
				}
			}
			else if (bAllowLowQualityLightMaps)
			{
				Action.template Process< FUniformLightMapPolicy >(Parameters, FUniformLightMapPolicy(LMP_LQ_LIGHTMAP), Parameters.Mesh.LCI);
			}
			else
			{
				Action.template Process< FUniformLightMapPolicy >(Parameters, FUniformLightMapPolicy(LMP_NO_LIGHTMAP), Parameters.Mesh.LCI);
			}
			break;
		default:
			if (bIsLitMaterial
				&& bAllowStaticLighting
				&& Action.UseVolumetricLightmap()
				&& Parameters.PrimitiveSceneProxy
				&& (Parameters.PrimitiveSceneProxy->IsMovable()
					/*|| Parameters.PrimitiveSceneProxy->NeedsUnbuiltPreviewLighting()*/
					/*|| Parameters.PrimitiveSceneProxy->GetLightmapType() == ELightmapType::ForceVolumetric)*/)
			{
				Action.template Process< FUniformLightMapPolicy >(Parameters, FUniformLightMapPolicy(LMP_PRECOMPUTED_IRRADIANCE_VOLUME_INDIRECT_LIGHTING), Parameters.Mesh.LCI);
			}
			else if (bIsLitMaterial
				&& /*IsIndirectLightingCacheAllowed(Parameters.FeatureLevel)*/true
				&& Action.AllowIndirectLightingCache()
				&& Parameters.PrimitiveSceneProxy)
			{
				const FIndirectLightingCacheAllocation* IndirectLightingCacheAllocation = Parameters.PrimitiveSceneProxy->GetPrimitiveSceneInfo()->IndirectLightingCacheAllocation;
				const bool bPrimitiveIsMovable = Parameters.PrimitiveSceneProxy->IsMovable();
				const bool bPrimitiveUsesILC = Parameters.PrimitiveSceneProxy->GetIndirectLightingCacheQuality() != ILCQ_Off;

				// Use the indirect lighting cache shaders if the object has a cache allocation
				// This happens for objects with unbuilt lighting
				if (bPrimitiveUsesILC &&
					((IndirectLightingCacheAllocation && IndirectLightingCacheAllocation->IsValid())
						// Use the indirect lighting cache shaders if the object is movable, it may not have a cache allocation yet because that is done in InitViews
						// And movable objects are sometimes rendered in the static draw lists
						|| bPrimitiveIsMovable))
				{
					if (/*CanIndirectLightingCacheUseVolumeTexture(Parameters.FeatureLevel)*/true
						// Translucency forces point sample for pixel performance
						&& Action.AllowIndirectLightingCacheVolumeTexture()
						&& ((IndirectLightingCacheAllocation && !IndirectLightingCacheAllocation->bPointSample)
							|| (bPrimitiveIsMovable && Parameters.PrimitiveSceneProxy->GetIndirectLightingCacheQuality() == ILCQ_Volume)))
					{
						// Use a lightmap policy that supports reading indirect lighting from a volume texture for dynamic objects
						Action.template Process< FUniformLightMapPolicy >(Parameters, FUniformLightMapPolicy(LMP_CACHED_VOLUME_INDIRECT_LIGHTING), Parameters.Mesh.LCI);
					}
					else
					{
						// Use a lightmap policy that supports reading indirect lighting from a single SH sample
						Action.template Process< FUniformLightMapPolicy >(Parameters, FUniformLightMapPolicy(LMP_CACHED_POINT_INDIRECT_LIGHTING), Parameters.Mesh.LCI);
					}
				}
				else
				{
					Action.template Process< FUniformLightMapPolicy >(Parameters, FUniformLightMapPolicy(LMP_NO_LIGHTMAP), Parameters.Mesh.LCI);
				}
			}
			else
			{
				Action.template Process< FUniformLightMapPolicy >(Parameters, FUniformLightMapPolicy(LMP_NO_LIGHTMAP), Parameters.Mesh.LCI);
			}
			break;
		};
	}
}
