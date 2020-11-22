#pragma once

#include "UnrealMath.h"
#include "D3D11RHI.h"
#include "SecureHash.h"
#include <vector>
#include <map>
#include <string>
#include <type_traits>

extern void InitializeShaderTypes();

/** Uninitializes cached shader type data.  This is needed before unloading modules that contain FShaderTypes. */
extern void UninitializeShaderTypes();

class FShaderParameterMap
{
public:

	FShaderParameterMap()
	{}

	bool FindParameterAllocation(const char* ParameterName, uint16& OutBufferIndex, uint16& OutBaseIndex, uint16& OutSize) const;
	bool ContainsParameterAllocation(const char* ParameterName) const;
	void AddParameterAllocation(const char* ParameterName, uint16 BufferIndex, uint16 BaseIndex, uint16 Size);
	void RemoveParameterAllocation(const char* ParameterName);
	/** Checks that all parameters are bound and asserts if any aren't in a debug build
	* @param InVertexFactoryType can be 0
	*/
	void VerifyBindingsAreComplete(const char* ShaderTypeName, class FVertexFactoryType* InVertexFactoryType) const;

	inline void GetAllParameterNames(std::vector<std::string>& OutNames) const
	{
		for (auto it = ParameterMap.begin(); it != ParameterMap.end(); ++it) {
			OutNames.push_back(it->first);
		}
	}

private:
	struct FParameterAllocation
	{
		uint16 BufferIndex;
		uint16 BaseIndex;
		uint16 Size;
		mutable bool bBound;

		FParameterAllocation() :
			bBound(false)
		{}

	};

	std::map<std::string, FParameterAllocation> ParameterMap;
public:

};
class FShaderCompilerDefinitions
{
public:
	FShaderCompilerDefinitions()
	{
		Definitions.clear();
	}
	void SetDefine(const char* Name, const char* Value)
	{
		Definitions.insert(std::make_pair(std::string(Name), std::string(Value)));
	}
	template<typename T>
	void SetDefine(const char* Name, T Value)
	{
		Definitions.insert(std::make_pair(std::string(Name), std::to_string(Value)));
		
	}
	const std::map<std::string, std::string>& GetDefinitionMap() const
	{
		return Definitions;
	}
	void Merge(const FShaderCompilerDefinitions& Other)
	{
		Definitions.insert(Other.Definitions.begin(), Other.Definitions.end());
	}
private:
	std::map<std::string, std::string> Definitions;
};
/** The environment used to compile a shader. */
struct FShaderCompilerEnvironment 
{
	// Map of the virtual file path -> content.
	// The virtual file paths are the ones that USF files query through the #include "<The Virtual Path of the file>"
	std::map<std::string, std::string> IncludeVirtualPathToContentsMap;

	std::map<std::string, std::shared_ptr<std::string>> IncludeVirtualPathToExternalContentsMap;

	//std::vector<uint32> CompilerFlags;
	//std::map<uint32, uint8> RenderTargetOutputFormatsMap;
	//std::map<std::string, FResourceTableEntry> ResourceTableMap;
	//std::map<std::string, uint32> ResourceTableLayoutHashes;
	//std::map<std::string, std::string> RemoteServerData;

	/** Default constructor. */
	FShaderCompilerEnvironment()
	{
		// Presize to reduce re-hashing while building shader jobs
		IncludeVirtualPathToContentsMap.clear();
	}

	/** Initialization constructor. */
	explicit FShaderCompilerEnvironment(const FShaderCompilerDefinitions& InDefinitions)
		: Definitions(InDefinitions)
	{
	}
	template<typename T>
	void SetDefine(const char* Name, T Value) { Definitions.SetDefine(Name, Value); }
	void SetDefine(const char* Name, bool Value) { Definitions.SetDefine(Name, (uint32)Value); }
	const std::map<std::string, std::string>& GetDefinitions() const
	{
		return Definitions.GetDefinitionMap();
	}
// 	void SetRenderTargetOutputFormat(uint32 RenderTargetIndex, EPixelFormat PixelFormat)
// 	{
// 		//RenderTargetOutputFormatsMap.Add(RenderTargetIndex, PixelFormat);
// 	}

	void Merge(const FShaderCompilerEnvironment& Other)
	{
		// Merge the include maps
		// Merge the values of any existing keys
		for (auto It = Other.IncludeVirtualPathToContentsMap.begin(); It != Other.IncludeVirtualPathToContentsMap.end(); ++It)
		{
			auto ExistingContentsIt = IncludeVirtualPathToContentsMap.find(It->first);

			if (ExistingContentsIt != IncludeVirtualPathToContentsMap.end())
			{
				ExistingContentsIt->second += It->second;
			}
			else
			{
				IncludeVirtualPathToContentsMap[It->first] = It->second;
			}
		}

		//check(Other.IncludeVirtualPathToExternalContentsMap.Num() == 0);

		//CompilerFlags.Append(Other.CompilerFlags);
		//ResourceTableMap.Append(Other.ResourceTableMap);
		//ResourceTableLayoutHashes.Append(Other.ResourceTableLayoutHashes);
		Definitions.Merge(Other.Definitions);
		//RenderTargetOutputFormatsMap.Append(Other.RenderTargetOutputFormatsMap);
		//RemoteServerData.Append(Other.RemoteServerData);
	}

private:

