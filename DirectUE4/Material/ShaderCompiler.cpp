#include "ShaderCompiler.h"
#include "VertexFactory.h"
#include "ShaderPreprocessor.h"
#include "Material.h"
#include "GlobalShader.h"
#include "log.h"

FShaderCompilingManager::FShaderCompilingManager()
{

}

void FShaderCompilingManager::AddJobs(std::vector<FShaderCompileJob*>& NewJobs/*, bool bApplyCompletedShaderMapForRendering, bool bOptimizeForLowLatency, bool bRecreateComponentRenderStateOnCompletion*/)
{
	CompileQueue.insert(CompileQueue.end(),NewJobs.begin(), NewJobs.end());


	for (uint32 JobIndex = 0; JobIndex < NewJobs.size(); JobIndex++)
	{
		//NewJobs[JobIndex]->bOptimizeForLowLatency = bOptimizeForLowLatency;
		FShaderMapCompileResults& ShaderMapInfo = ShaderMapJobs[NewJobs[JobIndex]->Id];
		//ShaderMapInfo.bApplyCompletedShaderMapForRendering = bApplyCompletedShaderMapForRendering;
		//ShaderMapInfo.bRecreateComponentRenderStateOnCompletion = bRecreateComponentRenderStateOnCompletion;
		ShaderMapInfo.NumJobsQueued++;
	}
}

void FShaderCompilingManager::FinishCompilation(const char* MaterialName, const std::vector<int32>& ShaderMapIdsToFinishCompiling)
{
	auto StringReplace = [](std::string& InOutString, const std::string& InReplaced, const std::string& InNewString) -> std::string&
	{
		for (std::string::size_type pos(0); pos != std::string::npos; pos += InNewString.length())
		{
			pos = InOutString.find(InReplaced, pos);
			if (pos != std::string::npos)
				InOutString.replace(pos, InReplaced.length(), InNewString);
			else
				break;
		}
		return InOutString;
	};

	for (FShaderCompileJob* Job : CompileQueue)
	{
		FShaderCompilerInput& Input = Job->Input;
		FShaderCompilerOutput& Output = Job->Output;
		std::string ShaderFileContent;
		FShaderCompilerDefinitions AdditionalDefines;
		AdditionalDefines.SetDefine("SM5_PROFILE", 1);
		AdditionalDefines.SetDefine("COMPILER_HLSL", 1);

		if (Input.SharedEnvironment)
			Input.Environment.Merge(*Input.SharedEnvironment);

		if (PreprocessShader(ShaderFileContent, Output, Input, AdditionalDefines))
		{
			std::vector<D3D_SHADER_MACRO> ShaderMacros;
			for (auto& Pair : Input.Environment.GetDefinitions())
			{
				D3D_SHADER_MACRO Macro;
				Macro.Name = Pair.first.c_str();
				Macro.Definition = Pair.second.c_str();
				ShaderMacros.push_back({ Pair.first.c_str(),Pair.second.c_str() });
			}
			ShaderMacros.push_back({ NULL, NULL });
			const char ShaderTargets[][7] = { "vs_5_0","hs_5_0" ,"ds_5_0" ,"ps_5_0" ,"gs_5_0" ,"cs_5_0" , };
			Job->bSucceeded = CompileShader(ShaderFileContent, Input.EntryPointName.c_str(), ShaderTargets[Input.Frequency], ShaderMacros.data(), Output.ShaderCode.GetAddressOf());
			if (Job->bSucceeded)
			{
				Output.Frequency = Input.Frequency;
				GetShaderParameterAllocations(Output.ShaderCode.Get(),Job->Output.ParameterMap);
			}
		}
		else
		{
			assert(false);
		}
	}

	for (uint32 JobIndex = 0; JobIndex < CompileQueue.size(); JobIndex++)
	{
		FShaderMapCompileResults& ShaderMapResults = ShaderMapJobs[CompileQueue[JobIndex]->Id];
		ShaderMapResults.FinishedJobs.push_back(CompileQueue[JobIndex]);
		ShaderMapResults.bAllJobsSucceeded = ShaderMapResults.bAllJobsSucceeded && CompileQueue[JobIndex]->bSucceeded;
	}

	std::map<int32, FShaderMapCompileResults> CompiledShaderMaps;
	for (uint32 ShaderMapIndex = 0; ShaderMapIndex < ShaderMapIdsToFinishCompiling.size(); ShaderMapIndex++)
	{
		if (ShaderMapJobs.find(ShaderMapIdsToFinishCompiling[ShaderMapIndex]) != ShaderMapJobs.end())
		{
			const FShaderMapCompileResults& Results = ShaderMapJobs[ShaderMapIdsToFinishCompiling[ShaderMapIndex]];
			assert(Results.FinishedJobs.size() == Results.NumJobsQueued);

			CompiledShaderMaps.insert(std::make_pair(ShaderMapIdsToFinishCompiling[ShaderMapIndex], Results));
			ShaderMapJobs.erase(ShaderMapIdsToFinishCompiling[ShaderMapIndex]);
		}
	}

	for (auto ProcessIt = CompiledShaderMaps.begin(); ProcessIt != CompiledShaderMaps.end(); ++ProcessIt)
	{
		FMaterialShaderMap* ShaderMap = NULL;
		std::vector<FMaterial*>* Materials = NULL;

		for (auto ShaderMapIt = FMaterialShaderMap::ShaderMapsBeingCompiled.begin(); ShaderMapIt != FMaterialShaderMap::ShaderMapsBeingCompiled.end(); ++ShaderMapIt)
		{
			if (ShaderMapIt->first->CompilingId == ProcessIt->first)
			{
				ShaderMap = ShaderMapIt->first;
				Materials = &ShaderMapIt->second;
				break;
			}
		}

		if (ShaderMap && Materials)
		{
			//TArray<FString> Errors;
			FShaderMapCompileResults& CompileResults = ProcessIt->second;
			const std::vector<FShaderCompileJob*>& ResultArray = CompileResults.FinishedJobs;

			// Make a copy of the array as this entry of FMaterialShaderMap::ShaderMapsBeingCompiled will be removed below
			std::vector<FMaterial*> MaterialsArray = *Materials;
			bool bSuccess = true;

			for (uint32 JobIndex = 0; JobIndex < ResultArray.size(); JobIndex++)
			{
				FShaderCompileJob& CurrentJob = *ResultArray[JobIndex];
				bSuccess = bSuccess && CurrentJob.bSucceeded;
			}

			bool bShaderMapComplete = true;

			if (bSuccess)
			{
				bShaderMapComplete = ShaderMap->ProcessCompilationResults(ResultArray);
			}

			if (bShaderMapComplete)
			{
				ShaderMap->bCompiledSuccessfully = bSuccess;
			}

			//ProcessIt = CompiledShaderMaps.erase(ShaderMap->CompilingId);
		}
		else if (ProcessIt->first == GlobalShaderMapId)
		{
			 auto It = CompiledShaderMaps.find(GlobalShaderMapId);

			if (It!= CompiledShaderMaps.end())
			{
				FShaderMapCompileResults* GlobalShaderResults = &It->second;
				const std::vector<FShaderCompileJob*>& CompilationResults = GlobalShaderResults->FinishedJobs;

				ProcessCompiledGlobalShaders(CompilationResults);

				for (uint32 ResultIndex = 0; ResultIndex < CompilationResults.size(); ResultIndex++)
				{
					delete CompilationResults[ResultIndex];
				}

				//ProcessIt = CompiledShaderMaps.erase(GlobalShaderMapId);
			}
		}
	}
	CompileQueue.clear();
	ShaderMapJobs.clear();
}

