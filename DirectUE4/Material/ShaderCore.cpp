#include "ShaderCore.h"
#include "VertexFactory.h"
#include "Shader.h"

#include "log.h"
#include <fstream>
#include <sstream>
#include <algorithm>

bool FShaderParameterMap::FindParameterAllocation(const char* ParameterName, uint16& OutBufferIndex, uint16& OutBaseIndex, uint16& OutSize) const
{
	const FParameterAllocation& Allocation = ParameterMap.at(ParameterName);

	OutBufferIndex = Allocation.BufferIndex;
	OutBaseIndex = Allocation.BaseIndex;
	OutSize = Allocation.Size;
	if (Allocation.bBound)
	{
		// Can detect copy-paste errors in binding parameters.  Need to fix all the false positives before enabling.
		//UE_LOG(LogShaders, Warning, TEXT("Parameter %s was bound multiple times. Code error?"), ParameterName);
	}
	Allocation.bBound = true;
	return true;
}

bool FShaderParameterMap::ContainsParameterAllocation(const char* ParameterName) const
{
	return ParameterMap.find(ParameterName) != ParameterMap.end();
}

void FShaderParameterMap::AddParameterAllocation(const char* ParameterName, uint16 BufferIndex, uint16 BaseIndex, uint16 Size)
{
	FParameterAllocation Allocation;
	Allocation.BufferIndex = BufferIndex;
	Allocation.BaseIndex = BaseIndex;
	Allocation.Size = Size;
	ParameterMap.insert(std::make_pair(std::string(ParameterName) , Allocation));
}

void FShaderParameterMap::RemoveParameterAllocation(const char* ParameterName)
{
	ParameterMap.erase(std::string(ParameterName));
}

void FShaderParameterMap::VerifyBindingsAreComplete(const char* ShaderTypeName, class FVertexFactoryType* InVertexFactoryType) const
{

}
std::map<std::string, std::string> GShaderFileCache;

static void AddShaderSourceFileEntry(std::vector<std::string>& OutVirtualFilePaths, std::string VirtualFilePath)
{
	//check(CheckVirtualShaderFilePath(VirtualFilePath));
	if (std::find(OutVirtualFilePaths.begin(), OutVirtualFilePaths.end(), VirtualFilePath) == OutVirtualFilePaths.end())
	{
		OutVirtualFilePaths.push_back(VirtualFilePath);

		std::vector<std::string> ShaderIncludes;
		GetShaderIncludes(VirtualFilePath.c_str(), VirtualFilePath.c_str(), OutVirtualFilePaths);
		for (uint32 IncludeIdx = 0; IncludeIdx < ShaderIncludes.size(); IncludeIdx++)
		{
			if (std::find(OutVirtualFilePaths.begin(), OutVirtualFilePaths.end(), ShaderIncludes[IncludeIdx]) == OutVirtualFilePaths.end())
			{
				OutVirtualFilePaths.push_back(ShaderIncludes[IncludeIdx]);
			}
		}
	}
}

static void GetAllVirtualShaderSourcePaths(std::vector<std::string>& OutVirtualFilePaths)
{
	// add all shader source files for hashing
	for (auto FactoryIt = FVertexFactoryType::GetTypeList().begin(); FactoryIt != FVertexFactoryType::GetTypeList().end(); ++FactoryIt)
	{
		FVertexFactoryType* VertexFactoryType = *FactoryIt;
		if (VertexFactoryType)
		{
			std::string ShaderFilename(VertexFactoryType->GetShaderFilename());
			AddShaderSourceFileEntry(OutVirtualFilePaths, ShaderFilename);
		}
	}
	for (auto ShaderIt = FShaderType::GetTypeList().begin(); ShaderIt != FShaderType::GetTypeList().end(); ++ShaderIt)
	{
		FShaderType* ShaderType = *ShaderIt;
		if (ShaderType)
		{
			std::string ShaderFilename(ShaderType->GetShaderFilename());
			AddShaderSourceFileEntry(OutVirtualFilePaths, ShaderFilename);
		}
	}

	//#todo-rco: No need to loop through Shader Pipeline Types (yet)

	// also always add the MaterialTemplate.usf shader file
	AddShaderSourceFileEntry(OutVirtualFilePaths, std::string("/Engine/Private/MaterialTemplate.ush"));
	AddShaderSourceFileEntry(OutVirtualFilePaths, std::string("/Engine/Private/Common.ush"));
	AddShaderSourceFileEntry(OutVirtualFilePaths, std::string("/Engine/Private/Definitions.usf"));
}

