#pragma once

class FViewInfo;

class CompositionLighting
{
public:
	void Init();

	void ProcessBeforeBasePass(FViewInfo& View);

	void ProcessAfterBasePass(FViewInfo& View);

	//void ProcessLpvIndirect(FRHICommandListImmediate& RHICmdList, FViewInfo& View);

	void ProcessAfterLighting(FViewInfo& View);
};

extern CompositionLighting GCompositionLighting;