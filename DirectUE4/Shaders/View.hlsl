//View
#ifndef __UniformBuffer_View_Definition__
#define __UniformBuffer_View_Definition__

cbuffer View 
{
    float4x4 View_TranslatedWorldToClip;
    float4x4 View_WorldToClip;
    float4x4 View_TranslatedWorldToView;
    float4x4 View_ViewToTranslatedWorld;
    float4x4 View_TranslatedWorldToCameraView;
    float4x4 View_CameraViewToTranslatedWorld;
    float4x4 View_ViewToClip;
    float4x4 View_ViewToClipNoAA;
    float4x4 View_ClipToView;
    float4x4 View_ClipToTranslatedWorld;
    float4x4 View_SVPositionToTranslatedWorld;
    float4x4 View_ScreenToWorld;
    float4x4 View_ScreenToTranslatedWorld;//832
    // half3 View_ViewForward;
    // half3 View_ViewUp;
    // half3 View_ViewRight;
    // half3 View_HMDViewNoRollUp;
    // half3 View_HMDViewNoRollRight;
    float4 View_InvDeviceZToWorldZTransform;
    float4 View_ScreenPositionScaleBias;
    float3 View_WorldCameraOrigin;
    float View_Padding01;
    float3 View_TranslatedWorldCameraOrigin;
    float View_Padding02;
    float3 View_WorldViewOrigin;
    float View_Padding03;

    
    float3 View_PreViewTranslation;
    float View_Padding04;
    float4 View_ViewRectMin;
    float4 View_ViewSizeAndInvSize;
    float4 View_BufferSizeAndInvSize;
	float4 View_BufferBilinearUVMinMax;

    uint View_Random;
	uint View_FrameNumber;
	uint View_StateFrameIndexMod8;
    uint View_Padding05;


    float View_DemosaicVposOffset;
    float3 View_IndirectLightingColorScale;

    float3 View_AtmosphericFogSunDirection;
	float View_AtmosphericFogSunPower;
	float View_AtmosphericFogPower;
	float View_AtmosphericFogDensityScale;
	float View_AtmosphericFogDensityOffset;
	float View_AtmosphericFogGroundOffset;
	float View_AtmosphericFogDistanceScale;
	float View_AtmosphericFogAltitudeScale;
	float View_AtmosphericFogHeightScaleRayleigh;
	float View_AtmosphericFogStartDistance;
	float View_AtmosphericFogDistanceOffset;
	float View_AtmosphericFogSunDiscScale;
	uint View_AtmosphericFogRenderMask;
	uint View_AtmosphericFogInscatterAltitudeSampleNum;
	float4 View_AtmosphericFogSunColor;

    float View_AmbientCubemapIntensity;
	float View_SkyLightParameters;
	float PrePadding_View_2472;
	float PrePadding_View_2476;
	float4 View_SkyLightColor;
    float4 View_SkyIrradianceEnvironmentMap[7];

    float PrePadding_View_2862;
    float3 View_VolumetricLightmapWorldToUVScale;
    float PrePadding_View_2861;
	float3 View_VolumetricLightmapWorldToUVAdd;
    float PrePadding_View_2876;
    float3 View_VolumetricLightmapIndirectionTextureSize;

    float View_VolumetricLightmapBrickSize;
	float3 View_VolumetricLightmapBrickTexelSize;
};

