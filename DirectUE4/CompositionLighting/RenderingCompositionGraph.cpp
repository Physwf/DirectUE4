#include "RenderingCompositionGraph.h"
#include "RenderTargetPool.h"
#include "Viewport.h"
#include "SystemTextures.h"

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
	ReferenceBufferSize = FSceneRenderTargets::Get().GetBufferSizeXY();
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
	Set(ShaderRHI, Context, Filter, FallbackColor, FilterOverrideArray);
}

void FPostProcessPassParameters::SetCS(ID3D11ComputeShader* const ShaderRHI, const FRenderingCompositePassContext& Context, ID3D11SamplerState* Filter /*= TStaticSamplerState<>::GetRHI()*/, EFallbackColor FallbackColor /*= eFC_0000*/, ID3D11SamplerState** FilterOverrideArray /*= 0*/)
{

}

void FPostProcessPassParameters::SetVS(ID3D11VertexShader* const ShaderRHI, const FRenderingCompositePassContext& Context, ID3D11SamplerState* Filter /*= TStaticSamplerState<>::GetRHI()*/, EFallbackColor FallbackColor /*= eFC_0000*/, ID3D11SamplerState** FilterOverrideArray /*= 0*/)
{
	Set(ShaderRHI, Context, Filter, FallbackColor, FilterOverrideArray);
}

