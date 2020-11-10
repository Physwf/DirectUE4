#ifndef __Deferred_Shading_Common__
#define __Deferred_Shading_Common__

#include "LightAccumulator.hlsl"
#include "SceneTexturesCommon.hlsl"
#include "SceneTexturesStruct.hlsl"

#define SHADINGMODELID_UNLIT				0
#define SHADINGMODELID_DEFAULT_LIT			1
#define SHADINGMODELID_SUBSURFACE			2
#define SHADINGMODELID_PREINTEGRATED_SKIN	3
#define SHADINGMODELID_CLEAR_COAT			4
#define SHADINGMODELID_SUBSURFACE_PROFILE	5
#define SHADINGMODELID_TWOSIDED_FOLIAGE		6
#define SHADINGMODELID_HAIR					7
#define SHADINGMODELID_CLOTH				8
#define SHADINGMODELID_EYE					9
#define SHADINGMODELID_NUM					10
#define SHADINGMODELID_MASK					0xF		

#define SKIP_CUSTOMDATA_MASK			(1 << 4)	// TODO remove. Can be inferred from shading model.
#define SKIP_PRECSHADOW_MASK			(1 << 5)
#define ZERO_PRECSHADOW_MASK			(1 << 6)
#define SKIP_VELOCITY_MASK				(1 << 7)

float EncodeShadingModelIdAndSelectiveOutputMask(uint ShadingModelId, uint SelectiveOutputMask)
{
	uint Value = (ShadingModelId & SHADINGMODELID_MASK) | SelectiveOutputMask;
	return (float)Value / (float)0xFF;
}

uint DecodeShadingModelId(float InPackedChannel)
{
	return ((uint)round(InPackedChannel * (float)0xFF)) & SHADINGMODELID_MASK;
}

uint DecodeSelectiveOutputMask(float InPackedChannel)
{
	return ((uint)round(InPackedChannel * (float)0xFF)) & ~SHADINGMODELID_MASK;
}

bool UseSubsurfaceProfile(int ShadingModel)
{
	return ShadingModel == SHADINGMODELID_SUBSURFACE_PROFILE || ShadingModel == SHADINGMODELID_EYE;
}

struct GBufferData
{
    float3 WorldNormal;
    float3 DiffuseColor;
    float3 SpecularColor;
    float3 BaseColor;
    float Metallic;
    float Specular;
    float4 CustomData;
    float IndirectIrradiance;
    float4 PrecomputedShadowFactors;
    float Roughness;
    float GBufferAO;
    uint ShadingModelID;
    uint SelectiveOutputMask;
    float PerObjectGBufferData;
    float CustomDepth;
    uint CustomStencil;
    float Depth;
    float4 Velocity;
    float3 StoredBaseColor;
    float StoredSpecular;
    float StoredMetallic;
};

float3 EncodeNormal( float3 N )
{
	return N * 0.5 + 0.5;
	//return Pack1212To888( UnitVectorToOctahedron( N ) * 0.5 + 0.5 );
}

float3 DecodeNormal( float3 N )
{
	return N * 2 - 1;
	//return OctahedronToUnitVector( Pack888To1212( N ) * 2 - 1 );
}

float3 EncodeBaseColor(float3 BaseColor)
{
	// we use sRGB on the render target to give more precision to the darks
	return BaseColor;
}

GBufferData DecodeGBufferData(
    float4 InGBufferA,
	float4 InGBufferB,
	float4 InGBufferC,
	float4 InGBufferD,
	float4 InGBufferE,
	float4 InGBufferVelocity,
	float CustomNativeDepth,
	uint CustomStencil,
	float SceneDepth,
	bool bGetNormalizedNormal,
	bool bChecker)
{
    GBufferData GBuffer = (GBufferData)0;
    GBuffer.WorldNormal = DecodeNormal(InGBufferA.xyz);
    if(bGetNormalizedNormal)
    {
        GBuffer.WorldNormal = normalize(GBuffer.WorldNormal);
    }
    GBuffer.PerObjectGBufferData = InGBufferA.a;
    GBuffer.Metallic = InGBufferB.r;
    GBuffer.Specular = InGBufferB.g;
    GBuffer.Roughness = InGBufferB.b;

    GBuffer.ShadingModelID = DecodeShadingModelId(InGBufferB.a);
	GBuffer.SelectiveOutputMask = DecodeSelectiveOutputMask(InGBufferB.a);

    GBuffer.BaseColor = InGBufferC.rgb;

    GBuffer.GBufferAO = InGBufferC.a;

    GBuffer.CustomData = InGBufferD;

    GBuffer.PrecomputedShadowFactors = 1.0f;
    GBuffer.PrecomputedShadowFactors = !(GBuffer.SelectiveOutputMask & SKIP_PRECSHADOW_MASK) ? InGBufferE :  ((GBuffer.SelectiveOutputMask & ZERO_PRECSHADOW_MASK) ? 0 :  1);
	//GBuffer.CustomDepth = ConvertFromDeviceZ(CustomNativeDepth);
	GBuffer.CustomStencil = CustomStencil;
	GBuffer.Depth = SceneDepth;

    GBuffer.StoredBaseColor = GBuffer.BaseColor;
	GBuffer.StoredMetallic = GBuffer.Metallic;
	GBuffer.StoredSpecular = GBuffer.Specular;

    // derived from BaseColor, Metalness, Specular
	{
		GBuffer.SpecularColor = lerp( 0.08 * GBuffer.Specular.xxx, GBuffer.BaseColor, GBuffer.Metallic );

		// if (UseSubsurfaceProfile(GBuffer.ShadingModelID))
		// {
		// 	AdjustBaseColorAndSpecularColorForSubsurfaceProfileLighting(GBuffer.BaseColor, GBuffer.SpecularColor, GBuffer.Specular, bChecker);
		// }

		GBuffer.DiffuseColor = GBuffer.BaseColor - GBuffer.BaseColor * GBuffer.Metallic;

		// #if USE_DEVELOPMENT_SHADERS
		// {
		// 	// this feature is only needed for development/editor - we can compile it out for a shipping build (see r.CompileShadersForDevelopment cvar help)
		// 	GBuffer.DiffuseColor = GBuffer.DiffuseColor * View.DiffuseOverrideParameter.www + View.DiffuseOverrideParameter.xyz;
		// 	GBuffer.SpecularColor = GBuffer.SpecularColor * View.SpecularOverrideParameter.w + View.SpecularOverrideParameter.xyz;
		// }
		// #endif //USE_DEVELOPMENT_SHADERS
	}

    return GBuffer;
}



