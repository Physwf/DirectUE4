#include "Common.hlsl"
#include "SceneTexturesCommon.hlsl"
//#include "SHCommon.ush"
#include "AtmosphereCommon.hlsl"

#ifndef ATMOSPHERIC_NO_LIGHT_SHAFT
#define ATMOSPHERIC_NO_LIGHT_SHAFT	0
#endif

#if !ATMOSPHERIC_NO_LIGHT_SHAFT
/** Result of the previous pass, rgb contains bloom color and a contains an occlusion mask. */
Texture2D OcclusionTexture;
SamplerState OcclusionTextureSampler;

static const float OcclusionMaskDarkness = 0.0;
#endif


void VS_Main(
    in float2 InPosition : ATTRIBUTE0, 
    out float2 OutTexCoord : TEXCOORD0,
    out float4 OutScreenVector : TEXCOORD1,
    out float4 OutPosition : SV_Position
)
{
    OutPosition = float4(InPosition,0,1);

    OutTexCoord = InPosition * View.ScreenPositionScaleBias.xy + View.ScreenPositionScaleBias.wz;

    OutScreenVector = mul(float4(InPosition,1,0),View.ScreenToTranslatedWorld);

}


void PS_Main(
    float2 TexCoord : TEXCOORD0,
    float4 ScreenVector : TEXCOORD1,
    out float4 OutColor : SV_Target0
)
{
    OutColor = GetAtmosphericFog(View.WorldCameraOrigin,ScreenVector.xyz,CalcSceneDepth(TexCoord),float3(0,0,0));
#if !ATMOSPHERIC_NO_LIGHT_SHAFT
    float LightShaftMask = Texture2DSample(OcclusionTexture, OcclusionTextureSampler, TexCoord).x;
    OutColor.rgb = OutColor.rgb * LightShaftMask;
#endif

}