FShaderCompilingManager* GShaderCompilingManager = new FShaderCompilingManager();

void GlobalBeginCompileShader(
	const std::string& DebugGroupName, 
	class FVertexFactoryType* VFType, 
	class FShaderType* ShaderType, 
	const char* SourceFilename, 
	const char* FunctionName,
	uint32 Frequency, 
	FShaderCompileJob* NewJob, 
	std::vector<FShaderCompileJob*>& NewJobs, 
	bool bAllowDevelopmentShaderCompile /*= true */)
{
	FShaderCompilerInput& Input = NewJob->Input;
	Input.Frequency = Frequency;
	Input.ShaderFormat = "PCD3D_SM5";// LegacyShaderPlatformToShaderFormat(EShaderPlatform(Target.Platform));
	Input.VirtualSourceFilePath = SourceFilename;
	Input.EntryPointName = FunctionName;
	Input.bCompilingForShaderPipeline = false;
	Input.bIncludeUsedOutputs = false;
	//Input.bGenerateDirectCompileFile = (GDumpShaderDebugInfoSCWCommandLine != 0);
	//Input.DumpDebugInfoRootPath = GShaderCompilingManager->GetAbsoluteShaderDebugInfoDirectory() / Input.ShaderFormat.ToString();
	// asset material name or "Global"
	Input.DebugGroupName = DebugGroupName;

	// Add the appropriate definitions for the shader frequency.
	{
		Input.Environment.SetDefine(("PIXELSHADER"), Frequency == SF_Pixel);
		Input.Environment.SetDefine(("DOMAINSHADER"), Frequency == SF_Domain);
		Input.Environment.SetDefine(("HULLSHADER"), Frequency == SF_Hull);
		Input.Environment.SetDefine(("VERTEXSHADER"), Frequency == SF_Vertex);
		Input.Environment.SetDefine(("GEOMETRYSHADER"), Frequency == SF_Geometry);
		Input.Environment.SetDefine(("COMPUTESHADER"), Frequency == SF_Compute);
	}

	Input.Environment.SetDefine(("SHADING_PATH_DEFERRED"), 1);

	Input.Environment.SetDefine(("HAS_INVERTED_Z_BUFFER"), 1);

	Input.Environment.SetDefine(("ALLOW_STATIC_LIGHTING"), 1);

	Input.Environment.SetDefine(("USE_DBUFFER"),  0);

	Input.Environment.SetDefine(("FORWARD_SHADING"), 0);

	ShaderType->AddReferencedUniformBufferIncludes(Input.Environment, Input.SourceFilePrefix);

	if (VFType)
	{
		VFType->AddReferencedUniformBufferIncludes(Input.Environment, Input.SourceFilePrefix);
	}

	NewJobs.push_back(NewJob);

}