void LoadShaderSourceFileChecked(const char* VirtualFilePath, std::string& OutFileContents)
{
	if (!LoadShaderSourceFile(VirtualFilePath, OutFileContents/*, nullptr*/))
	{
		assert(false);
	}
}

void BuildShaderFileToUniformBufferMap(std::map<std::string, std::vector<const char*> >& ShaderFileToUniformBufferVariables)
{
	std::vector<std::string> ShaderSourceFiles;
	GetAllVirtualShaderSourcePaths(ShaderSourceFiles);

	// Cache UB access strings, make it case sensitive for faster search
	struct FShaderVariable
	{
		FShaderVariable(const char* ShaderVariable) :
			OriginalShaderVariable(ShaderVariable)
		{
			SearchKey = OriginalShaderVariable;
			transform(SearchKey.begin(), SearchKey.end(), SearchKey.begin(), ::toupper);
			SearchKey += ".";

			SearchKeyWithSpace = OriginalShaderVariable;
			transform(SearchKeyWithSpace.begin(), SearchKeyWithSpace.end(), SearchKeyWithSpace.begin(), ::toupper);
			SearchKeyWithSpace += " .";
		}

		const char* OriginalShaderVariable;
		std::string SearchKey;
		std::string SearchKeyWithSpace;
	};
	// Cache each UB
	std::vector<FShaderVariable> SearchKeys;
	for (auto StructIt = GetUniformBufferInfoList().begin(); StructIt != GetUniformBufferInfoList().end(); ++StructIt)
	{
		SearchKeys.push_back(FShaderVariable(StructIt->ConstantBufferName.c_str()));
	}

	// Find for each shader file which UBs it needs
	for (uint32 FileIndex = 0; FileIndex < ShaderSourceFiles.size(); FileIndex++)
	{
		std::string ShaderFileContents;
		LoadShaderSourceFileChecked(ShaderSourceFiles[FileIndex].c_str(), ShaderFileContents);

		// To allow case sensitive search which is way faster on some platforms (no need to look up locale, etc)
		//ShaderFileContents. ToUpperInline();
		transform(ShaderFileContents.begin(), ShaderFileContents.end(), ShaderFileContents.begin(), ::toupper);
		std::vector<const char*>& ReferencedUniformBuffers = ShaderFileToUniformBufferVariables[ShaderSourceFiles[FileIndex]];

		for (uint32 SearchKeyIndex = 0; SearchKeyIndex < SearchKeys.size(); ++SearchKeyIndex)
		{
			// Searching for the uniform buffer shader variable being accessed with '.'
			if (ShaderFileContents.find(SearchKeys[SearchKeyIndex].SearchKey) != std::string::npos || ShaderFileContents.find(SearchKeys[SearchKeyIndex].SearchKeyWithSpace) != std::string::npos)
			{
				if (std::find(ReferencedUniformBuffers.begin(), ReferencedUniformBuffers.end(), SearchKeys[SearchKeyIndex].OriginalShaderVariable) == ReferencedUniformBuffers.end())
				{
					ReferencedUniformBuffers.push_back(SearchKeys[SearchKeyIndex].OriginalShaderVariable);
				}
			}
		}
	}
}

extern bool LoadFileToString(std::string& Result, const char* Filename)
{
	std::ifstream ShaderFileStream;
	ShaderFileStream.open(Filename);
	if (ShaderFileStream.is_open())
	{
		std::stringstream ShaderFileString;
		ShaderFileString << ShaderFileStream.rdbuf();
		Result = ShaderFileString.str();
		ShaderFileStream.close();
		return true;
	}
	return false;
}

bool LoadShaderSourceFile(const char* VirtualFilePath, std::string& OutFileContents/*, TArray<FShaderCompilerError>* OutCompileErrors*/)
{
	bool bResult = false;

	auto it = GShaderFileCache.find(VirtualFilePath);

	if (it != GShaderFileCache.end())
	{
		OutFileContents = it->second;
		bResult = true;
	}
	else
	{
		std::string ShaderFilePath = VirtualFilePath;// GetShaderSourceFilePath(VirtualFilePath, OutCompileErrors);

		// verify SHA hash of shader files on load. missing entries trigger an error
		if (ShaderFilePath.length() && LoadFileToString(OutFileContents, ShaderFilePath.c_str()))
		{
			//update the shader file cache
			GShaderFileCache.insert(std::make_pair(VirtualFilePath, OutFileContents));
			bResult = true;
		}
	}
	return bResult;
}

