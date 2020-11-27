#include "DeferredShading.h"
#include "ShadowRendering.h"
#include "Scene.h"
#include "MeshMaterialShader.h"
#include "GlobalShader.h"
#include "ShaderParameters.h"
#include "ShaderBaseClasses.h"

/**
* A vertex shader for rendering the depth of a mesh.
*/
class FShadowDepthVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FShadowDepthVS, MeshMaterial);
public:

	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		return false;
	}

	FShadowDepthVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FMeshMaterialShader(Initializer)
	{
		ShadowParameters.Bind(Initializer.ParameterMap);
		ShadowViewProjectionMatrices.Bind(Initializer.ParameterMap, ("ShadowViewProjectionMatrices"));
		MeshVisibleToFace.Bind(Initializer.ParameterMap, ("MeshVisibleToFace"));
		BindSceneTextureUniformBufferDependentOnShadingPath(Initializer, PassUniformBuffer/*, PassUniformBuffer*/);
	}

	FShadowDepthVS() {}

	void SetParameters(
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FMaterial& Material,
		const FSceneView& View,
		const FProjectedShadowInfo* ShadowInfo,
		const FDrawingPolicyRenderState& DrawRenderState
	)
	{
		FMeshMaterialShader::SetParameters(GetVertexShader(), MaterialRenderProxy, Material, View, DrawRenderState.GetViewUniformBuffer(), DrawRenderState.GetPassUniformBuffer());
		ShadowParameters.SetVertexShader(this, View, ShadowInfo, MaterialRenderProxy);

		if (ShadowViewProjectionMatrices.IsBound())
		{
			const FMatrix Translation = FTranslationMatrix(-View.ViewMatrices.GetPreViewTranslation());

			FMatrix TranslatedShadowViewProjectionMatrices[6];
			for (int32 FaceIndex = 0; FaceIndex < 6; FaceIndex++)
			{
				// Have to apply the pre-view translation to the view - projection matrices
				TranslatedShadowViewProjectionMatrices[FaceIndex] = Translation * ShadowInfo->OnePassShadowViewProjectionMatrices[FaceIndex];
			}

			// Set the view projection matrices that will transform positions from world to cube map face space
			SetShaderValueArray<ID3D11VertexShader*, FMatrix>(
				GetVertexShader(),
				ShadowViewProjectionMatrices,
				TranslatedShadowViewProjectionMatrices,
				sizeof(TranslatedShadowViewProjectionMatrices)/sizeof(FMatrix)
				);
		}
	}

	void SetMesh(const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatchElement& BatchElement, const FDrawingPolicyRenderState& DrawRenderState, FProjectedShadowInfo const* ShadowInfo)
	{
		FMeshMaterialShader::SetMesh(GetVertexShader(), VertexFactory, View, Proxy, BatchElement, DrawRenderState);

		if (MeshVisibleToFace.IsBound())
		{
			const FBoxSphereBounds& PrimitiveBounds = Proxy->GetBounds();

			Vector4 MeshVisibleToFaceValue[6];
			for (int32 FaceIndex = 0; FaceIndex < 6; FaceIndex++)
			{
				MeshVisibleToFaceValue[FaceIndex] = Vector4(ShadowInfo->OnePassShadowFrustums[FaceIndex].IntersectBox(PrimitiveBounds.Origin, PrimitiveBounds.BoxExtent), 0, 0, 0);
			}

			// Set the view projection matrices that will transform positions from world to cube map face space
			SetShaderValueArray<ID3D11VertexShader*, Vector4>(
				GetVertexShader(),
				MeshVisibleToFace,
				MeshVisibleToFaceValue,
				sizeof(MeshVisibleToFaceValue) / sizeof(Vector4)
				);
		}
	}

private:
	FShadowDepthShaderParameters ShadowParameters;
	FShaderParameter ShadowViewProjectionMatrices;
	FShaderParameter MeshVisibleToFace;
};

enum EShadowDepthVertexShaderMode
{
	VertexShadowDepth_PerspectiveCorrect,
	VertexShadowDepth_OutputDepth,
	VertexShadowDepth_OnePassPointLight
};

/**
* A vertex shader for rendering the depth of a mesh.
*/
template <EShadowDepthVertexShaderMode ShaderMode, bool bRenderReflectiveShadowMap, bool bUsePositionOnlyStream, bool bIsForGeometryShader = false>
class TShadowDepthVS : public FShadowDepthVS
{
	DECLARE_SHADER_TYPE(TShadowDepthVS, MeshMaterial);
public:

	TShadowDepthVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FShadowDepthVS(Initializer)
	{
	}

	TShadowDepthVS() {}

	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		//static const auto CVarSupportAllShaderPermutations = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.SupportAllShaderPermutations"));
		const bool bForceAllPermutations = false;//CVarSupportAllShaderPermutations && CVarSupportAllShaderPermutations->GetValueOnAnyThread() != 0;
		const bool bSupportPointLightWholeSceneShadows = true;// CVarSupportPointLightWholeSceneShadows.GetValueOnAnyThread() != 0 || bForceAllPermutations;
		const bool bRHISupportsShadowCastingPointLights = true;// RHISupportsGeometryShaders(Platform) || RHISupportsVertexShaderLayer(Platform);

		if (bIsForGeometryShader && (!bSupportPointLightWholeSceneShadows || !bRHISupportsShadowCastingPointLights))
		{
			return false;
		}

		//Note: This logic needs to stay in sync with OverrideWithDefaultMaterialForShadowDepth!
		// Compile for special engine materials.
		if (bRenderReflectiveShadowMap)
		{
			// Reflective shadow map shaders must be compiled for every material because they access the material normal
			return !bUsePositionOnlyStream
				// Don't render ShadowDepth for translucent unlit materials, unless we're injecting emissive
				&& (Material->ShouldCastDynamicShadows() || Material->ShouldInjectEmissiveIntoLPV()
					|| Material->ShouldBlockGI())
				/*&& IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5)*/;
		}
		else
		{
			return (Material->IsSpecialEngineMaterial()
				// Masked and WPO materials need their shaders but cannot be used with a position only stream.
				|| ((!Material->WritesEveryPixel(true) || Material->MaterialMayModifyMeshPosition()) && !bUsePositionOnlyStream))
				// Only compile one pass point light shaders for feature levels >= SM4
				&& (ShaderMode != VertexShadowDepth_OnePassPointLight /*|| IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4)*/)
				// Only compile position-only shaders for vertex factories that support it.
				&& (!bUsePositionOnlyStream || VertexFactoryType->SupportsPositionOnly())
				// Don't render ShadowDepth for translucent unlit materials
				&& Material->ShouldCastDynamicShadows()
				// Only compile perspective correct light shaders for feature levels >= SM4
				&& (ShaderMode != VertexShadowDepth_PerspectiveCorrect/* || IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4)*/);
		}
	}

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FShadowDepthVS::ModifyCompilationEnvironment(Material, OutEnvironment);
		OutEnvironment.SetDefine(("PERSPECTIVE_CORRECT_DEPTH"), (uint32)(ShaderMode == VertexShadowDepth_PerspectiveCorrect));
		OutEnvironment.SetDefine(("ONEPASS_POINTLIGHT_SHADOW"), (uint32)(ShaderMode == VertexShadowDepth_OnePassPointLight));
		OutEnvironment.SetDefine(("REFLECTIVE_SHADOW_MAP"), (uint32)bRenderReflectiveShadowMap);
		OutEnvironment.SetDefine(("POSITION_ONLY"), (uint32)bUsePositionOnlyStream);

		if (bIsForGeometryShader)
		{
			//OutEnvironment.CompilerFlags.push_back(CFLAG_VertexToGeometryShader);
		}
	}
};