class FShaderCompileJob* FGlobalShaderTypeCompiler::BeginCompileShader(FGlobalShaderType* ShaderType, int32 PermutationId, std::vector<FShaderCompileJob*>& NewJobs)
{
	FShaderCompileJob* NewJob = new FShaderCompileJob(GlobalShaderMapId, nullptr, ShaderType, PermutationId);
	FShaderCompilerEnvironment& ShaderEnvironment = NewJob->Input.Environment;

	// Allow the shader type to modify the compile environment.
	ShaderType->SetupCompileEnvironment(PermutationId, ShaderEnvironment);

	static std::string GlobalName(("Global"));

	// Compile the shader environment passed in with the shader type's source code.
	::GlobalBeginCompileShader(
		GlobalName,
		nullptr,
		ShaderType,
		ShaderType->GetShaderFilename(),
		ShaderType->GetFunctionName(),
		ShaderType->GetFrequency(),
		NewJob,
		NewJobs
	);

	return NewJob;
}

FShader* FGlobalShaderTypeCompiler::FinishCompileShader(FGlobalShaderType* ShaderType, const FShaderCompileJob& CurrentJob)
{
	FShader* Shader = nullptr;
	if (CurrentJob.bSucceeded)
	{
		FShaderType* SpecificType = nullptr;
		int32 SpecificPermutationId = 0;
		if (CurrentJob.ShaderType->LimitShaderResourceToThisType())
		{
			SpecificType = CurrentJob.ShaderType;
			SpecificPermutationId = CurrentJob.PermutationId;
		}

		// Reuse an existing resource with the same key or create a new one based on the compile output
		// This allows FShaders to share compiled bytecode and RHI shader references
		FShaderResource* Resource = FShaderResource::FindOrCreateShaderResource(CurrentJob.Output, SpecificType, SpecificPermutationId);
		assert(Resource);

		// Find a shader with the same key in memory
		Shader = CurrentJob.ShaderType->FindShaderById(FShaderId(GGlobalShaderMapHash, nullptr, CurrentJob.ShaderType, CurrentJob.PermutationId, CurrentJob.Input.Frequency));

		// There was no shader with the same key so create a new one with the compile output, which will bind shader parameters
		if (!Shader)
		{
			Shader = (*(ShaderType->ConstructCompiledRef))(FGlobalShaderType::CompiledShaderInitializerType(ShaderType, CurrentJob.PermutationId, CurrentJob.Output, Resource, GGlobalShaderMapHash, nullptr));
			CurrentJob.Output.ParameterMap.VerifyBindingsAreComplete(ShaderType->GetName(), CurrentJob.VFType);
		}
	}

	return Shader;
}

