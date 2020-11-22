#include "Material.h"
#include "Shader.h"
#include "VertexFactory.h"
#include "ShaderCompiler.h"
#include "HLSLMaterialTranslator.h"

#include <set>

bool FMaterialShaderMapId::operator==(const FMaterialShaderMapId& ReferenceSet) const
{
	return BaseMaterialId == ReferenceSet.BaseMaterialId;
}

void FMaterialShaderMapId::GetMaterialHash(FSHAHash& OutHash) const
{
	FSHA1 HashState;

	HashState.Update((const uint8*)&BaseMaterialId, sizeof(BaseMaterialId));

	HashState.GetHash(&OutHash.Hash[0]);
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

static bool ShouldCacheMaterialShader(const FMaterialShaderType* ShaderType, const FMaterial* Material)
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
	if(ShaderMapsBeingCompiled.find(this) != ShaderMapsBeingCompiled.end())
	{
		ShaderMapsBeingCompiled[this].push_back(Material);
	}
	else
	{
		std::vector<FMaterial*> NewCorrespondingMaterials;
		NewCorrespondingMaterials.push_back(Material);
		ShaderMapsBeingCompiled.insert(std::make_pair(this, NewCorrespondingMaterials));

		CompilingId = NextCompilingId;
		//check(NextCompilingId < UINT_MAX);
		NextCompilingId++;

		uint32 NumShaders = 0;
		uint32 NumVertexFactories = 0;
		std::vector<FShaderCompileJob*> NewJobs;

		Material->SetupMaterialEnvironment(InMaterialCompilationOutput.UniformExpressionSet, *MaterialEnvironment);

		MaterialCompilationOutput = InMaterialCompilationOutput;
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

		if (bSynchronousCompile)
		{
			std::vector<int32> CurrentShaderMapId;
			CurrentShaderMapId.push_back(CompilingId);
			GShaderCompilingManager->FinishCompilation(
				nullptr,
				CurrentShaderMapId);
		}
	}
}

bool FMaterialShaderMap::ProcessCompilationResults(const std::vector<FShaderCompileJob*>& InCompilationResults)
{
	FSHAHash MaterialShaderMapHash;
	ShaderMapId.GetMaterialHash(MaterialShaderMapHash);

	for (FShaderCompileJob* SingleJob : InCompilationResults)
	{
		ProcessCompilationResultsForSingleJob(SingleJob,  MaterialShaderMapHash);
	}
	InitOrderedMeshShaderMaps();
	return true;
}

const FMeshMaterialShaderMap* FMaterialShaderMap::GetMeshShaderMap(FVertexFactoryType* VertexFactoryType) const
{
	const FMeshMaterialShaderMap* MeshShaderMap = OrderedMeshShaderMaps[VertexFactoryType->GetId()];
	//checkSlow(!MeshShaderMap || MeshShaderMap->GetVertexFactoryType() == VertexFactoryType);
	return MeshShaderMap;
}

void FMaterialShaderMap::Register()
{

}

std::unordered_map<FMaterialShaderMapId, FMaterialShaderMap*> FMaterialShaderMap::GIdToMaterialShaderMap;

uint32 FMaterialShaderMap::NextCompilingId = 2;

std::vector<FMaterialShaderMap*> FMaterialShaderMap::AllMaterialShaderMaps;

