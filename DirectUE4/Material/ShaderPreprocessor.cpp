#include "ShaderPreprocessor.h"
#include "mcpp.h"

#include <algorithm>
#include <cctype>

enum class EMessageType
{
	Error = 0,
	Warn = 1,
	ShaderMetaData = 2,
};

/**
* Filter MCPP errors.
* @param ErrorMsg - The error message.
* @returns true if the message is valid and has not been filtered out.
*/
inline EMessageType FilterMcppError(const std::string& ErrorMsg)
{
	const char* SubstringsToFilter[] =
	{
		"Unknown encoding:",
		"with no newline, supplemented newline",
		"Converted [CR+LF] to [LF]"
	};
	const int32 FilteredSubstringCount = 3;

	if (ErrorMsg.find("UE4SHADERMETADATA") != std::string::npos)
	{
		return EMessageType::ShaderMetaData;
	}

	for (int32 SubstringIndex = 0; SubstringIndex < FilteredSubstringCount; ++SubstringIndex)
	{
		if (ErrorMsg.find(SubstringsToFilter[SubstringIndex]) != std::string::npos)
		{
			return EMessageType::Warn;
		}
	}
	return EMessageType::Error;
}

static void ExtractDirective(std::string& OutString, std::string WarningString)
{
	static const std::string PrefixString = "UE4SHADERMETADATA_";
	uint32 DirectiveStartPosition = WarningString.find(PrefixString) + PrefixString.size();
	uint32 DirectiveEndPosition = WarningString.find("\n");
	OutString = WarningString.substr(DirectiveStartPosition, (DirectiveEndPosition - DirectiveStartPosition));
}

void ParseIntoArray(const std::string& src, const std::string& separator, std::vector<std::string>& dest) //字符串分割到数组
{
	//参数1：要分割的字符串；参数2：作为分隔符的字符；参数3：存放分割后的字符串的vector向量
	std::string str = src;
	std::string substring;
	std::string::size_type start = 0, index;
	dest.clear();
	index = str.find_first_of(separator, start);
	do
	{
		if (index != std::string::npos)
		{
			substring = str.substr(start, index - start);
			dest.push_back(substring);
			start = index + separator.size();
			index = str.find(separator, start);
			if (start == std::string::npos) break;
		}
	} while (index != std::string::npos);

	//the last part
	substring = str.substr(start);
	dest.push_back(substring);
}
#include <sstream>
bool IsNumeric(std::string& s)
{
	std::stringstream sin(s);
	double t;
	char p;
	if (!(sin >> t))
		/*解释：
		sin>>t表示把sin转换成double的变量（其实对于int和float型的都会接收），如果转换成功，则值为非0，如果转换不成功就返回为0
		*/
		return false;
	if (sin >> p)
		/*解释：此部分用于检测错误输入中，数字加字符串的输入形式（例如：34.f），在上面的的部分（sin>>t）已经接收并转换了输入的数字部分，在stringstream中相应也会把那一部分给清除，如果此时传入字符串是数字加字符串的输入形式，则此部分可以识别并接收字符部分，例如上面所说的，接收的是.f这部分，所以条件成立，返回false;如果剩下的部分不是字符，那么则sin>>p就为0,则进行到下一步else里面
		*/
		return false;
	else
		return true;
}
extern bool StartsWith(const std::string& Str, const char* With);
// trim from start (in place)
static inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
		}));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
		}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
	ltrim(s);
	return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
	rtrim(s);
	return s;
}
/**
* Parses MCPP error output.
* @param ShaderOutput - Shader output to which to add errors.
* @param McppErrors - MCPP error output.
*/
static bool ParseMcppErrors(std::vector<std::string>& OutStrings, const std::string& McppErrors)
{
	bool bSuccess = true;
	if (McppErrors.size() > 0)
	{
		std::vector<std::string> Lines;
		ParseIntoArray(McppErrors,"\n", Lines);
		for (uint32 LineIndex = 0; LineIndex < Lines.size(); ++LineIndex)
		{
			const std::string& Line = Lines[LineIndex];
			int32 SepIndex1 = Line.find(":", 2);
			int32 SepIndex2 = Line.find(":", SepIndex1 + 1);
			if (SepIndex1 != -1 && SepIndex2 != -1)
			{
				std::string Filename = Line.substr(0, SepIndex1);
				std::string LineNumStr = Line.substr(SepIndex1 + 1, SepIndex2 - SepIndex1 - 1);
				std::string Message = Line.substr(SepIndex2 + 1, Line.length() - SepIndex2 - 1);
				if (Filename.length() && LineNumStr.length() && IsNumeric(LineNumStr) && Message.length())
				{
					while (++LineIndex < Lines.size() && Lines[LineIndex].length() && StartsWith(Lines[LineIndex]," ") )
					{
						Message += std::string("\n") + Lines[LineIndex];
					}
					--LineIndex;
					trim(Message);

					// Ignore the warning about files that don't end with a newline.
					switch (FilterMcppError(Message))
					{
					case EMessageType::Error:
					{
// 						FShaderCompilerError* CompilerError = new(OutErrors) FShaderCompilerError;
// 						CompilerError->ErrorVirtualFilePath = Filename;
// 						CompilerError->ErrorLineString = LineNumStr;
// 						CompilerError->StrippedErrorMessage = Message;
						bSuccess = false;
					}
					break;
					case EMessageType::Warn:
					{
						// Warnings are ignored.
					}
					break;
					case EMessageType::ShaderMetaData:
					{
						std::string Directive;
						ExtractDirective(Directive, Message);
						OutStrings.push_back(Directive);
					}
					break;
					default:
						break;
					}
				}

			}
		}
	}
	return bSuccess;
}


