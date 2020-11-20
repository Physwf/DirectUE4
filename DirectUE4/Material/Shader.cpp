#include "Shader.h"
#include "ShaderCompiler.h"
#include "Material.h"
#include "VertexFactory.h"
#include "MeshMaterialShader.h"

FShaderResource::FShaderResource()
{

}

FShaderResource::FShaderResource(const FShaderCompilerOutput& Output, FShaderType* InSpecificType, int32 InSpecificPermutationId)
{

}

FShaderResource::~FShaderResource()
{

}

void FShaderResource::Register()
{

}

FShaderResourceId FShaderResource::GetId() const
{
	return FShaderResourceId(Frequency, OutputHash, SpecificType ? SpecificType->GetName() : nullptr, SpecificPermutationId);
}

void FShaderResource::InitRHI()
{

}

void FShaderResource::ReleaseRHI()
{

}

FShaderResource* FShaderResource::FindShaderResourceById(const FShaderResourceId& Id)
{
	auto it = ShaderResourceIdMap.find(Id);
	return it != ShaderResourceIdMap.end() ? it->second : NULL;
}

FShaderResource* FShaderResource::FindOrCreateShaderResource(const FShaderCompilerOutput& Output, class FShaderType* SpecificType, int32 SpecificPermutationId)
{
	const FShaderResourceId ResourceId(Output.Frequency, Output.OutputHash, SpecificType ? SpecificType->GetName() : nullptr, SpecificPermutationId);
	FShaderResource* Resource = FindShaderResourceById(ResourceId);
	if (!Resource)
	{
		Resource = new FShaderResource(Output, SpecificType, SpecificPermutationId);
	}
	else
	{
		assert(Resource->Canary == FShader::ShaderMagic_Initialized);
	}

	return Resource;
}

void FShaderResource::GetAllShaderResourceId(std::vector<FShaderResourceId>& Ids)
{

}

std::unordered_map<FShaderResourceId, FShaderResource*> FShaderResource::ShaderResourceIdMap;

FShaderId::FShaderId(const FSHAHash& InMaterialShaderMapHash, FVertexFactoryType* InVertexFactoryType, FShaderType* InShaderType, int32 InPermutationId, uint32 InFrequency)
	: MaterialShaderMapHash(InMaterialShaderMapHash)
	, ShaderType(InShaderType)
	, PermutationId(InPermutationId)
	//, SourceHash(InShaderType->GetSourceHash())
	, Frequency(InFrequency)
{

}

FShader::FShader() :
	SerializedResource(nullptr),
	VFType(nullptr),
	Type(nullptr),
	PermutationId(0),
	SetParametersId(0),
	Canary(ShaderMagic_Uninitialized)
{

}

FShader::FShader(const CompiledShaderInitializerType& Initializer) :
	SerializedResource(nullptr),
	MaterialShaderMapHash(Initializer.MaterialShaderMapHash),
	VFType(Initializer.VertexFactoryType),
	Type(Initializer.Type),
	PermutationId(Initializer.PermutationId),
	Frequency(Initializer.Frequency),
	SetParametersId(0),
	Canary(ShaderMagic_Initialized)
{
	//SourceHash = Type->GetSourceHash();

// 	if (VFType)
// 	{
// 		VFSourceHash = VFType->GetSourceHash();
// 	}

	for (auto It = GetUniformBufferInfoList().begin(); It != GetUniformBufferInfoList().end(); ++It)
	{
		if (Initializer.ParameterMap.ContainsParameterAllocation(It->ConstantBufferName.c_str()))
		{
			std::shared_ptr<FShaderUniformBufferParameter> Parameter = std::make_shared<FShaderUniformBufferParameter>();
			UniformBufferParameters.insert(std::make_pair(It->ConstantBufferName, Parameter));
			Parameter->Bind(Initializer.ParameterMap, It->ConstantBufferName.c_str(), SPF_Mandatory);
			for(auto& NameIt : It->SRVNames)
			{
				if (Initializer.ParameterMap.ContainsParameterAllocation(NameIt.c_str()))
				{
					Parameter->BindSRV(Initializer.ParameterMap, NameIt.c_str(), SPF_Optional);
				}
			}
			for (auto& NameIt : It->SamplerNames)
			{
				if (Initializer.ParameterMap.ContainsParameterAllocation(NameIt.c_str()))
				{
					Parameter->BindSRV(Initializer.ParameterMap, NameIt.c_str(), SPF_Optional);
				}
			}
			for (auto& NameIt : It->UAVNames)
			{
				if (Initializer.ParameterMap.ContainsParameterAllocation(NameIt.c_str()))
				{
					Parameter->BindSRV(Initializer.ParameterMap, NameIt.c_str(), SPF_Optional);
				}
			}
		}
	}

	SetResource(Initializer.Resource);
	Register();
}