/**
* A Hull shader for rendering the depth of a mesh.
*/
template <EShadowDepthVertexShaderMode ShaderMode, bool bRenderReflectiveShadowMap>
class TShadowDepthHS : public FBaseHS
{
	DECLARE_SHADER_TYPE(TShadowDepthHS, MeshMaterial);
public:


	TShadowDepthHS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FBaseHS(Initializer)
	{}

	TShadowDepthHS() {}

	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		// Re-use ShouldCache from vertex shader
		return FBaseHS::ShouldCompilePermutation(Material, VertexFactoryType)
			&& TShadowDepthVS<ShaderMode, bRenderReflectiveShadowMap, false>::ShouldCompilePermutation(Material, VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		// Re-use compilation env from vertex shader

		TShadowDepthVS<ShaderMode, bRenderReflectiveShadowMap, false>::ModifyCompilationEnvironment(Material, OutEnvironment);
	}
};

/**
* A domain shader for rendering the depth of a mesh.
*/
class FShadowDepthDS : public FBaseDS
{
	DECLARE_SHADER_TYPE(FShadowDepthDS, MeshMaterial);
public:

	FShadowDepthDS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FBaseDS(Initializer)
	{
		ShadowParameters.Bind(Initializer.ParameterMap);
		ShadowViewProjectionMatrices.Bind(Initializer.ParameterMap, ("ShadowViewProjectionMatrices"));
	}

	FShadowDepthDS() {}

	void SetParameters(
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FSceneView& View,
		const FProjectedShadowInfo* ShadowInfo,
		const FDrawingPolicyRenderState& DrawRenderState
	)
	{
		FBaseDS::SetParameters(MaterialRenderProxy, View, DrawRenderState.GetViewUniformBuffer(), DrawRenderState.GetPassUniformBuffer());
		ShadowParameters.SetDomainShader(this, View, ShadowInfo, MaterialRenderProxy);

		if (ShadowViewProjectionMatrices.IsBound())
		{
			const FMatrix Translation = FTranslationMatrix(-View.ViewMatrices.GetPreViewTranslation());

			FMatrix TranslatedShadowViewProjectionMatrices[6];
			for (int32 FaceIndex = 0; FaceIndex < 6; FaceIndex++)
			{
				// Have to apply the pre-view translation to the view - projection matrices
				TranslatedShadowViewProjectionMatrices[FaceIndex] = Translation * ShadowInfo->OnePassShadowViewProjectionMatrices[FaceIndex];
			}

			// Set the view projection matrices that will transform positions from world to cube map face space
			SetShaderValueArray<ID3D11DomainShader*, FMatrix>(
				GetDomainShader(),
				ShadowViewProjectionMatrices,
				TranslatedShadowViewProjectionMatrices,
				sizeof(TranslatedShadowViewProjectionMatrices)/sizeof(FMatrix)
				);
		}
	}

private:
	FShadowDepthShaderParameters ShadowParameters;
	FShaderParameter ShadowViewProjectionMatrices;
};

/**
* A Domain shader for rendering the depth of a mesh.
*/
template <EShadowDepthVertexShaderMode ShaderMode, bool bRenderReflectiveShadowMap>
class TShadowDepthDS : public FShadowDepthDS
{
	DECLARE_SHADER_TYPE(TShadowDepthDS, MeshMaterial);
public:

	TShadowDepthDS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FShadowDepthDS(Initializer)
	{}

	TShadowDepthDS() {}

	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		// Re-use ShouldCache from vertex shader
		return FBaseDS::ShouldCompilePermutation(Material, VertexFactoryType)
			&& TShadowDepthVS<ShaderMode, bRenderReflectiveShadowMap, false>::ShouldCompilePermutation(Material, VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		// Re-use compilation env from vertex shader
		TShadowDepthVS<ShaderMode, bRenderReflectiveShadowMap, false>::ModifyCompilationEnvironment(Material, OutEnvironment);
	}
};

/** Geometry shader that allows one pass point light shadows by cloning triangles to all faces of the cube map. */
class FOnePassPointShadowDepthGS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FOnePassPointShadowDepthGS, MeshMaterial);
public:

	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		return /*RHISupportsGeometryShaders(Platform)*/true && TShadowDepthVS<VertexShadowDepth_OnePassPointLight, false, false, true>::ShouldCompilePermutation( Material, VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Material, OutEnvironment);
		TShadowDepthVS<VertexShadowDepth_OnePassPointLight, false, false, true>::ModifyCompilationEnvironment(Material, OutEnvironment);
	}

	FOnePassPointShadowDepthGS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{
		ShadowViewProjectionMatrices.Bind(Initializer.ParameterMap, ("ShadowViewProjectionMatrices"));
		MeshVisibleToFace.Bind(Initializer.ParameterMap, ("MeshVisibleToFace"));
		BindSceneTextureUniformBufferDependentOnShadingPath(Initializer, PassUniformBuffer/*, PassUniformBuffer*/);
	}

	FOnePassPointShadowDepthGS() {}

	void SetParameters(
		const FSceneView& View,
		const FProjectedShadowInfo* ShadowInfo
	)
	{
		FMaterialShader::SetViewParameters(GetGeometryShader(), View, View.ViewUniformBuffer);

		const FMatrix Translation = FTranslationMatrix(-View.ViewMatrices.GetPreViewTranslation());

		FMatrix TranslatedShadowViewProjectionMatrices[6];
		for (int32 FaceIndex = 0; FaceIndex < 6; FaceIndex++)
		{
			// Have to apply the pre-view translation to the view - projection matrices
			TranslatedShadowViewProjectionMatrices[FaceIndex] = Translation * ShadowInfo->OnePassShadowViewProjectionMatrices[FaceIndex];
		}

		// Set the view projection matrices that will transform positions from world to cube map face space
		SetShaderValueArray<ID3D11GeometryShader*, FMatrix>(
			GetGeometryShader(),
			ShadowViewProjectionMatrices,
			TranslatedShadowViewProjectionMatrices,
			sizeof(TranslatedShadowViewProjectionMatrices) / sizeof(FMatrix)
			);
	}

	void SetMesh(const FPrimitiveSceneProxy* PrimitiveSceneProxy, const FProjectedShadowInfo* ShadowInfo, const FSceneView& View)
	{
		if (MeshVisibleToFace.IsBound())
		{
			const FBoxSphereBounds& PrimitiveBounds = PrimitiveSceneProxy->GetBounds();

			Vector4 MeshVisibleToFaceValue[6];
			for (int32 FaceIndex = 0; FaceIndex < 6; FaceIndex++)
			{
				MeshVisibleToFaceValue[FaceIndex] = Vector4(ShadowInfo->OnePassShadowFrustums[FaceIndex].IntersectBox(PrimitiveBounds.Origin, PrimitiveBounds.BoxExtent), 0, 0, 0);
			}

			// Set the view projection matrices that will transform positions from world to cube map face space
			SetShaderValueArray<ID3D11GeometryShader*, Vector4>(
				GetGeometryShader(),
				MeshVisibleToFace,
				MeshVisibleToFaceValue,
				sizeof(MeshVisibleToFaceValue)/sizeof(Vector4)
				);
		}
	}

private:
	FShaderParameter ShadowViewProjectionMatrices;
	FShaderParameter MeshVisibleToFace;
};

