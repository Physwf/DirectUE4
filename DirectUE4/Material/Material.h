#pragma once

#include "ShaderCore.h"
#include "Shader.h"

enum EMaterialDomain
{
	/** The material's attributes describe a 3d surface. */
	MD_Surface ,
	/** The material's attributes describe a deferred decal, and will be mapped onto the decal's frustum. */
	MD_DeferredDecal ,
	/** The material's attributes describe a light's distribution. */
	MD_LightFunction,
	/** The material's attributes describe a 3d volume. */
	MD_Volume ,
	/** The material will be used in a custom post process pass. */
	MD_PostProcess,
	/** The material will be used for UMG or Slate UI */
	MD_UI ,

	MD_MAX
};
/** This is used by the drawing passes to determine tessellation policy, so changes here need to be supported in native code. */
enum EMaterialTessellationMode
{
	/** Tessellation disabled. */
	MTM_NoTessellation ,
	/** Simple tessellation. */
	MTM_FlatTessellation ,
	/** Simple spline based tessellation. */
	MTM_PNTriangles,
	MTM_MAX,
};
enum EBlendMode
{
	BLEND_Opaque ,
	BLEND_Masked ,
	BLEND_Translucent ,
	BLEND_Additive,
	BLEND_Modulate ,
	BLEND_AlphaComposite,
	BLEND_MAX,
};
enum ERefractionMode
{
	/**
	* Refraction is computed based on the camera vector entering a medium whose index of refraction is defined by the Refraction material input.
	* The new medium's surface is defined by the material's normal.  With this mode, a flat plane seen from the side will have a constant refraction offset.
	* This is a physical model of refraction but causes reading outside the scene color texture so is a poor fit for large refractive surfaces like water.
	*/
	RM_IndexOfRefraction,

	/**
	* The refraction offset into Scene Color is computed based on the difference between the per-pixel normal and the per-vertex normal.
	* With this mode, a material whose normal is the default (0, 0, 1) will never cause any refraction.  This mode is only valid with tangent space normals.
	* The refraction material input scales the offset, although a value of 1.0 maps to no refraction, and a value of 2 maps to a scale of 1.0 on the offset.
	* This is a non-physical model of refraction but is useful on large refractive surfaces like water, since offsets have to stay small to avoid reading outside scene color.
	*/
	RM_PixelNormalOffset
};
enum EMaterialShadingModel
{
	MSM_Unlit				,
	MSM_DefaultLit			,
	MSM_Subsurface			,
	MSM_PreintegratedSkin	,
	MSM_ClearCoat			,
	MSM_SubsurfaceProfile	,
	MSM_TwoSidedFoliage		,
	MSM_Hair				,
	MSM_Cloth				,
	MSM_Eye					,
	MSM_MAX,
};
enum ETranslucencyLightingMode
{
	/**
	* Lighting will be calculated for a volume, without directionality.  Use this on particle effects like smoke and dust.
	* This is the cheapest per-pixel lighting method, however the material normal is not taken into account.
	*/
	TLM_VolumetricNonDirectional,

	/**
	* Lighting will be calculated for a volume, with directionality so that the normal of the material is taken into account.
	* Note that the default particle tangent space is facing the camera, so enable bGenerateSphericalParticleNormals to get a more useful tangent space.
	*/
	TLM_VolumetricDirectional,

	/**
	* Same as Volumetric Non Directional, but lighting is only evaluated at vertices so the pixel shader cost is significantly less.
	* Note that lighting still comes from a volume texture, so it is limited in range.  Directional lights become unshadowed in the distance.
	*/
	TLM_VolumetricPerVertexNonDirectional,

	/**
	* Same as Volumetric Directional, but lighting is only evaluated at vertices so the pixel shader cost is significantly less.
	* Note that lighting still comes from a volume texture, so it is limited in range.  Directional lights become unshadowed in the distance.
	*/
	TLM_VolumetricPerVertexDirectional,

	/**
	* Lighting will be calculated for a surface. The light in accumulated in a volume so the result is blurry,
	* limited distance but the per pixel cost is very low. Use this on translucent surfaces like glass and water.
	* Only diffuse lighting is supported.
	*/
	TLM_Surface,

	/**
	* Lighting will be calculated for a surface. Use this on translucent surfaces like glass and water.
	* This is implemented with forward shading so specular highlights from local lights are supported, however many deferred-only features are not.
	* This is the most expensive translucency lighting method as each light's contribution is computed per-pixel.
	*/
	TLM_SurfacePerPixelLighting,

	TLM_MAX,
};
/** Contains all the information needed to uniquely identify a FMaterialShaderMap. */
class FMaterialShaderMapId
{
public:
	uint32 BaseMaterialId;

	void GetMaterialHash(FSHAHash& OutHash) const;

	bool operator==(const FMaterialShaderMapId& ReferenceSet) const;