FShader::~FShader()
{

}

void FShader::Register()
{
	FShaderId ShaderId = GetId();
	assert(ShaderId.MaterialShaderMapHash != FSHAHash());
	assert(Resource);
	Type->AddToShaderIdMap(ShaderId, this);
}

void FShader::Deregister()
{

}

FShaderId FShader::GetId() const
{
	FShaderId ShaderId;
	ShaderId.MaterialShaderMapHash = MaterialShaderMapHash;
	ShaderId.VertexFactoryType = VFType;
	ShaderId.VFSourceHash = VFSourceHash;
	ShaderId.ShaderType = Type;
	ShaderId.PermutationId = PermutationId;
	ShaderId.SourceHash = SourceHash;
	ShaderId.Frequency = Frequency;
	return ShaderId;
}

void FShader::SetResource(FShaderResource* InResource)
{
	Resource.reset(InResource);
}

void FShader::RegisterSerializedResource()
{
	if (SerializedResource)
	{
		FShaderResource* ExistingResource = FShaderResource::FindShaderResourceById(SerializedResource->GetId());

		// Reuse an existing shader resource if a matching one already exists in memory
		if (ExistingResource)
		{
			delete SerializedResource;
			SerializedResource = ExistingResource;
		}
		else
		{
			// Register the newly loaded shader resource so it can be reused by other shaders
			SerializedResource->Register();
		}

		SetResource(SerializedResource);
	}
}

static std::list<FShaderType*>	GShaderTypeList;

std::list<FShaderType*>& FShaderType::GetTypeList()
{
	return GShaderTypeList;
}

FShaderType* FShaderType::GetShaderTypeByName(const char* Name)
{
	for (auto It = GetTypeList().begin(); It != GetTypeList().end(); ++It)
	{
		FShaderType* Type = *It;
		if (strcmp(Name, Type->GetName()) == 0)
		{
			return Type;
		}
	}
	return nullptr;
}

std::vector<FShaderType*> FShaderType::GetShaderTypesByFilename(const char* Filename)
{
	std::vector<FShaderType*> OutShaders;
	for (auto It = GetTypeList().begin(); It!= GetTypeList().end(); ++It)
	{
		FShaderType* Type = *It;
		if (strcmp(Filename, Type->GetShaderFilename()) == 0)
		{
			OutShaders.push_back(Type);
		}
	}
	return OutShaders;
}

std::map<std::string, FShaderType*>& FShaderType::GetNameToTypeMap()
{
	static std::map<std::string, FShaderType*>* GShaderNameToTypeMap = NULL;
	if (!GShaderNameToTypeMap)
	{
		GShaderNameToTypeMap = new std::map<std::string, FShaderType*>();
	}
	return *GShaderNameToTypeMap;
}

void FShaderType::Initialize(const std::map<std::string, std::vector<const char*> >& ShaderFileToUniformBufferVariables)
{
	for (auto It = FShaderType::GetTypeList().begin(); It != FShaderType::GetTypeList().end() ; ++It)
	{
		FShaderType* Type = *It;
		GenerateReferencedUniformBuffers(Type->SourceFilename, Type->Name, ShaderFileToUniformBufferVariables, Type->ReferencedUniformBufferStructsCache);
	}
}

void FShaderType::Uninitialize()
{

}

FShaderType::FShaderType(
	EShaderTypeForDynamicCast InShaderTypeForDynamicCast,
	const char* InName,
	const char* InSourceFilename,
	const char* InFunctionName,
	uint32 InFrequency,
	int32 InTotalPermutationCount,
	ConstructSerializedType InConstructSerializedRef,
	GetStreamOutElementsType InGetStreamOutElementsRef
	) : 
	ShaderTypeForDynamicCast(InShaderTypeForDynamicCast),
	Name(InName),
	TypeName(InName),
	SourceFilename(InSourceFilename),
	FunctionName(InFunctionName),
	Frequency(InFrequency),
	TotalPermutationCount(InTotalPermutationCount),
	ConstructSerializedRef(InConstructSerializedRef),
	GetStreamOutElementsRef(InGetStreamOutElementsRef)
{
	bCachedUniformBufferStructDeclarations = false;
	GlobalListLink = GShaderTypeList.insert(GShaderTypeList.begin(),this);
	GetNameToTypeMap().insert(std::make_pair(TypeName,this));
	static uint32 NextHashIndex = 0;
	HashIndex = NextHashIndex++;
}

FShaderType::~FShaderType()
{
	GShaderTypeList.erase(GlobalListLink);
}

