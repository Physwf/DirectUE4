#include "RenderingCompositionGraph.h"

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

void FRenderingCompositionGraph::ProcessGatherDependency(const FRenderingCompositeOutputRef* OutputRefIt)
{

}

void FRenderingCompositionGraph::RecursivelyGatherDependencies(FRenderingCompositePass *Pass)
{

}

void FRenderingCompositionGraph::RecursivelyProcess(const FRenderingCompositeOutputRef& InOutputRef, FRenderingCompositePassContext& Context) const
{

}

int32 FRenderingCompositionGraph::ComputeUniquePassId(FRenderingCompositePass* Pass) const
{

}

int32 FRenderingCompositionGraph::ComputeUniqueOutputId(FRenderingCompositePass* Pass, EPassOutputId OutputId) const
{

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

void FRenderingCompositePassContext::Process(const TArray<FRenderingCompositePass*>& TargetedRoots, const TCHAR *GraphDebugName)
{

}

bool FRenderingCompositePassContext::IsViewFamilyRenderTarget(const FSceneRenderTargetItem& DestRenderTarget) const
{

}

uint32 FRenderingCompositePass::ComputeInputCount()
{

}

uint32 FRenderingCompositePass::ComputeOutputCount()
{

}

const PooledRenderTargetDesc* FRenderingCompositePass::GetInputDesc(EPassInputId InPassInputId) const
{

}

FRenderingCompositePass* FRenderingCompositeOutputRef::GetPass() const
{

}

FRenderingCompositeOutput* FRenderingCompositeOutputRef::GetOutput() const
{

}

void CompositionGraph_OnStartFrame()
{

}
