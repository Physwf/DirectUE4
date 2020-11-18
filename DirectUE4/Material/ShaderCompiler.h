#pragma once

#include "ShaderCore.h"
#include "Shader.h"

class FShaderCompileJob 
{
public:
	/** Vertex factory type that this shader belongs to, may be NULL */
	FVertexFactoryType * VFType;
	/** Shader type that this shader belongs to, must be valid */
	FShaderType* ShaderType;
	/** Unique permutation identifier of the global shader type. */
	int32 PermutationId;
	/** Input for the shader compile */
	FShaderCompilerInput Input;
	FShaderCompilerOutput Output;

	// List of pipelines that are sharing this job.
	//std::map<const FVertexFactoryType*, std::vector<const FShaderPipelineType*>> SharingPipelines;

	FShaderCompileJob(uint32 InId, FVertexFactoryType* InVFType, FShaderType* InShaderType, int32 InPermutationId) :
		//FShaderCommonCompileJob(InId),
		VFType(InVFType),
		ShaderType(InShaderType),
		PermutationId(InPermutationId)
	{
	}

	//virtual FShaderCompileJob* GetSingleShaderJob() override { return this; }
	//virtual const FShaderCompileJob* GetSingleShaderJob() const override { return this; }
};

class FGlobalShaderTypeCompiler
{
public:
	/**
	* Enqueues compilation of a shader of this type.
	*/
	static class FShaderCompileJob* BeginCompileShader(FGlobalShaderType* ShaderType, int32 PermutationId, std::vector<FShaderCompileJob*>& NewJobs);

	/**
	* Enqueues compilation of a shader pipeline of this type.

	/** Either returns an equivalent existing shader of this type, or constructs a new instance. */
	//static FShader* FinishCompileShader(FGlobalShaderType* ShaderType, const FShaderCompileJob& CompileJob, const FShaderPipelineType* ShaderPipelineType);
};

class FShaderCompilingManager
{
private:
	/** Queue of tasks that haven't been assigned to a worker yet. */
	std::vector<FShaderCompileJob*> CompileQueue;
	/** Map from shader map Id to the compile results for that map, used to gather compiled results. */
	//std::map<int32, FShaderMapCompileResults> ShaderMapJobs;

	/** Base directory where temporary files are written out during multi core shader compiling. */
	std::string ShaderBaseWorkingDirectory;
	/** Absolute version of ShaderBaseWorkingDirectory. */
	std::string AbsoluteShaderBaseWorkingDirectory;
	/** Absolute path to the directory to dump shader debug info to. */
	std::string AbsoluteShaderDebugInfoDirectory;

public:

	FShaderCompilingManager();


	const std::string& GetAbsoluteShaderDebugInfoDirectory() const
	{
		return AbsoluteShaderDebugInfoDirectory;
	}
	/**
	* Adds shader jobs to be asynchronously compiled.
	* FinishCompilation or ProcessAsyncResults must be used to get the results.
	*/
	void AddJobs(std::vector<FShaderCompileJob*>& NewJobs/*, bool bApplyCompletedShaderMapForRendering, bool bOptimizeForLowLatency, bool bRecreateComponentRenderStateOnCompletion*/);
	/**
	* Removes all outstanding compile jobs for the passed shader maps.
	*/
	//void CancelCompilation(const char* MaterialName, const TArray<int32>& ShaderMapIdsToCancel);
	/**
	* Blocks until completion of the requested shader maps.
	* This will not assign the shader map to any materials, the caller is responsible for that.
	*/
	void FinishCompilation(const char* MaterialName, const std::vector<int32>& ShaderMapIdsToFinishCompiling);

	/**
	* Blocks until completion of all async shader compiling, and assigns shader maps to relevant materials.
	* This should be called before exit if the DDC needs to be made up to date.
	*/
	//void FinishAllCompilation();

	/**
	* Shutdown the shader compiler manager, this will shutdown immediately and not process any more shader compile requests.
	*/
	//void Shutdown();

};

extern FShaderCompilingManager* GShaderCompilingManager;

extern void GlobalBeginCompileShader(
	const std::string& DebugGroupName,
	class FVertexFactoryType* VFType,
	class FShaderType* ShaderType,
	const class FShaderPipelineType* ShaderPipelineType,
	const char* SourceFilename,
	const char* FunctionName,
	uint32 Frequency,
	FShaderCompileJob* NewJob,
	std::vector<FShaderCompileJob*>& NewJobs,
	bool bAllowDevelopmentShaderCompile = true
);