FShader* FShaderType::FindShaderById(const FShaderId& Id)
{
	if (ShaderIdMap.find(Id) != ShaderIdMap.end())
	{
		return ShaderIdMap[Id];
	}
	return NULL;
}

FShader* FShaderType::ConstructForDeserialization() const
{
	return (*ConstructSerializedRef)();
}

void FShaderType::AddReferencedUniformBufferIncludes(FShaderCompilerEnvironment& OutEnvironment, std::string& OutSourceFilePrefix)
{
	if (!bCachedUniformBufferStructDeclarations)
	{
		CacheUniformBufferIncludes(ReferencedUniformBufferStructsCache);
		bCachedUniformBufferStructDeclarations = true;
	}

	std::string UniformBufferIncludes;

	for (auto It = ReferencedUniformBufferStructsCache.begin(); It != ReferencedUniformBufferStructsCache.end(); ++It)
	{
		assert(It->second.Declaration.get() != NULL);
		assert(It->second.Declaration->length());
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer), "#include \"/Generated/%s.hlsl\"" "\r\n", It->first);
		UniformBufferIncludes += std::string(buffer);
		sprintf_s(buffer, sizeof(buffer), "/Generated/%s.hlsl", It->first);

		OutEnvironment.IncludeVirtualPathToExternalContentsMap.insert(std::make_pair(
			std::string(buffer),
			It->second.Declaration
		));

// 		for (TLinkedList<FUniformBufferStruct*>::TIterator StructIt(FUniformBufferStruct::GetStructList()); StructIt; StructIt.Next())
// 		{
// 			if (It.Key() == StructIt->GetShaderVariableName())
// 			{
// 				StructIt->AddResourceTableEntries(OutEnvironment.ResourceTableMap, OutEnvironment.ResourceTableLayoutHashes);
// 			}
// 		}
	}

	std::string& GeneratedUniformBuffersInclude = OutEnvironment.IncludeVirtualPathToContentsMap["/Generated/GeneratedUniformBuffers.hlsl"];
	GeneratedUniformBuffersInclude.append(UniformBufferIncludes);

	OutEnvironment.SetDefine("PLATFORM_SUPPORTS_SRV_UB", "1");
}

class FShaderCompileJob* FMaterialShaderType::BeginCompileShader(
	uint32 ShaderMapId, 
	const FMaterial* Material, 
	FShaderCompilerEnvironment* MaterialEnvironment, 
	const FShaderPipelineType* ShaderPipeline, 
	std::vector<FShaderCompileJob*>& NewJobs
	)
{
	FShaderCompileJob* NewJob = new FShaderCompileJob(ShaderMapId, nullptr, this, /* PermutationId = */ 0);

	NewJob->Input.SharedEnvironment.reset(MaterialEnvironment);
	FShaderCompilerEnvironment& ShaderEnvironment = NewJob->Input.Environment;

	//UE_LOG(LogShaders, Verbose, TEXT("			%s"), GetName());
	//COOK_STAT(MaterialShaderCookStats::ShadersCompiled++);

	//update material shader stats
	//UpdateMaterialShaderCompilingStats(Material);

	//Material->SetupExtaCompilationSettings(Platform, NewJob->Input.ExtraSettings);

	// Allow the shader type to modify the compile environment.
	SetupCompileEnvironment(Material, ShaderEnvironment);

	// Compile the shader environment passed in with the shader type's source code.
	::GlobalBeginCompileShader(
		Material->GetFriendlyName(),
		nullptr,
		this,
		ShaderPipeline,
		GetShaderFilename(),
		GetFunctionName(),
		GetFrequency(),
		NewJob,
		NewJobs
	);
	return NewJob;
}

FShader* FMaterialShaderType::FinishCompileShader(const FUniformExpressionSet& UniformExpressionSet, const FSHAHash& MaterialShaderMapHash, const FShaderCompileJob& CurrentJob, const std::string& InDebugDescription)
{
	assert(CurrentJob.bSucceeded);

	FShaderType* SpecificType = CurrentJob.ShaderType->LimitShaderResourceToThisType() ? CurrentJob.ShaderType : NULL;

	// Reuse an existing resource with the same key or create a new one based on the compile output
	// This allows FShaders to share compiled bytecode and RHI shader references
	FShaderResource* Resource = FShaderResource::FindOrCreateShaderResource(CurrentJob.Output, SpecificType, /* PermutationId = */ 0);

// 	if (ShaderPipelineType && !ShaderPipelineType->ShouldOptimizeUnusedOutputs())
// 	{
// 		// If sharing shaders in this pipeline, remove it from the type/id so it uses the one in the shared shadermap list
// 		ShaderPipelineType = nullptr;
// 	}

	// Find a shader with the same key in memory
	FShader* Shader = CurrentJob.ShaderType->FindShaderById(FShaderId(MaterialShaderMapHash, CurrentJob.VFType, CurrentJob.ShaderType, /* PermutationId = */ 0, CurrentJob.Input.Frequency));

	// There was no shader with the same key so create a new one with the compile output, which will bind shader parameters
	if (!Shader)
	{
		Shader = (*ConstructCompiledRef)(CompiledShaderInitializerType(this, CurrentJob.Output, Resource, UniformExpressionSet, MaterialShaderMapHash, nullptr, InDebugDescription));
		CurrentJob.Output.ParameterMap.VerifyBindingsAreComplete(GetName(),/* CurrentJob.Output.Frequency,*/ CurrentJob.VFType);
	}

	return Shader;
}

