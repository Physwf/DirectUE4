#include "ScreenRendering.h"

// Shader implementations.
IMPLEMENT_SHADER_TYPE(, FScreenPS, ("ScreenPixelShader.dusf"), ("Main"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(, FScreenVS, ("ScreenVertexShader.dusf"), ("Main"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(template<>, TScreenVSForGS<false>, ("ScreenVertexShader.dusf"), ("MainForGS"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(template<>, TScreenVSForGS<true>, ("ScreenVertexShader.dusf"), ("MainForGS"), SF_Vertex);
