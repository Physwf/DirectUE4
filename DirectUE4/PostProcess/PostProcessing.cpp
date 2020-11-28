#include "PostProcessing.h"
#include "RenderTargets.h"


PostProcessContext::PostProcessContext()
{

}

IMPLEMENT_SHADER_TYPE(, FPostProcessVS, "PostProcessBloom.dusf", "MainPostprocessCommonVS", SF_Vertex);

