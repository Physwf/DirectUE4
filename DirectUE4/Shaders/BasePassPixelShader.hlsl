//#include "SHCommon.hlsl"
#include "/Generated/Material.hlsl"
#include "BasePassCommon.hlsl"
#include "LocalVertexFactory.hlsl"
#include "LightmapCommon.hlsl"  
#include "PlanarReflectionShared.hlsl"
// #include "BRDF.ush"
#include "Random.hlsl"
#include "LightAccumulator.hlsl"
#include "DeferredShadingCommon.hlsl"
// #include "VelocityCommon.ush"
#include "SphericalGaussian.hlsl"
// #include "DBufferDecalShared.ush"

#include "ShadingModelsMaterial.hlsl"

#define PREV_FRAME_COLOR	1
//#include "ScreenSpaceRayCast.ush"

#if NEEDS_BASEPASS_PIXEL_FOGGING || NEEDS_BASEPASS_PIXEL_VOLUMETRIC_FOGGING
	#include "HeightFogCommon.ush"
#endif

#include "ReflectionEnvironmentShared.hlsl"

#define POST_PROCESS_SUBSURFACE 0 //((MATERIAL_SHADINGMODEL_SUBSURFACE_PROFILE || MATERIAL_SHADINGMODEL_EYE) && USES_GBUFFER)

#if USES_GBUFFER

#define GetEffectiveSkySHDiffuse GetSkySHDiffuse

/** Computes sky diffuse lighting, including precomputed shadowing. */
void GetSkyLighting(float3 WorldNormal, float2 LightmapUV, float3 SkyOcclusionUV3D, out float3 OutDiffuseLighting, out float3 OutSubsurfaceLighting)
{
	OutDiffuseLighting = 0;
	OutSubsurfaceLighting = 0;

#if ENABLE_SKY_LIGHT

	float SkyVisibility = 1;
	float GeometryTerm = 1;
	float3 SkyLightingNormal = WorldNormal;
	
	#if HQ_TEXTURE_LIGHTMAP || CACHED_POINT_INDIRECT_LIGHTING || CACHED_VOLUME_INDIRECT_LIGHTING || PRECOMPUTED_IRRADIANCE_VOLUME_LIGHTING
		[branch]//BRANCH
		if (View.SkyLightParameters.x > 0)
		{
			#if PRECOMPUTED_IRRADIANCE_VOLUME_LIGHTING
			
				float3 SkyBentNormal = GetVolumetricLightmapSkyBentNormal(SkyOcclusionUV3D);
				SkyVisibility = length(SkyBentNormal);
				float3 NormalizedBentNormal = SkyBentNormal / max(SkyVisibility, .0001f);

			#elif HQ_TEXTURE_LIGHTMAP

				// Bent normal from precomputed texture
				float4 WorldSkyBentNormalAndOcclusion = GetSkyBentNormalAndOcclusion(LightmapUV * float2(1, 2));
				// Renormalize as vector was quantized and compressed
				float3 NormalizedBentNormal = normalize(WorldSkyBentNormalAndOcclusion.xyz);
				SkyVisibility = WorldSkyBentNormalAndOcclusion.w;

			#elif CACHED_POINT_INDIRECT_LIGHTING || CACHED_VOLUME_INDIRECT_LIGHTING

				// Bent normal from the indirect lighting cache - one value for the whole object
				float3 NormalizedBentNormal = PrecomputedLightingBuffer.PointSkyBentNormal.xyz;
				SkyVisibility = PrecomputedLightingBuffer.PointSkyBentNormal.w;

			#endif

			#if (MATERIALBLENDING_TRANSLUCENT || MATERIALBLENDING_ADDITIVE) && (TRANSLUCENCY_LIGHTING_VOLUMETRIC_NONDIRECTIONAL || TRANSLUCENCY_LIGHTING_VOLUMETRIC_PERVERTEX_NONDIRECTIONAL)
				// NonDirectional lighting can't depend on the normal
				SkyLightingNormal = NormalizedBentNormal;
			#else
				
				// Weight toward the material normal to increase directionality
				float BentNormalWeightFactor = 1 - (1 - SkyVisibility) * (1 - SkyVisibility);

				// We are lerping between the inputs of two lighting scenarios based on occlusion
				// In the mostly unoccluded case, evaluate sky lighting with the material normal, because it has higher detail
				// In the mostly occluded case, evaluate sky lighting with the bent normal, because it is a better representation of the incoming lighting
				// Then treat the lighting evaluated along the bent normal as an area light, so we must apply the lambert term
				SkyLightingNormal = lerp(NormalizedBentNormal, WorldNormal, BentNormalWeightFactor);

				float DotProductFactor = lerp(saturate(dot(NormalizedBentNormal, WorldNormal)), 1, BentNormalWeightFactor);
				// Account for darkening due to the geometry term
				GeometryTerm = DotProductFactor;
			#endif
		}
	#endif
			
	// Compute the preconvolved incoming lighting with the bent normal direction
	float3 DiffuseLookup = GetEffectiveSkySHDiffuse(SkyLightingNormal) * ResolvedView.SkyLightColor.rgb;

	// Apply AO to the sky diffuse
	OutDiffuseLighting += DiffuseLookup * (SkyVisibility * GeometryTerm);

	#if MATERIAL_SHADINGMODEL_TWOSIDED_FOLIAGE
		float3 BackfaceDiffuseLookup = GetEffectiveSkySHDiffuse(-WorldNormal) * ResolvedView.SkyLightColor.rgb;
		OutSubsurfaceLighting += BackfaceDiffuseLookup * SkyVisibility;
	#endif

#endif
}

