#pragma once

#include "D3D11RHI.h"
#include "SceneManagement.h"
#include "VertexFactory.h"

struct alignas(16) FPrecomputedLightingParameters
{
	FPrecomputedLightingParameters()
	{
		ConstructUniformBufferInfo(*this);
	}

	struct ConstantStruct
	{
		FVector IndirectLightingCachePrimitiveAdd; // FCachedVolumeIndirectLightingPolicy
		float Padding001;
		FVector IndirectLightingCachePrimitiveScale; // FCachedVolumeIndirectLightingPolicy
		float Padding002;
		FVector IndirectLightingCacheMinUV; // FCachedVolumeIndirectLightingPolicy
		float Padding003;
		FVector IndirectLightingCacheMaxUV; // FCachedVolumeIndirectLightingPolicy
		float Padding004;
		Vector4 PointSkyBentNormal; // FCachedPointIndirectLightingPolicy
		float DirectionalLightShadowing;// EShaderPrecisionModifier::Half // FCachedPointIndirectLightingPolicy
		FVector Pading005;
		Vector4 StaticShadowMapMasks; // TDistanceFieldShadowsAndLightMapPolicy
		Vector4 InvUniformPenumbraSizes; // TDistanceFieldShadowsAndLightMapPolicy
		Vector4 IndirectLightingSHCoefficients0[3]; // FCachedPointIndirectLightingPolicy
		Vector4 IndirectLightingSHCoefficients1[3]; // FCachedPointIndirectLightingPolicy
		Vector4 IndirectLightingSHCoefficients2; // FCachedPointIndirectLightingPolicy
		Vector4 IndirectLightingSHSingleCoefficient;// EShaderPrecisionModifier::Half; // FCachedPointIndirectLightingPolicy used in forward Translucent
		Vector4 LightMapCoordinateScaleBias; // TLightMapPolicy
		Vector4 ShadowMapCoordinateScaleBias; // TDistanceFieldShadowsAndLightMapPolicy
		Vector4 LightMapScale[MAX_NUM_LIGHTMAP_COEF];// EShaderPrecisionModifier::Half; // TLightMapPolicy
		Vector4 LightMapAdd[MAX_NUM_LIGHTMAP_COEF];// EShaderPrecisionModifier::Half; // TLightMapPolicy
	} Constants ;

	ID3D11ShaderResourceView* LightMapTexture; // TLightMapPolicy
	ID3D11ShaderResourceView* SkyOcclusionTexture; // TLightMapPolicy
	ID3D11ShaderResourceView* AOMaterialMaskTexture; // TLightMapPolicy
	ID3D11ShaderResourceView* IndirectLightingCacheTexture0; //Texture3D  FCachedVolumeIndirectLightingPolicy
	ID3D11ShaderResourceView* IndirectLightingCacheTexture1; //Texture3D FCachedVolumeIndirectLightingPolicy
	ID3D11ShaderResourceView* IndirectLightingCacheTexture2; //Texture3D FCachedVolumeIndirectLightingPolicy
	ID3D11ShaderResourceView* StaticShadowTexture; // 
	ID3D11SamplerState* LightMapSampler; // TLightMapPolicy
	ID3D11SamplerState* SkyOcclusionSampler; // TLightMapPolicy
	ID3D11SamplerState* AOMaterialMaskSampler; // TLightMapPolicy
	ID3D11SamplerState* IndirectLightingCacheTextureSampler0; // FCachedVolumeIndirectLightingPolicy
	ID3D11SamplerState* IndirectLightingCacheTextureSampler1; // FCachedVolumeIndirectLightingPolicy
	ID3D11SamplerState* IndirectLightingCacheTextureSampler2; // FCachedVolumeIndirectLightingPolicy
	ID3D11SamplerState* StaticShadowTextureSampler; // TDistanceFieldShadowsAndLightMapPolicy

