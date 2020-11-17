#pragma once

#include "D3D11RHI.h"

struct FSceneTexturesUniformParameters
{
	FSceneTexturesUniformParameters()
	{
		ConstructUniformBufferInfo(*this);
	}

	struct ConstantStruct
	{

	} Constants;

	ComPtr<ID3D11ShaderResourceView> SceneColorTexture;
	ComPtr<ID3D11SamplerState> SceneColorTextureSampler;
	ComPtr<ID3D11ShaderResourceView> SceneDepthTexture;
	ComPtr<ID3D11SamplerState> SceneDepthTextureSampler;
	ComPtr<ID3D11SamplerState> SceneDepthTextureNonMS;
	// GBuffer
	ComPtr<ID3D11ShaderResourceView> GBufferATexture;
	ComPtr<ID3D11ShaderResourceView> GBufferBTexture;
	ComPtr<ID3D11ShaderResourceView> GBufferCTexture;
	ComPtr<ID3D11ShaderResourceView> GBufferDTexture;
	ComPtr<ID3D11ShaderResourceView> GBufferETexture;
	ComPtr<ID3D11ShaderResourceView> GBufferVelocityTexture;
	ComPtr<ID3D11ShaderResourceView> GBufferATextureNonMS;
	ComPtr<ID3D11ShaderResourceView> GBufferBTextureNonMS;
	ComPtr<ID3D11ShaderResourceView> GBufferCTextureNonMS;
	ComPtr<ID3D11ShaderResourceView> GBufferDTextureNonMS;
	ComPtr<ID3D11ShaderResourceView> GBufferETextureNonMS;
	ComPtr<ID3D11ShaderResourceView> GBufferVelocityTextureNonMS;
	ComPtr<ID3D11SamplerState> GBufferATextureSampler;
	ComPtr<ID3D11SamplerState> GBufferBTextureSampler;
	ComPtr<ID3D11SamplerState> GBufferCTextureSampler;
	ComPtr<ID3D11SamplerState> GBufferDTextureSampler;
	ComPtr<ID3D11SamplerState> GBufferETextureSampler;
	ComPtr<ID3D11SamplerState> GBufferVelocityTextureSampler;
	// SSAO
	ComPtr<ID3D11ShaderResourceView> ScreenSpaceAOTexture;
	ComPtr<ID3D11SamplerState> ScreenSpaceAOTextureSampler;

	// Custom Depth / Stencil
	ComPtr<ID3D11ShaderResourceView> CustomDepthTextureNonMS;
	ComPtr<ID3D11ShaderResourceView> CustomDepthTexture;
	ComPtr<ID3D11SamplerState> CustomDepthTextureSampler;
	ComPtr<ID3D11ShaderResourceView> CustomStencilTexture;
	ComPtr<ID3D11ShaderResourceView> SceneStencilTexture;

	// Misc
	ComPtr<ID3D11ShaderResourceView> EyeAdaptation;
	ComPtr<ID3D11ShaderResourceView> SceneColorCopyTexture;
	ComPtr<ID3D11SamplerState> SceneColorCopyTextureSampler;

	static std::string GetConstantBufferName()
	{
		return "SceneTexturesStruct";
	}

#define ADD_RES(StructName, MemberName) List.insert(std::make_pair(std::string(#StructName) + "_" + std::string(#MemberName),StructName.MemberName))
	static std::map<std::string, ComPtr<ID3D11ShaderResourceView>> GetSRVs(const FSceneTexturesUniformParameters& SceneTexturesStruct)
	{
		std::map<std::string, ComPtr<ID3D11ShaderResourceView>> List;
		ADD_RES(SceneTexturesStruct, SceneColorTexture);
		ADD_RES(SceneTexturesStruct, SceneDepthTexture);
		ADD_RES(SceneTexturesStruct, GBufferATexture);
		ADD_RES(SceneTexturesStruct, GBufferCTexture);
		ADD_RES(SceneTexturesStruct, GBufferDTexture);
		ADD_RES(SceneTexturesStruct, GBufferETexture);
		ADD_RES(SceneTexturesStruct, GBufferVelocityTexture);
		ADD_RES(SceneTexturesStruct, GBufferATextureNonMS);
		ADD_RES(SceneTexturesStruct, GBufferBTextureNonMS);
		ADD_RES(SceneTexturesStruct, GBufferDTextureNonMS);
		ADD_RES(SceneTexturesStruct, GBufferETextureNonMS);
		ADD_RES(SceneTexturesStruct, GBufferVelocityTextureNonMS);
		ADD_RES(SceneTexturesStruct, ScreenSpaceAOTexture);
		ADD_RES(SceneTexturesStruct, CustomDepthTextureNonMS);
		ADD_RES(SceneTexturesStruct, CustomDepthTexture);
		ADD_RES(SceneTexturesStruct, CustomStencilTexture);
		ADD_RES(SceneTexturesStruct, SceneStencilTexture);
		ADD_RES(SceneTexturesStruct, EyeAdaptation);
		ADD_RES(SceneTexturesStruct, SceneColorCopyTexture);
		ADD_RES(SceneTexturesStruct, SceneColorCopyTexture);
		return List;
	}
	static std::map<std::string, ComPtr<ID3D11SamplerState>> GetSamplers(const FSceneTexturesUniformParameters& SceneTexturesStruct)
	{
		std::map<std::string, ComPtr<ID3D11SamplerState>> List;
		ADD_RES(SceneTexturesStruct, SceneColorTextureSampler);
		ADD_RES(SceneTexturesStruct, SceneDepthTextureNonMS);
		ADD_RES(SceneTexturesStruct, GBufferATextureSampler);
		ADD_RES(SceneTexturesStruct, GBufferBTextureSampler);
		ADD_RES(SceneTexturesStruct, GBufferCTextureSampler);
		ADD_RES(SceneTexturesStruct, GBufferDTextureSampler);
		ADD_RES(SceneTexturesStruct, GBufferETextureSampler);
		ADD_RES(SceneTexturesStruct, ScreenSpaceAOTextureSampler);
		ADD_RES(SceneTexturesStruct, CustomDepthTextureSampler);
		ADD_RES(SceneTexturesStruct, SceneColorCopyTextureSampler);
		return List;
	}
	static std::map<std::string, ComPtr<ID3D11UnorderedAccessView>> GetUAVs(const FSceneTexturesUniformParameters& SceneTexturesStruct)
	{
		std::map<std::string, ComPtr<ID3D11UnorderedAccessView>> List;
		return List;
	}
#undef ADD_RES
};

enum class ESceneTextureSetupMode : uint32
{
	None = 0,
	SceneDepth = 1,
	GBuffers = 2,
	SSAO = 4,
	CustomDepth = 8,
	All = SceneDepth | GBuffers | SSAO | CustomDepth
};

inline ESceneTextureSetupMode operator |(ESceneTextureSetupMode lhs, ESceneTextureSetupMode rhs)
{
	return static_cast<ESceneTextureSetupMode> (
		static_cast<uint32>(lhs) |
		static_cast<uint32>(rhs)
		);
}

inline ESceneTextureSetupMode operator &(ESceneTextureSetupMode lhs, ESceneTextureSetupMode rhs)
{
	return static_cast<ESceneTextureSetupMode> (
		static_cast<uint32>(lhs) &
		static_cast<uint32>(rhs)
		);
}

extern void SetupSceneTextureUniformParameters(
	RenderTargets& SceneContext,
	ESceneTextureSetupMode SetupMode,
	FSceneTexturesUniformParameters& OutParameters);


