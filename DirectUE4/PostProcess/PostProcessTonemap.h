#pragma once

#include "ShaderParameters.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "PostProcessParameters.h"
#include "RenderingCompositionGraph.h"

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