void VerifyGlobalShaders(bool bLoadedFromCacheFile)
{
	TShaderMap<FGlobalShaderType>* GlobalShaderMap = GetGlobalShaderMap();

	const bool bEmptyMap = GlobalShaderMap->IsEmpty();

	bool bErrorOnMissing = bLoadedFromCacheFile;
	// All jobs, single & pipeline
	std::vector<FShaderCompileJob*> GlobalShaderJobs;

	// Add the single jobs first
	std::unordered_map<TShaderTypePermutation<const FShaderType>, FShaderCompileJob*> SharedShaderJobs;

	for (auto ShaderTypeIt = FShaderType::GetTypeList().begin(); ShaderTypeIt != FShaderType::GetTypeList().end(); ++ShaderTypeIt)
	{
		FGlobalShaderType* GlobalShaderType = (*ShaderTypeIt)->GetGlobalShaderType();
		if (!GlobalShaderType)
		{
			continue;
		}

		int32 PermutationCountToCompile = 0;
		for (int32 PermutationId = 0; PermutationId < GlobalShaderType->GetPermutationCount(); PermutationId++)
		{
			if (GlobalShaderType->ShouldCompilePermutation(PermutationId) && !GlobalShaderMap->HasShader(GlobalShaderType, PermutationId))
			{
				if (bErrorOnMissing)
				{
					X_LOG("Missing global shader %s's permutation %i, Please make sure cooking was successful.", GlobalShaderType->GetName(), PermutationId);
					assert(false);
				}
				// Compile this global shader type.
				auto* Job = FGlobalShaderTypeCompiler::BeginCompileShader(GlobalShaderType, PermutationId, GlobalShaderJobs);
				TShaderTypePermutation<const FShaderType> ShaderTypePermutation(GlobalShaderType, PermutationId);
				assert(SharedShaderJobs.find(ShaderTypePermutation) == SharedShaderJobs.end());
				SharedShaderJobs.insert(std::make_pair(ShaderTypePermutation, Job));
				PermutationCountToCompile++;
			}
		}
		if (PermutationCountToCompile >= 200 || strcmp(GlobalShaderType->GetName(), ("FPostProcessTonemapPS_ES2"))==0)
		{
			X_LOG("Global shader %s has %i permutation: probably more that it needs.", GlobalShaderType->GetName(), PermutationCountToCompile);
			assert(false);
		}

		if (!bEmptyMap && PermutationCountToCompile > 0)
		{
			X_LOG("	%s (%i out of %i)", GlobalShaderType->GetName(), PermutationCountToCompile, GlobalShaderType->GetPermutationCount());
		}
	}

	if (GlobalShaderJobs.size() > 0)
	{
		GShaderCompilingManager->AddJobs(GlobalShaderJobs/*, true, true, false*/);

		const bool bAllowAsynchronousGlobalShaderCompiling = false;
// 			// OpenGL requires that global shader maps are compiled before attaching
// 			// primitives to the scene as it must be able to find FNULLPS.
// 			// TODO_OPENGL: Allow shaders to be compiled asynchronously.
// 			// Metal also needs this when using RHI thread because it uses TOneColorVS very early in RHIPostInit()
// 			!IsOpenGLPlatform(GMaxRHIShaderPlatform) && !IsVulkanPlatform(GMaxRHIShaderPlatform) &&
// 			!IsMetalPlatform(GMaxRHIShaderPlatform) &&
// 			GShaderCompilingManager->AllowAsynchronousShaderCompiling();

		if (!bAllowAsynchronousGlobalShaderCompiling)
		{
			std::vector<int32> ShaderMapIds;
			ShaderMapIds.push_back(GlobalShaderMapId);

			GShaderCompilingManager->FinishCompilation(("Global"), ShaderMapIds);
		}
	}
}

static inline FShader* ProcessCompiledJob(FShaderCompileJob* SingleJob)
{
	FGlobalShaderType* GlobalShaderType = SingleJob->ShaderType->GetGlobalShaderType();
	assert(GlobalShaderType);
	FShader* Shader = FGlobalShaderTypeCompiler::FinishCompileShader(GlobalShaderType, *SingleJob);
	if (Shader)
	{
		GGlobalShaderMap->AddShader(GlobalShaderType, SingleJob->PermutationId, Shader);
	}
	else
	{
		X_LOG("Failed to compile global shader %s .  Enable 'r.ShaderDevelopmentMode' in ConsoleVariables.ini for retries.",GlobalShaderType->GetName());
	}

	return Shader;
};

void ProcessCompiledGlobalShaders(const std::vector<FShaderCompileJob*>& CompilationResults)
{
	for (uint32 ResultIndex = 0; ResultIndex < CompilationResults.size(); ResultIndex++)
	{
		ProcessCompiledJob((FShaderCompileJob*)CompilationResults[ResultIndex]);
	}
}

void CompileGlobalShaderMap(bool bRefreshShaderMap /*= false*/)
{
	if (!GGlobalShaderMap)
	{
		VerifyShaderSourceFiles();

		GGlobalShaderMap = new TShaderMap<FGlobalShaderType>();
	}

	VerifyGlobalShaders(false);
}

