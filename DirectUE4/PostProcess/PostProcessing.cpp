#include "PostProcessing.h"
#include "RenderTargets.h"
#include "PostProcessInput.h"
#include "RenderingCompositionGraph.h"
#include "PostProcessTemporalAA.h"
#include "PostProcessTonemap.h"
#include "PostProcessCombineLUTs.h"
#include "PostProcessDownsample.h"
#include "Scene.h"
#include "SystemTextures.h"
#include "Viewport.h"

IMPLEMENT_SHADER_TYPE(, FPostProcessVS, "PostProcessBloom.dusf", "MainPostprocessCommonVS", SF_Vertex);


FPostprocessContext::FPostprocessContext(FRenderingCompositionGraph& InGraph, const FViewInfo& InView)
	: Graph(InGraph)
	, View(InView)
	, SceneColor(0)
	, SceneDepth(0)
{
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();
	if (SceneContext.IsSceneColorAllocated())
	{
		SceneColor = Graph.RegisterPass(new FRCPassPostProcessInput(SceneContext.GetSceneColor()));
	}

	SceneDepth = Graph.RegisterPass(new FRCPassPostProcessInput(SceneContext.SceneDepthZ));

	FinalOutput = FRenderingCompositeOutputRef(SceneColor);
}

bool FPostProcessing::AllowFullPostProcessing(const FViewInfo& View)
{
	return true;
}


static FTAAPassParameters MakeTAAPassParametersForView(const FViewInfo& View)
{
	FTAAPassParameters Parameters(View);

	Parameters.Pass = View.PrimaryScreenPercentageMethod == EPrimaryScreenPercentageMethod::TemporalUpscale
		? ETAAPassConfig::MainUpsampling
		: ETAAPassConfig::Main;

	Parameters.bIsComputePass = Parameters.Pass == ETAAPassConfig::MainUpsampling
		? true // TAAU is always a compute shader
		: false/*ShouldDoComputePostProcessing(View)*/;

	Parameters.SetupViewRect(View);

	{
		static const auto CVar = 4;// IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.PostProcessAAQuality"));
		uint32 Quality = FMath::Clamp(CVar/*->GetValueOnRenderThread()*/, 1, 6);
		Parameters.bUseFast = Quality == 3;
	}

	return Parameters;
}

static void AddTemporalAA(FPostprocessContext& Context, FRenderingCompositeOutputRef& VelocityInput, const FTAAPassParameters& Parameters, FRenderingCompositeOutputRef* OutSceneColorHalfRes)
{
	assert(VelocityInput.IsValid());
	assert(Context.View.ViewState);

	FSceneViewState* ViewState = Context.View.ViewState;

	FRCPassPostProcessTemporalAA* TemporalAAPass = Context.Graph.RegisterPass(new FRCPassPostProcessTemporalAA(
		Context,
		Parameters,
		Context.View.PrevViewInfo.TemporalAAHistory,
		&ViewState->PendingPrevFrameViewInfo.TemporalAAHistory));

	TemporalAAPass->SetInput(ePId_Input0, Context.FinalOutput);
	TemporalAAPass->SetInput(ePId_Input2, VelocityInput);
	Context.FinalOutput = FRenderingCompositeOutputRef(TemporalAAPass);

	if (OutSceneColorHalfRes && TemporalAAPass->IsDownsamplePossible())
	{
		*OutSceneColorHalfRes = FRenderingCompositeOutputRef(TemporalAAPass, ePId_Output2);
	}
}

