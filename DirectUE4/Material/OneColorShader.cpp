#include "OneColorShader.h"

#include <map>
#include <string>

struct alignas(16) FClearShaderUB
{
	FClearShaderUB()
	{
		ConstructUniformBufferInfo(*this);
	}
	struct ConstantStruct
	{
		Vector4 DrawColorMRT[8];
	} Constants;

	static std::string GetConstantBufferName()
	{
		return "ClearShaderUB";
	}
	static std::map<std::string, ID3D11ShaderResourceView*> GetSRVs(const FClearShaderUB& ClearShaderUB)
	{
		std::map<std::string, ID3D11ShaderResourceView*> List;
		return List;
	}
	static std::map<std::string, ID3D11SamplerState*> GetSamplers(const FClearShaderUB& ClearShaderUB)
	{
		std::map<std::string, ID3D11SamplerState*> List;
		return List;
	}
	static std::map<std::string, ID3D11UnorderedAccessView*> GetUAVs(const FClearShaderUB& ClearShaderUB)
	{
		std::map<std::string, ID3D11UnorderedAccessView*> List;
		return List;
	}
};

FClearShaderUB ClearShaderUB;

void FOneColorPS::SetColors(const FLinearColor* Colors, int32 NumColors)
{
	assert(NumColors <= 8);

	auto& ClearUBParam = GetUniformBufferParameter<FClearShaderUB>();
	if (ClearUBParam.IsInitialized())
	{
		if (ClearUBParam.IsBound())
		{
			FClearShaderUB ClearData;
			memset(ClearData.Constants.DrawColorMRT,0,sizeof(ClearData.Constants.DrawColorMRT));

			for (int32 i = 0; i < NumColors; ++i)
			{
				ClearData.Constants.DrawColorMRT[i].X = Colors[i].R;
				ClearData.Constants.DrawColorMRT[i].Y = Colors[i].G;
				ClearData.Constants.DrawColorMRT[i].Z = Colors[i].B;
				ClearData.Constants.DrawColorMRT[i].W = Colors[i].A;
			}

			TUniformBufferPtr<FClearShaderUB> LocalUB = TUniformBufferPtr<FClearShaderUB>::CreateUniformBufferImmediate(ClearData);
			SetUniformBufferParameter(GetPixelShader(), ClearUBParam, LocalUB.get());
		}
	}


}

// #define avoids a lot of code duplication
#define IMPLEMENT_ONECOLORVS(A,B) typedef TOneColorVS<A,B> TOneColorVS##A##B; \
IMPLEMENT_SHADER_TYPE2(TOneColorVS##A##B, SF_Vertex);

IMPLEMENT_ONECOLORVS(false, false)
IMPLEMENT_ONECOLORVS(false, true)
IMPLEMENT_ONECOLORVS(true, true)
IMPLEMENT_ONECOLORVS(true, false)
#undef IMPLEMENT_ONECOLORVS


IMPLEMENT_SHADER_TYPE(, FOneColorPS, ("OneColorShader.dusf"), ("MainPixelShader"), SF_Pixel);
// Compiling a version for every number of MRT's
// On AMD PC hardware, outputting to a color index in the shader without a matching render target set has a significant performance hit
IMPLEMENT_SHADER_TYPE(template<> , TOneColorPixelShaderMRT<1>, ("OneColorShader.dusf"), ("MainPixelShaderMRT"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<> , TOneColorPixelShaderMRT<2>, ("OneColorShader.dusf"), ("MainPixelShaderMRT"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<> , TOneColorPixelShaderMRT<3>, ("OneColorShader.dusf"), ("MainPixelShaderMRT"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<> , TOneColorPixelShaderMRT<4>, ("OneColorShader.dusf"), ("MainPixelShaderMRT"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<> , TOneColorPixelShaderMRT<5>, ("OneColorShader.dusf"), ("MainPixelShaderMRT"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<> , TOneColorPixelShaderMRT<6>, ("OneColorShader.dusf"), ("MainPixelShaderMRT"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<> , TOneColorPixelShaderMRT<7>, ("OneColorShader.dusf"), ("MainPixelShaderMRT"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<> , TOneColorPixelShaderMRT<8>, ("OneColorShader.dusf"), ("MainPixelShaderMRT"), SF_Pixel);