SamplerState View_MaterialTextureBilinearWrapedSampler;
SamplerState View_MaterialTextureBilinearClampedSampler;
Texture3D<uint4> View_VolumetricLightmapIndirectionTexture;
Texture3D View_VolumetricLightmapBrickAmbientVector;
Texture3D View_VolumetricLightmapBrickSHCoefficients0;
Texture3D View_VolumetricLightmapBrickSHCoefficients1;
Texture3D View_VolumetricLightmapBrickSHCoefficients2;
Texture3D View_VolumetricLightmapBrickSHCoefficients3;
Texture3D View_VolumetricLightmapBrickSHCoefficients4;
Texture3D View_VolumetricLightmapBrickSHCoefficients5;
Texture3D View_SkyBentNormalBrickTexture;
Texture3D View_DirectionalLightShadowingBrickTexture;
SamplerState View_VolumetricLightmapBrickAmbientVectorSampler;
SamplerState View_VolumetricLightmapTextureSampler0;
SamplerState View_VolumetricLightmapTextureSampler1;
SamplerState View_VolumetricLightmapTextureSampler2;
SamplerState View_VolumetricLightmapTextureSampler3;
SamplerState View_VolumetricLightmapTextureSampler4;
SamplerState View_VolumetricLightmapTextureSampler5;
SamplerState View_SkyBentNormalTextureSampler;
SamplerState View_DirectionalLightShadowingTextureSampler;
Texture3D View_GlobalDistanceFieldTexture0;
SamplerState View_GlobalDistanceFieldSampler0;
Texture3D View_GlobalDistanceFieldTexture1;
SamplerState View_GlobalDistanceFieldSampler1;
Texture3D View_GlobalDistanceFieldTexture2;
SamplerState View_GlobalDistanceFieldSampler2;
Texture3D View_GlobalDistanceFieldTexture3;
SamplerState View_GlobalDistanceFieldSampler3;
Texture2D View_AtmosphereTransmittanceTexture;
SamplerState View_AtmosphereTransmittanceTextureSampler;
Texture2D View_AtmosphereIrradianceTexture;
SamplerState View_AtmosphereIrradianceTextureSampler;
Texture3D View_AtmosphereInscatterTexture;
SamplerState View_AtmosphereInscatterTextureSampler;
Texture2D View_PerlinNoiseGradientTexture;
SamplerState View_PerlinNoiseGradientTextureSampler;
Texture3D View_PerlinNoise3DTexture;
SamplerState View_PerlinNoise3DTextureSampler;
Texture2D<uint> View_SobolSamplingTexture;
SamplerState View_SharedPointWrappedSampler;
SamplerState View_SharedPointClampedSampler;
SamplerState View_SharedBilinearWrappedSampler;
SamplerState View_SharedBilinearClampedSampler;
SamplerState View_SharedTrilinearWrappedSampler;
SamplerState View_SharedTrilinearClampedSampler;
Texture2D View_PreIntegratedBRDF;
SamplerState View_PreIntegratedBRDFSampler;

