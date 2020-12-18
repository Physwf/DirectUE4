#include "PostProcessInput.h"

FRCPassPostProcessInput::FRCPassPostProcessInput(const ComPtr<PooledRenderTarget>& InData)
	: Data(InData)
{
	assert(Data);
}

void FRCPassPostProcessInput::Process(FRenderingCompositePassContext& Context)
{
	PassOutputs[0].RenderTarget = Data;
}

PooledRenderTargetDesc FRCPassPostProcessInput::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	assert(Data);

	PooledRenderTargetDesc Ret = Data->GetDesc();

	return Ret;
}
