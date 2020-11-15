#include "Shader.h"
#include "ShaderCompiler.h"
#include "Material.h"
#include "VertexFactory.h"

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

FShaderId::FShaderId(const FSHAHash& InMaterialShaderMapHash, FVertexFactoryType* InVertexFactoryType, FShaderType* InShaderType, int32 PermutationId, uint32 InFrequency)
{

}

FShader::FShader()
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

}

FShader::~FShader()
{

}

void FShader::Register()
{

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
	FShader* Result = ShaderIdMap.at(Id);
	return Result;
}

FShader* FShaderType::ConstructForDeserialization() const
{
	return (*ConstructSerializedRef)();
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

