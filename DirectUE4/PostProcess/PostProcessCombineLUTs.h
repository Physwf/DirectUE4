#pragma once

#include "ShaderParameters.h"
#include "RenderingCompositionGraph.h"

// derives from TRenderingCompositePassBase<InputCount, OutputCount>
class FRCPassPostProcessCombineLUTs : public TRenderingCompositePassBase<0, 1>
{
public:
	FRCPassPostProcessCombineLUTs(bool bInAllocateOutput, bool InIsComputePass)
		:  bAllocateOutput(bInAllocateOutput)
	{
		bIsComputePass = InIsComputePass;
		bPreferAsyncCompute = false;
	}

	// interface FRenderingCompositePass ---------
	virtual void Process(FRenderingCompositePassContext& Context) override;
	virtual void Release() override { delete this; }
	virtual PooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const override;

	/** */
	uint32 GenerateFinalTable(/*const FFinalPostProcessSettings& Settings, FTexture* OutTextures[],*/ float OutWeights[], uint32 MaxCount) const;
	/** @return 0xffffffff if not found */
	uint32 FindIndex(/*const FFinalPostProcessSettings& Settings, UTexture* Tex*/) const;

	//virtual FComputeFenceRHIParamRef GetComputePassEndFence() const override { return AsyncEndFence; }

private:
// 	template <typename TRHICmdList>
// 	void DispatchCS(TRHICmdList& RHICmdList, FRenderingCompositePassContext& Context, const FIntRect& DestRect, FUnorderedAccessViewRHIParamRef DestUAV, int32 BlendCount, FTexture* Textures[], float Weights[]);

	//FComputeFenceRHIRef AsyncEndFence;

	bool bAllocateOutput;
};

/** Encapsulates the color remap parameters. */
class FColorRemapShaderParameters
{
public:
	FColorRemapShaderParameters() {}

	FColorRemapShaderParameters(const FShaderParameterMap& ParameterMap);

	// Explicit declarations here because templates unresolved when used in other files
	void Set(ID3D11PixelShader* const ShaderRHI);

// 	template <typename TRHICmdList>
// 	void Set(TRHICmdList& RHICmdList, const FComputeShaderRHIParamRef ShaderRHI);

	FShaderParameter MappingPolynomial;
};