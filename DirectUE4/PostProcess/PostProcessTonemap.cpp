#include "PostProcessTonemap.h"

void FilmPostSetConstants(Vector4* __restrict const Constants, /*const FPostProcessSettings* __restrict const FinalPostProcessSettings,*/ bool bMobile, bool UseColorMatrix, bool UseShadowTint, bool UseContrast)
{

}

FRCPassPostProcessTonemap::FRCPassPostProcessTonemap(const FViewInfo& InView, bool bInDoGammaOnly, bool bInDoEyeAdaptation, bool bInHDROutput, bool bInIsComputePass)
	: bDoGammaOnly(bInDoGammaOnly)
	, bDoScreenPercentageInTonemapper(false)
	, bDoEyeAdaptation(bInDoEyeAdaptation)
	, bHDROutput(bInHDROutput)
	, View(InView)
{
	bIsComputePass = bInIsComputePass;
	bPreferAsyncCompute = false;
}

void FRCPassPostProcessTonemap::Process(FRenderingCompositePassContext& Context)
{

}

PooledRenderTargetDesc FRCPassPostProcessTonemap::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	PooledRenderTargetDesc Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;

	Ret.Reset();

	Ret.TargetableFlags &= ~(TexCreate_RenderTargetable | TexCreate_UAV);
	Ret.TargetableFlags |= bIsComputePass ? TexCreate_UAV : TexCreate_RenderTargetable;
	Ret.Format = bIsComputePass ? PF_R8G8B8A8 : PF_B8G8R8A8;

	// RGB is the color in LDR, A is the luminance for PostprocessAA
	Ret.Format = bHDROutput ? GRHIHDRDisplayOutputFormat : Ret.Format;
	//Ret.DebugName = TEXT("Tonemap");
	Ret.ClearValue = FClearValueBinding(FLinearColor(0, 0, 0, 0));
	//Ret.Flags |= GFastVRamConfig.Tonemap;

	if (0/*CVarDisplayOutputDevice.GetValueOnRenderThread()*/ == 7)
	{
		Ret.Format = PF_A32B32G32R32F;
	}


	// Mobile needs to override the extent
	if (bDoScreenPercentageInTonemapper && false/*View.GetFeatureLevel() <= ERHIFeatureLevel::ES3_1*/)
	{
		Ret.Extent = View.UnscaledViewRect.Max;
	}
	return Ret;
}