FShader* FMaterialShaderMap::ProcessCompilationResultsForSingleJob(class FShaderCompileJob* SingleJob, const FSHAHash& MaterialShaderMapHash)
{
	assert(SingleJob);
	const FShaderCompileJob& CurrentJob = *SingleJob;
	assert(CurrentJob.Id == CompilingId);

	FShader* Shader = nullptr;
	if (CurrentJob.VFType)
	{
		FVertexFactoryType* VertexFactoryType = CurrentJob.VFType;
		assert(VertexFactoryType->IsUsedWithMaterials());
		FMeshMaterialShaderMap* MeshShaderMap = nullptr;
		int32 MeshShaderMapIndex = -1;

		// look for existing map for this vertex factory type
		for (uint32 ShaderMapIndex = 0; ShaderMapIndex < MeshShaderMaps.size(); ShaderMapIndex++)
		{
			if (MeshShaderMaps[ShaderMapIndex]->GetVertexFactoryType() == VertexFactoryType)
			{
				MeshShaderMap = MeshShaderMaps[ShaderMapIndex];
				MeshShaderMapIndex = ShaderMapIndex;
				break;
			}
		}

		assert(MeshShaderMap);
		FMeshMaterialShaderType* MeshMaterialShaderType = CurrentJob.ShaderType->GetMeshMaterialShaderType();
		assert(MeshMaterialShaderType);
		Shader = MeshMaterialShaderType->FinishCompileShader(FUniformExpressionSet() /*MaterialCompilationOutput.UniformExpressionSet*/, MaterialShaderMapHash, CurrentJob,  std::string());
		assert(Shader);
		//if (!ShaderPipeline)
		{
			assert(!MeshShaderMap->HasShader(MeshMaterialShaderType, /* PermutationId = */ 0));
			MeshShaderMap->AddShader(MeshMaterialShaderType, /* PermutationId = */ 0, Shader);
		}
	}
	else
	{
		FMaterialShaderType* MaterialShaderType = CurrentJob.ShaderType->GetMaterialShaderType();
		assert(MaterialShaderType);
		Shader = MaterialShaderType->FinishCompileShader(FUniformExpressionSet()/*MaterialCompilationOutput.UniformExpressionSet*/, MaterialShaderMapHash, CurrentJob, std::string());
		assert(Shader);
		//if (!ShaderPipeline)
		{
			assert(!HasShader(MaterialShaderType, /* PermutationId = */ 0));
			AddShader(MaterialShaderType, /* PermutationId = */ 0, Shader);
		}
	}

	return Shader;
}

void FMaterialShaderMap::InitOrderedMeshShaderMaps()
{
	OrderedMeshShaderMaps.clear();
	OrderedMeshShaderMaps.resize(FVertexFactoryType::GetNumVertexFactoryTypes());

	for (uint32 Index = 0; Index < MeshShaderMaps.size(); Index++)
	{
		assert(MeshShaderMaps[Index]->GetVertexFactoryType());
		const int32 VFIndex = MeshShaderMaps[Index]->GetVertexFactoryType()->GetId();
		OrderedMeshShaderMaps[VFIndex] = MeshShaderMaps[Index];
	}
}

std::map<FMaterialShaderMap*, std::vector<FMaterial*>> FMaterialShaderMap::ShaderMapsBeingCompiled;

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

class FMaterialShaderMap* FMaterial::GetRenderingThreadShaderMap() const
{
	return GameThreadShaderMap.get();
}

FShader* FMaterial::GetShader(class FMeshMaterialShaderType* ShaderType, FVertexFactoryType* VertexFactoryType, bool bFatalIfMissing /*= true*/) const
{
	const FMeshMaterialShaderMap* MeshShaderMap = GameThreadShaderMap->GetMeshShaderMap(VertexFactoryType);
	FShader* Shader = MeshShaderMap ? MeshShaderMap->GetShader(ShaderType) : nullptr;
	if (!Shader)
	{
		assert(false);
	}
	return Shader;
}

enum EMaterialTessellationMode FMaterial::GetTessellationMode() const
{
	return MTM_NoTessellation;
}

enum ERefractionMode FMaterial::GetRefractionMode() const
{
	return RM_IndexOfRefraction;
}

bool FMaterial::MaterialMayModifyMeshPosition() const
{
// 	return HasVertexPositionOffsetConnected() || HasPixelDepthOffsetConnected() || HasMaterialAttributesConnected() || GetTessellationMode() != MTM_NoTessellation
// 		|| (GetMaterialDomain() == MD_DeferredDecal && GetDecalBlendMode() == DBM_Volumetric_DistanceFunction);
	return false;
}

bool FMaterial::MaterialUsesPixelDepthOffset() const
{
	//return RenderingThreadShaderMap ? RenderingThreadShaderMap->UsesPixelDepthOffset() : false;
	return false;
}

