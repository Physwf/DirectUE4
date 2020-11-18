#include "ShaderCompiler.h"

FShaderCompilingManager::FShaderCompilingManager()
{

}

void FShaderCompilingManager::AddJobs(std::vector<FShaderCompileJob*>& NewJobs/*, bool bApplyCompletedShaderMapForRendering, bool bOptimizeForLowLatency, bool bRecreateComponentRenderStateOnCompletion*/)
{
	CompileQueue.insert(CompileQueue.end(),NewJobs.begin(), NewJobs.end());
}

void FShaderCompilingManager::FinishCompilation(const char* MaterialName, const std::vector<int32>& ShaderMapIdsToFinishCompiling)
{

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


	NewJobs.push_back(NewJob);

}