	bool operator!=(const FMaterialShaderMapId& ReferenceSet) const
	{
		return !(*this == ReferenceSet);
	}
};

namespace std
{
	template<> struct hash<FMaterialShaderMapId>
	{
		std::size_t operator()(FMaterialShaderMapId const& Ref) const noexcept
		{
			return Ref.BaseMaterialId;
		}
	};
}
/**
* The shaders which the render the material on a mesh generated by a particular vertex factory type.
*/
class FMeshMaterialShaderMap : public TShaderMap<FMeshMaterialShaderType>
{
public:

	FMeshMaterialShaderMap(FVertexFactoryType* InVFType)
		: TShaderMap<FMeshMaterialShaderType>()
		, VertexFactoryType(InVFType)
	{}

	/**
	* Enqueues compilation for all shaders for a material and vertex factory type.
	* @param Material - The material to compile shaders for.
	* @param VertexFactoryType - The vertex factory type to compile shaders for.
	* @param Platform - The platform to compile for.
	*/
	uint32 BeginCompile(
		uint32 ShaderMapId,
		const FMaterialShaderMapId& InShaderMapId,
		const FMaterial* Material,
		FShaderCompilerEnvironment* MaterialEnvironment,
		std::vector<FShaderCompileJob*>& NewJobs
	);

	/**
	* Checks whether a material shader map is missing any shader types necessary for the given material.
	* May be called with a NULL FMeshMaterialShaderMap, which is equivalent to a FMeshMaterialShaderMap with no shaders cached.
	* @param MeshShaderMap - The FMeshMaterialShaderMap to check for the necessary shaders.
	* @param Material - The material which is checked.
	* @return True if the shader map has all of the shader types necessary.
	*/
	static bool IsComplete(
		const FMeshMaterialShaderMap* MeshShaderMap,
		const FMaterial* Material,
		FVertexFactoryType* InVertexFactoryType,
		bool bSilent
	);

	void LoadMissingShadersFromMemory(
		const FSHAHash& MaterialShaderMapHash,
		const FMaterial* Material);

	/**
	* Removes all entries in the cache with exceptions based on a shader type
	* @param ShaderType - The shader type to flush
	*/
	void FlushShadersByShaderType(FShaderType* ShaderType);

	/**
	* Removes all entries in the cache with exceptions based on a shader type
	* @param ShaderType - The shader type to flush
	*/
	void FlushShadersByShaderPipelineType(const FShaderPipelineType* ShaderPipelineType);

	// Accessors.
	inline FVertexFactoryType* GetVertexFactoryType() const { return VertexFactoryType; }

private:
	/** The vertex factory type these shaders are for. */
	FVertexFactoryType * VertexFactoryType;

	static bool IsMeshShaderComplete(const FMeshMaterialShaderMap* MeshShaderMap, const FMaterial* Material, const FMeshMaterialShaderType* ShaderType, const FShaderPipelineType* Pipeline, FVertexFactoryType* InVertexFactoryType, bool bSilent);
};
/** Stores all uniform expressions for a material generated from a material translation. */
class FUniformExpressionSet 
{
public:
	FUniformExpressionSet() {}

	//bool IsEmpty() const;
	bool operator==(const FUniformExpressionSet& ReferenceSet) const;
	//std::string GetSummaryString() const;

	//void SetParameterCollections(const TArray<class UMaterialParameterCollection*>& Collections);
	//void CreateBufferStruct();
	//const FUniformBufferStruct& GetUniformBufferStruct() const;

	//FUniformBufferRHIRef CreateUniformBuffer(const FMaterialRenderContext& MaterialRenderContext, FRHICommandList* CommandListIfLocalMode, struct FLocalUniformBuffer* OutLocalUniformBuffer) const;

// 	uint32 GetAllocatedSize() const
// 	{
// 		return UniformVectorExpressions.GetAllocatedSize()
// 			+ UniformScalarExpressions.GetAllocatedSize()
// 			+ Uniform2DTextureExpressions.GetAllocatedSize()
// 			+ UniformCubeTextureExpressions.GetAllocatedSize()
// 			+ UniformVolumeTextureExpressions.GetAllocatedSize()
// 			+ UniformExternalTextureExpressions.GetAllocatedSize()
// 			+ ParameterCollections.GetAllocatedSize()
// 			+ (UniformBufferStruct ? (sizeof(FUniformBufferStruct) + UniformBufferStruct->GetMembers().GetAllocatedSize()) : 0);
// 	}

protected:

// 	TArray<TRefCountPtr<FMaterialUniformExpression> > UniformVectorExpressions;
// 	TArray<TRefCountPtr<FMaterialUniformExpression> > UniformScalarExpressions;
// 	TArray<TRefCountPtr<FMaterialUniformExpressionTexture> > Uniform2DTextureExpressions;
// 	TArray<TRefCountPtr<FMaterialUniformExpressionTexture> > UniformCubeTextureExpressions;
// 	TArray<TRefCountPtr<FMaterialUniformExpressionTexture> > UniformVolumeTextureExpressions;
// 	TArray<TRefCountPtr<FMaterialUniformExpressionExternalTexture> > UniformExternalTextureExpressions;

