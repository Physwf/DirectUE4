#include "PostProcessing.h"
#include "RenderTargets.h"


PostProcessContext::PostProcessContext()
{

}

IMPLEMENT_SHADER_TYPE(, FPostProcessVS, "PostProcessBloom.hlsl", "MainPostprocessCommonVS", SF_Vertex);