	static std::string GetConstantBufferName()
	{
		return "PrecomputedLightingBuffer";
	}
#define ADD_RES(StructName, MemberName) List.insert(std::make_pair(std::string(#StructName) + "_" + std::string(#MemberName),StructName.MemberName))
	static std::map<std::string, ID3D11ShaderResourceView*> GetSRVs(const FPrecomputedLightingParameters& PrecomputedLightingBuffer)
	{
		std::map<std::string,ID3D11ShaderResourceView*> List;
		ADD_RES(PrecomputedLightingBuffer, LightMapTexture);
		ADD_RES(PrecomputedLightingBuffer, SkyOcclusionTexture);
		ADD_RES(PrecomputedLightingBuffer, AOMaterialMaskTexture);
		ADD_RES(PrecomputedLightingBuffer, IndirectLightingCacheTexture0);
		ADD_RES(PrecomputedLightingBuffer, IndirectLightingCacheTexture1);
		ADD_RES(PrecomputedLightingBuffer, IndirectLightingCacheTexture2);
		ADD_RES(PrecomputedLightingBuffer, StaticShadowTexture);
		return List;
	}
	static std::map<std::string, ID3D11SamplerState*> GetSamplers(const FPrecomputedLightingParameters& PrecomputedLightingBuffer)
	{
		std::map<std::string, ID3D11SamplerState*> List;
		ADD_RES(PrecomputedLightingBuffer, LightMapSampler);
		ADD_RES(PrecomputedLightingBuffer, SkyOcclusionSampler);
		ADD_RES(PrecomputedLightingBuffer, AOMaterialMaskSampler);
		ADD_RES(PrecomputedLightingBuffer, IndirectLightingCacheTextureSampler0);
		ADD_RES(PrecomputedLightingBuffer, IndirectLightingCacheTextureSampler1);
		ADD_RES(PrecomputedLightingBuffer, IndirectLightingCacheTextureSampler2);
		ADD_RES(PrecomputedLightingBuffer, StaticShadowTextureSampler);
		return List;
	}
	static std::map<std::string, ID3D11UnorderedAccessView*> GetUAVs(const FPrecomputedLightingParameters& PrecomputedLightingBuffer)
	{
		std::map<std::string, ID3D11UnorderedAccessView*> List;
		return List;
	}
#undef ADD_RES
};

void GetPrecomputedLightingParameters(
	FPrecomputedLightingParameters& Parameters,
	/*const class FIndirectLightingCache* LightingCache,*/
	/*const class FIndirectLightingCacheAllocation* LightingAllocation,*/
	FVector VolumetricLightmapLookupPosition,
	uint32 SceneFrameNumber,
	/*class FVolumetricLightmapSceneData* VolumetricLightmapSceneData,*/
	const FLightCacheInterface* LCI
);

std::shared_ptr<FUniformBuffer> CreatePrecomputedLightingUniformBuffer(
	/*EUniformBufferUsage BufferUsage,*/
	/*const class FIndirectLightingCache* LightingCache,*/
	/*const class FIndirectLightingCacheAllocation* LightingAllocation,*/
	FVector VolumetricLightmapLookupPosition,
	uint32 SceneFrameNumber,
	/*FVolumetricLightmapSceneData* VolumetricLightmapSceneData,*/
	const FLightCacheInterface* LCI
);

/**
* Default precomputed lighting data. Used for fully dynamic lightmap policies.
*/
class FEmptyPrecomputedLightingUniformBuffer : public TUniformBuffer< FPrecomputedLightingParameters >
{
	typedef TUniformBuffer< FPrecomputedLightingParameters > Super;
public:
	virtual void InitDynamicRHI()
	{
	}
};

extern FEmptyPrecomputedLightingUniformBuffer GEmptyPrecomputedLightingUniformBuffer;
/**
* A policy for shaders without a light-map.
*/
struct FNoLightMapPolicy
{
	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		return true;
	}

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{}

	static bool RequiresSkylight()
	{
		return false;
	}
};

enum ELightmapQuality
{
	LQ_LIGHTMAP,
	HQ_LIGHTMAP,
};