#define IMPLEMENT_SHADOW_DEPTH_SHADERMODE_SHADERS(ShaderMode,bRenderReflectiveShadowMap) \
	typedef TShadowDepthVS<ShaderMode, bRenderReflectiveShadowMap, false> TShadowDepthVS##ShaderMode##bRenderReflectiveShadowMap;	\
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TShadowDepthVS##ShaderMode##bRenderReflectiveShadowMap,("ShadowDepthVertexShader.usf"),("Main"),SF_Vertex);	\
	typedef TShadowDepthVS<ShaderMode, bRenderReflectiveShadowMap, false, true> TShadowDepthVSForGS##ShaderMode##bRenderReflectiveShadowMap;	\
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TShadowDepthVSForGS##ShaderMode##bRenderReflectiveShadowMap,("ShadowDepthVertexShader.usf"),("MainForGS"),SF_Vertex);	\
	typedef TShadowDepthHS<ShaderMode, bRenderReflectiveShadowMap> TShadowDepthHS##ShaderMode##bRenderReflectiveShadowMap;	\
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TShadowDepthHS##ShaderMode##bRenderReflectiveShadowMap,("ShadowDepthVertexShader.usf"),("MainHull"),SF_Hull);	\
	typedef TShadowDepthDS<ShaderMode, bRenderReflectiveShadowMap> TShadowDepthDS##ShaderMode##bRenderReflectiveShadowMap;	\
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TShadowDepthDS##ShaderMode##bRenderReflectiveShadowMap,("ShadowDepthVertexShader.usf"),("MainDomain"),SF_Domain);

IMPLEMENT_SHADER_TYPE(, FOnePassPointShadowDepthGS, ("ShadowDepthVertexShader.usf"), ("MainOnePassPointLightGS"), SF_Geometry);

IMPLEMENT_SHADOW_DEPTH_SHADERMODE_SHADERS(VertexShadowDepth_PerspectiveCorrect, true);
IMPLEMENT_SHADOW_DEPTH_SHADERMODE_SHADERS(VertexShadowDepth_PerspectiveCorrect, false);
IMPLEMENT_SHADOW_DEPTH_SHADERMODE_SHADERS(VertexShadowDepth_OutputDepth, true);
IMPLEMENT_SHADOW_DEPTH_SHADERMODE_SHADERS(VertexShadowDepth_OutputDepth, false);
IMPLEMENT_SHADOW_DEPTH_SHADERMODE_SHADERS(VertexShadowDepth_OnePassPointLight, false);

// Position only vertex shaders.
typedef TShadowDepthVS<VertexShadowDepth_PerspectiveCorrect, false, true> TShadowDepthVSVertexShadowDepth_PerspectiveCorrectPositionOnly;
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>, TShadowDepthVSVertexShadowDepth_PerspectiveCorrectPositionOnly, ("ShadowDepthVertexShader.usf"), ("PositionOnlyMain"), SF_Vertex);
typedef TShadowDepthVS<VertexShadowDepth_OutputDepth, false, true> TShadowDepthVSVertexShadowDepth_OutputDepthPositionOnly;
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>, TShadowDepthVSVertexShadowDepth_OutputDepthPositionOnly, ("ShadowDepthVertexShader.usf"), ("PositionOnlyMain"), SF_Vertex);
typedef TShadowDepthVS<VertexShadowDepth_OnePassPointLight, false, true> TShadowDepthVSVertexShadowDepth_OnePassPointLightPositionOnly;
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>, TShadowDepthVSVertexShadowDepth_OnePassPointLightPositionOnly, ("ShadowDepthVertexShader.usf"), ("PositionOnlyMain"), SF_Vertex);
typedef TShadowDepthVS<VertexShadowDepth_OnePassPointLight, false, true, true> TShadowDepthVSForGSVertexShadowDepth_OnePassPointLightPositionOnly;
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>, TShadowDepthVSForGSVertexShadowDepth_OnePassPointLightPositionOnly, ("ShadowDepthVertexShader.usf"), ("PositionOnlyMainForGS"), SF_Vertex);

/**
* A pixel shader for rendering the depth of a mesh.
*/
template <bool bRenderReflectiveShadowMap>
class TShadowDepthBasePS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(TShadowDepthBasePS, MeshMaterial);
public:

	TShadowDepthBasePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{
		ShadowParams.Bind(Initializer.ParameterMap, ("ShadowParams"));
		ReflectiveShadowMapTextureResolution.Bind(Initializer.ParameterMap, ("ReflectiveShadowMapTextureResolution"));
		ProjectionMatrixParameter.Bind(Initializer.ParameterMap, ("ProjectionMatrix"));
		GvListBuffer.Bind(Initializer.ParameterMap, ("RWGvListBuffer"));
		GvListHeadBuffer.Bind(Initializer.ParameterMap, ("RWGvListHeadBuffer"));
		VplListBuffer.Bind(Initializer.ParameterMap, ("RWVplListBuffer"));
		VplListHeadBuffer.Bind(Initializer.ParameterMap, ("RWVplListHeadBuffer"));
		BindSceneTextureUniformBufferDependentOnShadingPath(Initializer, PassUniformBuffer/*, PassUniformBuffer*/);
	}

	TShadowDepthBasePS() {}

	void SetParameters(
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FMaterial& Material,
		const FSceneView& View,
		const FProjectedShadowInfo* ShadowInfo,
		const FDrawingPolicyRenderState& DrawRenderState
	)
	{
		ID3D11PixelShader* const ShaderRHI = GetPixelShader();

		FMeshMaterialShader::SetParameters(ShaderRHI, MaterialRenderProxy, Material, View, DrawRenderState.GetViewUniformBuffer(), DrawRenderState.GetPassUniformBuffer());

		SetShaderValue( ShaderRHI, ShadowParams, Vector2(ShadowInfo->GetShaderDepthBias(), ShadowInfo->InvMaxSubjectDepth));

// 		if (bRenderReflectiveShadowMap)
// 		{
// 			FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();
// 
// 			// LPV also propagates light transmission (for transmissive materials)
// 			SetShaderValue(RHICmdList, ShaderRHI, ReflectiveShadowMapTextureResolution,
// 				Vector2(SceneContext.GetReflectiveShadowMapResolution(),
// 					SceneContext.GetReflectiveShadowMapResolution()));
// 			SetShaderValue(
// 				ShaderRHI,
// 				ProjectionMatrixParameter,
// 				FTranslationMatrix(ShadowInfo->PreShadowTranslation - View.ViewMatrices.GetPreViewTranslation()) * ShadowInfo->SubjectAndReceiverMatrix
// 			);
// 
// 			const FSceneViewState* ViewState = (const FSceneViewState*)View.State;
// 			if (ViewState)
// 			{
// 				const FLightPropagationVolume* Lpv = ViewState->GetLightPropagationVolume(View.GetFeatureLevel());
// 
// 				if (Lpv)
// 				{
// 					SetUniformBufferParameter(ShaderRHI, GetUniformBufferParameter<FLpvWriteUniformBufferParameters>(), Lpv->GetRsmUniformBuffer());
// 				}
// 			}
// 		}
	}

	void SetMesh(const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatchElement& BatchElement, const FDrawingPolicyRenderState& DrawRenderState)
	{
		FMeshMaterialShader::SetMesh(GetPixelShader(), VertexFactory, View, Proxy, BatchElement, DrawRenderState);
	}


private:

	FShaderParameter ShadowParams;

	FShaderParameter ReflectiveShadowMapTextureResolution;
	FShaderParameter ProjectionMatrixParameter;

	FRWShaderParameter GvListBuffer;
	FRWShaderParameter GvListHeadBuffer;
	FRWShaderParameter VplListBuffer;
	FRWShaderParameter VplListHeadBuffer;
};

enum EShadowDepthPixelShaderMode
{
	PixelShadowDepth_NonPerspectiveCorrect,
	PixelShadowDepth_PerspectiveCorrect,
	PixelShadowDepth_OnePassPointLight
};

