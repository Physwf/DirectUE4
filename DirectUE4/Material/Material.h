#pragma once

#include "ShaderCore.h"
#include "Shader.h"

/** Contains all the information needed to uniquely identify a FMaterialShaderMap. */
class FMaterialShaderMapId
{
public:
	uint32 BaseMaterialId;

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

	const FMaterialShaderMapId& GetShaderMapId() const { return ShaderMapId; }
	uint32 GetCompilingId() const { return CompilingId; }
	bool CompiledSuccessfully() const { return bCompiledSuccessfully; }

	void Register();
private:

	static std::unordered_map<FMaterialShaderMapId, FMaterialShaderMap*> GIdToMaterialShaderMap;

	std::vector<FMeshMaterialShaderMap*> MeshShaderMaps;

	FMaterialShaderMapId ShaderMapId;

	static uint32 NextCompilingId;

	/** Uniquely identifies this shader map during compilation, needed for deferred compilation where shaders from multiple shader maps are compiled together. */
	uint32 CompilingId;

	uint32 bCompiledSuccessfully : 1;

	static std::vector<FMaterialShaderMap*> AllMaterialShaderMaps;
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
private:
	std::shared_ptr<FMaterialShaderMap> GameThreadShaderMap;

	bool BeginCompileShaderMap(
		const FMaterialShaderMapId& ShaderMapId,
		std::shared_ptr<class FMaterialShaderMap>& OutShaderMap,
		bool bApplyCompletedShaderMapForRendering);
};
class UMaterial;
class FMaterialResource : public FMaterial
{
public:
	void SetMaterial(UMaterial* InMaterial)
	{
		Material = InMaterial;
	}
	virtual uint32 GetMaterialId() const override;
	virtual std::string GetFriendlyName() const override;
protected:
	UMaterial* Material;
};

class UMaterial
{
public:
	UMaterial() { }

	void PostLoad();

	void CacheResourceShadersForRendering(bool bRegenerateId);
	void CacheShadersForResources(const std::vector<FMaterialResource*>& ResourcesToCache, bool bApplyCompletedShaderMapForRendering);

	uint32 StateId;
private:
	void UpdateResourceAllocations();
private:
	FMaterialResource* Resource;
};