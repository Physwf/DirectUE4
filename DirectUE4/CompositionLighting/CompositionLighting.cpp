#include "CompositionLighting.h"
#include "RenderTargets.h"
#include "PostProcessAmbientOcclution.h"
#include "SceneOcclusion.h"
#include "GPUProfiler.h"

RCPassPostProcessAmbientOcclusionSetup AOSetup1;
RCPassPostProcessAmbientOcclusionSetup AOSetup2;
RCPassPostProcessAmbientOcclusion AO0;
RCPassPostProcessAmbientOcclusion AO1;
RCPassPostProcessAmbientOcclusion AO2;
RCPassPostProcessBasePassAO BasePassAO;

void CompositionLighting::Init()
{
	/*
	int Levels = 2;

	RenderTargets& SceneContext = RenderTargets::Get();

	if (Levels >= 2)
	{
		AOSetup1.Inputs[0] = SceneContext.GetSceneDepthTexture();
		AOSetup1.Inputs[1] = NULL;
		AOSetup1.Init();
	}

	if (Levels >= 3)
	{
		AOSetup2.Inputs[0] = NULL;
		AOSetup2.Inputs[1] = AOSetup1.Output;
		AOSetup2.Init();
	}

	// upsample from lower resolution

	if (Levels >= 3)
	{
		AO2.Inputs[0] = AOSetup2.Output;
		AO2.Inputs[1] = AOSetup2.Output;
		AO2.Inputs[2] = NULL;
		//AO2.Inputs[3] = HZBSRV;
		AO2.Init();
	}

	if (Levels >= 2)
	{
		AO1.Inputs[0] = AOSetup1.Output;
		AO1.Inputs[1] = AOSetup1.Output;
		AO1.Inputs[2] = AO2.Output;
		//AO1.Inputs[3] = HZBSRV;
		AO1.Init();
	}

	AO0.Inputs[0] = SceneContext.GetGBufferATexture();
	AO0.Inputs[1] = AOSetup1.Output;
	AO0.Inputs[2] = AO1.Output;
	//AO0.Inputs[3] = HZBSRV;
	AO0.Init(false);

	BasePassAO.Init();

	*/
}

void CompositionLighting::ProcessBeforeBasePass(ViewInfo& View)
{
	
}

void CompositionLighting::ProcessAfterBasePass(ViewInfo& View)
{
	SCOPED_DRAW_EVENT_FORMAT(ProcessAfterBasePass,TEXT("ProcessAfterBasePass"));
	int Levels = 2;

	if (Levels >= 3)
	{
		AOSetup2.Process(View);
	}
	if (Levels >= 2)
	{
		AOSetup1.Process(View);
	}

	// upsample from lower resolution
	if (Levels >= 3)
	{
		AO2.Process(View);
	}
	if (Levels >= 2)
	{
		AO1.Process(View);
	}

	AO0.Process(View);

	BasePassAO.Process(View);
}

void CompositionLighting::ProcessAfterLighting(ViewInfo& View)
{

}

CompositionLighting GCompositionLighting;