static const struct
{
    float4x4 TranslatedWorldToClip;
    float4x4 WorldToClip;
    float4x4 TranslatedWorldToView;
    float4x4 ViewToTranslatedWorld;
    float4x4 TranslatedWorldToCameraView;
    float4x4 CameraViewToTranslatedWorld;
    float4x4 ViewToClip;
    float4x4 ViewToClipNoAA;
    float4x4 ClipToView;
    float4x4 ClipToTranslatedWorld;
    float4x4 SVPositionToTranslatedWorld;
    float4x4 ScreenToWorld;
    float4x4 ScreenToTranslatedWorld;
    // half3 ViewForward;
    // half3 ViewUp;
    // half3 ViewRight;
    // half3 HMDViewNoRollUp;
    // half3 HMDViewNoRollRight;
    float4 InvDeviceZToWorldZTransform;
    float4 ScreenPositionScaleBias;
    float3 WorldCameraOrigin;
    float3 TranslatedWorldCameraOrigin;
    float3 WorldViewOrigin;

    float3 PreViewTranslation;

    float4 ViewRectMin;
    float4 ViewSizeAndInvSize;
    float4 BufferSizeAndInvSize;
	float4 BufferBilinearUVMinMax;
    uint Random;
	uint FrameNumber;
	uint StateFrameIndexMod8;

    float DemosaicVposOffset;
    float3 IndirectLightingColorScale;

    float3 AtmosphericFogSunDirection;
	float AtmosphericFogSunPower;
	float AtmosphericFogPower;
	float AtmosphericFogDensityScale;
	float AtmosphericFogDensityOffset;
	float AtmosphericFogGroundOffset;
	float AtmosphericFogDistanceScale;
	float AtmosphericFogAltitudeScale;
	float AtmosphericFogHeightScaleRayleigh;
	float AtmosphericFogStartDistance;
	float AtmosphericFogDistanceOffset;
	float AtmosphericFogSunDiscScale;
	uint AtmosphericFogRenderMask;
	uint AtmosphericFogInscatterAltitudeSampleNum;
	float4 AtmosphericFogSunColor;

    float AmbientCubemapIntensity;
    float SkyLightParameters;
	float4 SkyLightColor;
    float4 SkyIrradianceEnvironmentMap[7];

    float3 VolumetricLightmapWorldToUVScale;
	float3 VolumetricLightmapWorldToUVAdd;
    float3 VolumetricLightmapIndirectionTextureSize;

    float VolumetricLightmapBrickSize;
	float3 VolumetricLightmapBrickTexelSize;

    Texture3D<uint4> VolumetricLightmapIndirectionTexture;

    Texture3D DirectionalLightShadowingBrickTexture;
    SamplerState DirectionalLightShadowingTextureSampler;
} View = 
{
    View_TranslatedWorldToClip,
    View_WorldToClip,
    View_TranslatedWorldToView,
    View_ViewToTranslatedWorld,
    View_TranslatedWorldToCameraView,
    View_CameraViewToTranslatedWorld,
    View_ViewToClip,
    View_ViewToClipNoAA,
    View_ClipToView,
    View_ClipToTranslatedWorld,
    View_SVPositionToTranslatedWorld,
    View_ScreenToWorld,
    View_ScreenToTranslatedWorld,
    // View_ViewForward,
    // View_ViewUp,
    // View_ViewRight,
    // View_HMDViewNoRollUp,
    // View_HMDViewNoRollRight,
    View_InvDeviceZToWorldZTransform,
    View_ScreenPositionScaleBias,
    View_WorldCameraOrigin,
    View_TranslatedWorldCameraOrigin,
    View_WorldViewOrigin,

    View_PreViewTranslation,

    View_ViewRectMin,
    View_ViewSizeAndInvSize,
    View_BufferSizeAndInvSize,
    View_BufferBilinearUVMinMax,
    
    View_Random,
	View_FrameNumber,
	View_StateFrameIndexMod8,

    View_DemosaicVposOffset,
    View_IndirectLightingColorScale,

    View_AtmosphericFogSunDirection,
	View_AtmosphericFogSunPower,
	View_AtmosphericFogPower,
	View_AtmosphericFogDensityScale,
	View_AtmosphericFogDensityOffset,
	View_AtmosphericFogGroundOffset,
	View_AtmosphericFogDistanceScale,
	View_AtmosphericFogAltitudeScale,
	View_AtmosphericFogHeightScaleRayleigh,
	View_AtmosphericFogStartDistance,
	View_AtmosphericFogDistanceOffset,
	View_AtmosphericFogSunDiscScale,
	View_AtmosphericFogRenderMask,
	View_AtmosphericFogInscatterAltitudeSampleNum,
	View_AtmosphericFogSunColor,

    View_AmbientCubemapIntensity,
	View_SkyLightParameters,
	View_SkyLightColor,
    {
        View_SkyIrradianceEnvironmentMap[0],
        View_SkyIrradianceEnvironmentMap[1],
        View_SkyIrradianceEnvironmentMap[2],
        View_SkyIrradianceEnvironmentMap[3],
        View_SkyIrradianceEnvironmentMap[4],
        View_SkyIrradianceEnvironmentMap[5],
        View_SkyIrradianceEnvironmentMap[6],
    },
    View_VolumetricLightmapWorldToUVScale,
	View_VolumetricLightmapWorldToUVAdd,
    View_VolumetricLightmapIndirectionTextureSize,

    View_VolumetricLightmapBrickSize,
	View_VolumetricLightmapBrickTexelSize,

    View_VolumetricLightmapIndirectionTexture,
    
    View_DirectionalLightShadowingBrickTexture,
    View_DirectionalLightShadowingTextureSampler,
};
#endif