	/** Ids of parameter collections referenced by the material that was translated. */
	//TArray<FGuid> ParameterCollections;

	/** The structure of a uniform buffer containing values for these uniform expressions. */
	//TOptional<FUniformBufferStruct> UniformBufferStruct;

	friend class FMaterial;
	friend class FHLSLMaterialTranslator;
	friend class FMaterialShaderMap;
	friend class FMaterialShader;
	//friend class FMaterialRenderProxy;
	//friend class FDebugUniformExpressionSet;
};
/** Stores outputs from the material compile that need to be saved. */
class FMaterialCompilationOutput
{
public:
	FMaterialCompilationOutput() :
		NumUsedUVScalars(0),
		NumUsedCustomInterpolatorScalars(0),
		EstimatedNumTextureSamplesVS(0),
		EstimatedNumTextureSamplesPS(0),
		bRequiresSceneColorCopy(false),
		bNeedsSceneTextures(false),
		bUsesEyeAdaptation(false),
		bModifiesMeshPosition(false),
		bUsesWorldPositionOffset(false),
		bNeedsGBuffer(false),
		bUsesGlobalDistanceField(false),
		bUsesPixelDepthOffset(false),
		bUsesSceneDepthLookup(false)
	{}

	FUniformExpressionSet UniformExpressionSet;

	/** Number of used custom UV scalars. */
	uint8 NumUsedUVScalars;

	/** Number of used custom vertex interpolation scalars. */
	uint8 NumUsedCustomInterpolatorScalars;

	/** Number of times SampleTexture is called, excludes custom nodes. */
	uint16 EstimatedNumTextureSamplesVS;
	uint16 EstimatedNumTextureSamplesPS;

	/** Indicates whether the material uses scene color. */
	bool bRequiresSceneColorCopy;

	/** true if the material needs the scenetexture lookups. */
	bool bNeedsSceneTextures;

	/** true if the material uses the EyeAdaptationLookup */
	bool bUsesEyeAdaptation;

	/** true if the material modifies the the mesh position. */
	bool bModifiesMeshPosition;

	/** Whether the material uses world position offset. */
	bool bUsesWorldPositionOffset;

	/** true if the material uses any GBuffer textures */
	bool bNeedsGBuffer;

	/** true if material uses the global distance field */
	bool bUsesGlobalDistanceField;

	/** true if the material writes a pixel depth offset */
	bool bUsesPixelDepthOffset;

	/** true if the material uses the SceneDepth lookup */
	bool bUsesSceneDepthLookup;
};

class FMaterialShaderMap : public TShaderMap<FMaterialShaderType>
{
public:
	static FMaterialShaderMap* FindId(const FMaterialShaderMapId& ShaderMapId);

	FMaterialShaderMap();
	~FMaterialShaderMap();
	void Compile(
		FMaterial* Material,
		const FMaterialShaderMapId& ShaderMapId,
		std::shared_ptr<FShaderCompilerEnvironment> MaterialEnvironment,
		const FMaterialCompilationOutput& InMaterialCompilationOutput,
		bool bSynchronousCompile,
		bool bApplyCompletedShaderMapForRendering
	);
	bool ProcessCompilationResults(const std::vector<FShaderCompileJob*>& InCompilationResults);
	const FMaterialShaderMapId& GetShaderMapId() const { return ShaderMapId; }
	uint32 GetCompilingId() const { return CompilingId; }
	bool CompiledSuccessfully() const { return bCompiledSuccessfully; }

	const FMeshMaterialShaderMap* GetMeshShaderMap(FVertexFactoryType* VertexFactoryType) const;

	void Register();
private:

	static std::unordered_map<FMaterialShaderMapId, FMaterialShaderMap*> GIdToMaterialShaderMap;

	std::vector<FMeshMaterialShaderMap*> MeshShaderMaps;

	FMaterialShaderMapId ShaderMapId;

	static uint32 NextCompilingId;

	FMaterialCompilationOutput MaterialCompilationOutput;

	/** Uniquely identifies this shader map during compilation, needed for deferred compilation where shaders from multiple shader maps are compiled together. */
	uint32 CompilingId;

	uint32 bCompiledSuccessfully : 1;

	static std::vector<FMaterialShaderMap*> AllMaterialShaderMaps;

	std::vector<FMeshMaterialShaderMap*> OrderedMeshShaderMaps;

	FShader* ProcessCompilationResultsForSingleJob(class FShaderCompileJob* SingleJob,  const FSHAHash& MaterialShaderMapHash);


