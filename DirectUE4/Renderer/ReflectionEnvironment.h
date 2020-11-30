#pragma once

#include "D3D11RHI.h"

extern bool IsReflectionEnvironmentAvailable();
extern bool IsReflectionCaptureAvailable();


struct alignas(16) FReflectionUniformParameters
{
	FReflectionUniformParameters()
	{
		ConstructUniformBufferInfo(*this);
	}

	struct ConstantStruct
	{
		Vector4, SkyLightParameters;
		float SkyLightCubemapBrightness;
		float Pading01;
		float Pading02;
		float Pading03;
	} Constants;

	ID3D11ShaderResourceView* SkyLightCubemap;//TextureCube
	ID3D11SamplerState* SkyLightCubemapSampler;
	ID3D11ShaderResourceView* SkyLightBlendDestinationCubemap;//TextureCube
	ID3D11SamplerState* SkyLightBlendDestinationCubemapSampler;
	ID3D11ShaderResourceView* ReflectionCubemap;//TextureCubeArray
	ID3D11SamplerState* ReflectionCubemapSampler;

	static std::string GetConstantBufferName()
	{
		return "ReflectionStruct";
	}

#define ADD_RES(StructName, MemberName) List.insert(std::make_pair(std::string(#StructName) + "_" + std::string(#MemberName),StructName.MemberName))
	static std::map<std::string, ID3D11ShaderResourceView*> GetSRVs(const FReflectionUniformParameters& ReflectionStruct)
	{
		std::map<std::string, ID3D11ShaderResourceView*> List;
		ADD_RES(ReflectionStruct, SkyLightCubemap);
		ADD_RES(ReflectionStruct, SkyLightBlendDestinationCubemap);
		ADD_RES(ReflectionStruct, ReflectionCubemap);
		return List;
	}
	static std::map<std::string, ID3D11SamplerState*> GetSamplers(const FReflectionUniformParameters& ReflectionStruct)
	{
		std::map<std::string, ID3D11SamplerState*> List;
		ADD_RES(ReflectionStruct, SkyLightCubemapSampler);
		ADD_RES(ReflectionStruct, SkyLightBlendDestinationCubemapSampler);
		ADD_RES(ReflectionStruct, ReflectionCubemapSampler);
		return List;
	}
	static std::map<std::string, ID3D11UnorderedAccessView*> GetUAVs(const FReflectionUniformParameters& ReflectionStruct)
	{
		std::map<std::string, ID3D11UnorderedAccessView*> List;
		return List;
	}
#undef ADD_RES
};

extern void SetupReflectionUniformParameters(const class FViewInfo& View, FReflectionUniformParameters& OutParameters);