#pragma once

#include "ShaderParameters.h"
#include "SceneView.h"

class FShaderParameterMap;

/** Shader parameters needed for atmosphere passes. */
class FAtmosphereShaderTextureParameters
{
public:
	void Bind(const FShaderParameterMap& ParameterMap);

	template< typename ShaderRHIParamRef >
	inline void Set(const ShaderRHIParamRef ShaderRHI, const FSceneView& View) const
	{
		if (TransmittanceTexture.IsBound() || IrradianceTexture.IsBound() || InscatterTexture.IsBound())
		{
			SetTextureParameter(ShaderRHI, TransmittanceTexture, TransmittanceTextureSampler,
				TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT>::GetRHI(), View.AtmosphereTransmittanceTexture);
			SetTextureParameter(ShaderRHI, IrradianceTexture, IrradianceTextureSampler,
				TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT>::GetRHI(), View.AtmosphereIrradianceTexture);
			SetTextureParameter(ShaderRHI, InscatterTexture, InscatterTextureSampler,
				TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT>::GetRHI(), View.AtmosphereInscatterTexture);
		}
	}

private:
	FShaderResourceParameter TransmittanceTexture;
	FShaderResourceParameter TransmittanceTextureSampler;
	FShaderResourceParameter IrradianceTexture;
	FShaderResourceParameter IrradianceTextureSampler;
	FShaderResourceParameter InscatterTexture;
	FShaderResourceParameter InscatterTextureSampler;
};