	void InitOrderedMeshShaderMaps();

	static std::map<FMaterialShaderMap*, std::vector<FMaterial*>> ShaderMapsBeingCompiled;
	friend class FShaderCompilingManager;
};

class FMaterial
{
public:
	bool CacheShaders(bool bApplyCompletedShaderMapForRendering);
	bool CacheShaders(const FMaterialShaderMapId& ShaderMapId, bool bApplyCompletedShaderMapForRendering);
	void GetDependentShaderAndVFTypes(std::vector<FShaderType*>& OutShaderTypes, std::vector<FVertexFactoryType*>& OutVFTypes) const;
	virtual bool ShouldCache(const FShaderType* ShaderType, const FVertexFactoryType* VertexFactoryType) const;
	virtual void GetShaderMapId(FMaterialShaderMapId& OutId) const;
	virtual uint32 GetMaterialId() const = 0;
	virtual std::string GetFriendlyName() const = 0;
	class FMaterialShaderMap* GetRenderingThreadShaderMap() const;

	FShader* GetShader(class FMeshMaterialShaderType* ShaderType, FVertexFactoryType* VertexFactoryType, bool bFatalIfMissing = true) const;

	template<typename ShaderType>
	ShaderType* GetShader(FVertexFactoryType* VertexFactoryType, bool bFatalIfMissing = true) const
	{
		return (ShaderType*)GetShader(&ShaderType::StaticType, VertexFactoryType, bFatalIfMissing);
	}

	virtual EMaterialDomain GetMaterialDomain() const = 0; // See EMaterialDomain.
	virtual bool IsTwoSided() const = 0;
	virtual bool IsDitheredLODTransition() const = 0;
	virtual bool IsTranslucencyWritingCustomDepth() const { return false; }
	virtual bool IsTangentSpaceNormal() const { return false; }
	virtual bool ShouldInjectEmissiveIntoLPV() const { return false; }
	virtual bool ShouldBlockGI() const { return false; }
	virtual bool ShouldGenerateSphericalParticleNormals() const { return false; }
// 	virtual	bool ShouldDisableDepthTest() const { return false; }
// 	virtual	bool ShouldWriteOnlyAlpha() const { return false; }
// 	virtual	bool ShouldEnableResponsiveAA() const { return false; }
	virtual bool ShouldDoSSR() const { return false; }
	virtual bool ShouldDoContactShadows() const { return false; }
// 	virtual bool IsLightFunction() const = 0;
// 	virtual bool IsUsedWithEditorCompositing() const { return false; }
// 	virtual bool IsDeferredDecal() const = 0;
// 	virtual bool IsVolumetricPrimitive() const = 0;
// 	virtual bool IsWireframe() const = 0;
// 	virtual bool IsUIMaterial() const { return false; }
	virtual bool IsSpecialEngineMaterial() const = 0;
// 	virtual bool IsUsedWithSkeletalMesh() const { return false; }
// 	virtual bool IsUsedWithLandscape() const { return false; }
// 	virtual bool IsUsedWithParticleSystem() const { return false; }
// 	virtual bool IsUsedWithParticleSprites() const { return false; }
// 	virtual bool IsUsedWithBeamTrails() const { return false; }
// 	virtual bool IsUsedWithMeshParticles() const { return false; }
// 	virtual bool IsUsedWithNiagaraSprites() const { return false; }
// 	virtual bool IsUsedWithNiagaraRibbons() const { return false; }
// 	virtual bool IsUsedWithNiagaraMeshParticles() const { return false; }
// 	virtual bool IsUsedWithStaticLighting() const { return false; }
// 	virtual	bool IsUsedWithMorphTargets() const { return false; }
// 	virtual bool IsUsedWithSplineMeshes() const { return false; }
	virtual bool IsUsedWithInstancedStaticMeshes() const { return false; }
// 	virtual bool IsUsedWithAPEXCloth() const { return false; }
// 	virtual bool IsUsedWithUI() const { return false; }
// 	virtual bool IsUsedWithGeometryCache() const { return false; }
	virtual enum EMaterialTessellationMode GetTessellationMode() const;
	virtual bool IsCrackFreeDisplacementEnabled() const { return false; }
	virtual bool IsAdaptiveTessellationEnabled() const { return false; }
// 	virtual bool IsFullyRough() const { return false; }
	virtual bool UseNormalCurvatureToRoughness() const { return false; }
	virtual bool IsUsingFullPrecision() const { return false; }
	virtual bool IsUsingHQForwardReflections() const { return false; }
	virtual bool IsUsingPlanarForwardReflections() const { return false; }
// 	virtual bool OutputsVelocityOnBasePass() const { return true; }
	virtual bool IsNonmetal() const { return false; }
	virtual bool UseLmDirectionality() const { return true; }
// 	virtual bool IsMasked() const = 0;
	virtual bool IsDitherMasked() const { return false; }
	virtual bool AllowNegativeEmissiveColor() const { return false; }
	virtual enum EBlendMode GetBlendMode() const = 0;
	virtual enum ERefractionMode GetRefractionMode() const;
	virtual enum EMaterialShadingModel GetShadingModel() const = 0;
	virtual enum ETranslucencyLightingMode GetTranslucencyLightingMode() const { return TLM_VolumetricNonDirectional; };
// 	virtual float GetOpacityMaskClipValue() const = 0;
	virtual bool GetCastDynamicShadowAsMasked() const = 0;
// 	virtual bool IsDistorted() const { return false; };
// 	virtual float GetTranslucencyDirectionalLightingIntensity() const { return 1.0f; }
// 	virtual float GetTranslucentShadowDensityScale() const { return 1.0f; }
// 	virtual float GetTranslucentSelfShadowDensityScale() const { return 1.0f; }
// 	virtual float GetTranslucentSelfShadowSecondDensityScale() const { return 1.0f; }
// 	virtual float GetTranslucentSelfShadowSecondOpacity() const { return 1.0f; }
// 	virtual float GetTranslucentBackscatteringExponent() const { return 1.0f; }
// 	virtual bool IsTranslucencyAfterDOFEnabled() const { return false; }
// 	virtual bool IsMobileSeparateTranslucencyEnabled() const { return false; }
// 	virtual FLinearColor GetTranslucentMultipleScatteringExtinction() const { return FLinearColor::White; }
// 	virtual float GetTranslucentShadowStartOffset() const { return 0.0f; }
// 	virtual float GetRefractionDepthBiasValue() const { return 0.0f; }
// 	virtual float GetMaxDisplacement() const { return 0.0f; }
// 	virtual bool ShouldApplyFogging() const { return false; }
// 	virtual bool ComputeFogPerPixel() const { return false; }
// 	virtual bool HasVertexPositionOffsetConnected() const { return false; }
	virtual bool HasPixelDepthOffsetConnected() const { return false; }
// 	virtual bool HasMaterialAttributesConnected() const { return false; }
// 	virtual uint32 GetDecalBlendMode() const { return 0; }
// 	virtual uint32 GetMaterialDecalResponse() const { return 0; }
// 	virtual bool HasNormalConnected() const { return false; }
// 	virtual bool RequiresSynchronousCompilation() const { return false; };
	virtual bool IsDefaultMaterial() const { return false; };
// 	virtual int32 GetNumCustomizedUVs() const { return 0; }
// 	virtual int32 GetBlendableLocation() const { return 0; }
	virtual bool GetBlendableOutputAlpha() const { return false; }