	FShaderCompilerDefinitions Definitions;
};

/** Struct that gathers all readonly inputs needed for the compilation of a single shader. */
struct FShaderCompilerInput
{
	uint32 Frequency;
	std::string ShaderFormat;
	std::string SourceFilePrefix;
	std::string VirtualSourceFilePath;
	std::string EntryPointName;

	// Skips the preprocessor and instead loads the usf file directly
	bool bSkipPreprocessedCache;

	bool bGenerateDirectCompileFile;

	// Shader pipeline information
	bool bCompilingForShaderPipeline;
	bool bIncludeUsedOutputs;
	std::vector<std::string> UsedOutputs;

	// Dump debug path (up to platform) e.g. "D:/MMittring-Z3941-A/UE4-Orion/OrionGame/Saved/ShaderDebugInfo/PCD3D_SM5"
	std::string DumpDebugInfoRootPath;
	// only used if enabled by r.DumpShaderDebugInfo (platform/groupname) e.g. ""
	std::string DumpDebugInfoPath;
	// materialname or "Global" "for debugging and better error messages
	std::string DebugGroupName;

	// Compilation Environment
	FShaderCompilerEnvironment Environment;
	std::shared_ptr<FShaderCompilerEnvironment> SharedEnvironment;

	// Additional compilation settings that can be filled by FMaterial::SetupExtaCompilationSettings
	// FMaterial::SetupExtaCompilationSettings is usually called by each (*)MaterialShaderType::BeginCompileShader() function
	//FExtraShaderCompilerSettings ExtraSettings;

	FShaderCompilerInput() :
		bSkipPreprocessedCache(false),
		bGenerateDirectCompileFile(false),
		bCompilingForShaderPipeline(false),
		bIncludeUsedOutputs(false)
	{
	}

	// generate human readable name for debugging
	std::string GenerateShaderName() const
	{
		std::string Name;

		if (DebugGroupName == ("Global"))
		{
			Name = VirtualSourceFilePath + ("|") + EntryPointName;
		}
		else
		{
			// we skip EntryPointName as it's usually not useful
			Name = DebugGroupName + (":") + VirtualSourceFilePath;
		}

		return Name;
	}

	std::string GetSourceFilename() const
	{
		//return FPaths::GetCleanFilename(VirtualSourceFilePath);
		return VirtualSourceFilePath;
	}

	void GatherSharedInputs(std::map<std::string, std::string>& ExternalIncludes, std::vector<FShaderCompilerEnvironment*>& SharedEnvironments)
	{
		assert(!SharedEnvironment || SharedEnvironment->IncludeVirtualPathToExternalContentsMap.size() == 0);

		for (auto It = Environment.IncludeVirtualPathToExternalContentsMap.begin(); 
			It != Environment.IncludeVirtualPathToExternalContentsMap.end();
			++It)
		{
			auto FoundEntryIt = ExternalIncludes.find(It->first);

			if (FoundEntryIt != ExternalIncludes.end())
			{
				ExternalIncludes.insert(std::make_pair(It->first, *It->second.get()));
			}
		}

		if (SharedEnvironment)
		{
			SharedEnvironments.push_back(SharedEnvironment.get());//addunique
		}
	}


};

/** The output of the shader compiler. */
struct FShaderCompilerOutput
{
	FShaderCompilerOutput()
		: NumInstructions(0)
		, NumTextureSamplers(0)
		, bSucceeded(false)
		, bFailedRemovingUnused(false)
		, bSupportsQueryingUsedAttributes(false)
	{
	}

	FShaderParameterMap ParameterMap;
	//TArray<FShaderCompilerError> Errors;
	//TArray<FString> PragmaDirectives;
	uint32 Frequency;
	ComPtr<ID3DBlob> ShaderCode;
	FSHAHash OutputHash;
	uint32 NumInstructions;
	uint32 NumTextureSamplers;
	bool bSucceeded;
	bool bFailedRemovingUnused;
	bool bSupportsQueryingUsedAttributes;
	//TArray<FString> UsedAttributes;

	//FString OptionalFinalShaderSource;

	/** Generates OutputHash from the compiler output. */
	//void GenerateOutputHash();

};

struct FCachedUniformBufferDeclaration
{
	// Using SharedPtr so we can hand off lifetime ownership to FShaderCompilerEnvironment::IncludeVirtualPathToExternalContentsMap when invalidating this cache
	std::shared_ptr<std::string> Declaration;
};

extern void BuildShaderFileToUniformBufferMap(std::map<std::string, std::vector<const char*> >& ShaderFileToUniformBufferVariables);
extern bool LoadFileToString(std::string& Result, const char* Filename);
extern bool LoadShaderSourceFile(const char* VirtualFilePath, std::string& OutFileContents/*, TArray<FShaderCompilerError>* OutCompileErrors*/);
extern void GetShaderIncludes(const char* EntryPointVirtualFilePath, const char* VirtualFilePath, std::vector<std::string>& IncludeVirtualFilePaths, uint32 DepthLimit = 100);

extern void GenerateReferencedUniformBuffers(
	const char* SourceFilename,
	const char* ShaderTypeName,
	const std::map<std::string, std::vector<const char*> >& ShaderFileToUniformBufferVariables,
	std::map<const char*, FCachedUniformBufferDeclaration>& UniformBufferEntries);

void VerifyShaderSourceFiles();