template <EShadowDepthPixelShaderMode ShaderMode, bool bRenderReflectiveShadowMap>
class TShadowDepthPS : public TShadowDepthBasePS<bRenderReflectiveShadowMap>
{
	DECLARE_SHADER_TYPE(TShadowDepthPS, MeshMaterial);
public:

	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		if (/*!IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4)*/false)
		{
			return (Material->IsSpecialEngineMaterial()
				// Only compile for masked or lit translucent materials
				|| !Material->WritesEveryPixel(true)
				|| (Material->MaterialMayModifyMeshPosition() && Material->IsUsedWithInstancedStaticMeshes())
				// Perspective correct rendering needs a pixel shader and WPO materials can't be overridden with default material.
				|| (ShaderMode == PixelShadowDepth_PerspectiveCorrect && Material->MaterialMayModifyMeshPosition()))
				&& ShaderMode == PixelShadowDepth_NonPerspectiveCorrect
				// Don't render ShadowDepth for translucent unlit materials
				&& Material->ShouldCastDynamicShadows()
				&& !bRenderReflectiveShadowMap;
		}

		if (bRenderReflectiveShadowMap)
		{
			//Note: This logic needs to stay in sync with OverrideWithDefaultMaterialForShadowDepth!
			// Reflective shadow map shaders must be compiled for every material because they access the material normal
			return
				// Only compile one pass point light shaders for feature levels >= SM4
				(Material->ShouldCastDynamicShadows() || Material->ShouldInjectEmissiveIntoLPV() || Material->ShouldBlockGI())
				/*&& IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5)*/;
		}
		else
		{
			//Note: This logic needs to stay in sync with OverrideWithDefaultMaterialForShadowDepth!
			return (Material->IsSpecialEngineMaterial()
				// Only compile for masked or lit translucent materials
				|| !Material->WritesEveryPixel(true)
				|| (Material->MaterialMayModifyMeshPosition() && Material->IsUsedWithInstancedStaticMeshes())
				// Perspective correct rendering needs a pixel shader and WPO materials can't be overridden with default material.
				|| (ShaderMode == PixelShadowDepth_PerspectiveCorrect && Material->MaterialMayModifyMeshPosition()))
				// Only compile one pass point light shaders for feature levels >= SM4
				&& (ShaderMode != PixelShadowDepth_OnePassPointLight/* || IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4)*/)
				// Don't render ShadowDepth for translucent unlit materials
				&& Material->ShouldCastDynamicShadows()
				/*&& IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4)*/;
		}
	}

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		TShadowDepthBasePS<bRenderReflectiveShadowMap>::ModifyCompilationEnvironment(Material, OutEnvironment);
		OutEnvironment.SetDefine(("PERSPECTIVE_CORRECT_DEPTH"), (uint32)(ShaderMode == PixelShadowDepth_PerspectiveCorrect));
		OutEnvironment.SetDefine(("ONEPASS_POINTLIGHT_SHADOW"), (uint32)(ShaderMode == PixelShadowDepth_OnePassPointLight));
		OutEnvironment.SetDefine(("REFLECTIVE_SHADOW_MAP"), (uint32)bRenderReflectiveShadowMap);
	}

	TShadowDepthPS()
	{
	}

	TShadowDepthPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: TShadowDepthBasePS<bRenderReflectiveShadowMap>(Initializer)
	{
	}
};

// typedef required to get around macro expansion failure due to commas in template argument list for TShadowDepthPixelShader
#define IMPLEMENT_SHADOWDEPTHPASS_PIXELSHADER_TYPE(ShaderMode, bRenderReflectiveShadowMap) \
	typedef TShadowDepthPS<ShaderMode, bRenderReflectiveShadowMap> TShadowDepthPS##ShaderMode##bRenderReflectiveShadowMap; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TShadowDepthPS##ShaderMode##bRenderReflectiveShadowMap,("ShadowDepthPixelShader.usf"),("Main"),SF_Pixel);

IMPLEMENT_SHADOWDEPTHPASS_PIXELSHADER_TYPE(PixelShadowDepth_NonPerspectiveCorrect, true);
IMPLEMENT_SHADOWDEPTHPASS_PIXELSHADER_TYPE(PixelShadowDepth_NonPerspectiveCorrect, false);
IMPLEMENT_SHADOWDEPTHPASS_PIXELSHADER_TYPE(PixelShadowDepth_PerspectiveCorrect, true);
IMPLEMENT_SHADOWDEPTHPASS_PIXELSHADER_TYPE(PixelShadowDepth_PerspectiveCorrect, false);
IMPLEMENT_SHADOWDEPTHPASS_PIXELSHADER_TYPE(PixelShadowDepth_OnePassPointLight, true);
IMPLEMENT_SHADOWDEPTHPASS_PIXELSHADER_TYPE(PixelShadowDepth_OnePassPointLight, false);

template <bool bReflectiveShadowmap>
void DrawMeshElements(
	FShadowDepthDrawingPolicy<bReflectiveShadowmap>& SharedDrawingPolicy, 
	const FShadowStaticMeshElement& State, 
	const FViewInfo& View, 
	FShadowDepthDrawingPolicyContext PolicyContext, 
	const FDrawingPolicyRenderState& DrawRenderState, 
	const FStaticMesh* Mesh)
{
	//TODO MaybeRemovable if ShadowDepth never support LOD Transitions
	FDrawingPolicyRenderState DrawRenderStateLocal(DrawRenderState);
	//SharedDrawingPolicy.ApplyDitheredLODTransitionState(DrawRenderStateLocal, View, *Mesh, false);

	// Render only those batch elements that match the current LOD
	//uint64 BatchElementMask = Mesh->bRequiresPerElementVisibility ? View.StaticMeshBatchVisibility[Mesh->BatchVisibilityId] : ((1ull << Mesh->Elements.Num()) - 1);
	int32 BatchElementIndex = 0;

	SharedDrawingPolicy.SetMeshRenderState(View, Mesh->PrimitiveSceneInfo->Proxy, *Mesh, BatchElementIndex, DrawRenderStateLocal, /*FMeshDrawingPolicy::ElementDataType(),*/ PolicyContext);
	SharedDrawingPolicy.DrawMesh(D3D11DeviceContext, View, *Mesh, BatchElementIndex);
}


template <bool bReflectiveShadowmap>
void DrawShadowMeshElements(const FViewInfo& View, const FDrawingPolicyRenderState& DrawRenderState, const FProjectedShadowInfo& ShadowInfo)
{
	FShadowDepthDrawingPolicyContext PolicyContext(&ShadowInfo);
	const FShadowStaticMeshElement& FirstShadowMesh = ShadowInfo.StaticSubjectMeshElements[0];
	const FMaterial* FirstMaterialResource = FirstShadowMesh.MaterialResource;

	FShadowDepthDrawingPolicy<bReflectiveShadowmap> SharedDrawingPolicy(
		FirstMaterialResource,
		ShadowInfo.bDirectionalLight,
		ShadowInfo.bOnePassPointLightShadow,
		ShadowInfo.bPreShadow//,
		/*ComputeMeshOverrideSettings(*FirstShadowMesh.Mesh)*/);

	FShadowStaticMeshElement OldState;

	FDrawingPolicyRenderState DrawRenderStateLocal(DrawRenderState);

	uint32 ElementCount = ShadowInfo.StaticSubjectMeshElements.size();
	for (uint32 ElementIndex = 0; ElementIndex < ElementCount; ++ElementIndex)
	{
		const FShadowStaticMeshElement& ShadowMesh = ShadowInfo.StaticSubjectMeshElements[ElementIndex];

		FShadowStaticMeshElement CurrentState(ShadowMesh.RenderProxy, ShadowMesh.MaterialResource, ShadowMesh.Mesh, ShadowMesh.bIsTwoSided);

		// Only call draw shared when the vertex factory or material have changed
		if (OldState.DoesDeltaRequireADrawSharedCall(CurrentState))
		{
			OldState = CurrentState;

			SharedDrawingPolicy.UpdateElementState(CurrentState);
			DrawRenderStateLocal.SetDitheredLODTransitionAlpha(ShadowMesh.Mesh->DitheredLODTransitionAlpha);
			//SetViewFlagsForShadowPass(DrawRenderStateLocal, View, SharedDrawingPolicy.IsTwoSided(), bReflectiveShadowmap, ShadowInfo.bOnePassPointLightShadow);
			SharedDrawingPolicy.SetupPipelineState(DrawRenderStateLocal, View);
			CommitGraphicsPipelineState(SharedDrawingPolicy, DrawRenderStateLocal, SharedDrawingPolicy.GetBoundShaderStateInput());
			SharedDrawingPolicy.SetSharedState( DrawRenderStateLocal, &View, PolicyContext);
		}

		DrawMeshElements( SharedDrawingPolicy, OldState, View, PolicyContext, DrawRenderStateLocal, ShadowMesh.Mesh);
	}
}