	bool WritesEveryPixel(bool bShadowPass = false) const
	{
// 		static TConsoleVariableData<int32>* CVarStencilDitheredLOD =
// 			IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.StencilForLODDither"));
// 
// 		return !IsMasked()
// 			// Render dithered material as masked if a stencil prepass is not used (UE-50064, UE-49537)
// 			&& !((bShadowPass || !CVarStencilDitheredLOD->GetValueOnAnyThread()) && IsDitheredLODTransition())
// 			&& !IsWireframe();
		return true;
	}
	inline bool ShouldCastDynamicShadows() const
	{
		return GetShadingModel() != MSM_Unlit &&
			(GetBlendMode() == BLEND_Opaque ||
				GetBlendMode() == BLEND_Masked ||
				(GetBlendMode() == BLEND_Translucent && GetCastDynamicShadowAsMasked()));
	}
	bool MaterialMayModifyMeshPosition() const;

	bool MaterialUsesPixelDepthOffset() const;
private:
	std::shared_ptr<FMaterialShaderMap> GameThreadShaderMap;

	bool BeginCompileShaderMap(
		const FMaterialShaderMapId& ShaderMapId,
		std::shared_ptr<class FMaterialShaderMap>& OutShaderMap,
		bool bApplyCompletedShaderMapForRendering);

	void SetupMaterialEnvironment(
		const FUniformExpressionSet& InUniformExpressionSet,
		FShaderCompilerEnvironment& OutEnvironment
	) const;

	friend class FMaterialShaderMap;
	friend class FShaderCompilingManager;
	friend class FHLSLMaterialTranslator;
};
class UMaterial;
/**
* @return True if BlendMode is translucent (should be part of the translucent rendering).
*/
inline bool IsTranslucentBlendMode(enum EBlendMode BlendMode)
{
	return BlendMode != BLEND_Opaque && BlendMode != BLEND_Masked;
}
class FMaterialResource : public FMaterial
{
public:
	void SetMaterial(UMaterial* InMaterial)
	{
		Material = InMaterial;
	}
	virtual uint32 GetMaterialId() const override;
	virtual std::string GetFriendlyName() const override;