const char* SkipToCharOnCurrentLine(const char* InStr, char TargetChar)
{
	const char* Str = InStr;
	if (Str)
	{
		while (*Str && *Str != TargetChar && *Str != ('\n'))
		{
			++Str;
		}
		if (*Str != TargetChar)
		{
			Str = NULL;
		}
	}
	return Str;
}

bool StartsWith(const std::string& Str, const char* With)
{
	return strncmp(Str.c_str(), With, strlen(With)) == 0;
}

extern void GetShaderIncludes(const char* EntryPointVirtualFilePath, const char* VirtualFilePath, std::vector<std::string>& IncludeVirtualFilePaths, uint32 DepthLimit , bool AddToIncludeFile)
{
	std::string FileContents;
	LoadShaderSourceFile(VirtualFilePath, FileContents/*, nullptr*/);

	//avoid an infinite loop with a 0 length string
	if (FileContents.length() > 0)
	{
		if (AddToIncludeFile)
		{
			IncludeVirtualFilePaths.push_back(VirtualFilePath);
		}

		//find the first include directive
		const char* IncludeBegin = strstr(FileContents.c_str(), "#include ");

		uint32 SearchCount = 0;
		const uint32 MaxSearchCount = 200;
		//keep searching for includes as long as we are finding new ones and haven't exceeded the fixed limit
		while (IncludeBegin != NULL && SearchCount < MaxSearchCount && DepthLimit > 0)
		{
			//find the first double quotation after the include directive
			const char* IncludeFilenameBegin = SkipToCharOnCurrentLine(IncludeBegin, '\"');

			if (IncludeFilenameBegin)
			{
				//find the trailing double quotation
				const char* IncludeFilenameEnd = SkipToCharOnCurrentLine(IncludeFilenameBegin + 1, '\"');

				if (IncludeFilenameEnd)
				{
					//construct a string between the double quotations
					std::string ExtractedIncludeFilename(std::string(IncludeFilenameBegin + 1, (int32)(IncludeFilenameEnd - IncludeFilenameBegin - 1)));

					// If the include is relative, then it must be relative to the current virtual file path.
					if (!StartsWith(ExtractedIncludeFilename,"/"))
					{
						ExtractedIncludeFilename = std::string("./Shaders/") + ExtractedIncludeFilename;
					}

					//CRC the template, not the filled out version so that this shader's CRC will be independent of which material references it.
					if (strcmp( ExtractedIncludeFilename.c_str(), "/Engine/Generated/Material.ush" ) == 0)
					{
						ExtractedIncludeFilename = "/Engine/Private/MaterialTemplate.ush";
					}

					// Ignore uniform buffer, vertex factory and instanced stereo includes
					bool bIgnoreInclude = StartsWith(ExtractedIncludeFilename, "/Engine/Generated/");

					// Check virtual.
					//bIgnoreInclude |= !CheckVirtualShaderFilePath(ExtractedIncludeFilename);

					// Some headers aren't required to be found (platforms that the user doesn't have access to)
					// @todo: Is there some way to generalize this"
// 					const bool bIsOptionalInclude = (ExtractedIncludeFilename == TEXT("/Engine/Public/PS4/PS4Common.ush")
// 						|| ExtractedIncludeFilename == TEXT("/Engine/Private/PS4/PostProcessHMDMorpheus.usf")
// 						|| ExtractedIncludeFilename == TEXT("/Engine/Private/PS4/RTWriteMaskProcessing.usf")
// 						|| ExtractedIncludeFilename == TEXT("/Engine/Private/XboxOne/RTWriteMaskProcessing.usf")
// 						|| ExtractedIncludeFilename == TEXT("/Engine/Private/PS4/RGBAToYUV420.ush")
// 						|| ExtractedIncludeFilename == TEXT("/Engine/Public/XboxOne/XboxOneCommon.ush")
// 						);
// 					// ignore the header if it's optional and doesn't exist
// 					if (bIsOptionalInclude)
// 					{
// 						// Search in the default engine shader folder and in the same folder as the shader (in case its a project or plugin shader)
// 						FString StrippedString = ExtractedIncludeFilename.Replace(TEXT("/Engine"), TEXT(""), ESearchCase::CaseSensitive);
// 						FString EngineShaderFilename = FPaths::Combine(FPlatformProcess::BaseDir(), FPlatformProcess::ShaderDir(), *StrippedString);
// 						FString LocalShaderFilename = FPaths::Combine(FPaths::GetPath(ExtractedIncludeFilename), *ExtractedIncludeFilename);
// 						if (!FPaths::FileExists(EngineShaderFilename) && !FPaths::FileExists(LocalShaderFilename))
// 						{
// 							bIgnoreInclude = true;
// 						}
// 					}

					//vertex factories need to be handled separately
					if (!bIgnoreInclude)
					{
						if (std::find(IncludeVirtualFilePaths.begin(), IncludeVirtualFilePaths.end(), ExtractedIncludeFilename) == IncludeVirtualFilePaths.end())
						{
							GetShaderIncludes(EntryPointVirtualFilePath, ExtractedIncludeFilename.c_str(), IncludeVirtualFilePaths, DepthLimit - 1, true);
						}
					}
				}
			}

			// Skip to the end of the line.
			IncludeBegin = SkipToCharOnCurrentLine(IncludeBegin, ('\n'));

			//find the next include directive
			if (IncludeBegin && *IncludeBegin != 0)
			{
				IncludeBegin = strstr(IncludeBegin + 1, ("#include "));
			}
			SearchCount++;
		}

		if (SearchCount == MaxSearchCount || DepthLimit == 0)
		{
			X_LOG("GetShaderIncludes parsing terminated early to avoid infinite looping!\n Entrypoint \'%s\' CurrentInclude \'%s\' SearchCount %u Depth %u",
				EntryPointVirtualFilePath,
				VirtualFilePath,
				SearchCount,
				DepthLimit);
		}
	}
}
void GetShaderIncludes(const char* EntryPointVirtualFilePath, const char* VirtualFilePath, std::vector<std::string>& IncludeVirtualFilePaths, uint32 DepthLimit)
{
	GetShaderIncludes(EntryPointVirtualFilePath, VirtualFilePath, IncludeVirtualFilePaths, DepthLimit, false);
}