template <bool bRenderingReflectiveShadowMaps>
FShadowDepthDrawingPolicy<bRenderingReflectiveShadowMaps>::FShadowDepthDrawingPolicy(
	const FMaterial* InMaterialResource, 
	bool bInDirectionalLight, 
	bool bInOnePassPointLightShadow, 
	bool bInPreShadow, 
	/*const FMeshDrawingPolicyOverrideSettings& InOverrideSettings, */ 
	/*ERHIFeatureLevel::Type InFeatureLevel, */ 
	const FVertexFactory* InVertexFactory /*= 0*/, 
	const FMaterialRenderProxy* InMaterialRenderProxy /*= 0*/,
	bool bInReverseCulling /*= false */) :
	FMeshDrawingPolicy(InVertexFactory, InMaterialRenderProxy, *InMaterialResource/*, InOverrideSettings, DVSM_None*/),
	GeometryShader(NULL),
	bDirectionalLight(bInDirectionalLight),
	bReverseCulling(bInReverseCulling),
	bOnePassPointLightShadow(bInOnePassPointLightShadow),
	bPreShadow(bInPreShadow)
{
	assert(!bInOnePassPointLightShadow || !bRenderingReflectiveShadowMaps);

	InstanceFactor = !bOnePassPointLightShadow ? 1 : 6;

	if (!InVertexFactory)
	{
		// dummy object, needs call to UpdateElementState() to be fully initialized
		return;
	}

	const bool bUsePerspectiveCorrectShadowDepths = !bInDirectionalLight && !bInOnePassPointLightShadow;

	HullShader = NULL;
	DomainShader = NULL;

	FVertexFactoryType* VFType = InVertexFactory->GetType();

	const bool bInitializeTessellationShaders =
		MaterialResource->GetTessellationMode() != MTM_NoTessellation
		/*&& RHISupportsTessellation(GShaderPlatformForFeatureLevel[InFeatureLevel])*/
		&& VFType->SupportsTessellationShaders();

	bUsePositionOnlyVS = !bRenderingReflectiveShadowMaps
		&& VertexFactory->SupportsPositionOnlyStream()
		&& MaterialResource->WritesEveryPixel(true)
		/*&& !MaterialResource->MaterialModifiesMeshPosition_RenderThread()*/;

	// Vertex related shaders
	if (bOnePassPointLightShadow)
	{
		if (bUsePositionOnlyVS)
		{
			VertexShader = MaterialResource->GetShader<TShadowDepthVS<VertexShadowDepth_OnePassPointLight, false, true, true> >(VFType);
		}
		else
		{
			VertexShader = MaterialResource->GetShader<TShadowDepthVS<VertexShadowDepth_OnePassPointLight, false, false, true> >(VFType);
		}
		if (/*RHISupportsGeometryShaders(GShaderPlatformForFeatureLevel[InFeatureLevel])*/true)
		{
			// Use the geometry shader which will clone output triangles to all faces of the cube map
			GeometryShader = MaterialResource->GetShader<FOnePassPointShadowDepthGS>(VFType);
		}
		if (bInitializeTessellationShaders)
		{
			HullShader = MaterialResource->GetShader<TShadowDepthHS<VertexShadowDepth_OnePassPointLight, false> >(VFType);
			DomainShader = MaterialResource->GetShader<TShadowDepthDS<VertexShadowDepth_OnePassPointLight, false> >(VFType);
		}
	}
	else if (bUsePerspectiveCorrectShadowDepths)
	{
		if (bRenderingReflectiveShadowMaps)
		{
			VertexShader = MaterialResource->GetShader<TShadowDepthVS<VertexShadowDepth_PerspectiveCorrect, true, false> >(VFType);
		}
		else
		{
			if (bUsePositionOnlyVS)
			{
				VertexShader = MaterialResource->GetShader<TShadowDepthVS<VertexShadowDepth_PerspectiveCorrect, false, true> >(VFType);
			}
			else
			{
				VertexShader = MaterialResource->GetShader<TShadowDepthVS<VertexShadowDepth_PerspectiveCorrect, false, false> >(VFType);
			}
		}
		if (bInitializeTessellationShaders)
		{
			HullShader = MaterialResource->GetShader<TShadowDepthHS<VertexShadowDepth_PerspectiveCorrect, bRenderingReflectiveShadowMaps> >(VFType);
			DomainShader = MaterialResource->GetShader<TShadowDepthDS<VertexShadowDepth_PerspectiveCorrect, bRenderingReflectiveShadowMaps> >(VFType);
		}
	}
	else
	{
		if (bRenderingReflectiveShadowMaps)
		{
			VertexShader = MaterialResource->GetShader<TShadowDepthVS<VertexShadowDepth_OutputDepth, true, false> >(VFType);
			if (bInitializeTessellationShaders)
			{
				HullShader = MaterialResource->GetShader<TShadowDepthHS<VertexShadowDepth_OutputDepth, true> >(VFType);
				DomainShader = MaterialResource->GetShader<TShadowDepthDS<VertexShadowDepth_OutputDepth, true> >(VFType);
			}
		}
		else
		{
			if (bUsePositionOnlyVS)
			{
				VertexShader = MaterialResource->GetShader<TShadowDepthVS<VertexShadowDepth_OutputDepth, false, true> >(VFType);
			}
			else
			{
				VertexShader = MaterialResource->GetShader<TShadowDepthVS<VertexShadowDepth_OutputDepth, false, false> >(VFType);
			}
			if (bInitializeTessellationShaders)
			{
				HullShader = MaterialResource->GetShader<TShadowDepthHS<VertexShadowDepth_OutputDepth, false> >(VFType);
				DomainShader = MaterialResource->GetShader<TShadowDepthDS<VertexShadowDepth_OutputDepth, false> >(VFType);
			}
		}
	}

	// Pixel shaders
	if (MaterialResource->WritesEveryPixel(true) && !bUsePerspectiveCorrectShadowDepths && !bRenderingReflectiveShadowMaps && VertexFactory->SupportsNullPixelShader())
	{
		// No pixel shader necessary.
		PixelShader = NULL;
	}
	else
	{
		if (bUsePerspectiveCorrectShadowDepths)
		{
			PixelShader = (TShadowDepthBasePS<bRenderingReflectiveShadowMaps> *)MaterialResource->GetShader<TShadowDepthPS<PixelShadowDepth_PerspectiveCorrect, bRenderingReflectiveShadowMaps> >(VFType, false);
		}
		else if (bOnePassPointLightShadow)
		{
			PixelShader = (TShadowDepthBasePS<bRenderingReflectiveShadowMaps> *)MaterialResource->GetShader<TShadowDepthPS<PixelShadowDepth_OnePassPointLight, false> >(VFType, false);
		}
		else
		{
			PixelShader = (TShadowDepthBasePS<bRenderingReflectiveShadowMaps> *)MaterialResource->GetShader<TShadowDepthPS<PixelShadowDepth_NonPerspectiveCorrect, bRenderingReflectiveShadowMaps> >(VFType, false);
		}
	}
	BaseVertexShader = VertexShader;
}

