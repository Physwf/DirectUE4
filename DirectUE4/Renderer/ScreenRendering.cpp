#include "ScreenRendering.h"

// Shader implementations.
IMPLEMENT_SHADER_TYPE(, FScreenPS, ("ScreenPixelShader.usf"), ("Main"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(, FScreenVS, ("ScreenVertexShader.usf"), ("Main"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(template<>, TScreenVSForGS<false>, ("ScreenVertexShader.usf"), ("MainForGS"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(template<>, TScreenVSForGS<true>, ("ScreenVertexShader.usf"), ("MainForGS"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(, FScreenPS_OSE, ("ScreenPixelShaderOES.usf"), ("Main"), SF_Pixel);