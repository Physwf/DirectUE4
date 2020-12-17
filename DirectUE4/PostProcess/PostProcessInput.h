#pragma once

#include "PostProcess/RenderingCompositionGraph.h"

class FRCPassPostProcessInput : public TRenderingCompositePassBase<0, 1>
{
public:
	// constructor
	FRCPassPostProcessInput(const ComPtr<PooledRenderTarget>& InData);

	// interface FRenderingCompositePass ---------

	virtual void Process(FRenderingCompositePassContext& Context) override;
	virtual void Release() override { delete this; }
	virtual PooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const override;

protected:

	TRefCountPtr<PooledRenderTarget> Data;
};