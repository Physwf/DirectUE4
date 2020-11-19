#include "/Generated/View.hlsl"
#include "/Generated/InstancedView.hlsl"

// ViewState, GetPrimaryView and GetInstancedView are generated by the shader compiler to ensure View uniform buffer changes are up to date.
// see GenerateInstancedStereoCode()
//#include "/Generated/GeneratedInstancedStereo.hlsl"

#ifndef __ViewState__
#define __ViewState__
struct ViewState
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
	// float4 InvDeviceZToWorldZTransform;
	float4 ScreenPositionScaleBias;
	// float3 WorldCameraOrigin;
	// float3 TranslatedWorldCameraOrigin;
	// float3 WorldViewOrigin;
	float3 PreViewTranslation;
	// float4x4 PrevProjection;
	// float4x4 PrevViewProj;
	// float4x4 PrevViewRotationProj;
	// float4x4 PrevViewToClip;
	// float4x4 PrevClipToView;
	// float4x4 PrevTranslatedWorldToClip;
	// float4x4 PrevTranslatedWorldToView;
	// float4x4 PrevViewToTranslatedWorld;
	// float4x4 PrevTranslatedWorldToCameraView;
	// float4x4 PrevCameraViewToTranslatedWorld;
	// float3 PrevWorldCameraOrigin;
	// float3 PrevWorldViewOrigin;
	// float3 PrevPreViewTranslation;
	// float4x4 PrevInvViewProj;
	// float4x4 PrevScreenToTranslatedWorld;
	// float4x4 ClipToPrevClip;
	// float4 TemporalAAJitter;
	// float4 GlobalClippingPlane;
	// float2 FieldOfViewWideAngles;
	// float2 PrevFieldOfViewWideAngles;
	float4 ViewRectMin;
	float4 ViewSizeAndInvSize;
	// float4 BufferSizeAndInvSize;
	// float4 BufferBilinearUVMinMax;
	// int NumSceneColorMSAASamples;
	// half PreExposure;
	// half OneOverPreExposure;
	// half4 DiffuseOverrideParameter;
	// half4 SpecularOverrideParameter;
	// half4 NormalOverrideParameter;
	// half2 RoughnessOverrideParameter;
	// float PrevFrameGameTime;
	// float PrevFrameRealTime;
	// half OutOfBoundsMask;
	// float3 WorldCameraMovementSinceLastFrame;
	// float CullingSign;
	// half NearPlane;
	// float AdaptiveTessellationFactor;
	// float GameTime;
	// float RealTime;
	// float MaterialTextureMipBias;
	// float MaterialTextureDerivativeMultiply;
	// uint Random;
	// uint FrameNumber;
	// uint StateFrameIndexMod8;
	// half CameraCut;
	// half UnlitViewmodeMask;
	// half4 DirectionalLightColor;
	// half3 DirectionalLightDirection;
	// float4 TranslucencyLightingVolumeMin[2];
	// float4 TranslucencyLightingVolumeInvSize[2];
	// float4 TemporalAAParams;
	// float4 CircleDOFParams;
	// float DepthOfFieldSensorWidth;
	// float DepthOfFieldFocalDistance;
	// float DepthOfFieldScale;
	// float DepthOfFieldFocalLength;
	// float DepthOfFieldFocalRegion;
	// float DepthOfFieldNearTransitionRegion;
	// float DepthOfFieldFarTransitionRegion;
	// float MotionBlurNormalizedToPixel;
	// float bSubsurfacePostprocessEnabled;
	// float GeneralPurposeTweak;
	// half DemosaicVposOffset;
	// float3 IndirectLightingColorScale;
	// half HDR32bppEncodingMode;
	// float3 AtmosphericFogSunDirection;
	// half AtmosphericFogSunPower;
	// half AtmosphericFogPower;
	// half AtmosphericFogDensityScale;
	// half AtmosphericFogDensityOffset;
	// half AtmosphericFogGroundOffset;
	// half AtmosphericFogDistanceScale;
	// half AtmosphericFogAltitudeScale;
	// half AtmosphericFogHeightScaleRayleigh;
	// half AtmosphericFogStartDistance;
	// half AtmosphericFogDistanceOffset;
	// half AtmosphericFogSunDiscScale;
	// uint AtmosphericFogRenderMask;
	// uint AtmosphericFogInscatterAltitudeSampleNum;
	// float4 AtmosphericFogSunColor;
	// float3 NormalCurvatureToRoughnessScaleBias;
	// float RenderingReflectionCaptureMask;
	// float4 AmbientCubemapTint;
	// float AmbientCubemapIntensity;
	float SkyLightParameters;
	float4 SkyLightColor;
	float4 SkyIrradianceEnvironmentMap[7];
	// float MobilePreviewMode;
	// float HMDEyePaddingOffset;
	// half ReflectionCubemapMaxMip;
	// float ShowDecalsMask;
	// uint DistanceFieldAOSpecularOcclusionMode;
	// float IndirectCapsuleSelfShadowingIntensity;
	// float3 ReflectionEnvironmentRoughnessMixingScaleBiasAndLargestWeight;
	// int StereoPassIndex;
	// float4 GlobalVolumeCenterAndExtent[4];
	// float4 GlobalVolumeWorldToUVAddAndMul[4];
	// float GlobalVolumeDimension;
	// float GlobalVolumeTexelSize;
	// float MaxGlobalDistance;
	// float bCheckerboardSubsurfaceProfileRendering;
	// float3 VolumetricFogInvGridSize;
	// float3 VolumetricFogGridZParams;
	// float2 VolumetricFogSVPosToVolumeUV;
	// float VolumetricFogMaxDistance;
	// float3 VolumetricLightmapWorldToUVScale;
	// float3 VolumetricLightmapWorldToUVAdd;
	// float3 VolumetricLightmapIndirectionTextureSize;
	// float VolumetricLightmapBrickSize;
	// float3 VolumetricLightmapBrickTexelSize;
	// float StereoIPD;
    
};

