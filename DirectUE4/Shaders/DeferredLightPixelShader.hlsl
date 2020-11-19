#include "Common.hlsl"
#include "DeferredShadingCommon.hlsl"
#include "DeferredLightingCommon.hlsl"

DeferredLightData SetupLightDataForStandardDeferred()
{
    DeferredLightData LightData;
    LightData.LightPositionAndInvRadius = float4(DeferredLightUniform.LightPosition,DeferredLightUniform.LightInvRadius);
    LightData.LightColorAndFalloffExponent = float4(DeferredLightUniform.LightColor,DeferredLightUniform.LightFalloffExponent);
    LightData.LightDirection = DeferredLightUniform.NormalizedLightDirection;
    LightData.LightTangent = DeferredLightUniform.NormalizedLightTangent;
    LightData.SpotAnglesAndSourceRadius = float4(DeferredLightUniform.SpotAngles,DeferredLightUniform.SourceRadius,DeferredLightUniform.SourceLength);
    LightData.SoftSourceRadius = DeferredLightUniform.SoftSourceRadius;
    LightData.SpecularScale = DeferredLightUniform.SpecularScale;
    LightData.ContactShadowLength = abs(DeferredLightUniform.ContactShadowLength);
    LightData.ContactShadowLengthInWS = DeferredLightUniform.ContactShadowLength < 0.0f;
    LightData.DistanceFadeMAD = DeferredLightUniform.DistanceFadeMAD;
    LightData.ShadowMapChannelMask = DeferredLightUniform.ShadowMapChannelMask;
    //DeferredLightUniformsValue.ShadowedBits  = LightSceneInfo->Proxy->CastsStaticShadow() || bHasLightFunction ? 1 : 0;
	//DeferredLightUniformsValue.ShadowedBits |= LightSceneInfo->Proxy->CastsDynamicShadow() && View.Family->EngineShowFlags.DynamicShadows ? 3 : 0;
    LightData.ShadowedBits = DeferredLightUniform.ShadowedBits;

    // enum class ELightSourceShape
    // {
    //     Directional,
    //     Capsule,
    //     Rect,

    //     MAX
    // };

    LightData.bInverseSquared = 0;//INVERSE_SQUARED_FALLOFF;
    LightData.bRadialLight = LIGHT_SOURCE_SHAPE > 0;
    LightData.bSpotLight = LIGHT_SOURCE_SHAPE > 0;
    LightData.bRectLight = LIGHT_SOURCE_SHAPE == 2;

    return LightData;
}


void PS_Main(
#if LIGHT_SOURCE_SHAPE > 0//non-directional
    float4 InScreenPosition : TEXCOORD0
#else
    float2 ScreenUV		    : TEXCOORD0,
	float3 ScreenVector	    : TEXCOORD1,
#endif
    float4 SVPos		    : SV_POSITION,
    out float4 OutColor     : SV_Target
    ) 
{
#if LIGHT_SOURCE_SHAPE > 0//non-directional
    float2 ScreenUV = InScreenPosition.xy / InScreenPosition.w * View.ScreenPositionScaleBias.xy + View.ScreenPositionScaleBias.wz;
#else
    float3 CameraVector = normalize(ScreenVector);
#endif

    ScreenSpaceData SSD = GetScreenSpaceData(ScreenUV);
    SSD.AmbientOcclusion = 1.0f;
    OutColor = 0;
    [branch]
    if(SSD.GBuffer.ShadingModelID > 0
#if USE_LIGHTING_CHANNELS
     && (GetLightingChannelMask(ScreenUV) & DeferredLightUniforms.LightChannelMask)
#endif
        )
     {
        float SceneDepth = CalcSceneDepth(ScreenUV);

#if LIGHT_SOURCE_SHAPE > 0//non-directional
        float2 ClipPosition = InScreenPosition.xy / InScreenPosition.w * (View.ViewToClip[3][3] ? SceneDepth : 1.0f);
        float3 WorldPosition = mul(float4(ClipPosition,SceneDepth,1), View.ScreenToWorld).xyz;
        float3 CameraVector = normalize(WorldPosition - View.WorldCameraOrigin);
#else
        float3 WorldPosition = ScreenVector * SceneDepth + View.WorldCameraOrigin;
#endif

        DeferredLightData LightData = SetupLightDataForStandardDeferred();

        float Dither = InterleaveGradientNosie( SVPos.xy, View.StateFrameIndexMod8 );

        OutColor = GetDynamicLighting(
            WorldPosition,
            CameraVector,
            SSD.GBuffer,
            SSD.AmbientOcclusion,
            SSD.GBuffer.ShadingModelID,
            LightData, 
            GetPerPixelLightAttenuation(ScreenUV),
            Dither, 
            uint2( SVPos.xy )
        );  
        //OutColor *= ComputeLightProfileMultiplier(WorldPosition, DeferredLightUniforms.LightPosition, -DeferredLightUniforms.NormalizedLightDirection, DeferredLightUniforms.NormalizedLightTangent);
        //OutColor.xyz += CalcSceneColor(ScreenUV);
        //OutColor += float4(SSD.GBuffer.ShadingModelID,0,0,1.0f);
// #if USE_PREEXPOSURE
// 		OutColor *= View.PreExposure;
// #endif
     }
   
}
