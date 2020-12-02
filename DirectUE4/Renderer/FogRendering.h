#pragma once

#include "D3D11RHI.h"


struct alignas(16) FFogUniformParameters
{
	FFogUniformParameters()
	{
		ConstructUniformBufferInfo(*this);
	}
	struct ConstantStruct
	{
		Vector4 ExponentialFogParameters;
		Vector4 ExponentialFogColorParameter;
		Vector4 ExponentialFogParameters3;
		Vector2 SinCosInscatteringColorCubemapRotation;
		float Pading01;
		float Pading02;
		FVector FogInscatteringTextureParameters;
		float Pading03;
		Vector4 InscatteringLightDirection;
		Vector4 DirectionalInscatteringColor;
		float DirectionalInscatteringStartDistance;
		float ApplyVolumetricFog;
		float Pading04;
		float Pading05;
	} Constants;
	
	ID3D11ShaderResourceView* FogInscatteringColorCubemap;//TextureCube
	ID3D11SamplerState* FogInscatteringColorSampler;
	ID3D11ShaderResourceView* IntegratedLightScattering;//Texture3D
	ID3D11SamplerState* IntegratedLightScatteringSampler;

	static std::string GetConstantBufferName()
	{
		return "FogStruct";
	}

#define ADD_RES(StructName, MemberName) List.insert(std::make_pair(std::string(#StructName) + "_" + std::string(#MemberName),StructName.MemberName))
	static std::map<std::string, ID3D11ShaderResourceView*> GetSRVs(const FFogUniformParameters& FogStruct)
	{
		std::map<std::string, ID3D11ShaderResourceView*> List;
		ADD_RES(FogStruct, FogInscatteringColorCubemap);
		ADD_RES(FogStruct, IntegratedLightScattering);
		return List;
	}
	static std::map<std::string, ID3D11SamplerState*> GetSamplers(const FFogUniformParameters& FogStruct)
	{
		std::map<std::string, ID3D11SamplerState*> List;
		ADD_RES(FogStruct, FogInscatteringColorSampler);
		ADD_RES(FogStruct, IntegratedLightScatteringSampler);
		return List;
	}
	static std::map<std::string, ID3D11UnorderedAccessView*> GetUAVs(const FFogUniformParameters& FogStruct)
	{
		std::map<std::string, ID3D11UnorderedAccessView*> List;
		return List;
	}
#undef ADD_RES
};

extern void SetupFogUniformParameters(const class FViewInfo& View, FFogUniformParameters& OutParameters);

extern bool ShouldRenderFog(const class FSceneViewFamily& Family);