static FRCPassPostProcessTonemap* AddTonemapper(
	FPostprocessContext& Context,
	const FRenderingCompositeOutputRef& BloomOutputCombined,
	const FRenderingCompositeOutputRef& EyeAdaptation,
	const EAutoExposureMethod& EyeAdapationMethodId,
	const bool bDoGammaOnly,
	const bool bHDRTonemapperOutput)
{
	const FViewInfo& View = Context.View;
	//const EStereoscopicPass StereoPass = View.StereoPass;

	//const FEngineShowFlags& EngineShowFlags = View.Family->EngineShowFlags;
	const bool bIsComputePass = false;// ShouldDoComputePostProcessing(View);

	FRenderingCompositeOutputRef TonemapperCombinedLUTOutputRef;
	//if (StereoPass != eSSP_RIGHT_EYE)
	{
		FRenderingCompositePass* CombinedLUT = Context.Graph.RegisterPass(new FRCPassPostProcessCombineLUTs(View.State == nullptr, bIsComputePass));
		TonemapperCombinedLUTOutputRef = FRenderingCompositeOutputRef(CombinedLUT);
	}

	const bool bDoEyeAdaptation = false;//IsAutoExposureMethodSupported(View.GetFeatureLevel(), EyeAdapationMethodId);
	FRCPassPostProcessTonemap* PostProcessTonemap = Context.Graph.RegisterPass(new FRCPassPostProcessTonemap(View, bDoGammaOnly, bDoEyeAdaptation, bHDRTonemapperOutput, bIsComputePass));

	PostProcessTonemap->SetInput(ePId_Input0, Context.FinalOutput);
	PostProcessTonemap->SetInput(ePId_Input1, BloomOutputCombined);
	PostProcessTonemap->SetInput(ePId_Input2, EyeAdaptation);
	PostProcessTonemap->SetInput(ePId_Input3, TonemapperCombinedLUTOutputRef);

	Context.FinalOutput = FRenderingCompositeOutputRef(PostProcessTonemap);

	return PostProcessTonemap;
}

// could be moved into the graph
// allows for Framebuffer blending optimization with the composition graph
void OverrideRenderTarget(FRenderingCompositeOutputRef It, ComPtr<PooledRenderTarget>& RT, PooledRenderTargetDesc& Desc)
{
	for (;;)
	{
		It.GetOutput()->RenderTarget = RT;
		It.GetOutput()->RenderTargetDesc = Desc;

		if (!It.GetPass()->FrameBufferBlendingWithInput0())
		{
			break;
		}

		It = *It.GetPass()->GetInput(ePId_Input0);
	}
}

