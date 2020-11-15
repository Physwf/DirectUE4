#include "Material.h"
#include "Shader.h"
#include "VertexFactory.h"
#include "ShaderCompiler.h"

#include <set>

bool FMaterialShaderMapId::operator==(const FMaterialShaderMapId& ReferenceSet) const
{
	return BaseMaterialId == ReferenceSet.BaseMaterialId;
}


static inline bool ShouldCacheMeshShader(const FMeshMaterialShaderType* ShaderType,  const FMaterial* Material, FVertexFactoryType* InVertexFactoryType)
{
	return ShaderType->ShouldCache(Material, InVertexFactoryType) && Material->ShouldCache(ShaderType, InVertexFactoryType) && InVertexFactoryType->ShouldCache(Material, ShaderType);
}

uint32 FMeshMaterialShaderMap::BeginCompile(
	uint32 ShaderMapId, 
	const FMaterialShaderMapId& InShaderMapId, 
	const FMaterial* Material, 
	FShaderCompilerEnvironment* MaterialEnvironment, 
	std::vector<FShaderCompileJob*>& NewJobs
	)
{
	if (!VertexFactoryType)
	{
		return 0;
	}

	uint32 NumShadersPerVF = 0;
	std::set<std::string> ShaderTypeNames;

	// Iterate over all mesh material shader types.
	std::map<FShaderType*, FShaderCompileJob*> SharedShaderJobs;
	for (auto ShaderTypeIt = FShaderType::GetTypeList().begin(); ShaderTypeIt != FShaderType::GetTypeList().end(); ++ShaderTypeIt)
	{
		FMeshMaterialShaderType* ShaderType = (*ShaderTypeIt)->GetMeshMaterialShaderType();
		if (ShaderType && ShouldCacheMeshShader(ShaderType, Material, VertexFactoryType))
		{
			// Verify that the shader map Id contains inputs for any shaders that will be put into this shader map
			//assert(InShaderMapId.ContainsVertexFactoryType(VertexFactoryType));
			//assert(InShaderMapId.ContainsShaderType(ShaderType));

			NumShadersPerVF++;
			// only compile the shader if we don't already have it
			if (!HasShader(ShaderType, /* PermutationId = */ 0))
			{
				// Compile this mesh material shader for this material and vertex factory type.
				auto* Job = ShaderType->BeginCompileShader(
					ShaderMapId,
					Material,
					MaterialEnvironment,
					VertexFactoryType,
					nullptr,
					NewJobs
				);
				//assert(!SharedShaderJobs.Find(ShaderType));
				SharedShaderJobs.insert(std::make_pair(ShaderType, Job));
			}
		}
	}

	return NumShadersPerVF;
}

static inline bool ShouldCacheMaterialShader(const FMaterialShaderType* ShaderType, const FMaterial* Material)
{
	return ShaderType->ShouldCache(Material) && Material->ShouldCache(ShaderType, nullptr);
}

FMaterialShaderMap* FMaterialShaderMap::FindId(const FMaterialShaderMapId& ShaderMapId)
{
	auto it = GIdToMaterialShaderMap.find(ShaderMapId);
	return it != GIdToMaterialShaderMap.end() ? it->second : NULL;
}

FMaterialShaderMap::FMaterialShaderMap() :
	CompilingId(1),
	//bDeletedThroughDeferredCleanup(false),
	//bRegistered(false),
	//bCompilationFinalized(true),
	bCompiledSuccessfully(true)//,
	//bIsPersistent(true)
{
	AllMaterialShaderMaps.push_back(this);
}

FMaterialShaderMap::~FMaterialShaderMap()
{
	AllMaterialShaderMaps.erase(std::find(AllMaterialShaderMaps.begin(),AllMaterialShaderMaps.end(),this));
}