// One of these per lightmap quality
extern const char* GLightmapDefineName[2];
extern int32 GNumLightmapCoefficients[2];

/**
* Base policy for shaders with lightmaps.
*/
template< ELightmapQuality LightmapQuality >
struct TLightMapPolicy
{
	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(GLightmapDefineName[LightmapQuality], ("1"));
		OutEnvironment.SetDefine(("NUM_LIGHTMAP_COEFFICIENTS"), GNumLightmapCoefficients[LightmapQuality]);
	}

	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		//static const auto AllowStaticLightingVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.AllowStaticLighting"));
		//static const auto CVarProjectCanHaveLowQualityLightmaps = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.SupportLowQualityLightmaps"));
		//static const auto CVarSupportAllShadersPermutations = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.SupportAllShaderPermutations"));
		const bool bForceAllPermutations = false;// CVarSupportAllShadersPermutations && CVarSupportAllShadersPermutations->GetValueOnAnyThread() != 0;

		// if GEngine doesn't exist yet to have the project flag then we should be conservative and cache the LQ lightmap policy
		const bool bProjectCanHaveLowQualityLightmaps = bForceAllPermutations || true/*(!CVarProjectCanHaveLowQualityLightmaps) || (CVarProjectCanHaveLowQualityLightmaps->GetValueOnAnyThread() != 0)*/;

		const bool bShouldCacheQuality = (LightmapQuality != ELightmapQuality::LQ_LIGHTMAP) || bProjectCanHaveLowQualityLightmaps;

		// GetValueOnAnyThread() as it's possible that ShouldCache is called from rendering thread. That is to output some error message.
		return (Material->GetShadingModel() != MSM_Unlit)
			&& bShouldCacheQuality
			&& VertexFactoryType->SupportsStaticLighting()
			&& true/*(!AllowStaticLightingVar || AllowStaticLightingVar->GetValueOnAnyThread() != 0)*/
			&& (Material->IsUsedWithStaticLighting() || Material->IsSpecialEngineMaterial());
	}

	static bool RequiresSkylight()
	{
		return false;
	}
};

// A light map policy for computing up to 4 signed distance field shadow factors in the base pass.
template< ELightmapQuality LightmapQuality >
struct TDistanceFieldShadowsAndLightMapPolicy : public TLightMapPolicy< LightmapQuality >
{
	typedef TLightMapPolicy< LightmapQuality >	Super;

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(("STATICLIGHTING_TEXTUREMASK"), 1);
		OutEnvironment.SetDefine(("STATICLIGHTING_SIGNEDDISTANCEFIELD"), 1);
		Super::ModifyCompilationEnvironment(Material, OutEnvironment);
	}
};

/**
* Policy for 'fake' texture lightmaps, such as the LightMap density rendering mode
*/
struct FDummyLightMapPolicy : public TLightMapPolicy< HQ_LIGHTMAP >
{
public:

	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		return Material->GetShadingModel() != MSM_Unlit && VertexFactoryType->SupportsStaticLighting();
	}
};

/**
* Allows precomputed irradiance lookups at any point in space.
*/
struct FPrecomputedVolumetricLightmapLightingPolicy
{
	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		//static const auto AllowStaticLightingVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.AllowStaticLighting"));

		return Material->GetShadingModel() != MSM_Unlit /*&& (!AllowStaticLightingVar || AllowStaticLightingVar->GetValueOnAnyThread() != 0)*/;
	}

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(("PRECOMPUTED_IRRADIANCE_VOLUME_LIGHTING"), ("1"));
	}
};

/**
* Allows a dynamic object to access indirect lighting through a per-object allocation in a volume texture atlas
*/
struct FCachedVolumeIndirectLightingPolicy
{
	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		//static const auto AllowStaticLightingVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.AllowStaticLighting"));

