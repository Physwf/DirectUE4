cbuffer View : register(b0)
{
	float4x4 WorldToView;
	float4x4 ViewToProj;
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
    half3 ViewForward;
    half3 ViewUp;
    half3 ViewRight;
    // half3 HMDViewNoRollUp;
    // half3 HMDViewNoRollRight;
    float4 InvDeviceZToWorldZTransform;
    half4 ScreenPositionScaleBias;
    float4 WorldCameraOrigin;
    float4 TranslatedWorldCameraOrigin;
    float4 WorldViewOrigin;
};

cbuffer	Primitive : register(b1)
{
	float4x4 LocalToWorld;
	float4x4 WorldToLocal;
	// float4 ObjectWorldPositionAndRadius;
    // float3 ObjectBounds;
    // half LocalToWorldDeterminantSign;
    // float3 ActorWorldPosition;
    // float DecalReceiverMask;
    // float PerObjectGBufferData;
    // float UseSingleSampleShadowFromStationaryLights;
    // float UseVolumetricLightmapShadowFromStationaryLights;
    // float UseEditorDepthTest;
    // float4 ObjectOrientation;
    // float4 NonUniformScale;
    // float4 InvNonUniformScale;
    // float3 LocalObjectBoundsMin;
    // float3 LocalObjectBoundsMax;
    // uint LightingChannelMask;
    // float LpvBiasMultiplier;
};

cbuffer DrawRectangleParameters : register(b2)
{
    float4 PosScaleBias;
    float4 UVScaleBias;
    float4 InvTargetSizeAndTextureSize;
};

cbuffer DeferredLightUniform : register(b3)
{
    float3 LightPosition;
    float LightInvRadius;
    float3 LightColor;
    float LightFalloffExponent;
    float3 NormalizedLightDirection;
    float3 NormalizedLightTangent;
    float2 SpotAngles;
    float SpecularScale;
    float SourceRadius;
    float SoftSourceRadius;
    float SourceLength;
    float ContactShadowLength;
    float2 DistanceFadeMAD;
    float4 ShadowMapChannelMask;
    uint ShadowedBits;
    uint LightingChannelMask;
    float VolumetricScatteringIntensity;
    Texture2D SourceTexture;
};