void FMaterialShaderMap::Compile(
	FMaterial* Material, 
	const FMaterialShaderMapId& InShaderMapId, 
	std::shared_ptr<FShaderCompilerEnvironment> MaterialEnvironment, 
	const FMaterialCompilationOutput& InMaterialCompilationOutput, 
	bool bSynchronousCompile, 
	bool bApplyCompletedShaderMapForRendering
	)
{

	CompilingId = NextCompilingId;
	//check(NextCompilingId < UINT_MAX);
	NextCompilingId++;

	uint32 NumShaders = 0;
	uint32 NumVertexFactories = 0;
	std::vector<FShaderCompileJob*> NewJobs;

	ShaderMapId = InShaderMapId;

	for (auto VertexFactoryTypeIt = FVertexFactoryType::GetTypeList().begin(); VertexFactoryTypeIt != FVertexFactoryType::GetTypeList().end(); ++VertexFactoryTypeIt)
	{
		FVertexFactoryType* VertexFactoryType = *VertexFactoryTypeIt;
		assert(VertexFactoryType);

		if (VertexFactoryType->IsUsedWithMaterials())
		{
			FMeshMaterialShaderMap* MeshShaderMap = nullptr;

			// look for existing map for this vertex factory type
			int32 MeshShaderMapIndex = -1;
			for (uint32 ShaderMapIndex = 0; ShaderMapIndex < MeshShaderMaps.size(); ShaderMapIndex++)
			{
				if (MeshShaderMaps[ShaderMapIndex]->GetVertexFactoryType() == VertexFactoryType)
				{
					MeshShaderMap = MeshShaderMaps[ShaderMapIndex];
					MeshShaderMapIndex = ShaderMapIndex;
					break;
				}
			}

			if (MeshShaderMap == nullptr)
			{
				// Create a new mesh material shader map.
				MeshShaderMapIndex = (int32)MeshShaderMaps.size();
				MeshShaderMaps.push_back(new FMeshMaterialShaderMap(VertexFactoryType));
				MeshShaderMap = MeshShaderMaps.back();
			}

			// Enqueue compilation all mesh material shaders for this material and vertex factory type combo.
			const uint32 MeshShaders = MeshShaderMap->BeginCompile(
				CompilingId,
				InShaderMapId,
				Material,
				MaterialEnvironment.get(),
				NewJobs
			);
			NumShaders += MeshShaders;
			if (MeshShaders > 0)
			{
				NumVertexFactories++;
			}
		}
	}

	// Iterate over all material shader types.
	std::map<FShaderType*, FShaderCompileJob*> SharedShaderJobs;
	for (auto ShaderTypeIt = FShaderType::GetTypeList().begin(); ShaderTypeIt != FShaderType::GetTypeList().end(); ++ShaderTypeIt)
	{
		FMaterialShaderType* ShaderType = (*ShaderTypeIt)->GetMaterialShaderType();
		if (ShaderType &&  ShouldCacheMaterialShader(ShaderType, Material))
		{
			// Verify that the shader map Id contains inputs for any shaders that will be put into this shader map
			//check(InShaderMapId.ContainsShaderType(ShaderType));

			// Compile this material shader for this material.
			//TArray<FString> ShaderErrors;

			// Only compile the shader if we don't already have it
			if (!HasShader(ShaderType, /* PermutationId = */ 0))
			{
				auto* Job = ShaderType->BeginCompileShader(
					CompilingId,
					Material,
					MaterialEnvironment.get(),
					nullptr,
					NewJobs
				);
				//check(!SharedShaderJobs.Find(ShaderType));
				SharedShaderJobs.insert(std::make_pair(ShaderType, Job));
			}
			NumShaders++;
		}
	}

	Register();

	bCompiledSuccessfully = false;

	GShaderCompilingManager->AddJobs(NewJobs/*, bApplyCompletedShaderMapForRendering && !bSynchronousCompile, bSynchronousCompile || !Material->IsPersistent(), bRecreateComponentRenderStateOnCompletion*/);
}

	void FMaterialShaderMap::Register()
	{

	}

	std::unordered_map<FMaterialShaderMapId, FMaterialShaderMap*> FMaterialShaderMap::GIdToMaterialShaderMap;

	uint32 FMaterialShaderMap::NextCompilingId = 2;

	std::vector<FMaterialShaderMap*> FMaterialShaderMap::AllMaterialShaderMaps;

bool FMaterial::CacheShaders(bool bApplyCompletedShaderMapForRendering)
{
	FMaterialShaderMapId NoStaticParametersId;
	GetShaderMapId(NoStaticParametersId);
	return CacheShaders(NoStaticParametersId, bApplyCompletedShaderMapForRendering);
}