		return Material->GetShadingModel() != MSM_Unlit
			&& !IsTranslucentBlendMode(Material->GetBlendMode())
			&& true/*(!AllowStaticLightingVar || AllowStaticLightingVar->GetValueOnAnyThread() != 0)*/
			&& true/*IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4)*/;
	}

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(("CACHED_VOLUME_INDIRECT_LIGHTING"), ("1"));
	}

	static bool RequiresSkylight()
	{
		return false;
	}
};


/**
* Allows a dynamic object to access indirect lighting through a per-object lighting sample
*/
struct FCachedPointIndirectLightingPolicy
{
	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		//static const auto AllowStaticLightingVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.AllowStaticLighting"));

		return Material->GetShadingModel() != MSM_Unlit/* && (!AllowStaticLightingVar || AllowStaticLightingVar->GetValueOnAnyThread() != 0)*/;
	}

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment);

	static bool RequiresSkylight()
	{
		return false;
	}
};

enum ELightMapPolicyType
{
	LMP_NO_LIGHTMAP,
	LMP_PRECOMPUTED_IRRADIANCE_VOLUME_INDIRECT_LIGHTING,
	LMP_CACHED_VOLUME_INDIRECT_LIGHTING,
	LMP_CACHED_POINT_INDIRECT_LIGHTING,
	LMP_SIMPLE_NO_LIGHTMAP,
	LMP_SIMPLE_LIGHTMAP_ONLY_LIGHTING,
	LMP_SIMPLE_DIRECTIONAL_LIGHT_LIGHTING,
	LMP_SIMPLE_STATIONARY_PRECOMPUTED_SHADOW_LIGHTING,
	LMP_SIMPLE_STATIONARY_SINGLESAMPLE_SHADOW_LIGHTING,
	LMP_SIMPLE_STATIONARY_VOLUMETRICLIGHTMAP_SHADOW_LIGHTING,
	LMP_LQ_LIGHTMAP,
	LMP_HQ_LIGHTMAP,
	LMP_DISTANCE_FIELD_SHADOWS_AND_HQ_LIGHTMAP,
	// Mobile specific
	LMP_MOBILE_DISTANCE_FIELD_SHADOWS_AND_LQ_LIGHTMAP,
	LMP_MOBILE_DISTANCE_FIELD_SHADOWS_LIGHTMAP_AND_CSM,
	LMP_MOBILE_DIRECTIONAL_LIGHT_AND_SH_INDIRECT,
	LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT_AND_SH_INDIRECT,
	LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT_CSM_AND_SH_INDIRECT,
	LMP_MOBILE_DIRECTIONAL_LIGHT_CSM_AND_SH_INDIRECT,
	LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT,
	LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT_CSM,
	LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT_WITH_LIGHTMAP,
	LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT_CSM_WITH_LIGHTMAP,
	// LightMapDensity
	LMP_DUMMY
};

class FUniformLightMapPolicyShaderParametersType
{
public:
	void Bind(const FShaderParameterMap& ParameterMap)
	{
		BufferParameter.Bind(ParameterMap, ("PrecomputedLightingBuffer"));
		for (auto& Pair : FPrecomputedLightingParameters::GetSRVs(FPrecomputedLightingParameters()))
		{
			BufferParameter.BindSRV(ParameterMap, Pair.first.c_str());
		}
		for (auto& Pair : FPrecomputedLightingParameters::GetSamplers(FPrecomputedLightingParameters()))
		{
			BufferParameter.BindSampler(ParameterMap, Pair.first.c_str());
		}
	}


	FShaderUniformBufferParameter BufferParameter;
};

class FUniformLightMapPolicy
{
public:

	typedef  const FLightCacheInterface* ElementDataType;

	typedef FUniformLightMapPolicyShaderParametersType PixelParametersType;
	typedef FUniformLightMapPolicyShaderParametersType VertexParametersType;

	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		return false; // This one does not compile shaders since we can't tell which policy to use.
	}

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{}

	FUniformLightMapPolicy(ELightMapPolicyType InIndirectPolicy) : IndirectPolicy(InIndirectPolicy) {}

	void SetMesh(
		const FSceneView& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const VertexParametersType* VertexShaderParameters,
		const PixelParametersType* PixelShaderParameters,
		FShader* VertexShader,
		FShader* PixelShader,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FLightCacheInterface* LCI
	) const;

	friend bool operator==(const FUniformLightMapPolicy A, const FUniformLightMapPolicy B)
	{
		return A.IndirectPolicy == B.IndirectPolicy;
	}

