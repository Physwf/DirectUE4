#include "GlobalShader.h"

const int32 GlobalShaderMapId = 0;

TShaderMap<FGlobalShaderType>* GGlobalShaderMap = NULL;


IMPLEMENT_SHADER_TYPE(, FNULLPS, "NullPixelShader.hlsl", "Main", SF_Pixel);

FGlobalShader::FGlobalShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	: FShader(Initializer)
{

}

extern TShaderMap<FGlobalShaderType>* GetGlobalShaderMap()
{
	return GGlobalShaderMap;
}