void FPostProcessing::Process(const FViewInfo& View, ComPtr<PooledRenderTarget>& VelocityRT)
{
	{
		FRenderingCompositePassContext CompositeContext(View);

		FPostprocessContext Context(CompositeContext.Graph, View);

		class FAutoExposure
		{
		public:
			FAutoExposure(const FViewInfo& InView) :
				MethodId(EAutoExposureMethod::AEM_Histogram/*GetAutoExposureMethod(InView)*/)
			{}
			// distinguish between Basic and Histogram-based
			EAutoExposureMethod          MethodId;
			// not always valid
			FRenderingCompositeOutputRef EyeAdaptation;
		} AutoExposure(View);

		// optional
		FRenderingCompositeOutputRef BloomOutputCombined;

		FSceneViewState* ViewState = (FSceneViewState*)Context.View.State;

		const bool bHDRTonemapperOutput = false;// bAllowTonemapper && (GetHighResScreenshotConfig().bCaptureHDR || CVarDumpFramesAsHDR->GetValueOnRenderThread() || bHDROutputEnabled);
		FRCPassPostProcessTonemap* Tonemapper = 0;

		if (AllowFullPostProcessing(View))
		{
			EAntiAliasingMethod AntiAliasingMethod = Context.View.AntiAliasingMethod;

			const int32 DownsampleQuality = FMath::Clamp(3, 0, 1);

			FRenderingCompositeOutputRef SceneColorHalfRes;
			const EPixelFormat SceneColorHalfResFormat = PF_FloatRGB;

			if (AntiAliasingMethod == AAM_TemporalAA && ViewState)
			{
				FTAAPassParameters Parameters = MakeTAAPassParametersForView(Context.View);

				const bool bDownsampleDuringTemporalAA = false;//(CVarTemporalAAAllowDownsampling.GetValueOnRenderThread() != 0)
// 					&& !IsMotionBlurEnabled(View)
// 					&& !bVisualizeMotionBlur
// 					&& Parameters.bIsComputePass
// 					&& (DownsampleQuality == 0);

				Parameters.bDownsample = bDownsampleDuringTemporalAA;
				Parameters.DownsampleOverrideFormat = SceneColorHalfResFormat;

// 				if (VelocityInput.IsValid())
// 				{
// 					AddTemporalAA(Context, VelocityInput, Parameters, bDownsampleDuringTemporalAA ? &SceneColorHalfRes : nullptr);
// 				}
// 				else
				{
					// black is how we clear the velocity buffer so this means no velocity
					FRenderingCompositePass* NoVelocity = Context.Graph.RegisterPass(new FRCPassPostProcessInput(GSystemTextures.BlackDummy));
					FRenderingCompositeOutputRef NoVelocityRef(NoVelocity);
					AddTemporalAA(Context, NoVelocityRef, Parameters, bDownsampleDuringTemporalAA ? &SceneColorHalfRes : nullptr);
				}
			}

			// Down sample Scene color from full to half res (this may have been done during TAA).
			if (!SceneColorHalfRes.IsValid())
			{
				// doesn't have to be as high quality as the Scene color
				const bool bIsComputePass = false;// ShouldDoComputePostProcessing(Context.View);

				FRenderingCompositePass* HalfResPass = Context.Graph.RegisterPass(new FRCPassPostProcessDownsample(SceneColorHalfResFormat, DownsampleQuality, bIsComputePass, TEXT("SceneColorHalfRes")));
				HalfResPass->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));

				SceneColorHalfRes = FRenderingCompositeOutputRef(HalfResPass);
			}

			Tonemapper = AddTonemapper(Context, BloomOutputCombined, AutoExposure.EyeAdaptation, AutoExposure.MethodId, false, bHDRTonemapperOutput);
		}

		{
			// currently created on the heap each frame but View.Family->RenderTarget could keep this object and all would be cleaner
			ComPtr<PooledRenderTarget> Temp;
			

			PooledRenderTargetDesc Desc;

			// Texture could be bigger than viewport
			if (View.Family->RenderTarget->GetRenderTargetTexture())
			{
				Desc.Extent.X = View.Family->RenderTarget->GetRenderTargetTexture()->GetSizeX();
				Desc.Extent.Y = View.Family->RenderTarget->GetRenderTargetTexture()->GetSizeY();
			}
			else
			{
				Desc.Extent = View.Family->RenderTarget->GetSizeXY();
			}

			const bool bIsFinalOutputComputePass = Context.FinalOutput.IsComputePass();
			Desc.TargetableFlags |= bIsFinalOutputComputePass ? TexCreate_UAV : TexCreate_RenderTargetable;
			Desc.Format = bIsFinalOutputComputePass ? PF_R8G8B8A8 : PF_B8G8R8A8;

			// todo: this should come from View.Family->RenderTarget
			Desc.Format = /*bHDROutputEnabled*/true ? GRHIHDRDisplayOutputFormat : Desc.Format;
			Desc.NumMips = 1;
			Desc.DebugName = TEXT("FinalPostProcessColor");

			//GRenderTargetPool.CreateUntrackedElement(Desc, Temp, Item);
			Temp = new PooledRenderTarget(Desc, NULL);

			Temp->TargetableTexture = /*(FTextureRHIRef&)*/View.Family->RenderTarget->GetRenderTargetTexture();
			Temp->ShaderResourceTexture = /* (FTextureRHIRef&)*/View.Family->RenderTarget->GetRenderTargetTexture();
			//Item.UAV = View.Family->RenderTarget->GetRenderTargetUAV();

			OverrideRenderTarget(Context.FinalOutput, Temp, Desc);

			std::vector<FRenderingCompositePass*> TargetedRoots;
			TargetedRoots.push_back(Context.FinalOutput.GetPass());

			CompositeContext.Process(TargetedRoots, TEXT("PostProcessing"));
		}


	}

}

FPostProcessing GPostProcessing;