#if SUPPORTS_INDEPENDENT_SAMPLERS
	#define ILCSharedSampler1 View.SharedBilinearClampedSampler
	#define ILCSharedSampler2 View.SharedBilinearClampedSampler
#else
	#define ILCSharedSampler1 PrecomputedLightingBuffer.IndirectLightingCacheTextureSampler1
	#define ILCSharedSampler2 PrecomputedLightingBuffer.IndirectLightingCacheTextureSampler2
#endif

/** Calculates indirect lighting contribution on this object from precomputed data. */
void GetPrecomputedIndirectLightingAndSkyLight(
	MaterialPixelParameters MaterialParameters, 
	VertexFactoryInterpolantsVSToPS Interpolants,
	BasePassInterpolantsVSToPS BasePassInterpolants,
	float3 DiffuseDir,
	float3 VolumetricLightmapBrickTextureUVs,
	out float3 OutDiffuseLighting,
	out float3 OutSubsurfaceLighting,
	out float OutIndirectIrradiance)
{
	OutIndirectIrradiance = 0;
	OutDiffuseLighting = 0;
	OutSubsurfaceLighting = 0;
	float2 SkyOcclusionUV = 0;

	#if PRECOMPUTED_IRRADIANCE_VOLUME_LIGHTING
	// Method for movable components which want to use a volume texture of interpolated SH samples
	#elif CACHED_VOLUME_INDIRECT_LIGHTING
	// Method for movable components which want to use a single interpolated SH sample
	#elif CACHED_POINT_INDIRECT_LIGHTING 
	// High quality texture lightmaps
	#elif HQ_TEXTURE_LIGHTMAP
		float2 LightmapUV0, LightmapUV1;
		GetLightMapCoordinates(Interpolants, LightmapUV0, LightmapUV1);
		SkyOcclusionUV = LightmapUV0;
		GetLightMapColorHQ(LightmapUV0, LightmapUV1, DiffuseDir, OutDiffuseLighting, OutSubsurfaceLighting);
	// Low quality texture lightmaps
	#elif LQ_TEXTURE_LIGHTMAP
		float2 LightmapUV0, LightmapUV1;
		GetLightMapCoordinates(Interpolants, LightmapUV0, LightmapUV1);
		OutDiffuseLighting = GetLightMapColorLQ(LightmapUV0, LightmapUV1, DiffuseDir).rgb;
	#endif

	// Apply indirect lighting scale while we have only accumulated lightmaps
	OutDiffuseLighting *= View.IndirectLightingColorScale;
	OutSubsurfaceLighting *= View.IndirectLightingColorScale;

	float3 SkyDiffuseLighting;
	float3 SkySubsurfaceLighting;
	GetSkyLighting(DiffuseDir, SkyOcclusionUV, VolumetricLightmapBrickTextureUVs, SkyDiffuseLighting, SkySubsurfaceLighting);

	OutSubsurfaceLighting += SkySubsurfaceLighting;

	// Sky lighting must contribute to IndirectIrradiance for ReflectionEnvironment lightmap mixing
	OutDiffuseLighting += SkyDiffuseLighting;

	#if HQ_TEXTURE_LIGHTMAP || LQ_TEXTURE_LIGHTMAP || CACHED_VOLUME_INDIRECT_LIGHTING || CACHED_POINT_INDIRECT_LIGHTING || PRECOMPUTED_IRRADIANCE_VOLUME_LIGHTING
		OutIndirectIrradiance = Luminance(OutDiffuseLighting);
	#endif
}