	virtual EMaterialDomain GetMaterialDomain() const override;
	virtual bool IsTwoSided() const override;
	virtual bool IsDitheredLODTransition() const override;
	virtual bool IsTranslucencyWritingCustomDepth() const override;
	virtual bool IsTangentSpaceNormal() const override;
	virtual bool ShouldInjectEmissiveIntoLPV() const override;
// 	virtual bool ShouldBlockGI() const override;
	virtual bool ShouldGenerateSphericalParticleNormals() const override;
// 	virtual bool ShouldDisableDepthTest() const override;
// 	virtual bool ShouldWriteOnlyAlpha() const override;
// 	virtual bool ShouldEnableResponsiveAA() const override;
	virtual bool ShouldDoSSR() const override;
	virtual bool ShouldDoContactShadows() const override;
// 	virtual bool IsLightFunction() const override;
// 	virtual bool IsUsedWithEditorCompositing() const override;
// 	virtual bool IsDeferredDecal() const override;
// 	virtual bool IsVolumetricPrimitive() const override;
// 	virtual bool IsWireframe() const override;
// 	virtual bool IsUIMaterial() const override;
	virtual bool IsSpecialEngineMaterial() const override;
// 	virtual bool IsUsedWithSkeletalMesh() const override;
// 	virtual bool IsUsedWithLandscape() const override;
// 	virtual bool IsUsedWithParticleSystem() const override;
// 	virtual bool IsUsedWithParticleSprites() const override;
// 	virtual bool IsUsedWithBeamTrails() const override;
// 	virtual bool IsUsedWithMeshParticles() const override;
// 	virtual bool IsUsedWithNiagaraSprites() const override;
// 	virtual bool IsUsedWithNiagaraRibbons() const override;
// 	virtual bool IsUsedWithNiagaraMeshParticles() const override;
// 	virtual bool IsUsedWithStaticLighting() const override;
// 	virtual bool IsUsedWithMorphTargets() const override;
// 	virtual bool IsUsedWithSplineMeshes() const override;
	virtual bool IsUsedWithInstancedStaticMeshes() const override;
// 	virtual bool IsUsedWithAPEXCloth() const override;
// 	virtual bool IsUsedWithGeometryCache() const override;
	virtual enum EMaterialTessellationMode GetTessellationMode() const override;
	virtual bool IsCrackFreeDisplacementEnabled() const override;
	virtual bool IsAdaptiveTessellationEnabled() const override;
// 	virtual bool IsFullyRough() const override;
// 	virtual bool UseNormalCurvatureToRoughness() const override;
	virtual bool IsUsingFullPrecision() const override;
	virtual bool IsUsingHQForwardReflections() const override;
	virtual bool IsUsingPlanarForwardReflections() const override;
// 	virtual bool OutputsVelocityOnBasePass() const override;
	virtual bool IsNonmetal() const override;
	virtual bool UseLmDirectionality() const override;
	virtual enum EBlendMode GetBlendMode() const override;
	virtual enum ERefractionMode GetRefractionMode() const override;
// 	virtual uint32 GetDecalBlendMode() const override;
// 	virtual uint32 GetMaterialDecalResponse() const override;
// 	virtual bool HasNormalConnected() const override;
	virtual enum EMaterialShadingModel GetShadingModel() const override;
	virtual enum ETranslucencyLightingMode GetTranslucencyLightingMode() const override;
// 	virtual float GetOpacityMaskClipValue() const override;
	virtual bool GetCastDynamicShadowAsMasked() const override;
// 	virtual bool IsDistorted() const override;
// 	virtual float GetTranslucencyDirectionalLightingIntensity() const override;
// 	virtual float GetTranslucentShadowDensityScale() const override;
// 	virtual float GetTranslucentSelfShadowDensityScale() const override;
// 	virtual float GetTranslucentSelfShadowSecondDensityScale() const override;
// 	virtual float GetTranslucentSelfShadowSecondOpacity() const override;
// 	virtual float GetTranslucentBackscatteringExponent() const override;
// 	virtual bool IsTranslucencyAfterDOFEnabled() const override;
// 	virtual bool IsMobileSeparateTranslucencyEnabled() const override;
// 	virtual FLinearColor GetTranslucentMultipleScatteringExtinction() const override;
// 	virtual float GetTranslucentShadowStartOffset() const override;
// 	virtual bool IsMasked() const override;
	virtual bool IsDitherMasked() const override;
// 	virtual bool AllowNegativeEmissiveColor() const override;
// 	virtual bool RequiresSynchronousCompilation() const override;
	virtual bool IsDefaultMaterial() const override;
// 	virtual int32 GetNumCustomizedUVs() const override;
// 	virtual int32 GetBlendableLocation() const override;
// 	virtual bool GetBlendableOutputAlpha() const override;
// 	virtual float GetRefractionDepthBiasValue() const override;
// 	virtual float GetMaxDisplacement() const override;
// 	virtual bool ShouldApplyFogging() const override;
// 	virtual bool ComputeFogPerPixel() const override;



protected:
	UMaterial* Material;
};

