#include "RenderingCompositionGraph.h"
#include "RenderTargetPool.h"
#include "Viewport.h"

FRenderingCompositionGraph::FRenderingCompositionGraph()
{

}

FRenderingCompositionGraph::~FRenderingCompositionGraph()
{
	Free();
}

void FRenderingCompositionGraph::Free()
{

}

void FRenderingCompositionGraph::RecursivelyGatherDependencies(FRenderingCompositePass *Pass)
{
	assert(Pass);

	if (Pass->bComputeOutputDescWasCalled)
	{
		// already processed
		return;
	}
	Pass->bComputeOutputDescWasCalled = true;

	uint32 Index = 0;
	while (const FRenderingCompositeOutputRef* OutputRefIt = Pass->GetDependency(Index++))
	{
		FRenderingCompositeOutput* InputOutput = OutputRefIt->GetOutput();

		if (InputOutput)
		{
			// add a dependency to this output as we are referencing to it
			InputOutput->AddDependency();
		}

		if (FRenderingCompositePass* OutputRefItPass = OutputRefIt->GetPass())
		{
			// recursively process all inputs of this Pass
			RecursivelyGatherDependencies(OutputRefItPass);
		}
	}

	// the pass is asked what the intermediate surface/texture format needs to be for all its outputs.
	for (uint32 OutputId = 0; ; ++OutputId)
	{
		EPassOutputId PassOutputId = (EPassOutputId)(OutputId);
		FRenderingCompositeOutput* Output = Pass->GetOutput(PassOutputId);

		if (!Output)
		{
			break;
		}

		Output->RenderTargetDesc = Pass->ComputeOutputDesc(PassOutputId);

		// Allow format overrides for high-precision work
		static const auto CVarPostProcessingColorFormat = 0;// IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.PostProcessingColorFormat"));

		if (CVarPostProcessingColorFormat/*->GetValueOnRenderThread()*/ == 1)
		{
			if (Output->RenderTargetDesc.Format == PF_FloatRGBA ||
				Output->RenderTargetDesc.Format == PF_FloatRGB ||
				Output->RenderTargetDesc.Format == PF_FloatR11G11B10)
			{
				Output->RenderTargetDesc.Format = PF_A32B32G32R32F;
			}
		}
	}
}

void FRenderingCompositionGraph::RecursivelyProcess(const FRenderingCompositeOutputRef& InOutputRef, FRenderingCompositePassContext& Context) const
{
	FRenderingCompositePass *Pass = InOutputRef.GetPass();
	FRenderingCompositeOutput *Output = InOutputRef.GetOutput();

	assert(Pass);
	assert(Output);

	if (Pass->bProcessWasCalled)
	{
		// already processed
		return;
	}
	Pass->bProcessWasCalled = true;

	// iterate through all inputs and additional dependencies of this pass
	{
		uint32 Index = 0;

		while (const FRenderingCompositeOutputRef* OutputRefIt = Pass->GetDependency(Index++))
		{
			if (OutputRefIt->GetPass())
			{
				if (!OutputRefIt)
				{
					// Pass doesn't have more inputs
					break;
				}

				FRenderingCompositeOutput* Input = OutputRefIt->GetOutput();

				// to track down an issue, should never happen
				assert(OutputRefIt->GetPass());

// 				if (GRenderTargetPool.IsEventRecordingEnabled())
// 				{
// 					GRenderTargetPool.AddPhaseEvent(*Pass->ConstructDebugName());
// 				}

				Context.Pass = Pass;
				RecursivelyProcess(*OutputRefIt, Context);
			}
		}
	}

	Context.Pass = Pass;
	Context.SetViewportInvalid();

	// then process the pass itself
	Pass->Process(Context);

	// iterate through all inputs of this pass and decrement the references for it's inputs
	// this can release some intermediate RT so they can be reused
	{
		uint32 InputId = 0;

		while (const FRenderingCompositeOutputRef* OutputRefIt = Pass->GetDependency(InputId++))
		{
			FRenderingCompositeOutput* Input = OutputRefIt->GetOutput();

			if (Input)
			{
				Input->ResolveDependencies();
			}
		}
	}
}

int32 FRenderingCompositionGraph::ComputeUniquePassId(FRenderingCompositePass* Pass) const
{
	for (uint32 i = 0; i < (uint32)Nodes.size(); ++i)
	{
		FRenderingCompositePass *Element = Nodes[i];

		if (Element == Pass)
		{
			return i;
		}
	}

	return -1;
}