// [ Jimenez et al. 2016, "Practical Realtime Strategies for Accurate Indirect Occlusion" ]
float3 AOMultiBounce( float3 BaseColor, float AO )
{
	float3 a =  2.0404 * BaseColor - 0.3324;
	float3 b = -4.7951 * BaseColor + 0.6417;
	float3 c =  2.7552 * BaseColor + 0.6903;
	return max( AO, ( ( AO * a + b ) * AO + c ) * AO );
}

void ApplyBentNormal( in MaterialPixelParameters MaterialParameters, in float Roughness, inout float3 BentNormal, inout float DiffOcclusion, inout float SpecOcclusion )
{
#if NUM_MATERIAL_OUTPUTS_GETBENTNORMAL > 0
	#if MATERIAL_TANGENTSPACENORMAL
		BentNormal = normalize( TransformTangentVectorToWorld( MaterialParameters.TangentToWorld, GetBentNormal0(MaterialParameters) ) );
	#else
		BentNormal = GetBentNormal0(MaterialParameters);
	#endif

	FSphericalGaussian HemisphereSG = Hemisphere_ToSphericalGaussian( MaterialParameters.WorldNormal );
	FSphericalGaussian NormalSG = ClampedCosine_ToSphericalGaussian( MaterialParameters.WorldNormal );
	FSphericalGaussian VisibleSG = BentNormalAO_ToSphericalGaussian( BentNormal, DiffOcclusion );
	FSphericalGaussian DiffuseSG = Mul( NormalSG, VisibleSG );
	
	float VisibleCosAngle = sqrt( 1 - DiffOcclusion );

#if 1	// Mix full resolution normal with low res bent normal
	BentNormal = DiffuseSG.Axis;
	//DiffOcclusion = saturate( Integral( DiffuseSG ) / Dot( NormalSG, HemisphereSG ) );
	DiffOcclusion = saturate( Integral( DiffuseSG ) * 0.42276995 );
#endif

	float3 N = MaterialParameters.WorldNormal;
	float3 V = MaterialParameters.CameraVector;

	SpecOcclusion  = DotSpecularSG( Roughness, N, V, VisibleSG );
	SpecOcclusion /= DotSpecularSG( Roughness, N, V, HemisphereSG );

	SpecOcclusion = saturate( SpecOcclusion );
#endif
}

// The selective output mask can only depend on defines, since the shadow will not export the data.
uint GetSelectiveOutputMask()
{
	uint Mask = 0;
#if !WRITES_CUSTOMDATA_TO_GBUFFER
	Mask |= SKIP_CUSTOMDATA_MASK;
#endif
#if !GBUFFER_HAS_PRECSHADOWFACTOR
	Mask |= SKIP_PRECSHADOW_MASK;
#endif
#if (GBUFFER_HAS_PRECSHADOWFACTOR && WRITES_PRECSHADOWFACTOR_ZERO)
	Mask |= ZERO_PRECSHADOW_MASK;
#endif
#if !WRITES_VELOCITY_TO_GBUFFER
	Mask |= SKIP_VELOCITY_MASK;
#endif
	return Mask;
}
#endif // USES_GBUFFER