class FShaderCompileJob* FMeshMaterialShaderType::BeginCompileShader(
	uint32 ShaderMapId, 
	const FMaterial* Material, 
	FShaderCompilerEnvironment* MaterialEnvironment, 
	FVertexFactoryType* VertexFactoryType, 
	const FShaderPipelineType* ShaderPipeline, 
	std::vector<FShaderCompileJob*>& NewJobs)
{
	FShaderCompileJob* NewJob = new FShaderCompileJob(ShaderMapId, VertexFactoryType, this, /* PermutationId = */ 0);

	NewJob->Input.SharedEnvironment.reset(MaterialEnvironment);
	FShaderCompilerEnvironment& ShaderEnvironment = NewJob->Input.Environment;

	// apply the vertex factory changes to the compile environment
	assert(VertexFactoryType);
	VertexFactoryType->ModifyCompilationEnvironment( Material, ShaderEnvironment);

	//Material->SetupExtaCompilationSettings(NewJob->Input.ExtraSettings);

	//update material shader stats
	//UpdateMaterialShaderCompilingStats(Material);

	//UE_LOG(LogShaders, Verbose, TEXT("			%s"), GetName());
	//COOK_STAT(MaterialMeshCookStats::ShadersCompiled++);

	// Allow the shader type to modify the compile environment.
	SetupCompileEnvironment(Material, ShaderEnvironment);

	bool bAllowDevelopmentShaderCompile = false;// Material->GetAllowDevelopmentShaderCompile();

	// Compile the shader environment passed in with the shader type's source code.
	::GlobalBeginCompileShader(
		Material->GetFriendlyName(),
		VertexFactoryType,
		this,
		ShaderPipeline,
		GetShaderFilename(),
		GetFunctionName(),
		GetFrequency(),
		NewJob,
		NewJobs,
		bAllowDevelopmentShaderCompile
	);
	return NewJob;
}

FShader* FMeshMaterialShaderType::FinishCompileShader(const FUniformExpressionSet& UniformExpressionSet, const FSHAHash& MaterialShaderMapHash, const FShaderCompileJob& CurrentJob, const std::string& InDebugDescription)
{
	FShaderType* SpecificType = CurrentJob.ShaderType->LimitShaderResourceToThisType() ? CurrentJob.ShaderType : NULL;

	// Reuse an existing resource with the same key or create a new one based on the compile output
	// This allows FShaders to share compiled bytecode and RHI shader references
	FShaderResource* Resource = FShaderResource::FindOrCreateShaderResource(CurrentJob.Output, SpecificType, /* PermutationId = */ 0);

// 	if (ShaderPipelineType && !ShaderPipelineType->ShouldOptimizeUnusedOutputs())
// 	{
// 		// If sharing shaders in this pipeline, remove it from the type/id so it uses the one in the shared shadermap list
// 		ShaderPipelineType = nullptr;
// 	}

	// Find a shader with the same key in memory
	FShader* Shader = CurrentJob.ShaderType->FindShaderById(FShaderId(MaterialShaderMapHash,  CurrentJob.VFType, CurrentJob.ShaderType, /** PermutationId = */ 0, CurrentJob.Input.Frequency));

	// There was no shader with the same key so create a new one with the compile output, which will bind shader parameters
	if (!Shader)
	{
		Shader = (*ConstructCompiledRef)(CompiledShaderInitializerType(this, CurrentJob.Output, Resource, UniformExpressionSet, MaterialShaderMapHash, InDebugDescription, CurrentJob.VFType));
		((FMeshMaterialShader*)Shader)->ValidateAfterBind();
		CurrentJob.Output.ParameterMap.VerifyBindingsAreComplete(GetName(), /*CurrentJob.Output.Target,*/ CurrentJob.VFType);
	}

	return Shader;
}