ViewState GetPrimaryView()
{
	ViewState Result;
	Result.TranslatedWorldToClip = View.TranslatedWorldToClip;
	Result.WorldToClip = View.WorldToClip;
	Result.TranslatedWorldToView = View.TranslatedWorldToView;
	Result.ViewToTranslatedWorld = View.ViewToTranslatedWorld;
	Result.TranslatedWorldToCameraView = View.TranslatedWorldToCameraView;
	Result.CameraViewToTranslatedWorld = View.CameraViewToTranslatedWorld;
	Result.ViewToClip = View.ViewToClip;
	Result.ViewToClipNoAA = View.ViewToClipNoAA;
	Result.ClipToView = View.ClipToView;
	Result.ClipToTranslatedWorld = View.ClipToTranslatedWorld;
	Result.SVPositionToTranslatedWorld = View.SVPositionToTranslatedWorld;
	Result.ScreenToWorld = View.ScreenToWorld;
	Result.ScreenToTranslatedWorld = View.ScreenToTranslatedWorld;
    
	// Result.ViewForward = View.ViewForward;
	// Result.ViewUp = View.ViewUp;
	// Result.ViewRight = View.ViewRight;
	// Result.HMDViewNoRollUp = View.HMDViewNoRollUp;
	// Result.HMDViewNoRollRight = View.HMDViewNoRollRight;
	// Result.InvDeviceZToWorldZTransform = View.InvDeviceZToWorldZTransform;
	Result.ScreenPositionScaleBias = View.ScreenPositionScaleBias;
	// Result.WorldCameraOrigin = View.WorldCameraOrigin;
	// Result.TranslatedWorldCameraOrigin = View.TranslatedWorldCameraOrigin;
	// Result.WorldViewOrigin = View.WorldViewOrigin;
	Result.PreViewTranslation = View.PreViewTranslation;
	// Result.PrevProjection = View.PrevProjection;
	// Result.PrevViewProj = View.PrevViewProj;
	// Result.PrevViewRotationProj = View.PrevViewRotationProj;
	// Result.PrevViewToClip = View.PrevViewToClip;
	// Result.PrevClipToView = View.PrevClipToView;
	// Result.PrevTranslatedWorldToClip = View.PrevTranslatedWorldToClip;
	// Result.PrevTranslatedWorldToView = View.PrevTranslatedWorldToView;
	// Result.PrevViewToTranslatedWorld = View.PrevViewToTranslatedWorld;
	// Result.PrevTranslatedWorldToCameraView = View.PrevTranslatedWorldToCameraView;
	// Result.PrevCameraViewToTranslatedWorld = View.PrevCameraViewToTranslatedWorld;
	// Result.PrevWorldCameraOrigin = View.PrevWorldCameraOrigin;
	// Result.PrevWorldViewOrigin = View.PrevWorldViewOrigin;
	// Result.PrevPreViewTranslation = View.PrevPreViewTranslation;
	// Result.PrevInvViewProj = View.PrevInvViewProj;
	// Result.PrevScreenToTranslatedWorld = View.PrevScreenToTranslatedWorld;
	// Result.ClipToPrevClip = View.ClipToPrevClip;
	// Result.TemporalAAJitter = View.TemporalAAJitter;
	// Result.GlobalClippingPlane = View.GlobalClippingPlane;
	// Result.FieldOfViewWideAngles = View.FieldOfViewWideAngles;
	// Result.PrevFieldOfViewWideAngles = View.PrevFieldOfViewWideAngles;
	Result.ViewRectMin = View.ViewRectMin;
	Result.ViewSizeAndInvSize = View.ViewSizeAndInvSize;
	// Result.BufferSizeAndInvSize = View.BufferSizeAndInvSize;
	// Result.BufferBilinearUVMinMax = View.BufferBilinearUVMinMax;
	// Result.NumSceneColorMSAASamples = View.NumSceneColorMSAASamples;
	// Result.PreExposure = View.PreExposure;
	// Result.OneOverPreExposure = View.OneOverPreExposure;
	// Result.DiffuseOverrideParameter = View.DiffuseOverrideParameter;
	// Result.SpecularOverrideParameter = View.SpecularOverrideParameter;
	// Result.NormalOverrideParameter = View.NormalOverrideParameter;
	// Result.RoughnessOverrideParameter = View.RoughnessOverrideParameter;
	// Result.PrevFrameGameTime = View.PrevFrameGameTime;
	// Result.PrevFrameRealTime = View.PrevFrameRealTime;
	// Result.OutOfBoundsMask = View.OutOfBoundsMask;
	// Result.WorldCameraMovementSinceLastFrame = View.WorldCameraMovementSinceLastFrame;
	// Result.CullingSign = View.CullingSign;
	// Result.NearPlane = View.NearPlane;
	// Result.AdaptiveTessellationFactor = View.AdaptiveTessellationFactor;
	// Result.GameTime = View.GameTime;
	// Result.RealTime = View.RealTime;
	// Result.MaterialTextureMipBias = View.MaterialTextureMipBias;
	// Result.MaterialTextureDerivativeMultiply = View.MaterialTextureDerivativeMultiply;
	// Result.Random = View.Random;
	// Result.FrameNumber = View.FrameNumber;
	// Result.StateFrameIndexMod8 = View.StateFrameIndexMod8;
	// Result.CameraCut = View.CameraCut;
	// Result.UnlitViewmodeMask = View.UnlitViewmodeMask;
	// Result.DirectionalLightColor = View.DirectionalLightColor;
	// Result.DirectionalLightDirection = View.DirectionalLightDirection;
	// Result.TranslucencyLightingVolumeMin = View.TranslucencyLightingVolumeMin;
	// Result.TranslucencyLightingVolumeInvSize = View.TranslucencyLightingVolumeInvSize;
	// Result.TemporalAAParams = View.TemporalAAParams;
	// Result.CircleDOFParams = View.CircleDOFParams;
	// Result.DepthOfFieldSensorWidth = View.DepthOfFieldSensorWidth;
	// Result.DepthOfFieldFocalDistance = View.DepthOfFieldFocalDistance;
	// Result.DepthOfFieldScale = View.DepthOfFieldScale;
	// Result.DepthOfFieldFocalLength = View.DepthOfFieldFocalLength;
	// Result.DepthOfFieldFocalRegion = View.DepthOfFieldFocalRegion;
	// Result.DepthOfFieldNearTransitionRegion = View.DepthOfFieldNearTransitionRegion;
	// Result.DepthOfFieldFarTransitionRegion = View.DepthOfFieldFarTransitionRegion;
	// Result.MotionBlurNormalizedToPixel = View.MotionBlurNormalizedToPixel;
	// Result.bSubsurfacePostprocessEnabled = View.bSubsurfacePostprocessEnabled;
	// Result.GeneralPurposeTweak = View.GeneralPurposeTweak;
	// Result.DemosaicVposOffset = View.DemosaicVposOffset;
	// Result.IndirectLightingColorScale = View.IndirectLightingColorScale;
	// Result.HDR32bppEncodingMode = View.HDR32bppEncodingMode;
	// Result.AtmosphericFogSunDirection = View.AtmosphericFogSunDirection;
	// Result.AtmosphericFogSunPower = View.AtmosphericFogSunPower;
	// Result.AtmosphericFogPower = View.AtmosphericFogPower;
	// Result.AtmosphericFogDensityScale = View.AtmosphericFogDensityScale;
	// Result.AtmosphericFogDensityOffset = View.AtmosphericFogDensityOffset;
	// Result.AtmosphericFogGroundOffset = View.AtmosphericFogGroundOffset;
	// Result.AtmosphericFogDistanceScale = View.AtmosphericFogDistanceScale;
	// Result.AtmosphericFogAltitudeScale = View.AtmosphericFogAltitudeScale;
	// Result.AtmosphericFogHeightScaleRayleigh = View.AtmosphericFogHeightScaleRayleigh;
	// Result.AtmosphericFogStartDistance = View.AtmosphericFogStartDistance;
	// Result.AtmosphericFogDistanceOffset = View.AtmosphericFogDistanceOffset;
	// Result.AtmosphericFogSunDiscScale = View.AtmosphericFogSunDiscScale;
	// Result.AtmosphericFogRenderMask = View.AtmosphericFogRenderMask;
	// Result.AtmosphericFogInscatterAltitudeSampleNum = View.AtmosphericFogInscatterAltitudeSampleNum;
	// Result.AtmosphericFogSunColor = View.AtmosphericFogSunColor;
	// Result.NormalCurvatureToRoughnessScaleBias = View.NormalCurvatureToRoughnessScaleBias;
	// Result.RenderingReflectionCaptureMask = View.RenderingReflectionCaptureMask;
	// Result.AmbientCubemapTint = View.AmbientCubemapTint;
	// Result.AmbientCubemapIntensity = View.AmbientCubemapIntensity;
	Result.SkyLightParameters = View.SkyLightParameters;
	Result.SkyLightColor = View.SkyLightColor;
	Result.SkyIrradianceEnvironmentMap = View.SkyIrradianceEnvironmentMap;
	// Result.MobilePreviewMode = View.MobilePreviewMode;
	// Result.HMDEyePaddingOffset = View.HMDEyePaddingOffset;
	// Result.ReflectionCubemapMaxMip = View.ReflectionCubemapMaxMip;
	// Result.ShowDecalsMask = View.ShowDecalsMask;
	// Result.DistanceFieldAOSpecularOcclusionMode = View.DistanceFieldAOSpecularOcclusionMode;
	// Result.IndirectCapsuleSelfShadowingIntensity = View.IndirectCapsuleSelfShadowingIntensity;
	// Result.ReflectionEnvironmentRoughnessMixingScaleBiasAndLargestWeight = View.ReflectionEnvironmentRoughnessMixingScaleBiasAndLargestWeight;
	// Result.StereoPassIndex = View.StereoPassIndex;
	// Result.GlobalVolumeCenterAndExtent = View.GlobalVolumeCenterAndExtent;
	// Result.GlobalVolumeWorldToUVAddAndMul = View.GlobalVolumeWorldToUVAddAndMul;
	// Result.GlobalVolumeDimension = View.GlobalVolumeDimension;
	// Result.GlobalVolumeTexelSize = View.GlobalVolumeTexelSize;
	// Result.MaxGlobalDistance = View.MaxGlobalDistance;
	// Result.bCheckerboardSubsurfaceProfileRendering = View.bCheckerboardSubsurfaceProfileRendering;
	// Result.VolumetricFogInvGridSize = View.VolumetricFogInvGridSize;
	// Result.VolumetricFogGridZParams = View.VolumetricFogGridZParams;
	// Result.VolumetricFogSVPosToVolumeUV = View.VolumetricFogSVPosToVolumeUV;
	// Result.VolumetricFogMaxDistance = View.VolumetricFogMaxDistance;
	// Result.VolumetricLightmapWorldToUVScale = View.VolumetricLightmapWorldToUVScale;
	// Result.VolumetricLightmapWorldToUVAdd = View.VolumetricLightmapWorldToUVAdd;
	// Result.VolumetricLightmapIndirectionTextureSize = View.VolumetricLightmapIndirectionTextureSize;
	// Result.VolumetricLightmapBrickSize = View.VolumetricLightmapBrickSize;
	// Result.VolumetricLightmapBrickTexelSize = View.VolumetricLightmapBrickTexelSize;
	// Result.StereoIPD = View.StereoIPD;
	return Result;
}

static ViewState ResolvedView;

ViewState ResolveView()
{
	return GetPrimaryView();
}

#endif