int32 FRenderingCompositionGraph::ComputeUniqueOutputId(FRenderingCompositePass* Pass, EPassOutputId OutputId) const
{
	uint32 Ret = Nodes.size();

	for (uint32 i = 0; i < (uint32)Nodes.size(); ++i)
	{
		FRenderingCompositePass *Element = Nodes[i];

		if (Element == Pass)
		{
			return (int32)(Ret + (uint32)OutputId);
		}

		uint32 OutputCount = 0;
		while (Pass->GetOutput((EPassOutputId)OutputCount))
		{
			++OutputCount;
		}

		Ret += OutputCount;
	}

	return -1;
}

FRenderingCompositePassContext::FRenderingCompositePassContext(const FViewInfo& InView)
	: View(InView)
	, SceneColorViewRect(InView.ViewRect)
	//, ViewState((FSceneViewState*)InView.State)
	, Pass(0)
	, ViewPortRect(0, 0, 0, 0)
	, ShaderMap(InView.ShaderMap)
	, bWasProcessed(false)
	, bHasHmdMesh(false)
{

}

FRenderingCompositePassContext::~FRenderingCompositePassContext()
{
	Graph.Free();
}

void FRenderingCompositePassContext::Process(const std::vector<FRenderingCompositePass*>& TargetedRoots, const TCHAR *GraphDebugName)
{
	assert(!bWasProcessed);

	bWasProcessed = true;

	// query if we have a custom HMD post process mesh to use
	//static const auto* const HiddenAreaMaskCVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("vr.HiddenAreaMask"));
	bHasHmdMesh = false;// (HiddenAreaMaskCVar != nullptr &&
// 		HiddenAreaMaskCVar->GetValueOnRenderThread() == 1 &&
// 		GEngine &&
// 		GEngine->XRSystem.IsValid() && GEngine->XRSystem->GetHMDDevice() &&
// 		GEngine->XRSystem->GetHMDDevice()->HasVisibleAreaMesh());

	if (TargetedRoots.size() == 0)
	{
		return;
	}

	bool bNewOrder = 1;// CVarCompositionGraphOrder.GetValueOnRenderThread() != 0;

	for (FRenderingCompositePass* Root : TargetedRoots)
	{
		Graph.RecursivelyGatherDependencies(Root);
	}

	if (bNewOrder)
	{
		// process in the order the nodes have been created (for more control), unless the dependencies require it differently
		for (FRenderingCompositePass* Node : Graph.Nodes)
		{
			// only if this is true the node is actually needed - no need to compute it when it's not needed
			if (Node->WasComputeOutputDescCalled())
			{
				Graph.RecursivelyProcess(Node, *this);
			}
		}
	}
	else
	{
		// process in the order of the dependencies, starting from the root (without processing unreferenced nodes)
		for (FRenderingCompositePass* Root : TargetedRoots)
		{
			Graph.RecursivelyProcess(Root, *this);
		}
	}

}

bool FRenderingCompositePassContext::IsViewFamilyRenderTarget(const PooledRenderTarget& DestRenderTarget) const
{
	assert(DestRenderTarget.ShaderResourceTexture);
	return DestRenderTarget.ShaderResourceTexture == View.Family->RenderTarget->GetRenderTargetTexture();
}

uint32 FRenderingCompositePass::ComputeInputCount()
{
	for (uint32 i = 0; ; ++i)
	{
		if (!GetInput((EPassInputId)i))
		{
			return i;
		}
	}
}

uint32 FRenderingCompositePass::ComputeOutputCount()
{
	for (uint32 i = 0; ; ++i)
	{
		if (!GetOutput((EPassOutputId)i))
		{
			return i;
		}
	}
}

const PooledRenderTargetDesc* FRenderingCompositePass::GetInputDesc(EPassInputId InPassInputId) const
{
	// to overcome const issues, this way it's kept local
	FRenderingCompositePass* This = (FRenderingCompositePass*)this;

	const FRenderingCompositeOutputRef* OutputRef = This->GetInput(InPassInputId);

	if (!OutputRef)
	{
		return 0;
	}

	FRenderingCompositeOutput* Input = OutputRef->GetOutput();

	if (!Input)
	{
		return 0;
	}

	return &Input->RenderTargetDesc;
}