void FPixelShaderInOut_PS_Main(
	VertexFactoryInterpolantsVSToPS Interpolants,
	BasePassInterpolantsVSToPS BasePassInterpolants,
	in PixelShaderIn In, 
	inout PixelShaderOut Out)
{
    ResolvedView = ResolveView();

	// Velocity
	float4 OutVelocity = 0;
	// CustomData
	float4 OutGBufferD = 0;
	// PreShadowFactor
	float4 OutGBufferE = 0;

    MaterialPixelParameters MaterialParameters = GetMaterialPixelParameters(Interpolants, In.SvPosition);
    PixelMaterialInputs PMInputs;

#if HQ_TEXTURE_LIGHTMAP && USES_AO_MATERIAL_MASK && !MATERIAL_SHADINGMODEL_UNLIT
	float2 LightmapUV0, LightmapUV1;
	GetLightMapCoordinates(Interpolants, LightmapUV0, LightmapUV1);
	// Must be computed before BaseColor, Normal, etc are evaluated
	MaterialParameters.AOMaterialMask = GetAOMaterialMask(LightmapUV0 * float2(1, 2));
#endif

    //#if USE_WORLD_POSITION_EXCLUDING_SHADER_OFFSETS
		{
			float4 ScreenPosition = SvPositionToResolvedScreenPosition(In.SvPosition);
			float3 TranslatedWorldPosition = SvPositionToResolvedTranslatedWorld(In.SvPosition);
			CalcMaterialParametersEx(
										MaterialParameters, 
										PMInputs, 
										In.SvPosition, 
										ScreenPosition, 
										In.bIsFrontFace, 
										TranslatedWorldPosition, 
										BasePassInterpolants.PixelPositionExcludingWPO);
		}
	// #else
	// 	{
	// 		float4 ScreenPosition = SvPositionToResolvedScreenPosition(In.SvPosition);
	// 		float3 TranslatedWorldPosition = SvPositionToResolvedTranslatedWorld(In.SvPosition);
	// 		CalcMaterialParametersEx(MaterialParameters, PMInputs, In.SvPosition, ScreenPosition, In.bIsFrontFace, TranslatedWorldPosition, TranslatedWorldPosition);
	// 	}
	// #endif


    half3 BaseColor = GetMaterialBaseColor(PMInputs);
	half  Metallic = GetMaterialMetallic(PMInputs);
	half  Specular = GetMaterialSpecular(PMInputs);

	float MaterialAO = GetMaterialAmbientOcclusion(PMInputs);
	float Roughness = GetMaterialRoughness(PMInputs);


    half Opacity = GetMaterialOpacity(PMInputs);

	float3 VolumetricLightmapBrickTextureUVs;
#if PRECOMPUTED_IRRADIANCE_VOLUME_LIGHTING
	VolumetricLightmapBrickTextureUVs = ComputeVolumetricLightmapBrickTextureUVs(MaterialParameters.AbsoluteWorldPosition);
#endif

	float SubsurfaceProfile = 0;
	float3 SubsurfaceColor = 0;

    GBufferData GBuffer = (GBufferData)0;

    GBuffer.GBufferAO = MaterialAO;
	GBuffer.PerObjectGBufferData = Primitive.PerObjectGBufferData;
	GBuffer.Depth = MaterialParameters.ScreenPosition.w;
    //see FPrecomputedLightingParameters
	GBuffer.PrecomputedShadowFactors = GetPrecomputedShadowMasks(Interpolants, MaterialParameters.AbsoluteWorldPosition, VolumetricLightmapBrickTextureUVs);

    SetGBufferForShadingModel(
        GBuffer, 
        MaterialParameters, 
        Opacity, 
        BaseColor, 
        Metallic, 
        Specular, 
        Roughness, 
        SubsurfaceColor,
        SubsurfaceProfile);

#if USES_GBUFFER
	    GBuffer.SelectiveOutputMask = GetSelectiveOutputMask();

    // #if WRITES_VELOCITY_TO_GBUFFER
	// {
	// 	// 2d velocity, includes camera an object motion
    // #if WRITES_VELOCITY_TO_GBUFFER_USE_POS_INTERPOLATOR
	// 	float2 Velocity = Calculate2DVelocity(BasePassInterpolants.VelocityScreenPosition, BasePassInterpolants.VelocityPrevScreenPosition);
	// #else
	// 	float2 Velocity = Calculate2DVelocity(MaterialParameters.ScreenPosition, BasePassInterpolants.VelocityPrevScreenPosition);
	// #endif

	// 	// Make sure not to touch 0,0 which is clear color
	// 	GBuffer.Velocity = float4(EncodeVelocityToTexture(Velocity), 0, 0) * BasePassInterpolants.VelocityPrevScreenPosition.z;
	// }
	// #else
		GBuffer.Velocity = 0;
	//#endif
#endif
    //???
    GBuffer.SpecularColor = lerp(0.08 * Specular.xxx, BaseColor, Metallic.xxx);

// #if MATERIAL_NORMAL_CURVATURE_TO_ROUGHNESS

// #endif

    GBuffer.DiffuseColor = BaseColor - BaseColor * Metallic;

    float3 BentNormal = MaterialParameters.WorldNormal;
	float DiffOcclusion = MaterialAO;
	float SpecOcclusion = MaterialAO;
    ApplyBentNormal( MaterialParameters, GBuffer.Roughness, BentNormal, DiffOcclusion, SpecOcclusion );
	// FIXME: ALLOW_STATIC_LIGHTING == 0 expects this to be AO
	GBuffer.GBufferAO = AOMultiBounce( Luminance( GBuffer.SpecularColor ), SpecOcclusion ).g;
    half3 DiffuseColor = 0;
	half3 Color = 0;
	float IndirectIrradiance = 0;

    #if !MATERIAL_SHADINGMODEL_UNLIT
        float3 DiffuseDir = BentNormal;
        float3 DiffuseColorForIndirect = GBuffer.DiffuseColor;

		#if MATERIAL_SHADINGMODEL_SUBSURFACE || MATERIAL_SHADINGMODEL_PREINTEGRATED_SKIN
		#endif
		#if MATERIAL_SHADINGMODEL_CLOTH
		#endif
		#if MATERIAL_SHADINGMODEL_HAIR
		#endif

        float3 DiffuseIndirectLighting;
        float3 SubsurfaceIndirectLighting;
        GetPrecomputedIndirectLightingAndSkyLight(MaterialParameters, Interpolants, BasePassInterpolants, DiffuseDir, VolumetricLightmapBrickTextureUVs, DiffuseIndirectLighting, SubsurfaceIndirectLighting, IndirectIrradiance);

        float IndirectOcclusion = 1.0f;
        float2 NearestResolvedDepthScreenUV = 0;

        //#if FORWARD_SHADING && (MATERIALBLENDING_SOLID || MATERIALBLENDING_MASKED)
        //#endif
		
        DiffuseColor += (DiffuseIndirectLighting * DiffuseColorForIndirect + SubsurfaceIndirectLighting * SubsurfaceColor) * AOMultiBounce( GBuffer.BaseColor, DiffOcclusion );

		// #if TRANSLUCENCY_PERVERTEX_FORWARD_SHADING
		// #elif FORWARD_SHADING || TRANSLUCENCY_LIGHTING_SURFACE_FORWARDSHADING || TRANSLUCENCY_LIGHTING_SURFACE_LIGHTINGVOLUME
		// #endif

		// #if SIMPLE_FORWARD_DIRECTIONAL_LIGHT
		// #endif
	#endif

	#if NEEDS_BASEPASS_VERTEX_FOGGING
		//float4 HeightFogging = BasePassInterpolants.VertexFog;
	#elif NEEDS_BASEPASS_PIXEL_FOGGING
		//float4 HeightFogging = CalculateHeightFog(MaterialParameters.WorldPosition_CamRelative);
	#else
		float4 HeightFogging = float4(0,0,0,1);
	#endif

#if NEEDS_BASEPASS_PIXEL_VOLUMETRIC_FOGGING
#endif

	float4 Fogging = HeightFogging;

	// Volume lighting for lit translucency
	// #if (MATERIAL_SHADINGMODEL_DEFAULT_LIT || MATERIAL_SHADINGMODEL_SUBSURFACE) && (MATERIALBLENDING_TRANSLUCENT || MATERIALBLENDING_ADDITIVE) && !SIMPLE_FORWARD_SHADING && !FORWARD_SHADING
	// #endif

	// #if !MATERIAL_SHADINGMODEL_UNLIT && USE_DEVELOPMENT_SHADERS
	// 	Color = lerp(Color, GBuffer.DiffuseColor + GBuffer.SpecularColor * 0.45, View.UnlitViewmodeMask);
	// #endif

    half3 Emissive = GetMaterialEmissive(PMInputs);

// #if USE_DEVELOPMENT_SHADERS
// #endif

#if !POST_PROCESS_SUBSURFACE
 	// For non-skin, we just add diffuse and non-diffuse color together, otherwise we need to keep them separate
	Color += DiffuseColor;
#endif
	Color += Emissive;

	#if MATERIAL_DOMAIN_POSTPROCESS
	#elif MATERIALBLENDING_ALPHACOMPOSITE
	#elif MATERIALBLENDING_TRANSLUCENT
	#elif MATERIALBLENDING_ADDITIVE
	#elif MATERIALBLENDING_MODULATE
	#else

	LightAccumulator LA = (LightAccumulator)0;

	// Apply vertex fog
	Color = Color * Fogging.a + Fogging.rgb;

#if POST_PROCESS_SUBSURFACE
#else
	LightAccumulator_Add(LA, Color, 0, 1.0f, false);
#endif

	Out.MRT[0] = RETURN_COLOR(LightAccumulator_GetResult(LA));

	#endif

    #if USES_GBUFFER
		GBuffer.IndirectIrradiance = IndirectIrradiance;
        float QuantizationBias = PseudoRandom( MaterialParameters.SvPosition.xy ) - 0.5f;
		EncodeGBuffer(GBuffer, Out.MRT[1], Out.MRT[2], Out.MRT[3], OutGBufferD, OutGBufferE, OutVelocity, QuantizationBias);
    #endif 

#if USES_GBUFFER
	#if GBUFFER_HAS_VELOCITY
		Out.MRT[4] = OutVelocity;
	#endif

	Out.MRT[GBUFFER_HAS_VELOCITY ? 5 : 4] = OutGBufferD;

	#if GBUFFER_HAS_PRECSHADOWFACTOR
		Out.MRT[GBUFFER_HAS_VELOCITY ? 6 : 5] = OutGBufferE;
	#endif
#endif

}