GBufferData GetGBufferData(float2 UV, bool bGetNormalizedNormal = true)
{
    float4 GBufferA = Texture2DSampleLevel(SceneTexturesStruct.GBufferATexture, SceneTexturesStruct.GBufferATextureSampler,UV,0);
	float4 GBufferB = Texture2DSampleLevel(SceneTexturesStruct.GBufferBTexture, SceneTexturesStruct.GBufferBTextureSampler,UV,0);
	float4 GBufferC = Texture2DSampleLevel(SceneTexturesStruct.GBufferCTexture, SceneTexturesStruct.GBufferCTextureSampler,UV,0);
	float4 GBufferD = Texture2DSampleLevel(SceneTexturesStruct.GBufferDTexture, SceneTexturesStruct.GBufferDTextureSampler,UV,0);
	float CustomNativeDepth = Texture2DSampleLevel(SceneTexturesStruct.CustomDepthTexture, SceneTexturesStruct.CustomDepthTextureSampler,UV,0).r;
    
#if FEATURE_LEVEL >= FEATURE_LEVEL_SM4
	//int2 IntUV = (int2)trunc(UV * View.BufferSizeAndInvSize.xy);
	//uint CustomStencil = SceneTexturesStruct.CustomStencilTexture.Load(int3(IntUV, 0)) STENCIL_COMPONENT_SWIZZLE;
    uint CustomStencil = 0;
#else
	uint CustomStencil = 0;
#endif

#if ALLOW_STATIC_LIGHTING
	float4 GBufferE = Texture2DSampleLevel(SceneTexturesStruct.GBufferETexture, SceneTexturesStruct.GBufferETextureSampler,UV,0);
#else
    float4 GBufferE = 1;
#endif
    float4 GBufferVelocity = Texture2DSampleLevel(SceneTexturesStruct.GBufferVelocityTexture,SceneTexturesStruct.GBufferVelocityTextureSampler,UV,0);

    float SceneDepth = CalcSceneDepth(UV);
    bool bChecker = false;

    return DecodeGBufferData(GBufferA,GBufferB,GBufferC,GBufferD,GBufferE,GBufferVelocity,CustomNativeDepth,CustomNativeDepth,SceneDepth,bGetNormalizedNormal,bChecker);
}


struct ScreenSpaceData
{
    GBufferData GBuffer;
    float AmbientOcclusion;
};

ScreenSpaceData GetScreenSpaceData(float2 UV, bool bGetNormalizedNormal = true)
{
    ScreenSpaceData Out;
    Out.GBuffer = GetGBufferData(UV, bGetNormalizedNormal);

    float4 ScreenSpaceAO = Texture2DSampleLevel(SceneTexturesStruct.ScreenSpaceAOTexture, SceneTexturesStruct.ScreenSpaceAOTextureSampler, UV, 0);

	Out.AmbientOcclusion = ScreenSpaceAO.r;

    return Out;
}


/** Populates OutGBufferA, B and C */
void EncodeGBuffer(
	GBufferData GBuffer,
	out float4 OutGBufferA,
	out float4 OutGBufferB,
	out float4 OutGBufferC,
	out float4 OutGBufferD,
	out float4 OutGBufferE,
	out float4 OutGBufferVelocity,
	float QuantizationBias = 0		// -0.5 to 0.5 random float. Used to bias quantization.
	)
{
    OutGBufferA.rgb = EncodeNormal( GBuffer.WorldNormal );
    OutGBufferA.a = GBuffer.PerObjectGBufferData;

    OutGBufferB.r = GBuffer.Metallic;
    OutGBufferB.g = GBuffer.Specular;
    OutGBufferB.b = GBuffer.Roughness;
    OutGBufferB.a = EncodeShadingModelIdAndSelectiveOutputMask(GBuffer.ShadingModelID, GBuffer.SelectiveOutputMask);

    OutGBufferC.rgb = EncodeBaseColor( GBuffer.BaseColor );

    // #if ALLOW_STATIC_LIGHTING
	// 	// No space for AO. Multiply IndirectIrradiance by AO instead of storing.
	// 	OutGBufferC.a = EncodeIndirectIrradiance(GBuffer.IndirectIrradiance * GBuffer.GBufferAO) + QuantizationBias * (1.0 / 255.0);
    // #else
	    OutGBufferC.a = GBuffer.GBufferAO;
    // #endif

    OutGBufferD = GBuffer.CustomData;
    OutGBufferE = GBuffer.PrecomputedShadowFactors;

    OutGBufferVelocity = 0;
}

#endif