FRenderingCompositePass* FRenderingCompositeOutputRef::GetPass() const
{
	return Source;
}

FRenderingCompositeOutput* FRenderingCompositeOutputRef::GetOutput() const
{
	if (Source == 0)
	{
		return 0;
	}

	return Source->GetOutput(PassOutputId);
}

void CompositionGraph_OnStartFrame()
{

}

const PooledRenderTarget& FRenderingCompositeOutput::RequestSurface(const FRenderingCompositePassContext& Context)
{
	if (RenderTarget)
	{
		//Context.RHICmdList.TransitionResource(EResourceTransitionAccess::EWritable, PooledRenderTarget->GetRenderTargetItem().TargetableTexture);
		return *RenderTarget.Get();
	}

// 	if (!RenderTargetDesc.IsValid())
// 	{
// 		// useful to use the CompositingGraph dependency resolve but pass the data between nodes differently
// 		static PooledRenderTarget Null;
// 
// 		return Null;
// 	}

	if (!RenderTarget)
	{
		GRenderTargetPool.FindFreeElement(RenderTargetDesc, RenderTarget,TEXT("DebugName") /*RenderTargetDesc.DebugName*/);
	}

	assert(!RenderTarget->IsFree());

	PooledRenderTarget& RenderTargetItem = *RenderTarget.Get();

	return RenderTargetItem;
}

void FPostProcessPassParameters::Bind(const FShaderParameterMap& ParameterMap)
{
	BilinearTextureSampler0.Bind(ParameterMap, ("BilinearTextureSampler0"));
	BilinearTextureSampler1.Bind(ParameterMap, ("BilinearTextureSampler1"));
	ViewportSize.Bind(ParameterMap, ("ViewportSize"));
	ViewportRect.Bind(ParameterMap, ("ViewportRect"));
	ScreenPosToPixel.Bind(ParameterMap, ("ScreenPosToPixel"));
	SceneColorBufferUVViewport.Bind(ParameterMap, ("SceneColorBufferUVViewport"));

	for (uint32 i = 0; i < ePId_Input_MAX; ++i)
	{
		std::string PostprocessInput = std::string("PostprocessInput") + std::to_string(i);
		std::string PostprocessInputSampler = PostprocessInput + std::string("Sampler");
		std::string PostprocessInputSize = PostprocessInput + std::string("Size");
		std::string PostprocessInputMinMax = PostprocessInput + std::string("MinMax");
		PostprocessInputParameter[i].Bind(ParameterMap, PostprocessInput.c_str());
		PostprocessInputParameterSampler[i].Bind(ParameterMap, PostprocessInputSampler.c_str());
		PostprocessInputSizeParameter[i].Bind(ParameterMap, PostprocessInputSize.c_str());
		PostProcessInputMinMaxParameter[i].Bind(ParameterMap, PostprocessInputMinMax.c_str());
	}
}

void FPostProcessPassParameters::SetPS(ID3D11PixelShader* const ShaderRHI, const FRenderingCompositePassContext& Context, ID3D11SamplerState* Filter /*= TStaticSamplerState<>::GetRHI()*/, EFallbackColor FallbackColor /*= eFC_0000*/, ID3D11SamplerState** FilterOverrideArray /*= 0*/)
{

}

void FPostProcessPassParameters::SetCS(ID3D11ComputeShader* const ShaderRHI, const FRenderingCompositePassContext& Context, ID3D11SamplerState* Filter /*= TStaticSamplerState<>::GetRHI()*/, EFallbackColor FallbackColor /*= eFC_0000*/, ID3D11SamplerState** FilterOverrideArray /*= 0*/)
{

}

void FPostProcessPassParameters::SetVS(ID3D11VertexShader* const ShaderRHI, const FRenderingCompositePassContext& Context, ID3D11SamplerState* Filter /*= TStaticSamplerState<>::GetRHI()*/, EFallbackColor FallbackColor /*= eFC_0000*/, ID3D11SamplerState** FilterOverrideArray /*= 0*/)
{

}

template< typename TShaderRHIParamRef>
void FPostProcessPassParameters::Set(const TShaderRHIParamRef& ShaderRHI, const FRenderingCompositePassContext& Context, ID3D11SamplerState* Filter, EFallbackColor FallbackColor, ID3D11SamplerState** FilterOverrideArray /*= 0 */)
{

}