#define PIXELSHADEROUTPUT_MRT0 	(!USES_GBUFFER || ALLOW_STATIC_LIGHTING)
#define PIXELSHADEROUTPUT_MRT1  USES_GBUFFER
#define PIXELSHADEROUTPUT_MRT2  USES_GBUFFER
#define PIXELSHADEROUTPUT_MRT3  USES_GBUFFER


#ifndef PIXELSHADEROUTPUT_MRT0
	#define PIXELSHADEROUTPUT_MRT0 0
#endif
#ifndef PIXELSHADEROUTPUT_MRT1
	#define PIXELSHADEROUTPUT_MRT1 0
#endif
#ifndef PIXELSHADEROUTPUT_MRT2
	#define PIXELSHADEROUTPUT_MRT2 0
#endif
#ifndef PIXELSHADEROUTPUT_MRT3
	#define PIXELSHADEROUTPUT_MRT3 0
#endif
#ifndef PIXELSHADEROUTPUT_MRT4
	#define PIXELSHADEROUTPUT_MRT4 0
#endif
#ifndef PIXELSHADEROUTPUT_MRT5
	#define PIXELSHADEROUTPUT_MRT5 0
#endif
#ifndef PIXELSHADEROUTPUT_MRT6
	#define PIXELSHADEROUTPUT_MRT6 0