template< typename TShaderRHIParamRef>
void FPostProcessPassParameters::Set(const TShaderRHIParamRef& ShaderRHI, const FRenderingCompositePassContext& Context, ID3D11SamplerState* Filter, EFallbackColor FallbackColor, ID3D11SamplerState** FilterOverrideArray /*= 0 */)
{
	// assuming all outputs have the same size
	FRenderingCompositeOutput* Output = Context.Pass->GetOutput(ePId_Output0);

	// Output0 should always exist
	assert(Output);

	// one should be on
	assert(FilterOverrideArray || Filter);
	// but not both
	assert(!FilterOverrideArray || !Filter);

	if (BilinearTextureSampler0.IsBound())
	{
		SetShaderSampler(
			ShaderRHI,
			BilinearTextureSampler0.GetBaseIndex(),
			TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT>::GetRHI()
		);
	}

	if (BilinearTextureSampler1.IsBound())
	{
		SetShaderSampler(
			ShaderRHI,
			BilinearTextureSampler1.GetBaseIndex(),
			TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT>::GetRHI()
		);
	}

	if (ViewportSize.IsBound() || ScreenPosToPixel.IsBound() || ViewportRect.IsBound())
	{
		FIntRect LocalViewport = Context.GetViewport();

		FIntPoint ViewportOffset = LocalViewport.Min;
		FIntPoint ViewportExtent = LocalViewport.Size();

		{
			Vector4 Value(ViewportExtent.X, ViewportExtent.Y, 1.0f / ViewportExtent.X, 1.0f / ViewportExtent.Y);

			SetShaderValue(ShaderRHI, ViewportSize, Value);
		}

		{
			SetShaderValue(ShaderRHI, ViewportRect, Context.GetViewport());
		}

		{
			Vector4 ScreenPosToPixelValue(
				ViewportExtent.X * 0.5f,
				-ViewportExtent.Y * 0.5f,
				ViewportExtent.X * 0.5f - 0.5f + ViewportOffset.X,
				ViewportExtent.Y * 0.5f - 0.5f + ViewportOffset.Y);
			SetShaderValue(ShaderRHI, ScreenPosToPixel, ScreenPosToPixelValue);
		}
	}

	//Calculate a base scene texture min max which will be pulled in by a pixel for each PP input.
	FIntRect ContextViewportRect = Context.IsViewportValid() ? Context.SceneColorViewRect : FIntRect(0, 0, 0, 0);
	const FIntPoint SceneRTSize = Context.ReferenceBufferSize;
	Vector4 BaseSceneTexMinMax(((float)ContextViewportRect.Min.X / SceneRTSize.X),
		((float)ContextViewportRect.Min.Y / SceneRTSize.Y),
		((float)ContextViewportRect.Max.X / SceneRTSize.X),
		((float)ContextViewportRect.Max.Y / SceneRTSize.Y));

	if (SceneColorBufferUVViewport.IsBound())
	{
		Vector4 SceneColorBufferUVViewportValue(
			ContextViewportRect.Width() / float(SceneRTSize.X), ContextViewportRect.Height() / float(SceneRTSize.Y),
			BaseSceneTexMinMax.X, BaseSceneTexMinMax.Y);
		SetShaderValue(ShaderRHI, SceneColorBufferUVViewport, SceneColorBufferUVViewportValue);
	}

	PooledRenderTarget* FallbackTexture = 0;

	switch (FallbackColor)
	{
	case eFC_0000: FallbackTexture = GSystemTextures.BlackDummy.Get(); break;
	case eFC_0001: FallbackTexture = GSystemTextures.BlackAlphaOneDummy.Get(); break;
	case eFC_1111: FallbackTexture = GSystemTextures.WhiteDummy.Get(); break;
	default:
		assert(!"Unhandled enum in EFallbackColor");
	}

	for (uint32 Id = 0; Id < (uint32)ePId_Input_MAX; ++Id)
	{
		FRenderingCompositeOutputRef* OutputRef = Context.Pass->GetInput((EPassInputId)Id);

		if (!OutputRef)
		{
			// Pass doesn't have more inputs
			break;
		}


		FRenderingCompositeOutput* Input = OutputRef->GetOutput();

		ComPtr<PooledRenderTarget> InputPooledElement;

		if (Input)
		{
			InputPooledElement = Input->RequestInput();
		}

		ID3D11SamplerState* LocalFilter = FilterOverrideArray ? FilterOverrideArray[Id] : Filter;

		if (InputPooledElement)
		{
			assert(!InputPooledElement->IsFree());

			const std::shared_ptr<FD3D11Texture>& SrcTexture = InputPooledElement->ShaderResourceTexture;

			SetTextureParameter(ShaderRHI, PostprocessInputParameter[Id], PostprocessInputParameterSampler[Id], LocalFilter, SrcTexture->GetShaderResourceView());

			if (PostprocessInputSizeParameter[Id].IsBound() || PostProcessInputMinMaxParameter[Id].IsBound())
			{
				float Width = InputPooledElement->GetDesc().Extent.X;
				float Height = InputPooledElement->GetDesc().Extent.Y;

				Vector2 OnePPInputPixelUVSize = Vector2(1.0f / Width, 1.0f / Height);

				Vector4 TextureSize(Width, Height, OnePPInputPixelUVSize.X, OnePPInputPixelUVSize.Y);
				SetShaderValue(ShaderRHI, PostprocessInputSizeParameter[Id], TextureSize);

				//We could use the main scene min max here if it weren't that we need to pull the max in by a pixel on a per input basis.
				Vector4 PPInputMinMax = BaseSceneTexMinMax;
				PPInputMinMax.X += 0.5f * OnePPInputPixelUVSize.X;
				PPInputMinMax.Y += 0.5f * OnePPInputPixelUVSize.Y;
				PPInputMinMax.Z -= 0.5f * OnePPInputPixelUVSize.X;
				PPInputMinMax.W -= 0.5f * OnePPInputPixelUVSize.Y;
				SetShaderValue(ShaderRHI, PostProcessInputMinMaxParameter[Id], PPInputMinMax);
			}
		}
		else
		{
			// if the input is not there but the shader request it we give it at least some data to avoid d3ddebug errors and shader permutations
			// to make features optional we use default black for additive passes without shader permutations
			SetTextureParameter(ShaderRHI, PostprocessInputParameter[Id], PostprocessInputParameterSampler[Id], LocalFilter, FallbackTexture->TargetableTexture->GetShaderResourceView());

			Vector4 Dummy(1, 1, 1, 1);
			SetShaderValue(ShaderRHI, PostprocessInputSizeParameter[Id], Dummy);
			SetShaderValue(ShaderRHI, PostProcessInputMinMaxParameter[Id], Dummy);
		}
	}

}