void GenerateReferencedUniformBuffers(
	const char* SourceFilename,
	const char* ShaderTypeName,
	const std::map<std::string, std::vector<const char*> >& ShaderFileToUniformBufferVariables,
	std::map<const char*, FCachedUniformBufferDeclaration>& UniformBufferEntries)
{
	std::vector<std::string> FilesToSearch;
	GetShaderIncludes(SourceFilename, SourceFilename, FilesToSearch);
	FilesToSearch.push_back(SourceFilename);

	for (uint32 FileIndex = 0; FileIndex < FilesToSearch.size(); FileIndex++)
	{
		const std::vector<const char*>& FoundUniformBufferVariables = ShaderFileToUniformBufferVariables.at(FilesToSearch[FileIndex]);

		for (uint32 VariableIndex = 0; VariableIndex < FoundUniformBufferVariables.size(); VariableIndex++)
		{
			UniformBufferEntries.insert(std::make_pair(FoundUniformBufferVariables[VariableIndex], FCachedUniformBufferDeclaration()));
		}
	}
}

void InitializeShaderTypes()
{
	X_LOG("InitializeShaderTypes() begin");

	//LogShaderSourceDirectoryMappings();

	std::map<std::string, std::vector<const char*> > ShaderFileToUniformBufferVariables;
	BuildShaderFileToUniformBufferMap(ShaderFileToUniformBufferVariables);

	FShaderType::Initialize(ShaderFileToUniformBufferVariables);
	FVertexFactoryType::Initialize(ShaderFileToUniformBufferVariables);


	X_LOG("InitializeShaderTypes() end");
}

void UninitializeShaderTypes()
{
	X_LOG("UninitializeShaderTypes() begin");

	FShaderType::Uninitialize();
	FVertexFactoryType::Uninitialize();

	X_LOG("UninitializeShaderTypes() end");
}