#endif
#ifndef PIXELSHADEROUTPUT_MRT7
	#define PIXELSHADEROUTPUT_MRT7 0
#endif

//[earlydepthstencil]
void PS_Main(
    VertexFactoryInterpolantsVSToPS Interpolants, 
    BasePassInterpolantsVSToPS BasePassInterpolants,
    in INPUT_POSITION_QUALIFIERS float4 SvPosition : SV_Position

#if PIXELSHADEROUTPUT_MRT0
	, out float4 OutTarget0 : SV_Target0
#endif

#if PIXELSHADEROUTPUT_MRT1
    , out float4 OutTarget1 : SV_Target1
#endif

#if PIXELSHADEROUTPUT_MRT2
    , out float4 OutTarget2 : SV_Target2
#endif

#if PIXELSHADEROUTPUT_MRT3
    , out float4 OutTarget3 : SV_Target3
#endif

#if PIXELSHADEROUTPUT_MRT4
    , out float4 OutTarget4 : SV_Target4
#endif

#if PIXELSHADEROUTPUT_MRT5
    , out float4 OutTarget5 : SV_Target5
#endif
    )
{
    PixelShaderIn PSIn = (PixelShaderIn)0;
    PixelShaderOut PSOut = (PixelShaderOut)0;

    FPixelShaderInOut_PS_Main(Interpolants, BasePassInterpolants, PSIn, PSOut);

#if PIXELSHADEROUTPUT_MRT0
	OutTarget0 = PSOut.MRT[0];
#endif

#if PIXELSHADEROUTPUT_MRT1
	OutTarget1 = PSOut.MRT[1];
#endif

#if PIXELSHADEROUTPUT_MRT2
	OutTarget2 = PSOut.MRT[2];
#endif

#if PIXELSHADEROUTPUT_MRT3
	OutTarget3 = PSOut.MRT[3];
#endif

#if PIXELSHADEROUTPUT_MRT4
	OutTarget4 = PSOut.MRT[4];
#endif

#if PIXELSHADEROUTPUT_MRT5
	OutTarget5 = PSOut.MRT[5];
#endif

#if PIXELSHADEROUTPUT_MRT6
	OutTarget6 = PSOut.MRT[6];
#endif

#if PIXELSHADEROUTPUT_MRT7
	OutTarget7 = PSOut.MRT[7];
#endif
}