template <bool bRenderingReflectiveShadowMaps>
FBoundShaderStateInput FShadowDepthDrawingPolicy<bRenderingReflectiveShadowMaps>::GetBoundShaderStateInput() const
{
	std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>> VertexDeclaration;
	if (bUsePositionOnlyVS)
	{
		VertexDeclaration = VertexFactory->GetPositionDeclaration();
	}
	else
	{
		VertexDeclaration = FMeshDrawingPolicy::GetVertexDeclaration();
	}

	return FBoundShaderStateInput(
		VertexDeclaration,
		VertexShader->GetCode().Get(),
		VertexShader->GetVertexShader(),
		HullShader ? HullShader->GetHullShader() : NULL,
		DomainShader ? DomainShader->GetDomainShader() : NULL,
		PixelShader ? PixelShader->GetPixelShader() : NULL,
		GeometryShader ? GeometryShader->GetGeometryShader() : NULL);
}

template <bool bRenderingReflectiveShadowMaps>
void FShadowDepthDrawingPolicy<bRenderingReflectiveShadowMaps>::SetMeshRenderState(const FSceneView& View, const FPrimitiveSceneProxy* PrimitiveSceneProxy, const FMeshBatch& Mesh, int32 BatchElementIndex, const FDrawingPolicyRenderState& DrawRenderState,  /* const ElementDataType& ElementData, */ const ContextDataType PolicyContext ) const
{
	const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];

	VertexShader->SetMesh(VertexFactory, View, PrimitiveSceneProxy, BatchElement, DrawRenderState, PolicyContext.ShadowInfo);

	if (HullShader && DomainShader)
	{
		HullShader->SetMesh(VertexFactory, View, PrimitiveSceneProxy, BatchElement, DrawRenderState);
		DomainShader->SetMesh(VertexFactory, View, PrimitiveSceneProxy, BatchElement, DrawRenderState);
	}
	if (GeometryShader)
	{
		GeometryShader->SetMesh(PrimitiveSceneProxy, PolicyContext.ShadowInfo, View);
	}
	if (PixelShader)
	{
		PixelShader->SetMesh(VertexFactory, View, PrimitiveSceneProxy, BatchElement, DrawRenderState);
	}
}

template <bool bRenderingReflectiveShadowMaps>
void FShadowDepthDrawingPolicy<bRenderingReflectiveShadowMaps>::SetSharedState(const FDrawingPolicyRenderState& DrawRenderState, const FSceneView* View, const ContextDataType PolicyContext) const
{
	assert(bDirectionalLight == PolicyContext.ShadowInfo->bDirectionalLight && bPreShadow == PolicyContext.ShadowInfo->bPreShadow);

	VertexShader->SetParameters(MaterialRenderProxy, *MaterialResource, *View, PolicyContext.ShadowInfo, DrawRenderState);

	if (GeometryShader)
	{
		GeometryShader->SetParameters(*View, PolicyContext.ShadowInfo);
	}

	if (HullShader && DomainShader)
	{
		HullShader->SetParameters(MaterialRenderProxy, *View, DrawRenderState.GetViewUniformBuffer(), DrawRenderState.GetPassUniformBuffer());
		DomainShader->SetParameters(MaterialRenderProxy, *View, PolicyContext.ShadowInfo, DrawRenderState);
	}

	if (PixelShader)
	{
		PixelShader->SetParameters(MaterialRenderProxy, *MaterialResource, *View, PolicyContext.ShadowInfo, DrawRenderState);
	}

	// Set the shared mesh resources.
	if (bUsePositionOnlyVS)
	{
		VertexFactory->SetPositionStream(D3D11DeviceContext);
	}
	else
	{
		FMeshDrawingPolicy::SetSharedState(D3D11DeviceContext, DrawRenderState, View, PolicyContext);
	}
}

template <bool bRenderingReflectiveShadowMaps>
void FShadowDepthDrawingPolicy<bRenderingReflectiveShadowMaps>::UpdateElementState(FShadowStaticMeshElement& State)
{

}

void FProjectedShadowInfo::RenderDepthDynamic(class FSceneRenderer* SceneRenderer, const FViewInfo* FoundView, const FDrawingPolicyRenderState& DrawRenderState)
{
	FShadowDepthDrawingPolicyFactory::ContextType Context(this);

	for (uint32 MeshBatchIndex = 0; MeshBatchIndex < DynamicSubjectMeshElements.size(); MeshBatchIndex++)
	{
		const FMeshBatchAndRelevance& MeshBatchAndRelevance = DynamicSubjectMeshElements[MeshBatchIndex];
		const FMeshBatch& MeshBatch = *MeshBatchAndRelevance.Mesh;
		FShadowDepthDrawingPolicyFactory::DrawDynamicMesh(*FoundView, Context, MeshBatch, true, DrawRenderState, MeshBatchAndRelevance.PrimitiveSceneProxy/*, MeshBatch.BatchHitProxyId*/);
	}
}

void FProjectedShadowInfo::CopyCachedShadowMap(const FDrawingPolicyRenderState& DrawRenderState, FSceneRenderer* SceneRenderer, const FViewInfo& View, FSetShadowRenderTargetFunction SetShadowRenderTargets)
{

}

void FProjectedShadowInfo::RenderDepthInner(class FSceneRenderer* SceneRenderer, const FViewInfo* FoundView, FSetShadowRenderTargetFunction SetShadowRenderTargets, EShadowDepthRenderMode RenderMode)
{
	TUniformBufferPtr<FSceneTexturesUniformParameters> DeferredPassUniformBuffer;
	FUniformBuffer* PassUniformBuffer = nullptr;

	FSceneTexturesUniformParameters SceneTextureParameters;
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();
	SetupSceneTextureUniformParameters(SceneContext, ESceneTextureSetupMode::None, SceneTextureParameters);
	DeferredPassUniformBuffer = TUniformBufferPtr<FSceneTexturesUniformParameters>::CreateUniformBufferImmediate(SceneTextureParameters);
	PassUniformBuffer = DeferredPassUniformBuffer.get();

	FDrawingPolicyRenderState DrawRenderState(*FoundView, PassUniformBuffer);
	SetStateForDepth(RenderMode, DrawRenderState);

	if (CacheMode == SDCM_MovablePrimitivesOnly)
	{
		// Copy in depths of static primitives before we render movable primitives
		CopyCachedShadowMap(DrawRenderState, SceneRenderer, *FoundView, SetShadowRenderTargets);
	}

	//FShadowDepthDrawingPolicyContext StackPolicyContext(this);
	//FShadowDepthDrawingPolicyContext* PolicyContext(&StackPolicyContext);

	bool bIsWholeSceneDirectionalShadow = IsWholeSceneDirectionalShadow();

	// single threaded version
	SetStateForDepth(RenderMode, DrawRenderState);

	// Draw the subject's static elements using static draw lists
	if (bIsWholeSceneDirectionalShadow && RenderMode != ShadowDepthRenderMode_EmissiveOnly && RenderMode != ShadowDepthRenderMode_GIBlockingVolumes)
	{
		if (bReflectiveShadowmap)
		{
			//SceneRenderer->Scene->WholeSceneReflectiveShadowMapDrawList.DrawVisible(*FoundView, /**PolicyContext,*/ DrawRenderState/*, StaticMeshWholeSceneShadowDepthMap, StaticMeshWholeSceneShadowBatchVisibility*/);
		}
		else
		{
			// Use the scene's shadow depth draw list with this shadow's visibility map
			//SceneRenderer->Scene->WholeSceneShadowDepthDrawList.DrawVisible(*FoundView, /**PolicyContext,*/ DrawRenderState/*, StaticMeshWholeSceneShadowDepthMap, StaticMeshWholeSceneShadowBatchVisibility*/);
		}
	}
	// Draw the subject's static elements using manual state filtering
	else if (StaticSubjectMeshElements.size() > 0)
	{
		if (bReflectiveShadowmap && !bOnePassPointLightShadow)
		{
			// reflective shadow map
			DrawShadowMeshElements<true>( *FoundView, DrawRenderState, *this);
		}
		else
		{
			// normal shadow map
			DrawShadowMeshElements<false>( *FoundView, DrawRenderState, *this);
		}
	}
	RenderDepthDynamic( SceneRenderer, FoundView, DrawRenderState);
}