/**
* Append defines to an MCPP command line.
* @param OutOptions - Upon return contains MCPP command line parameters as a string appended to the current string.
* @param Definitions - Definitions to add.
*/
static void AddMcppDefines(std::string& OutOptions, const std::map<std::string, std::string>& Definitions)
{
	for (auto It = Definitions.begin(); It != Definitions.end(); ++It)
	{
		char buffer[128];
		sprintf_s(buffer, sizeof(buffer), " \"-D%s=%s\"", It->first.c_str(), It->second.c_str());
		OutOptions += std::string(buffer);
	}
}

/**
* Helper class used to load shader source files for MCPP.
*/
class FMcppFileLoader
{
public:
	/** Initialization constructor. */
	explicit FMcppFileLoader(const FShaderCompilerInput& InShaderInput, FShaderCompilerOutput& InShaderOutput)
		: ShaderInput(InShaderInput)
		, ShaderOutput(InShaderOutput)
	{
		std::string InputShaderSource;
		if (LoadShaderSourceFile(InShaderInput.VirtualSourceFilePath.c_str(), InputShaderSource/*, nullptr*/))
		{
			char buffer[512 * 1024];
			sprintf_s(buffer, sizeof(buffer), "%s\n#line 1\n%s", ShaderInput.SourceFilePrefix.c_str(), InputShaderSource.c_str());
			InputShaderSource = buffer;
			CachedFileContents.insert(std::make_pair(InShaderInput.VirtualSourceFilePath, InputShaderSource));
		}
	}

	/** Retrieves the MCPP file loader interface. */
	file_loader GetMcppInterface()
	{
		file_loader Loader;
		Loader.get_file_contents = GetFileContents;
		Loader.user_data = (void*)this;
		return Loader;
	}

private:
	/** Holder for shader contents (string + size). */
	typedef std::string FShaderContents;

