#include "ShaderCompiler.h"
#include "VertexFactory.h"
#include "ShaderPreprocessor.h"
#include "Material.h"

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
		}
	}
}

FShaderCompilingManager* GShaderCompilingManager = new FShaderCompilingManager();

void GlobalBeginCompileShader(
	const std::string& DebugGroupName, 
	class FVertexFactoryType* VFType, 
	class FShaderType* ShaderType, 
	const class FShaderPipelineType* ShaderPipelineType, 
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

