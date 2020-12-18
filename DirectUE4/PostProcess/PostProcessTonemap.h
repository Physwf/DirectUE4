#pragma once

#include "ShaderParameters.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "PostProcessParameters.h"
#include "RenderingCompositionGraph.h"
#include "Scene.h"


static float GrainHalton(int32 Index, int32 Base)
{
	float Result = 0.0f;
	float InvBase = 1.0f / Base;
	float Fraction = InvBase;
	while (Index > 0)
	{
		Result += (Index % Base) * Fraction;
		Index /= Base;
		Fraction *= InvBase;
	}
	return Result;
}

static void GrainRandomFromFrame(FVector* __restrict const Constant, uint32 FrameNumber)
{
	Constant->X = GrainHalton(FrameNumber & 1023, 2);
	Constant->Y = GrainHalton(FrameNumber & 1023, 3);
}

void FilmPostSetConstants(Vector4* __restrict const Constants, /*const FPostProcessSettings* __restrict const FinalPostProcessSettings,*/ bool bMobile, bool UseColorMatrix, bool UseShadowTint, bool UseContrast);

// derives from TRenderingCompositePassBase<InputCount, OutputCount>
// ePId_Input0: SceneColor
// ePId_Input1: BloomCombined (not needed for bDoGammaOnly)
// ePId_Input2: EyeAdaptation (not needed for bDoGammaOnly)
// ePId_Input3: LUTsCombined (not needed for bDoGammaOnly)
class FRCPassPostProcessTonemap : public TRenderingCompositePassBase<4, 1>
{
public:
	// constructor
	FRCPassPostProcessTonemap(const FViewInfo& InView, bool bInDoGammaOnly, bool bDoEyeAdaptation, bool bHDROutput, bool InIsComputePass);

	// interface FRenderingCompositePass ---------

	virtual void Process(FRenderingCompositePassContext& Context) override;
	virtual void Release() override { delete this; }
	virtual PooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const override;

	//virtual FComputeFenceRHIParamRef GetComputePassEndFence() const override { return AsyncEndFence; }

	bool bDoGammaOnly;
	bool bDoScreenPercentageInTonemapper;
private:
	bool bDoEyeAdaptation;
	bool bHDROutput;

	const FViewInfo& View;

	//FComputeFenceRHIRef AsyncEndFence;
};

/** Encapsulates the post processing tone map vertex shader. */
template< bool bUseAutoExposure>
class TPostProcessTonemapVS : public FGlobalShader
{
	// This class is in the header so that Temporal AA can share this vertex shader.
	DECLARE_SHADER_TYPE(TPostProcessTonemapVS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	/** Default constructor. */
	TPostProcessTonemapVS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FShaderResourceParameter EyeAdaptation;
	FShaderParameter GrainRandomFull;
	FShaderParameter DefaultEyeExposure;
	FShaderParameter ScreenPosToScenePixel;

	/** Initialization constructor. */
	TPostProcessTonemapVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		EyeAdaptation.Bind(Initializer.ParameterMap, ("EyeAdaptation"));
		GrainRandomFull.Bind(Initializer.ParameterMap, ("GrainRandomFull"));
		DefaultEyeExposure.Bind(Initializer.ParameterMap, ("DefaultEyeExposure"));
		ScreenPosToScenePixel.Bind(Initializer.ParameterMap, ("ScreenPosToScenePixel"));
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		// Compile time template-based conditional
		OutEnvironment.SetDefine(("EYEADAPTATION_EXPOSURE_FIX"), (uint32)bUseAutoExposure);
	}

	void TransitionResources(const FRenderingCompositePassContext& Context)
	{
// 		if (Context.View.HasValidEyeAdaptation())
// 		{
// 			IPooledRenderTarget* EyeAdaptationRT = Context.View.GetEyeAdaptation(Context.RHICmdList);
// 			FTextureRHIParamRef EyeAdaptationRTRef = EyeAdaptationRT->GetRenderTargetItem().TargetableTexture;
// 			if (EyeAdaptationRTRef)
// 			{
// 				Context.RHICmdList.TransitionResources(EResourceTransitionAccess::EReadable, &EyeAdaptationRTRef, 1);
// 			}
// 		}
	}

	void SetVS(const FRenderingCompositePassContext& Context)
	{
		ID3D11VertexShader* const ShaderRHI = GetVertexShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(ShaderRHI, Context.View.ViewUniformBuffer.get());

		PostprocessParameter.SetVS(ShaderRHI, Context, TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI());

		FVector GrainRandomFullValue;
		{
			uint8 FrameIndexMod8 = 0;
			if (Context.View.State)
			{
				FrameIndexMod8 = Context.View.State->GetFrameIndexMod8();
			}
			GrainRandomFromFrame(&GrainRandomFullValue, FrameIndexMod8);
		}

		SetShaderValue(ShaderRHI, GrainRandomFull, GrainRandomFullValue);


		if (Context.View.HasValidEyeAdaptation())
		{
			PooledRenderTarget* EyeAdaptationRT = Context.View.GetEyeAdaptation();
			FD3D11Texture2D* EyeAdaptationRTRef = EyeAdaptationRT->TargetableTexture.get();
			if (EyeAdaptationRTRef)
			{
				//Context.RHICmdList.TransitionResources(EResourceTransitionAccess::EReadable, &EyeAdaptationRTRef, 1);
			}
			SetTextureParameter(ShaderRHI, EyeAdaptation, EyeAdaptationRT->TargetableTexture->GetShaderResourceView());
		}
		else
		{
			// some views don't have a state, thumbnail rendering?
			SetTextureParameter(ShaderRHI, EyeAdaptation, GWhiteTextureSRV);
		}

		// Compile time template-based conditional
		if (!bUseAutoExposure)
		{
			// Compute a CPU-based default.  NB: reverts to "1" if SM5 feature level is not supported
// 			float FixedExposure = FRCPassPostProcessEyeAdaptation::GetFixedExposure(Context.View);
// 			// Load a default value 
// 			SetShaderValue(ShaderRHI, DefaultEyeExposure, FixedExposure);
		}

		{
			FIntPoint ViewportOffset = Context.SceneColorViewRect.Min;
			FIntPoint ViewportExtent = Context.SceneColorViewRect.Size();
			Vector4 ScreenPosToScenePixelValue(
				ViewportExtent.X * 0.5f,
				-ViewportExtent.Y * 0.5f,
				ViewportExtent.X * 0.5f - 0.5f + ViewportOffset.X,
				ViewportExtent.Y * 0.5f - 0.5f + ViewportOffset.Y);
			SetShaderValue(ShaderRHI, ScreenPosToScenePixel, ScreenPosToScenePixelValue);
		}
	}
};


typedef TPostProcessTonemapVS<true/*bUseEyeAdaptation*/> FPostProcessTonemapVS;