	/** MCPP callback for retrieving file contents. */
	static int GetFileContents(void* InUserData, const char* InVirtualFilePath, const char** OutContents, size_t* OutContentSize)
	{
		FMcppFileLoader* This = (FMcppFileLoader*)InUserData;

		std::string VirtualFilePath = InVirtualFilePath;

		// Collapse any relative directories to allow #include "../MyFile.ush"
		//FPaths::CollapseRelativeDirectories(VirtualFilePath);

		FShaderContents* CachedContents;
		auto It = This->CachedFileContents.find(VirtualFilePath);
		if (It == This->CachedFileContents.end())
		{
			std::string FileContents;

			if (This->ShaderInput.Environment.IncludeVirtualPathToContentsMap.find(VirtualFilePath) != This->ShaderInput.Environment.IncludeVirtualPathToContentsMap.end())
			{
				FileContents = This->ShaderInput.Environment.IncludeVirtualPathToContentsMap.at(VirtualFilePath);
			}
			else if (This->ShaderInput.Environment.IncludeVirtualPathToExternalContentsMap.find(VirtualFilePath) != This->ShaderInput.Environment.IncludeVirtualPathToExternalContentsMap.end())
			{
				FileContents = *This->ShaderInput.Environment.IncludeVirtualPathToExternalContentsMap.at(VirtualFilePath);
			}
			else
			{
				LoadShaderSourceFile(VirtualFilePath.c_str(), FileContents/*, &This->ShaderOutput.Errors*/);
			}

			if (FileContents.size() > 0)
			{
				// Adds a #line 1 "<Absolute file path>" on top of every file content to have nice absolute virtual source
				// file path in error messages.
				char buffer[64 * 1024];
				sprintf_s(buffer,sizeof(buffer), "#line 1 \"%s\"\n%s", VirtualFilePath.c_str(), FileContents.c_str());
				FileContents = buffer;
				This->CachedFileContents.insert(std::make_pair(VirtualFilePath, FileContents));
				CachedContents = &This->CachedFileContents[VirtualFilePath];
			}
		}
		else
		{
			CachedContents = &It->second;
		}

		if (OutContents)
		{
			*OutContents = CachedContents ? CachedContents->c_str() : NULL;
		}
		if (OutContentSize)
		{
			*OutContentSize = CachedContents ? CachedContents->size() : 0;
		}

		return CachedContents != nullptr;
	}

	/** Shader input data. */
	const FShaderCompilerInput& ShaderInput;
	/** Shader output data. */
	FShaderCompilerOutput& ShaderOutput;
	/** File contents are cached as needed. */
	std::map<std::string, FShaderContents> CachedFileContents;
};

/**
* Preprocess a shader.
* @param OutPreprocessedShader - Upon return contains the preprocessed source code.
* @param ShaderOutput - ShaderOutput to which errors can be added.
* @param ShaderInput - The shader compiler input.
* @param AdditionalDefines - Additional defines with which to preprocess the shader.
* @returns true if the shader is preprocessed without error.
*/
bool PreprocessShader(
	std::string& OutPreprocessedShader,
	FShaderCompilerOutput& ShaderOutput,
	const FShaderCompilerInput& ShaderInput,
	const FShaderCompilerDefinitions& AdditionalDefines
)
{
	// Skip the cache system and directly load the file path (used for debugging)
	if (ShaderInput.bSkipPreprocessedCache)
	{
		return LoadFileToString(OutPreprocessedShader, ShaderInput.VirtualSourceFilePath.c_str());
	}
	else
	{
		//check(CheckVirtualShaderFilePath(ShaderInput.VirtualSourceFilePath));
	}

	std::string McppOptions;
	std::string McppOutput, McppErrors;
	char* McppOutAnsi = NULL;
	char* McppErrAnsi = NULL;
	bool bSuccess = false;


	FMcppFileLoader FileLoader(ShaderInput, ShaderOutput);

	AddMcppDefines(McppOptions, ShaderInput.Environment.GetDefinitions());
	AddMcppDefines(McppOptions, AdditionalDefines.GetDefinitionMap());
	McppOptions += " -V199901L";
	McppOptions += " -I1";

	int32 Result = mcpp_run(
		McppOptions.c_str(),
		ShaderInput.VirtualSourceFilePath.c_str(),
		&McppOutAnsi,
		&McppErrAnsi,
		FileLoader.GetMcppInterface()
	);

	McppOutput = McppOutAnsi;
	McppErrors = McppErrAnsi;

	std::vector<std::string> PragmaDirectives;
	if (ParseMcppErrors(/*ShaderOutput.Errors,*/ PragmaDirectives, McppErrors))
	{
		// exchange strings
		OutPreprocessedShader = McppOutput;
		McppOutput.clear();
		bSuccess = true;
	}
	else
	{
		assert(false);
	}
	return bSuccess;
}