// 	friend int32 CompareDrawingPolicy(const FUniformLightMapPolicy& A, const FUniformLightMapPolicy& B)
// 	{
// 		COMPAREDRAWINGPOLICYMEMBERS(IndirectPolicy);
// 		return  0;
// 	}

	ELightMapPolicyType GetIndirectPolicy() const { return IndirectPolicy; }

private:

	ELightMapPolicyType IndirectPolicy;
};

template <ELightMapPolicyType Policy>
class TUniformLightMapPolicy : public FUniformLightMapPolicy
{
public:

	TUniformLightMapPolicy() : FUniformLightMapPolicy(Policy) {}

	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		//CA_SUPPRESS(6326);
		switch (Policy)
		{
		case LMP_NO_LIGHTMAP:
			return FNoLightMapPolicy::ShouldCompilePermutation(Material, VertexFactoryType);
		case LMP_PRECOMPUTED_IRRADIANCE_VOLUME_INDIRECT_LIGHTING:
			return FPrecomputedVolumetricLightmapLightingPolicy::ShouldCompilePermutation(Material, VertexFactoryType);
		case LMP_CACHED_VOLUME_INDIRECT_LIGHTING:
			return FCachedVolumeIndirectLightingPolicy::ShouldCompilePermutation(Material, VertexFactoryType);
		case LMP_CACHED_POINT_INDIRECT_LIGHTING:
			return FCachedPointIndirectLightingPolicy::ShouldCompilePermutation(Material, VertexFactoryType);
// 		case LMP_SIMPLE_NO_LIGHTMAP:
// 			return FSimpleNoLightmapLightingPolicy::ShouldCompilePermutation(Platform, Material, VertexFactoryType);
// 		case LMP_SIMPLE_LIGHTMAP_ONLY_LIGHTING:
// 			return FSimpleLightmapOnlyLightingPolicy::ShouldCompilePermutation(Platform, Material, VertexFactoryType);
// 		case LMP_SIMPLE_DIRECTIONAL_LIGHT_LIGHTING:
// 			return FSimpleDirectionalLightLightingPolicy::ShouldCompilePermutation(Platform, Material, VertexFactoryType);
// 		case LMP_SIMPLE_STATIONARY_PRECOMPUTED_SHADOW_LIGHTING:
// 			return FSimpleStationaryLightPrecomputedShadowsLightingPolicy::ShouldCompilePermutation(Platform, Material, VertexFactoryType);
// 		case LMP_SIMPLE_STATIONARY_SINGLESAMPLE_SHADOW_LIGHTING:
// 			return FSimpleStationaryLightSingleSampleShadowsLightingPolicy::ShouldCompilePermutation(Platform, Material, VertexFactoryType);
// 		case LMP_SIMPLE_STATIONARY_VOLUMETRICLIGHTMAP_SHADOW_LIGHTING:
// 			return FSimpleStationaryLightVolumetricLightmapShadowsLightingPolicy::ShouldCompilePermutation(Platform, Material, VertexFactoryType);
		case LMP_LQ_LIGHTMAP:
			return TLightMapPolicy<LQ_LIGHTMAP>::ShouldCompilePermutation(Material, VertexFactoryType);
		case LMP_HQ_LIGHTMAP:
			return TLightMapPolicy<HQ_LIGHTMAP>::ShouldCompilePermutation(Material, VertexFactoryType);
		case LMP_DISTANCE_FIELD_SHADOWS_AND_HQ_LIGHTMAP:
			return TDistanceFieldShadowsAndLightMapPolicy<HQ_LIGHTMAP>::ShouldCompilePermutation(Material, VertexFactoryType);

			// Mobile specific

// 		case LMP_MOBILE_DISTANCE_FIELD_SHADOWS_AND_LQ_LIGHTMAP:
// 			return FMobileDistanceFieldShadowsAndLQLightMapPolicy::ShouldCompilePermutation(Platform, Material, VertexFactoryType);
// 		case LMP_MOBILE_DISTANCE_FIELD_SHADOWS_LIGHTMAP_AND_CSM:
// 			return FMobileDistanceFieldShadowsLightMapAndCSMLightingPolicy::ShouldCompilePermutation(Platform, Material, VertexFactoryType);
// 		case LMP_MOBILE_DIRECTIONAL_LIGHT_AND_SH_INDIRECT:
// 			return FMobileDirectionalLightAndSHIndirectPolicy::ShouldCompilePermutation(Platform, Material, VertexFactoryType);
// 		case LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT_AND_SH_INDIRECT:
// 			return FMobileMovableDirectionalLightAndSHIndirectPolicy::ShouldCompilePermutation(Platform, Material, VertexFactoryType);
// 		case LMP_MOBILE_DIRECTIONAL_LIGHT_CSM_AND_SH_INDIRECT:
// 			return FMobileDirectionalLightCSMAndSHIndirectPolicy::ShouldCompilePermutation(Platform, Material, VertexFactoryType);
// 		case LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT_CSM_AND_SH_INDIRECT:
// 			return FMobileMovableDirectionalLightCSMAndSHIndirectPolicy::ShouldCompilePermutation(Platform, Material, VertexFactoryType);
// 		case LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT:
// 			return FMobileMovableDirectionalLightLightingPolicy::ShouldCompilePermutation(Platform, Material, VertexFactoryType);
// 		case LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT_CSM:
// 			return FMobileMovableDirectionalLightCSMLightingPolicy::ShouldCompilePermutation(Platform, Material, VertexFactoryType);
// 		case LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT_WITH_LIGHTMAP:
// 			return FMobileMovableDirectionalLightWithLightmapPolicy::ShouldCompilePermutation(Platform, Material, VertexFactoryType);
// 		case LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT_CSM_WITH_LIGHTMAP:
// 			return FMobileMovableDirectionalLightCSMWithLightmapPolicy::ShouldCompilePermutation(Platform, Material, VertexFactoryType);

			// LightMapDensity

		case LMP_DUMMY:
			return FDummyLightMapPolicy::ShouldCompilePermutation(Material, VertexFactoryType);

		default:
			assert(false);
			return false;
		};
	}

	static void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(("MAX_NUM_LIGHTMAP_COEF"), MAX_NUM_LIGHTMAP_COEF);

		//CA_SUPPRESS(6326);
		switch (Policy)
		{
		case LMP_NO_LIGHTMAP:
			FNoLightMapPolicy::ModifyCompilationEnvironment(Material, OutEnvironment);
			break;
		case LMP_PRECOMPUTED_IRRADIANCE_VOLUME_INDIRECT_LIGHTING:
			FPrecomputedVolumetricLightmapLightingPolicy::ModifyCompilationEnvironment(Material, OutEnvironment);
			break;
		case LMP_CACHED_VOLUME_INDIRECT_LIGHTING:
			FCachedVolumeIndirectLightingPolicy::ModifyCompilationEnvironment(Material, OutEnvironment);
			break;
		case LMP_CACHED_POINT_INDIRECT_LIGHTING:
			FCachedPointIndirectLightingPolicy::ModifyCompilationEnvironment(Material, OutEnvironment);
			break;
// 		case LMP_SIMPLE_NO_LIGHTMAP:
// 			return FSimpleNoLightmapLightingPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
// 			break;
// 		case LMP_SIMPLE_LIGHTMAP_ONLY_LIGHTING:
// 			return FSimpleLightmapOnlyLightingPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
// 			break;
// 		case LMP_SIMPLE_DIRECTIONAL_LIGHT_LIGHTING:
// 			FSimpleDirectionalLightLightingPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
// 			break;
// 		case LMP_SIMPLE_STATIONARY_PRECOMPUTED_SHADOW_LIGHTING:
// 			return FSimpleStationaryLightPrecomputedShadowsLightingPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
// 			break;
// 		case LMP_SIMPLE_STATIONARY_SINGLESAMPLE_SHADOW_LIGHTING:
// 			return FSimpleStationaryLightSingleSampleShadowsLightingPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
// 			break;
// 		case LMP_SIMPLE_STATIONARY_VOLUMETRICLIGHTMAP_SHADOW_LIGHTING:
// 			return FSimpleStationaryLightVolumetricLightmapShadowsLightingPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
// 			break;
		case LMP_LQ_LIGHTMAP:
			TLightMapPolicy<LQ_LIGHTMAP>::ModifyCompilationEnvironment(Material, OutEnvironment);
			break;
		case LMP_HQ_LIGHTMAP:
			TLightMapPolicy<HQ_LIGHTMAP>::ModifyCompilationEnvironment(Material, OutEnvironment);
			break;
		case LMP_DISTANCE_FIELD_SHADOWS_AND_HQ_LIGHTMAP:
			TDistanceFieldShadowsAndLightMapPolicy<HQ_LIGHTMAP>::ModifyCompilationEnvironment( Material, OutEnvironment);
			break;

// 			// Mobile specific
// 		case LMP_MOBILE_DISTANCE_FIELD_SHADOWS_AND_LQ_LIGHTMAP:
// 			FMobileDistanceFieldShadowsAndLQLightMapPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
// 			break;
// 		case LMP_MOBILE_DISTANCE_FIELD_SHADOWS_LIGHTMAP_AND_CSM:
// 			FMobileDistanceFieldShadowsLightMapAndCSMLightingPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
// 			break;
// 		case LMP_MOBILE_DIRECTIONAL_LIGHT_AND_SH_INDIRECT:
// 			FMobileDirectionalLightAndSHIndirectPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
// 			break;
// 		case LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT_AND_SH_INDIRECT:
// 			FMobileMovableDirectionalLightAndSHIndirectPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
// 			break;
// 		case LMP_MOBILE_DIRECTIONAL_LIGHT_CSM_AND_SH_INDIRECT:
// 			FMobileDirectionalLightCSMAndSHIndirectPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
// 			break;
// 		case LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT_CSM_AND_SH_INDIRECT:
// 			FMobileMovableDirectionalLightCSMAndSHIndirectPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
// 			break;
// 		case LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT:
// 			FMobileMovableDirectionalLightLightingPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
// 			break;
// 		case LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT_CSM:
// 			FMobileMovableDirectionalLightCSMLightingPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
// 			break;
// 		case LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT_WITH_LIGHTMAP:
// 			FMobileMovableDirectionalLightWithLightmapPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
// 			break;
// 		case LMP_MOBILE_MOVABLE_DIRECTIONAL_LIGHT_CSM_WITH_LIGHTMAP:
// 			FMobileMovableDirectionalLightCSMWithLightmapPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
// 			break;

			// LightMapDensity

		case LMP_DUMMY:
			FDummyLightMapPolicy::ModifyCompilationEnvironment(Material, OutEnvironment);
			break;

		default:
			assert(false);
			break;
		}
	}

	static bool RequiresSkylight()
	{
		//CA_SUPPRESS(6326);
		switch (Policy)
		{
			// Simple forward
		case LMP_SIMPLE_NO_LIGHTMAP:
		case LMP_SIMPLE_LIGHTMAP_ONLY_LIGHTING:
		case LMP_SIMPLE_DIRECTIONAL_LIGHT_LIGHTING:
		case LMP_SIMPLE_STATIONARY_PRECOMPUTED_SHADOW_LIGHTING:
		case LMP_SIMPLE_STATIONARY_SINGLESAMPLE_SHADOW_LIGHTING:
			return true;
		default:
			return false;
		};
	}
};