/**
* Cached uniform expression values.
*/
struct FUniformExpressionCache
{
	/** Material uniform buffer. */
	std::shared_ptr<FUniformBuffer> UniformBuffer;
	/** Material uniform buffer. */
	//FLocalUniformBuffer LocalUniformBuffer;
	/** Ids of parameter collections needed for rendering. */
	//TArray<FGuid> ParameterCollections;
	/** True if the cache is up to date. */
	bool bUpToDate;

	/** Shader map that was used to cache uniform expressions on this material.  This is used for debugging and verifying correct behavior. */
	const FMaterialShaderMap* CachedUniformExpressionShaderMap;

	FUniformExpressionCache() :
		bUpToDate(false),
		CachedUniformExpressionShaderMap(NULL)
	{}

	/** Destructor. */
	~FUniformExpressionCache()
	{
	}
};

/**
* A material render proxy used by the renderer.
*/
class FMaterialRenderProxy 
{
public:

	/** Cached uniform expressions. */
	mutable FUniformExpressionCache UniformExpressionCache;

	/** Default constructor. */
	FMaterialRenderProxy();

	/** Initialization constructor. */
	FMaterialRenderProxy(bool bInSelected, bool bInHovered);

	/** Destructor. */
	virtual ~FMaterialRenderProxy();

	/**
	* Evaluates uniform expressions and stores them in OutUniformExpressionCache.
	* @param OutUniformExpressionCache - The uniform expression cache to build.
	* @param MaterialRenderContext - The context for which to cache expressions.
	*/
	//void EvaluateUniformExpressions(FUniformExpressionCache& OutUniformExpressionCache, const FMaterialRenderContext& Context, class FRHICommandList* CommandListIfLocalMode = nullptr) const;

	/**
	* Caches uniform expressions for efficient runtime evaluation.
	*/
	void CacheUniformExpressions();

	/**
	* Enqueues a rendering command to cache uniform expressions for efficient runtime evaluation.
	*/
	void CacheUniformExpressions_GameThread();

	/**
	* Invalidates the uniform expression cache.
	*/
	void InvalidateUniformExpressionCache();

	// These functions should only be called by the rendering thread.
	/** Returns the effective FMaterial, which can be a fallback if this material's shader map is invalid.  Always returns a valid material pointer. */
	const class FMaterial* GetMaterial() const
	{
		const FMaterialRenderProxy* Unused;
		const FMaterial* OutMaterial = nullptr;
		GetMaterialWithFallback(Unused, OutMaterial);
		return OutMaterial;
	}

	virtual void GetMaterialWithFallback(const FMaterialRenderProxy*& OutMaterialRenderProxy, const class FMaterial*& OutMaterial) const = 0;
	/** Returns the FMaterial, without using a fallback if the FMaterial doesn't have a valid shader map. Can return NULL. */
	virtual FMaterial* GetMaterialNoFallback() const { return NULL; }
	virtual UMaterial* GetMaterialInterface() const { return NULL; }
// 	virtual bool GetVectorValue(const FMaterialParameterInfo& ParameterInfo, FLinearColor* OutValue, const FMaterialRenderContext& Context) const = 0;
// 	virtual bool GetScalarValue(const FMaterialParameterInfo& ParameterInfo, float* OutValue, const FMaterialRenderContext& Context) const = 0;
// 	virtual bool GetTextureValue(const FMaterialParameterInfo& ParameterInfo, const UTexture** OutValue, const FMaterialRenderContext& Context) const = 0;
	bool IsSelected() const { return bSelected; }
	bool IsHovered() const { return bHovered; }
	bool IsDeleted() const
	{
		return DeletedFlag != 0;
	}

	// FRenderResource interface.
	void InitDynamicRHI();
	void ReleaseDynamicRHI();

	static const std::set<FMaterialRenderProxy*>& GetMaterialRenderProxyMap()
	{
		//check(!FPlatformProperties::RequiresCookedData());
		return MaterialRenderProxyMap;
	}

	//void SetSubsurfaceProfileRT(const USubsurfaceProfile* Ptr) { SubsurfaceProfileRT = Ptr; }
	//const USubsurfaceProfile* GetSubsurfaceProfileRT() const { return SubsurfaceProfileRT; }

	void SetReferencedInDrawList() const
	{
		bIsStaticDrawListReferenced = 1;
	}

	void SetUnreferencedInDrawList() const
	{
		bIsStaticDrawListReferenced = 0;
	}

	bool IsReferencedInDrawList() const
	{
		return bIsStaticDrawListReferenced != 0;
	}

	static void UpdateDeferredCachedUniformExpressions();