bool FMaterial::BeginCompileShaderMap(const FMaterialShaderMapId& ShaderMapId, std::shared_ptr<class FMaterialShaderMap>& OutShaderMap, bool bApplyCompletedShaderMapForRendering)
{
	bool bSuccess = false;

	std::shared_ptr<FMaterialShaderMap> NewShaderMap = std::make_shared<FMaterialShaderMap>();

	FMaterialCompilationOutput NewCompilationOutput;
	FHLSLMaterialTranslator MaterialTranslator(this, NewCompilationOutput/*, ShaderMapId.GetParameterSet()*/);
	bSuccess = MaterialTranslator.Translate();

	//if (bSuccess)
	{
		// Create a shader compiler environment for the material that will be shared by all jobs from this material
		std::shared_ptr<FShaderCompilerEnvironment> MaterialEnvironment = std::make_shared<FShaderCompilerEnvironment>();

		MaterialTranslator.GetMaterialEnvironment(*MaterialEnvironment);
		const std::string MaterialShaderCode = MaterialTranslator.GetMaterialShaderCode();
		const bool bSynchronousCompile = true;// RequiresSynchronousCompilation() || !GShaderCompilingManager->AllowAsynchronousShaderCompiling();

		MaterialEnvironment->IncludeVirtualPathToContentsMap.insert(std::make_pair(std::string("/Generated/Material.hlsl"), MaterialShaderCode));

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

void FMaterial::SetupMaterialEnvironment(const FUniformExpressionSet& InUniformExpressionSet, FShaderCompilerEnvironment& OutEnvironment) const
{
	// Add the material uniform buffer definition.
// 	FShaderUniformBufferParameter::ModifyCompilationEnvironment(TEXT("Material"), InUniformExpressionSet.GetUniformBufferStruct(), Platform, OutEnvironment);
// 
// 	// Mark as using external texture if uniformexpression contains external texture
// 	if (InUniformExpressionSet.UniformExternalTextureExpressions.Num() > 0)
// 	{
// 		OutEnvironment.CompilerFlags.Add(CFLAG_UsesExternalTexture);
// 	}

	if (/*(RHISupportsTessellation(Platform) == false) || */(GetTessellationMode() == MTM_NoTessellation))
	{
		OutEnvironment.SetDefine(("USING_TESSELLATION"), ("0"));
	}
	else
	{
		OutEnvironment.SetDefine(("USING_TESSELLATION"), ("1"));
		if (GetTessellationMode() == MTM_FlatTessellation)
		{
			OutEnvironment.SetDefine(("TESSELLATION_TYPE_FLAT"), ("1"));
		}
		else if (GetTessellationMode() == MTM_PNTriangles)
		{
			OutEnvironment.SetDefine(("TESSELLATION_TYPE_PNTRIANGLES"), ("1"));
		}

		// This is dominant vertex/edge information.  Note, mesh must have preprocessed neighbors IB of material will fallback to default.
		//  PN triangles needs preprocessed buffers regardless of c
		if (IsCrackFreeDisplacementEnabled())
		{
			OutEnvironment.SetDefine(("DISPLACEMENT_ANTICRACK"), ("1"));
		}
		else
		{
			OutEnvironment.SetDefine(("DISPLACEMENT_ANTICRACK"), ("0"));
		}

		// Set whether to enable the adaptive tessellation, which tries to maintain a uniform number of pixels per triangle.
		if (IsAdaptiveTessellationEnabled())
		{
			OutEnvironment.SetDefine(("USE_ADAPTIVE_TESSELLATION_FACTOR"), ("1"));
		}
		else
		{
			OutEnvironment.SetDefine(("USE_ADAPTIVE_TESSELLATION_FACTOR"), ("0"));
		}

	}

	switch (GetBlendMode())
	{
	case BLEND_Opaque:
	case BLEND_Masked:
	{
		// Only set MATERIALBLENDING_MASKED if the material is truly masked
		//@todo - this may cause mismatches with what the shader compiles and what the renderer thinks the shader needs
		// For example IsTranslucentBlendMode doesn't check IsMasked
		if (!WritesEveryPixel())
		{
			OutEnvironment.SetDefine(("MATERIALBLENDING_MASKED"), ("1"));
		}
		else
		{
			OutEnvironment.SetDefine(("MATERIALBLENDING_SOLID"), ("1"));
		}
		break;
	}
	case BLEND_AlphaComposite:
	{
		// Fall through the case, blend mode will reuse MATERIALBLENDING_TRANSLUCENT
		OutEnvironment.SetDefine(("MATERIALBLENDING_ALPHACOMPOSITE"), ("1"));
	}
	case BLEND_Translucent: OutEnvironment.SetDefine(("MATERIALBLENDING_TRANSLUCENT"), ("1")); break;
	case BLEND_Additive: OutEnvironment.SetDefine(("MATERIALBLENDING_ADDITIVE"), ("1")); break;
	case BLEND_Modulate: OutEnvironment.SetDefine(("MATERIALBLENDING_MODULATE"), ("1")); break;
	default:
		//UE_LOG(LogMaterial, Warning, TEXT("Unknown material blend mode: %u  Setting to BLEND_Opaque"), (int32)GetBlendMode());
		OutEnvironment.SetDefine(("MATERIALBLENDING_SOLID"), ("1"));
	}

// 	{
// 		EMaterialDecalResponse MaterialDecalResponse = (EMaterialDecalResponse)GetMaterialDecalResponse();
// 
// 		// bit 0:color/1:normal/2:roughness to enable/disable parts of the DBuffer decal effect
// 		int32 MaterialDecalResponseMask = 0;
// 
// 		switch (MaterialDecalResponse)
// 		{
// 		case MDR_None:					MaterialDecalResponseMask = 0; break;
// 		case MDR_ColorNormalRoughness:	MaterialDecalResponseMask = 1 + 2 + 4; break;
// 		case MDR_Color:					MaterialDecalResponseMask = 1; break;
// 		case MDR_ColorNormal:			MaterialDecalResponseMask = 1 + 2; break;
// 		case MDR_ColorRoughness:		MaterialDecalResponseMask = 1 + 4; break;
// 		case MDR_Normal:				MaterialDecalResponseMask = 2; break;
// 		case MDR_NormalRoughness:		MaterialDecalResponseMask = 2 + 4; break;
// 		case MDR_Roughness:				MaterialDecalResponseMask = 4; break;
// 		default:
// 			check(0);
// 		}
// 
// 		OutEnvironment.SetDefine(TEXT("MATERIALDECALRESPONSEMASK"), MaterialDecalResponseMask);
// 	}

	switch (GetRefractionMode())
	{
	case RM_IndexOfRefraction: OutEnvironment.SetDefine(("REFRACTION_USE_INDEX_OF_REFRACTION"), ("1")); break;
	case RM_PixelNormalOffset: OutEnvironment.SetDefine(("REFRACTION_USE_PIXEL_NORMAL_OFFSET"), ("1")); break;
	default:
		//UE_LOG(LogMaterial, Warning, ("Unknown material refraction mode: %u  Setting to RM_IndexOfRefraction"), (int32)GetRefractionMode());
		OutEnvironment.SetDefine(("REFRACTION_USE_INDEX_OF_REFRACTION"), ("1"));
	}

	OutEnvironment.SetDefine(("USE_DITHERED_LOD_TRANSITION_FROM_MATERIAL"), IsDitheredLODTransition());
	OutEnvironment.SetDefine(("MATERIAL_TWOSIDED"), IsTwoSided());
	OutEnvironment.SetDefine(("MATERIAL_TANGENTSPACENORMAL"), IsTangentSpaceNormal());
	OutEnvironment.SetDefine(("GENERATE_SPHERICAL_PARTICLE_NORMALS"), ShouldGenerateSphericalParticleNormals());
	//OutEnvironment.SetDefine(("MATERIAL_USES_SCENE_COLOR_COPY"), RequiresSceneColorCopy_GameThread());
	OutEnvironment.SetDefine(("MATERIAL_HQ_FORWARD_REFLECTIONS"), IsUsingHQForwardReflections());
	OutEnvironment.SetDefine(("MATERIAL_PLANAR_FORWARD_REFLECTIONS"), IsUsingPlanarForwardReflections());
	OutEnvironment.SetDefine(("MATERIAL_NONMETAL"), IsNonmetal());
	OutEnvironment.SetDefine(("MATERIAL_USE_LM_DIRECTIONALITY"), UseLmDirectionality());
	OutEnvironment.SetDefine(("MATERIAL_INJECT_EMISSIVE_INTO_LPV"), ShouldInjectEmissiveIntoLPV());
	OutEnvironment.SetDefine(("MATERIAL_SSR"), ShouldDoSSR() && IsTranslucentBlendMode(GetBlendMode()));
	OutEnvironment.SetDefine(("MATERIAL_CONTACT_SHADOWS"), ShouldDoContactShadows() && IsTranslucentBlendMode(GetBlendMode()));
	OutEnvironment.SetDefine(("MATERIAL_BLOCK_GI"), ShouldBlockGI());
	OutEnvironment.SetDefine(("MATERIAL_DITHER_OPACITY_MASK"), IsDitherMasked());
	OutEnvironment.SetDefine(("MATERIAL_NORMAL_CURVATURE_TO_ROUGHNESS"), UseNormalCurvatureToRoughness() ? ("1") : ("0"));
	OutEnvironment.SetDefine(("MATERIAL_ALLOW_NEGATIVE_EMISSIVECOLOR"), AllowNegativeEmissiveColor());
	OutEnvironment.SetDefine(("MATERIAL_OUTPUT_OPACITY_AS_ALPHA"), GetBlendableOutputAlpha());
	OutEnvironment.SetDefine(("TRANSLUCENT_SHADOW_WITH_MASKED_OPACITY"), GetCastDynamicShadowAsMasked());

// 	if (IsUsingFullPrecision())
// 	{
// 		OutEnvironment.CompilerFlags.Add(CFLAG_UseFullPrecisionInPS);
// 	}

// 	{
// 		auto DecalBlendMode = (EDecalBlendMode)GetDecalBlendMode();
// 
// 		uint8 bDBufferMask = ComputeDBufferMRTMask(DecalBlendMode);
// 
// 		OutEnvironment.SetDefine(("MATERIAL_DBUFFERA"), (bDBufferMask & 0x1) != 0);
// 		OutEnvironment.SetDefine(("MATERIAL_DBUFFERB"), (bDBufferMask & 0x2) != 0);
// 		OutEnvironment.SetDefine(("MATERIAL_DBUFFERC"), (bDBufferMask & 0x4) != 0);
// 	}

// 	if (GetMaterialDomain() == MD_DeferredDecal)
// 	{
// 		bool bHasNormalConnected = HasNormalConnected();
// 		EDecalBlendMode DecalBlendMode = FDecalRenderingCommon::ComputeFinalDecalBlendMode(Platform, (EDecalBlendMode)GetDecalBlendMode(), bHasNormalConnected);
// 		FDecalRenderingCommon::ERenderTargetMode RenderTargetMode = FDecalRenderingCommon::ComputeRenderTargetMode(Platform, DecalBlendMode, bHasNormalConnected);
// 		uint32 RenderTargetCount = FDecalRenderingCommon::ComputeRenderTargetCount(Platform, RenderTargetMode);
// 
// 		uint32 BindTarget1 = (RenderTargetMode == FDecalRenderingCommon::RTM_SceneColorAndGBufferNoNormal || RenderTargetMode == FDecalRenderingCommon::RTM_SceneColorAndGBufferDepthWriteNoNormal) ? 0 : 1;
// 		OutEnvironment.SetDefine(("BIND_RENDERTARGET1"), BindTarget1);
// 
// 		// avoid using the index directly, better use DECALBLENDMODEID_VOLUMETRIC, DECALBLENDMODEID_STAIN, ...
// 		OutEnvironment.SetDefine(("DECAL_BLEND_MODE"), (uint32)DecalBlendMode);
// 		OutEnvironment.SetDefine(("DECAL_PROJECTION"), 1);
// 		OutEnvironment.SetDefine(("DECAL_RENDERTARGET_COUNT"), RenderTargetCount);
// 		OutEnvironment.SetDefine(("DECAL_RENDERSTAGE"), (uint32)FDecalRenderingCommon::ComputeRenderStage(Platform, DecalBlendMode));
// 
// 		// to compare against DECAL_BLEND_MODE, we can expose more if needed
// 		OutEnvironment.SetDefine(("DECALBLENDMODEID_VOLUMETRIC"), (uint32)DBM_Volumetric_DistanceFunction);
// 		OutEnvironment.SetDefine(("DECALBLENDMODEID_STAIN"), (uint32)DBM_Stain);
// 		OutEnvironment.SetDefine(("DECALBLENDMODEID_NORMAL"), (uint32)DBM_Normal);
// 		OutEnvironment.SetDefine(("DECALBLENDMODEID_EMISSIVE"), (uint32)DBM_Emissive);
// 		OutEnvironment.SetDefine(("DECALBLENDMODEID_TRANSLUCENT"), (uint32)DBM_Translucent);
// 		OutEnvironment.SetDefine(("DECALBLENDMODEID_AO"), (uint32)DBM_AmbientOcclusion);
// 		OutEnvironment.SetDefine(("DECALBLENDMODEID_ALPHACOMPOSITE"), (uint32)DBM_AlphaComposite);
// 	}

	switch (GetMaterialDomain())
	{
	case MD_Surface:			OutEnvironment.SetDefine(("MATERIAL_DOMAIN_SURFACE"), ("1")); break;
	case MD_DeferredDecal:		OutEnvironment.SetDefine(("MATERIAL_DOMAIN_DEFERREDDECAL"), ("1")); break;
	case MD_LightFunction:		OutEnvironment.SetDefine(("MATERIAL_DOMAIN_LIGHTFUNCTION"), ("1")); break;
	case MD_Volume:				OutEnvironment.SetDefine(("MATERIAL_DOMAIN_VOLUME"), ("1")); break;
	case MD_PostProcess:		OutEnvironment.SetDefine(("MATERIAL_DOMAIN_POSTPROCESS"), ("1")); break;
	case MD_UI:					OutEnvironment.SetDefine(("MATERIAL_DOMAIN_UI"), ("1")); break;
	default:
		//UE_LOG(LogMaterial, Warning, ("Unknown material domain: %u  Setting to MD_Surface"), (int32)GetMaterialDomain());
		OutEnvironment.SetDefine(("MATERIAL_DOMAIN_SURFACE"), ("1"));
	};

	switch (GetShadingModel())
	{
	case MSM_Unlit:				OutEnvironment.SetDefine(("MATERIAL_SHADINGMODEL_UNLIT"), ("1")); break;
	case MSM_DefaultLit:		OutEnvironment.SetDefine(("MATERIAL_SHADINGMODEL_DEFAULT_LIT"), ("1")); break;
	case MSM_Subsurface:		OutEnvironment.SetDefine(("MATERIAL_SHADINGMODEL_SUBSURFACE"), ("1")); break;
	case MSM_PreintegratedSkin: OutEnvironment.SetDefine(("MATERIAL_SHADINGMODEL_PREINTEGRATED_SKIN"), ("1")); break;
	case MSM_SubsurfaceProfile: OutEnvironment.SetDefine(("MATERIAL_SHADINGMODEL_SUBSURFACE_PROFILE"), ("1")); break;
	case MSM_ClearCoat:			OutEnvironment.SetDefine(("MATERIAL_SHADINGMODEL_CLEAR_COAT"), ("1")); break;
	case MSM_TwoSidedFoliage:	OutEnvironment.SetDefine(("MATERIAL_SHADINGMODEL_TWOSIDED_FOLIAGE"), ("1")); break;
	case MSM_Hair:				OutEnvironment.SetDefine(("MATERIAL_SHADINGMODEL_HAIR"), ("1")); break;
	case MSM_Cloth:				OutEnvironment.SetDefine(("MATERIAL_SHADINGMODEL_CLOTH"), ("1")); break;
	case MSM_Eye:				OutEnvironment.SetDefine(("MATERIAL_SHADINGMODEL_EYE"), ("1")); break;
	default:
		//UE_LOG(LogMaterial, Warning, ("Unknown material shading model: %u  Setting to MSM_DefaultLit"), (int32)GetShadingModel());
		OutEnvironment.SetDefine(("MATERIAL_SHADINGMODEL_DEFAULT_LIT"), ("1"));
	};

	if (IsTranslucentBlendMode(GetBlendMode()))
	{
		switch (GetTranslucencyLightingMode())
		{
		case TLM_VolumetricNonDirectional: OutEnvironment.SetDefine(("TRANSLUCENCY_LIGHTING_VOLUMETRIC_NONDIRECTIONAL"), ("1")); break;
		case TLM_VolumetricDirectional: OutEnvironment.SetDefine(("TRANSLUCENCY_LIGHTING_VOLUMETRIC_DIRECTIONAL"), ("1")); break;
		case TLM_VolumetricPerVertexNonDirectional: OutEnvironment.SetDefine(("TRANSLUCENCY_LIGHTING_VOLUMETRIC_PERVERTEX_NONDIRECTIONAL"), ("1")); break;
		case TLM_VolumetricPerVertexDirectional: OutEnvironment.SetDefine(("TRANSLUCENCY_LIGHTING_VOLUMETRIC_PERVERTEX_DIRECTIONAL"), ("1")); break;
		case TLM_Surface: OutEnvironment.SetDefine(("TRANSLUCENCY_LIGHTING_SURFACE_LIGHTINGVOLUME"), ("1")); break;
		case TLM_SurfacePerPixelLighting: OutEnvironment.SetDefine(("TRANSLUCENCY_LIGHTING_SURFACE_FORWARDSHADING"), ("1")); break;

		default:
			//UE_LOG(LogMaterial, Warning, ("Unknown lighting mode: %u"), (int32)GetTranslucencyLightingMode());
			OutEnvironment.SetDefine(("TRANSLUCENCY_LIGHTING_VOLUMETRIC_NONDIRECTIONAL"), ("1")); break;
		};
	}

// 	if (IsUsedWithEditorCompositing())
// 	{
// 		OutEnvironment.SetDefine(("EDITOR_PRIMITIVE_MATERIAL"), ("1"));
// 	}
// 
// 	{
// 		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(("r.StencilForLODDither"));
// 		OutEnvironment.SetDefine(("USE_STENCIL_LOD_DITHER_DEFAULT"), CVar->GetValueOnAnyThread() != 0 ? 1 : 0);
// 	}

	{
		switch (GetMaterialDomain())
		{
		case MD_Surface:		OutEnvironment.SetDefine(("MATERIALDOMAIN_SURFACE"), 1u); break;
		case MD_DeferredDecal:	OutEnvironment.SetDefine(("MATERIALDOMAIN_DEFERREDDECAL"), 1u); break;
		case MD_LightFunction:	OutEnvironment.SetDefine(("MATERIALDOMAIN_LIGHTFUNCTION"), 1u); break;
		case MD_PostProcess:	OutEnvironment.SetDefine(("MATERIALDOMAIN_POSTPROCESS"), 1u); break;
		case MD_UI:				OutEnvironment.SetDefine(("MATERIALDOMAIN_UI"), 1u); break;
		}
	}
}

uint32 FMaterialResource::GetMaterialId() const
{
	return Material->StateId;
}

std::string FMaterialResource::GetFriendlyName() const
{
	return "Material";
}

EMaterialDomain FMaterialResource::GetMaterialDomain() const
{
	return Material->MaterialDomain;
}

bool FMaterialResource::IsTwoSided() const
{
	return false;
}

bool FMaterialResource::IsDitheredLODTransition() const
{
	//return MaterialInstance ? MaterialInstance->IsDitheredLODTransition() : Material->IsDitheredLODTransition();
	return false;
}

bool FMaterialResource::IsTranslucencyWritingCustomDepth() const
{
	return false;
}

bool FMaterialResource::IsTangentSpaceNormal() const
{
	//return Material->bTangentSpaceNormal || (!Material->Normal.IsConnected() && !Material->bUseMaterialAttributes);
	return true;
}

bool FMaterialResource::ShouldInjectEmissiveIntoLPV() const
{
	//return Material->bUseEmissiveForDynamicAreaLighting;
	return true;
}

bool FMaterialResource::ShouldGenerateSphericalParticleNormals() const
{
	//return Material->bGenerateSphericalParticleNormals;
	return false;
}

bool FMaterialResource::ShouldDoSSR() const
{
	//return Material->bScreenSpaceReflections;
	return false;
}

bool FMaterialResource::ShouldDoContactShadows() const
{
	//return Material->bContactShadows;
	return true;
}

bool FMaterialResource::IsSpecialEngineMaterial() const
{
	return true;
}

enum EMaterialTessellationMode FMaterialResource::GetTessellationMode() const
{
	//return (EMaterialTessellationMode)Material->D3D11TessellationMode;
	return MTM_NoTessellation;
}

bool FMaterialResource::IsCrackFreeDisplacementEnabled() const
{
	//return Material->bEnableCrackFreeDisplacement;
	return false;
}

bool FMaterialResource::IsAdaptiveTessellationEnabled() const
{
	//return Material->bEnableAdaptiveTessellation;
	return false;
}

bool FMaterialResource::IsUsingFullPrecision() const
{
	//return Material->bUseFullPrecision;
	return true;
}

bool FMaterialResource::IsUsingHQForwardReflections() const
{
	//return Material->bUseHQForwardReflections;
	return false;
}

bool FMaterialResource::IsUsingPlanarForwardReflections() const
{
	//return Material->bUsePlanarForwardReflections;
	return false;
}

bool FMaterialResource::IsNonmetal() const
{
// 	return !Material->bUseMaterialAttributes ?
// 		(!Material->Metallic.IsConnected() && !Material->Specular.IsConnected()) :
// 		!(Material->MaterialAttributes.IsConnected(MP_Specular) || Material->MaterialAttributes.IsConnected(MP_Metallic));
	return false;
}

bool FMaterialResource::UseLmDirectionality() const
{
	//return Material->bUseLightmapDirectionality;
	return false;
}

enum EBlendMode FMaterialResource::GetBlendMode() const
{
	//return MaterialInstance ? MaterialInstance->GetBlendMode() : Material->GetBlendMode();
	return BLEND_Opaque;
}

enum ERefractionMode FMaterialResource::GetRefractionMode() const
{
	//return Material->RefractionMode;
	return RM_IndexOfRefraction;
}

enum EMaterialShadingModel FMaterialResource::GetShadingModel() const
{
	//return MaterialInstance ? MaterialInstance->GetShadingModel() : Material->GetShadingModel();
	return MSM_DefaultLit;
}

enum ETranslucencyLightingMode FMaterialResource::GetTranslucencyLightingMode() const
{
	//return (ETranslucencyLightingMode)Material->TranslucencyLightingMode;
	return TLM_VolumetricNonDirectional;
}

bool FMaterialResource::GetCastDynamicShadowAsMasked() const
{
	//return MaterialInstance ? MaterialInstance->GetCastDynamicShadowAsMasked() : Material->GetCastDynamicShadowAsMasked();
	return false;
}

bool FMaterialResource::IsDitherMasked() const
{
	//return Material->DitherOpacityMask;
	return false;
}

bool FMaterialResource::IsDefaultMaterial() const
{
	return true;
}

static UMaterial* GDefaultMaterials[MD_MAX] = { 0 };

void UMaterial::InitDefaultMaterials()
{
	static bool bInitialized = false;
	if (!bInitialized)
	{
		for (int32 Domain = 0; Domain < MD_DeferredDecal; ++Domain)
		{
			GDefaultMaterials[Domain] = new UMaterial();
			GDefaultMaterials[Domain]->PostLoad();
		}
		bInitialized = true;
	}

}

UMaterial* UMaterial::GetDefaultMaterial(EMaterialDomain Domain)
{
	InitDefaultMaterials();
	//check(Domain >= MD_Surface && Domain < MD_MAX);
	//check(GDefaultMaterials[Domain] != NULL);
	UMaterial* Default = GDefaultMaterials[Domain];
	return Default;
}

void UMaterial::PostLoad()
{
	DefaultMaterialInstances[0] = new FDefaultMaterialInstance(this, false, false);
	DefaultMaterialInstances[1] = new FDefaultMaterialInstance(this, true, false);
	DefaultMaterialInstances[2] = new FDefaultMaterialInstance(this, false, true);

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

FMaterialResource* UMaterial::AllocateResource()
{
	return new FMaterialResource();
}

FMaterialRenderProxy* UMaterial::GetRenderProxy(bool Selected, bool bHovered /*= false*/) const
{
	return DefaultMaterialInstances[Selected ? 1 : (bHovered ? 2 : 0)];
}

FMaterialResource* UMaterial::GetMaterialResource()
{
	return Resource;
}

const FMaterialResource* UMaterial::GetMaterialResource() const
{
	return Resource;
}

void UMaterial::UpdateResourceAllocations()
{
	Resource = AllocateResource();
	Resource->SetMaterial(this);
}

FMaterialRenderProxy::FMaterialRenderProxy()
	: bSelected(false)
	, bHovered(false)
{

}

FMaterialRenderProxy::FMaterialRenderProxy(bool bInSelected, bool bInHovered)
	: bSelected(bInSelected)
	, bHovered(bInHovered)
{

}

FMaterialRenderProxy::~FMaterialRenderProxy()
{

}

void FMaterialRenderProxy::CacheUniformExpressions()
{
	InitDynamicRHI();
}

void FMaterialRenderProxy::InitDynamicRHI()
{

}

void FMaterialRenderProxy::ReleaseDynamicRHI()
{

}
