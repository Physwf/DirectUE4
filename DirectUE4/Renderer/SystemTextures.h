#pragma once

#include "RenderTargetPool.h"
#include "RefCounting.h"

class FSystemTextures
{
public:
	FSystemTextures()
		: bTexturesInitialized(false)
	{}

	/**
	* Initialize/allocate textures if not already.
	*/
	inline void InitializeTextures()
	{
		if (bTexturesInitialized)
		{
			// Already initialized up to at least the feature level we need, so do nothing
			return;
		}
		InternalInitializeTextures();
	}

	// FRenderResource interface.
	/**
	* Release textures when device is lost/destroyed.
	*/
	virtual void ReleaseDynamicRHI();

	// -----------

	/**
	Any Textures added here MUST be explicitly released on ReleaseDynamicRHI()!
	Some RHIs need all their references released during destruction!
	*/

	// float4(1,1,1,1) can be used in case a light is not shadow casting
	ComPtr<PooledRenderTarget> WhiteDummy;
	// float4(0,0,0,0) can be used in additive postprocessing to avoid a shader combination
	ComPtr<PooledRenderTarget> BlackDummy;
	// float4(0,0,0,1)
	ComPtr<PooledRenderTarget> BlackAlphaOneDummy;
	// used by the material expression Noise
	ComPtr<PooledRenderTarget> PerlinNoiseGradient;
	// used by the material expression Noise (faster version, should replace old version), todo: move out of SceneRenderTargets
	ComPtr<PooledRenderTarget> PerlinNoise3D;
	// Sobol sampling texture, the first sample points for four sobol dimensions in RGBA
	ComPtr<PooledRenderTarget> SobolSampling;
	/** SSAO randomization */
	ComPtr<PooledRenderTarget> SSAORandomization;
	/** Preintegrated GF for single sample IBL */
	ComPtr<PooledRenderTarget> PreintegratedGF;
	/** Linearly Transformed Cosines LUTs */
	ComPtr<PooledRenderTarget> LTCMat;
	ComPtr<PooledRenderTarget> LTCAmp;
	/** Texture that holds a single value containing the maximum depth that can be stored as FP16. */
	ComPtr<PooledRenderTarget> MaxFP16Depth;
	/** Depth texture that holds a single depth value */
	ComPtr<PooledRenderTarget> DepthDummy;
	// float4(0,1,0,1)
	ComPtr<PooledRenderTarget> GreenDummy;
	// float4(0.5,0.5,0.5,1)
	ComPtr<PooledRenderTarget> DefaultNormal8Bit;

protected:
	/** Maximum feature level that the textures have been initialized up to */
	bool bTexturesInitialized;

	void InternalInitializeTextures();
};

/** The global system textures used for scene rendering. */
extern FSystemTextures GSystemTextures;