bool FMaterial::CacheShaders(const FMaterialShaderMapId& ShaderMapId, bool bApplyCompletedShaderMapForRendering)
{
	bool bSucceeded = false;

	bSucceeded = BeginCompileShaderMap(ShaderMapId, GameThreadShaderMap, bApplyCompletedShaderMapForRendering);

	return bSucceeded;
}

void FMaterial::GetDependentShaderAndVFTypes(std::vector<FShaderType*>& OutShaderTypes, std::vector<FVertexFactoryType*>& OutVFTypes) const
{
	}

bool FMaterial::ShouldCache(const FShaderType* ShaderType, const FVertexFactoryType* VertexFactoryType) const
{
	return true;
}

void FMaterial::GetShaderMapId(FMaterialShaderMapId& OutId) const
{
// 	TArray<FShaderType*> ShaderTypes;
// 	TArray<FVertexFactoryType*> VFTypes;
// 	TArray<const FShaderPipelineType*> ShaderPipelineTypes;
// 
// 	GetDependentShaderAndVFTypes(Platform, ShaderTypes, ShaderPipelineTypes, VFTypes);

	//OutId.Usage = GetShaderMapUsage();
	OutId.BaseMaterialId = GetMaterialId();
	//OutId.SetShaderDependencies(ShaderTypes, ShaderPipelineTypes, VFTypes);
	//GetReferencedTexturesHash(Platform, OutId.TextureReferencesHash);
}

bool FMaterial::BeginCompileShaderMap(const FMaterialShaderMapId& ShaderMapId, std::shared_ptr<class FMaterialShaderMap>& OutShaderMap, bool bApplyCompletedShaderMapForRendering)
{
	bool bSuccess = false;

	std::shared_ptr<FMaterialShaderMap> NewShaderMap = std::make_shared<FMaterialShaderMap>();

	FMaterialCompilationOutput NewCompilationOutput;
	//FHLSLMaterialTranslator MaterialTranslator(this, NewCompilationOutput, ShaderMapId.GetParameterSet(), Platform, GetQualityLevel(), ShaderMapId.FeatureLevel);
	//bSuccess = MaterialTranslator.Translate();

	//if (bSuccess)
	{
		// Create a shader compiler environment for the material that will be shared by all jobs from this material
		std::shared_ptr<FShaderCompilerEnvironment> MaterialEnvironment = std::make_shared<FShaderCompilerEnvironment>();

		//MaterialTranslator.GetMaterialEnvironment(Platform, *MaterialEnvironment);
		const std::string MaterialShaderCode;// = //MaterialTranslator.GetMaterialShaderCode();
		const bool bSynchronousCompile = true;// RequiresSynchronousCompilation() || !GShaderCompilingManager->AllowAsynchronousShaderCompiling();

		MaterialEnvironment->IncludeVirtualPathToContentsMap.insert(std::make_pair(std::string("/Engine/Generated/Material.ush"), MaterialShaderCode));

		// Compile the shaders for the material.
		NewShaderMap->Compile(this, ShaderMapId, MaterialEnvironment, NewCompilationOutput, bSynchronousCompile, bApplyCompletedShaderMapForRendering);

		if (bSynchronousCompile)
		{
			// If this is a synchronous compile, assign the compile result to the output
			OutShaderMap = NewShaderMap->CompiledSuccessfully() ? NewShaderMap : nullptr;
		}
	}

	return bSuccess;
}

uint32 FMaterialResource::GetMaterialId() const
{
	return Material->StateId;
}

std::string FMaterialResource::GetFriendlyName() const
{
	return "Material";
}

void UMaterial::PostLoad()
{
	CacheResourceShadersForRendering(false);

	static uint32 GUIDGenerator = 0;
	StateId = GUIDGenerator++;
}

void UMaterial::CacheResourceShadersForRendering(bool bRegenerateId)
{
	UpdateResourceAllocations();
	std::vector<FMaterialResource*> ResourcesToCache;
	ResourcesToCache.push_back(Resource);
	CacheShadersForResources(ResourcesToCache,true);
}

void UMaterial::CacheShadersForResources(const std::vector<FMaterialResource*>& ResourcesToCache, bool bApplyCompletedShaderMapForRendering)
{
	Resource->CacheShaders(bApplyCompletedShaderMapForRendering);
}

void UMaterial::UpdateResourceAllocations()
{
	Resource = new FMaterialResource();
	Resource->SetMaterial(this);
}