	static inline bool HasDeferredUniformExpressionCacheRequests()
	{
		return DeferredUniformExpressionCacheRequests.size() > 0;
	}

private:

	/** true if the material is selected. */
	bool bSelected : 1;
	/** true if the material is hovered. */
	bool bHovered : 1;
	/** 0 if not set, game thread pointer, do not dereference, only for comparison */
	//const USubsurfaceProfile* SubsurfaceProfileRT;

	/** For tracking down a bug accessing a deleted proxy. */
	mutable int32 DeletedFlag : 1;
	mutable int32 bIsStaticDrawListReferenced : 1;

	/**
	* Tracks all material render proxies in all scenes, can only be accessed on the rendering thread.
	* This is used to propagate new shader maps to materials being used for rendering.
	*/
	static std::set<FMaterialRenderProxy*> MaterialRenderProxyMap;

	static std::set<FMaterialRenderProxy*> DeferredUniformExpressionCacheRequests;
};


class UMaterial
{
public:
	UMaterial() { }

	static void InitDefaultMaterials();
	static UMaterial* GetDefaultMaterial(EMaterialDomain Domain);

	void PostLoad();

	void CacheResourceShadersForRendering(bool bRegenerateId);
	void CacheShadersForResources(const std::vector<FMaterialResource*>& ResourcesToCache, bool bApplyCompletedShaderMapForRendering);

	FMaterialResource* AllocateResource();
	FMaterialRenderProxy* GetRenderProxy(bool Selected, bool bHovered = false) const;
	FMaterialResource* GetMaterialResource();
	const FMaterialResource* GetMaterialResource() const;

	uint32 StateId;

	EMaterialDomain MaterialDomain = MD_Surface;
private:
	void UpdateResourceAllocations();
private:
	FMaterialResource* Resource;

	class FDefaultMaterialInstance* DefaultMaterialInstances[3];
};

/**
* A resource which represents the default instance of a UMaterial to the renderer.
* Note that default parameter values are stored in the FMaterialUniformExpressionXxxParameter objects now.
* This resource is only responsible for the selection color.
*/
class FDefaultMaterialInstance : public FMaterialRenderProxy
{
public:

	/**
	* Called from the game thread to destroy the material instance on the rendering thread.
	*/
	void GameThread_Destroy()
	{

	}

	// FMaterialRenderProxy interface.
	virtual void GetMaterialWithFallback(const FMaterialRenderProxy*& OutMaterialRenderProxy, const class FMaterial*& OutMaterial) const
	{
		const FMaterialResource* MaterialResource = Material->GetMaterialResource();
		if (MaterialResource && MaterialResource->GetRenderingThreadShaderMap())
		{
			// Verify that compilation has been finalized, the rendering thread shouldn't be touching it otherwise
			//checkSlow(MaterialResource->GetRenderingThreadShaderMap()->IsCompilationFinalized());
			// The shader map reference should have been NULL'ed if it did not compile successfully
			//checkSlow(MaterialResource->GetRenderingThreadShaderMap()->CompiledSuccessfully());
			OutMaterialRenderProxy = this;
			OutMaterial = MaterialResource;
			return;
		}

		// If we are the default material, must not try to fall back to the default material in an error state as that will be infinite recursion
		//check(!Material->IsDefaultMaterial());

		GetFallbackRenderProxy().GetMaterialWithFallback(OutMaterialRenderProxy, OutMaterial);
	}

	virtual FMaterial* GetMaterialNoFallback() const
	{
		return Material->GetMaterialResource();
	}

	virtual UMaterial* GetMaterialInterface() const override
	{
		return Material;
	}

// 	virtual bool GetVectorValue(const FMaterialParameterInfo& ParameterInfo, FLinearColor* OutValue, const FMaterialRenderContext& Context) const
// 	{
// 		
// 	}
// 	virtual bool GetScalarValue(const FMaterialParameterInfo& ParameterInfo, float* OutValue, const FMaterialRenderContext& Context) const
// 	{
// 		
// 	}
// 	virtual bool GetTextureValue(const FMaterialParameterInfo& ParameterInfo, const UTexture** OutValue, const FMaterialRenderContext& Context) const
// 	{
// 		
// 	}

	// FRenderResource interface.
	virtual std::string GetFriendlyName() const { return "Material"/* Material->GetName()*/; }

	// Constructor.
	FDefaultMaterialInstance(UMaterial* InMaterial, bool bInSelected, bool bInHovered) :
		FMaterialRenderProxy(bInSelected, bInHovered),
		Material(InMaterial)
	{}

private:
	/** Get the fallback material. */
	FMaterialRenderProxy & GetFallbackRenderProxy() const
	{
		return *(UMaterial::GetDefaultMaterial(Material->MaterialDomain)->GetRenderProxy(IsSelected(), IsHovered()));
	}

	UMaterial* Material;
};
