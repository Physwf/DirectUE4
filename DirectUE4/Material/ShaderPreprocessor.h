#pragma once

#include "ShaderCore.h"

/**
* Preprocess a shader.
* @param OutPreprocessedShader - Upon return contains the preprocessed source code.
* @param ShaderOutput - ShaderOutput to which errors can be added.
* @param ShaderInput - The shader compiler input.
* @param AdditionalDefines - Additional defines with which to preprocess the shader.
* @returns true if the shader is preprocessed without error.
*/
extern bool PreprocessShader(
	std::string& OutPreprocessedShader,
	FShaderCompilerOutput& ShaderOutput,
	const FShaderCompilerInput& ShaderInput,
	const FShaderCompilerDefinitions& AdditionalDefines
);