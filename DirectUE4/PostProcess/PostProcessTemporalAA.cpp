#include "PostProcessTemporalAA.h"

FRCPassPostProcessTemporalAA::FRCPassPostProcessTemporalAA(
	const class FPostprocessContext& Context, 
	const FTAAPassParameters& InParameters,
	const FTemporalAAHistory& InInputHistory, 
	FTemporalAAHistory* OutOutputHistory)
	: Parameters(InParameters)
	, OutputExtent(0, 0)
	, InputHistory(InInputHistory)
	, OutputHistory(OutOutputHistory)
{
	bIsComputePass = Parameters.bIsComputePass;
	bPreferAsyncCompute = false;
}

void FRCPassPostProcessTemporalAA::Process(FRenderingCompositePassContext& Context)
{

}

PooledRenderTargetDesc FRCPassPostProcessTemporalAA::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	PooledRenderTargetDesc Ret;

	switch (InPassOutputId)
	{
	case ePId_Output0: // main color output
	case ePId_Output1:

		Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;
		Ret.Flags &= ~(TexCreate_FastVRAM | TexCreate_Transient);
		Ret.Reset();
		//regardless of input type, PF_FloatRGBA is required to properly accumulate between frames for a good result.
		Ret.Format = PF_FloatRGBA;
		//Ret.DebugName = kTAAOutputNames[static_cast<int32>(Parameters.Pass)];
		Ret.AutoWritable = false;
		Ret.TargetableFlags &= ~(TexCreate_RenderTargetable | TexCreate_UAV);
		Ret.TargetableFlags |= bIsComputePass ? TexCreate_UAV : TexCreate_RenderTargetable;

		if (OutputExtent.X > 0)
		{
			assert(OutputExtent.X % Parameters.ResolutionDivisor == 0);
			assert(OutputExtent.Y % Parameters.ResolutionDivisor == 0);
			Ret.Extent = OutputExtent / Parameters.ResolutionDivisor;
		}

		// Need a UAV to resource transition from gfx to compute.
		if (IsDOFTAAConfig(Parameters.Pass))
		{
			Ret.TargetableFlags |= TexCreate_UAV;
		}

		break;

	case ePId_Output2: // downsampled color output

		if (!bDownsamplePossible)
		{
			break;
		}

		assert(bIsComputePass);

		Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;
		Ret.Flags &= ~(TexCreate_FastVRAM);
		Ret.Reset();

		if (Parameters.DownsampleOverrideFormat != PF_Unknown)
		{
			Ret.Format = Parameters.DownsampleOverrideFormat;
		}

		//Ret.DebugName = TEXT("SceneColorHalfRes");
		Ret.AutoWritable = false;
		Ret.TargetableFlags &= ~TexCreate_RenderTargetable;
		Ret.TargetableFlags |= TexCreate_UAV;

		if (OutputExtent.X > 0)
		{
			assert(OutputExtent.X % Parameters.ResolutionDivisor == 0);
			assert(OutputExtent.Y % Parameters.ResolutionDivisor == 0);
			Ret.Extent = OutputExtent / Parameters.ResolutionDivisor;
		}

		Ret.Extent = FIntPoint::DivideAndRoundUp(Ret.Extent, 2);
		Ret.Extent.X = FMath::Max(1, Ret.Extent.X);
		Ret.Extent.Y = FMath::Max(1, Ret.Extent.Y);

		break;

	default:
		assert(false);
	}

	return Ret;
}