void FProjectedShadowInfo::RenderDepth(class FSceneRenderer* SceneRenderer, FSetShadowRenderTargetFunction SetShadowRenderTargets, EShadowDepthRenderMode RenderMode)
{
	std::vector<FShadowStaticMeshElement>* PtrCurrentMeshElements = nullptr;
	PrimitiveArrayType* PtrCurrentPrimitives = nullptr;

	switch (RenderMode)
	{
	case ShadowDepthRenderMode_Normal:
		PtrCurrentMeshElements = &StaticSubjectMeshElements;
		PtrCurrentPrimitives = &DynamicSubjectPrimitives;
		break;
	case ShadowDepthRenderMode_EmissiveOnly:
		//PtrCurrentMeshElements = &EmissiveOnlyMeshElements;
		//PtrCurrentPrimitives = &EmissiveOnlyPrimitives;
		break;
	case ShadowDepthRenderMode_GIBlockingVolumes:
		//PtrCurrentMeshElements = &GIBlockingMeshElements;
		//PtrCurrentPrimitives = &GIBlockingPrimitives;
		break;
	default:
		assert(0);
	}

	std::vector<FShadowStaticMeshElement>& CurrentMeshElements = *PtrCurrentMeshElements;
	PrimitiveArrayType& CurrentPrimitives = *PtrCurrentPrimitives;

	if (RenderMode != ShadowDepthRenderMode_Normal && CurrentMeshElements.size() == 0 && CurrentPrimitives.size() == 0)
	{
		return;
	}

	RenderDepthInner(SceneRenderer, ShadowDepthView, SetShadowRenderTargets, RenderMode);
}

void FProjectedShadowInfo::SetStateForDepth(EShadowDepthRenderMode RenderMode, FDrawingPolicyRenderState& DrawRenderState)
{
	assert(bAllocated);

	RHISetViewport(
		X + BorderSize,
		Y + BorderSize,
		0.0f,
		X + BorderSize + ResolutionX,
		Y + BorderSize + ResolutionY,
		1.0f
	);

	// GIBlockingVolumes render mode only affects the reflective shadow map, using the opacity of the material to multiply against the existing color.
	if (RenderMode == ShadowDepthRenderMode_GIBlockingVolumes)
	{
		DrawRenderState.SetBlendState(TStaticBlendState<FALSE,FALSE,
			0,								D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_ONE,			D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_ONE, 
			D3D11_COLOR_WRITE_ENABLE_ALL,	D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_ONE>::GetRHI());
	}
	// The EmissiveOnly render mode shouldn't write into the reflective shadow map, only into the LPV.
	else if (RenderMode == ShadowDepthRenderMode_EmissiveOnly)
	{
		DrawRenderState.SetBlendState(TStaticBlendState<FALSE, TRUE, 0, D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_ONE, 0>::GetRHI());
	}
	else if (bReflectiveShadowmap && !bOnePassPointLightShadow)
	{
		// Enable color writes to the reflective shadow map targets with opaque blending
		DrawRenderState.SetBlendState(TStaticBlendStateWriteMask<0, 0>::GetRHI());
	}
	else
	{
		// Disable color writes
		DrawRenderState.SetBlendState(TStaticBlendState<FALSE, TRUE>::GetRHI());
	}

	if (RenderMode == ShadowDepthRenderMode_EmissiveOnly || RenderMode == ShadowDepthRenderMode_GIBlockingVolumes)
	{
		DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, D3D11_COMPARISON_LESS_EQUAL>::GetRHI());
	}
	else
	{
		DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<true, D3D11_COMPARISON_LESS_EQUAL>::GetRHI());
	}
}

void FSceneRenderer::RenderShadowDepthMapAtlases()
{
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();


}

void FSceneRenderer::RenderShadowDepthMaps()
{
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();

	FSceneRenderer::RenderShadowDepthMapAtlases();

	for (uint32 CubemapIndex = 0; CubemapIndex < SortedShadowsForShadowDepthPass.ShadowMapCubemaps.size(); CubemapIndex++)
	{
		const FSortedShadowMapAtlas& ShadowMap = SortedShadowsForShadowDepthPass.ShadowMapCubemaps[CubemapIndex];
		PooledRenderTarget& RenderTarget = *ShadowMap.RenderTargets.DepthTarget.Get();
		FIntPoint TargetSize = ShadowMap.RenderTargets.DepthTarget->GetDesc().Extent;

		assert(ShadowMap.Shadows.size() == 1);
		FProjectedShadowInfo* ProjectedShadowInfo = ShadowMap.Shadows[0];

		auto SetShadowRenderTargets = [this, &RenderTarget, &SceneContext](bool bPerformClear)
		{
			SetRenderTarget(nullptr, RenderTarget.TargetableTexture.get(), false, true, false);
		};

		{
			bool bDoClear = true;

			if (ProjectedShadowInfo->CacheMode == SDCM_MovablePrimitivesOnly && Scene->CachedShadowMaps.at(ProjectedShadowInfo->GetLightSceneInfo().Id).bCachedShadowMapHasPrimitives)
			{
				// Skip the clear when we'll copy from a cached shadowmap
				bDoClear = false;
			}

			//SCOPED_CONDITIONAL_DRAW_EVENT(RHICmdList, Clear, bDoClear);
			SetShadowRenderTargets(bDoClear);
		}

		ProjectedShadowInfo->RenderDepth(this, SetShadowRenderTargets, ShadowDepthRenderMode_Normal);

		//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, RenderTarget.TargetableTexture);
	}
}

