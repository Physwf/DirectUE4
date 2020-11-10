#pragma once

class ViewInfo;

class CompositionLighting
{
public:
	void Init();

	void ProcessBeforeBasePass(ViewInfo& View);

	void ProcessAfterBasePass(ViewInfo& View);

	//void ProcessLpvIndirect(FRHICommandListImmediate& RHICmdList, FViewInfo& View);

	void ProcessAfterLighting(ViewInfo& View);
};

extern CompositionLighting GCompositionLighting;