void FShadowDepthDrawingPolicyFactory::AddStaticMesh(FScene* Scene, FStaticMesh* StaticMesh)
{
	if (StaticMesh->CastShadow)
	{
		const FMaterialRenderProxy* MaterialRenderProxy = StaticMesh->MaterialRenderProxy;
		const FMaterial* Material = MaterialRenderProxy->GetMaterial();
		const EBlendMode BlendMode = Material->GetBlendMode();
		const EMaterialShadingModel ShadingModel = Material->GetShadingModel();

		const bool bLightPropagationVolume = false;// UseLightPropagationVolumeRT(FeatureLevel);
		const bool bTwoSided = false;// Material->IsTwoSided() || StaticMesh->PrimitiveSceneInfo->Proxy->CastsShadowAsTwoSided();
		const bool bLitOpaque = !IsTranslucentBlendMode(BlendMode) && ShadingModel != MSM_Unlit;

		//FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(*StaticMesh);
		//OverrideSettings.MeshOverrideFlags |= bTwoSided ? EDrawingPolicyOverrideFlags::TwoSided : EDrawingPolicyOverrideFlags::None;

// 		if (bLightPropagationVolume && ((!IsTranslucentBlendMode(BlendMode) && ShadingModel != MSM_Unlit) || Material->ShouldInjectEmissiveIntoLPV() || Material->ShouldBlockGI()))
// 		{
// 			// Add the static mesh to the shadow's subject draw list.
// 			if (StaticMesh->PrimitiveSceneInfo->Proxy->AffectsDynamicIndirectLighting())
// 			{
// 				Scene->WholeSceneReflectiveShadowMapDrawList.AddMesh(
// 					StaticMesh,
// 					FShadowDepthDrawingPolicy<true>::ElementDataType(),
// 					FShadowDepthDrawingPolicy<true>(
// 						Material,
// 						true,
// 						false,
// 						false,
// 						OverrideSettings,
// 						FeatureLevel,
// 						StaticMesh->VertexFactory,
// 						MaterialRenderProxy,
// 						StaticMesh->ReverseCulling),
// 					FeatureLevel
// 				);
// 			}
// 		}
		if (bLitOpaque)
		{
			//OverrideWithDefaultMaterialForShadowDepth(MaterialRenderProxy, Material, false, FeatureLevel);

			// Add the static mesh to the shadow's subject draw list.
			Scene->WholeSceneShadowDepthDrawList.AddMesh(
				StaticMesh,
				/*FShadowDepthDrawingPolicy<false>::ElementDataType(),*/
				FShadowDepthDrawingPolicy<false>(
					Material,
					true,
					false,
					false,
					//OverrideSettings,
					StaticMesh->VertexFactory,
					MaterialRenderProxy,
					StaticMesh->ReverseCulling)//,
			);
		}
	}
}

bool FShadowDepthDrawingPolicyFactory::DrawDynamicMesh(
	const FSceneView& View, 
	ContextType Context, 
	const FMeshBatch& Mesh, 
	bool bPreFog, 
	const FDrawingPolicyRenderState& DrawRenderState, 
	const FPrimitiveSceneProxy* PrimitiveSceneProxy)
{
	bool bDirty = false;

	// Use a per-FMeshBatch check on top of the per-primitive check because dynamic primitives can submit multiple FMeshElements.
	if (Mesh.CastShadow)
	{
		const FMaterialRenderProxy* MaterialRenderProxy = Mesh.MaterialRenderProxy;
		const FMaterial* Material = MaterialRenderProxy->GetMaterial();
		const EBlendMode BlendMode = Material->GetBlendMode();
		const EMaterialShadingModel ShadingModel = Material->GetShadingModel();

		const bool bLocalOnePassPointLightShadow = Context.ShadowInfo->bOnePassPointLightShadow;
		const bool bReflectiveShadowmap = Context.ShadowInfo->bReflectiveShadowmap && !bLocalOnePassPointLightShadow;

		bool bProcess = !IsTranslucentBlendMode(BlendMode) && ShadingModel != MSM_Unlit && ShouldIncludeDomainInMeshPass(Material->GetMaterialDomain());

		if (bReflectiveShadowmap && Material->ShouldInjectEmissiveIntoLPV())
		{
			bProcess = true;
		}

		if (bProcess)
		{
			const bool bLocalDirectionalLight = Context.ShadowInfo->bDirectionalLight;
			const bool bPreShadow = Context.ShadowInfo->bPreShadow;
			const bool bTwoSided = false;// Material->IsTwoSided() || PrimitiveSceneProxy->CastsShadowAsTwoSided();
			const FShadowDepthDrawingPolicyContext PolicyContext(Context.ShadowInfo);

			//FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(Mesh);
			//OverrideSettings.MeshOverrideFlags |= bTwoSided ? EDrawingPolicyOverrideFlags::TwoSided : EDrawingPolicyOverrideFlags::None;

			//OverrideWithDefaultMaterialForShadowDepth(MaterialRenderProxy, Material, bReflectiveShadowmap, FeatureLevel);

			if (bReflectiveShadowmap)
			{
				FShadowDepthDrawingPolicy<true> DrawingPolicy(
					MaterialRenderProxy->GetMaterial(),
					bLocalDirectionalLight,
					bLocalOnePassPointLightShadow,
					bPreShadow,
					//OverrideSettings,
					Mesh.VertexFactory,
					MaterialRenderProxy,
					Mesh.ReverseCulling
				);

				//TODO MaybeRemovable if ShadowDepth never support LOD Transitions
				FDrawingPolicyRenderState DrawRenderStateLocal(DrawRenderState);
				DrawRenderStateLocal.SetDitheredLODTransitionAlpha(Mesh.DitheredLODTransitionAlpha);
				//SetViewFlagsForShadowPass(DrawRenderStateLocal, View, View.GetFeatureLevel(), DrawingPolicy.IsTwoSided(), true, bLocalOnePassPointLightShadow);
				DrawingPolicy.SetupPipelineState(DrawRenderStateLocal, View);
				CommitGraphicsPipelineState(DrawingPolicy, DrawRenderStateLocal, DrawingPolicy.GetBoundShaderStateInput());
				DrawingPolicy.SetSharedState(DrawRenderStateLocal, &View, PolicyContext);

				for (uint32 BatchElementIndex = 0, Num = Mesh.Elements.size(); BatchElementIndex < Num; BatchElementIndex++)
				{
					//TDrawEvent<FRHICommandList> MeshEvent;
					//BeginMeshDrawEvent(RHICmdList, PrimitiveSceneProxy, Mesh, MeshEvent, EnumHasAnyFlags(EShowMaterialDrawEventTypes(GShowMaterialDrawEventTypes), EShowMaterialDrawEventTypes::ShadowDepthRsm));

					DrawingPolicy.SetMeshRenderState(View, PrimitiveSceneProxy, Mesh, BatchElementIndex, DrawRenderStateLocal, /*FMeshDrawingPolicy::ElementDataType(), */PolicyContext);
					DrawingPolicy.DrawMesh(D3D11DeviceContext, View, Mesh, BatchElementIndex);
				}
			}
			else
			{
				FShadowDepthDrawingPolicy<false> DrawingPolicy(
					MaterialRenderProxy->GetMaterial(),
					bLocalDirectionalLight,
					bLocalOnePassPointLightShadow,
					bPreShadow,
					//OverrideSettings,
					Mesh.VertexFactory,
					MaterialRenderProxy,
					Mesh.ReverseCulling
				);

				//TODO MaybeRemovable if ShadowDepth never support LOD Transitions
				FDrawingPolicyRenderState DrawRenderStateLocal(DrawRenderState);
				DrawRenderStateLocal.SetDitheredLODTransitionAlpha(Mesh.DitheredLODTransitionAlpha);
				//SetViewFlagsForShadowPass(DrawRenderStateLocal, View, View.GetFeatureLevel(), DrawingPolicy.IsTwoSided(), false, bLocalOnePassPointLightShadow);
				DrawingPolicy.SetupPipelineState(DrawRenderStateLocal, View);
				CommitGraphicsPipelineState(DrawingPolicy, DrawRenderStateLocal, DrawingPolicy.GetBoundShaderStateInput());
				DrawingPolicy.SetSharedState(DrawRenderStateLocal, &View, PolicyContext);

				for (uint32 BatchElementIndex = 0; BatchElementIndex < Mesh.Elements.size(); BatchElementIndex++)
				{
					DrawingPolicy.SetMeshRenderState(View, PrimitiveSceneProxy, Mesh, BatchElementIndex, DrawRenderStateLocal, /*FMeshDrawingPolicy::ElementDataType(),*/ PolicyContext);
					DrawingPolicy.DrawMesh(D3D11DeviceContext, View, Mesh, BatchElementIndex);
				}
			}


			bDirty = true;
		}
	}

	return bDirty;
}
