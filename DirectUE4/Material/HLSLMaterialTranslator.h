#pragma once

#include "MaterialCompiler.h"
#include "Material.h"

class FHLSLMaterialTranslator : public FMaterialCompiler
{
protected:
	FMaterial* Material;
	FMaterialCompilationOutput & MaterialCompilationOutput;

	/** Whether the translation succeeded. */
	uint32 bSuccess : 1;
	/** Whether the compute shader material inputs were compiled. */
	uint32 bCompileForComputeShader : 1;
	/** Whether the compiled material uses scene depth. */
	uint32 bUsesSceneDepth : 1;
	/** true if the material needs particle position. */
	uint32 bNeedsParticlePosition : 1;
	/** true if the material needs particle velocity. */
	uint32 bNeedsParticleVelocity : 1;
	/** true if the material needs particle relative time. */
	uint32 bNeedsParticleTime : 1;
	/** true if the material uses particle motion blur. */
	uint32 bUsesParticleMotionBlur : 1;
	/** true if the material needs particle random value. */
	uint32 bNeedsParticleRandom : 1;
	/** true if the material uses spherical particle opacity. */
	uint32 bUsesSphericalParticleOpacity : 1;
	/** true if the material uses particle sub uvs. */
	uint32 bUsesParticleSubUVs : 1;
	/** Boolean indicating using LightmapUvs */
	uint32 bUsesLightmapUVs : 1;
	/** Whether the material uses AO Material Mask */
	uint32 bUsesAOMaterialMask : 1;
	/** true if needs SpeedTree code */
	uint32 bUsesSpeedTree : 1;
	/** Boolean indicating the material uses worldspace position without shader offsets applied */
	uint32 bNeedsWorldPositionExcludingShaderOffsets : 1;
	/** true if the material needs particle size. */
	uint32 bNeedsParticleSize : 1;
	/** true if any scene texture expressions are reading from post process inputs */
	uint32 bNeedsSceneTexturePostProcessInputs : 1;
	/** true if any atmospheric fog expressions are used */
	uint32 bUsesAtmosphericFog : 1;
	/** true if the material reads vertex color in the pixel shader. */
	uint32 bUsesVertexColor : 1;
	/** true if the material reads particle color in the pixel shader. */
	uint32 bUsesParticleColor : 1;
	/** true if the material reads mesh particle transform in the pixel shader. */
	uint32 bUsesParticleTransform : 1;

	/** true if the material uses any type of vertex position */
	uint32 bUsesVertexPosition : 1;

	uint32 bUsesTransformVector : 1;
	// True if the current property requires last frame's information
	uint32 bCompilingPreviousFrame : 1;
	/** True if material will output accurate velocities during base pass rendering. */
	uint32 bOutputsBasePassVelocities : 1;
	uint32 bUsesPixelDepthOffset : 1;
	uint32 bUsesWorldPositionOffset : 1;
	uint32 bUsesEmissiveColor : 1;
	/** true if the Roughness input evaluates to a constant 1.0 */
	uint32 bIsFullyRough : 1;
	/** Tracks the number of texture coordinates used by this material. */
	uint32 NumUserTexCoords;
	/** Tracks the number of texture coordinates used by the vertex shader in this material. */
	uint32 NumUserVertexTexCoords;

	uint32 NumParticleDynamicParameters;
public:
	FHLSLMaterialTranslator(FMaterial* InMaterial, 
		FMaterialCompilationOutput& InMaterialCompilationOutput
		) :
		  Material(InMaterial)
		, MaterialCompilationOutput(InMaterialCompilationOutput)
		, bSuccess(false)
		, bCompileForComputeShader(false)
		, bUsesSceneDepth(false)
		, bNeedsParticlePosition(false)
		, bNeedsParticleVelocity(false)
		, bNeedsParticleTime(false)
		, bUsesParticleMotionBlur(false)
		, bNeedsParticleRandom(false)
		, bUsesSphericalParticleOpacity(false)
		, bUsesParticleSubUVs(false)
		, bUsesLightmapUVs(false)
		, bUsesAOMaterialMask(false)
		, bUsesSpeedTree(false)
		, bNeedsWorldPositionExcludingShaderOffsets(false)
		, bNeedsParticleSize(false)
		, bNeedsSceneTexturePostProcessInputs(false)
		, bUsesAtmosphericFog(false)
		, bUsesVertexColor(false)
		, bUsesParticleColor(false)
		, bUsesParticleTransform(false)
		, bUsesVertexPosition(false)
		, bUsesTransformVector(false)
		, bCompilingPreviousFrame(false)
		, bOutputsBasePassVelocities(true)
		, bUsesPixelDepthOffset(false)
		, bUsesWorldPositionOffset(false)
		, bUsesEmissiveColor(0)
		, bIsFullyRough(0)
		, NumUserTexCoords(0)
		, NumUserVertexTexCoords(0)
		, NumParticleDynamicParameters(0)
	{}

	bool Translate()
	{
		bUsesPixelDepthOffset = false;
		bUsesEmissiveColor = true;
		return true;
	}

	void GetMaterialEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
// 		if (bNeedsParticlePosition || Material->ShouldGenerateSphericalParticleNormals() || bUsesSphericalParticleOpacity)
// 		{
// 			OutEnvironment.SetDefine(TEXT("NEEDS_PARTICLE_POSITION"), 1);
// 		}

// 		if (bNeedsParticleVelocity)
// 		{
// 			OutEnvironment.SetDefine(TEXT("NEEDS_PARTICLE_VELOCITY"), 1);
// 		}

// 		if (NumParticleDynamicParameters > 0)
// 		{
// 			OutEnvironment.SetDefine(TEXT("USE_DYNAMIC_PARAMETERS"), 1);
// 			OutEnvironment.SetDefine(TEXT("NUM_DYNAMIC_PARAMETERS"), NumParticleDynamicParameters);
// 		}

// 		if (bNeedsParticleTime)
// 		{
// 			OutEnvironment.SetDefine(TEXT("NEEDS_PARTICLE_TIME"), 1);
// 		}

// 		if (bUsesParticleMotionBlur)
// 		{
// 			OutEnvironment.SetDefine(TEXT("USES_PARTICLE_MOTION_BLUR"), 1);
// 		}

// 		if (bNeedsParticleRandom)
// 		{
// 			OutEnvironment.SetDefine(TEXT("NEEDS_PARTICLE_RANDOM"), 1);
// 		}

// 		if (bUsesSphericalParticleOpacity)
// 		{
// 			OutEnvironment.SetDefine(TEXT("SPHERICAL_PARTICLE_OPACITY"), TEXT("1"));
// 		}

// 		if (bUsesParticleSubUVs)
// 		{
// 			OutEnvironment.SetDefine(TEXT("USE_PARTICLE_SUBUVS"), TEXT("1"));
// 		}

// 		if (bUsesLightmapUVs)
// 		{
// 			OutEnvironment.SetDefine(("LIGHTMAP_UV_ACCESS"), ("1"));
// 		}

// 		if (bUsesAOMaterialMask)
// 		{
// 			OutEnvironment.SetDefine(("USES_AO_MATERIAL_MASK"), ("1"));
// 		}

// 		if (bUsesSpeedTree)
// 		{
// 			OutEnvironment.SetDefine(TEXT("USES_SPEEDTREE"), TEXT("1"));
// 		}

// 		if (bNeedsWorldPositionExcludingShaderOffsets)
// 		{
// 			OutEnvironment.SetDefine(("NEEDS_WORLD_POSITION_EXCLUDING_SHADER_OFFSETS"), TEXT("1"));
// 		}

// 		if (bNeedsParticleSize)
// 		{
// 			OutEnvironment.SetDefine(TEXT("NEEDS_PARTICLE_SIZE"), TEXT("1"));
// 		}

// 		if (MaterialCompilationOutput.bNeedsSceneTextures)
// 		{
// 			OutEnvironment.SetDefine(("NEEDS_SCENE_TEXTURES"), ("1"));
// 		}
// 		if (MaterialCompilationOutput.bUsesEyeAdaptation)
// 		{
// 			OutEnvironment.SetDefine(("USES_EYE_ADAPTATION"), ("1"));
// 		}

		// @todo MetalMRT: Remove this hack and implement proper atmospheric-fog solution for Metal MRT...
// 		OutEnvironment.SetDefine(("MATERIAL_ATMOSPHERIC_FOG"), (InPlatform != SP_METAL_MRT && InPlatform != SP_METAL_MRT_MAC) ? bUsesAtmosphericFog : 0);
// 		OutEnvironment.SetDefine(("INTERPOLATE_VERTEX_COLOR"), bUsesVertexColor);
// 		OutEnvironment.SetDefine(("NEEDS_PARTICLE_COLOR"), bUsesParticleColor);
// 		OutEnvironment.SetDefine(("NEEDS_PARTICLE_TRANSFORM"), bUsesParticleTransform);
// 		OutEnvironment.SetDefine(("USES_TRANSFORM_VECTOR"), bUsesTransformVector);
		OutEnvironment.SetDefine(("WANT_PIXEL_DEPTH_OFFSET"), bUsesPixelDepthOffset);
// 		if (IsMetalPlatform(InPlatform))
// 		{
// 			OutEnvironment.SetDefine(("USES_WORLD_POSITION_OFFSET"), bUsesWorldPositionOffset);
// 		}
		OutEnvironment.SetDefine(("USES_EMISSIVE_COLOR"), bUsesEmissiveColor);
		// Distortion uses tangent space transform 
		//OutEnvironment.SetDefine(("USES_DISTORTION"), Material->IsDistorted());

		//OutEnvironment.SetDefine(("MATERIAL_ENABLE_TRANSLUCENCY_FOGGING"), Material->ShouldApplyFogging());
		//OutEnvironment.SetDefine(("MATERIAL_COMPUTE_FOG_PER_PIXEL"), Material->ComputeFogPerPixel());
		//OutEnvironment.SetDefine(("MATERIAL_FULLY_ROUGH"), bIsFullyRough || Material->IsFullyRough());
// 
// 		for (int32 CollectionIndex = 0; CollectionIndex < ParameterCollections.Num(); CollectionIndex++)
// 		{
// 			// Add uniform buffer declarations for any parameter collections referenced
// 			const FString CollectionName = FString::Printf(TEXT("MaterialCollection%u"), CollectionIndex);
// 			FShaderUniformBufferParameter::ModifyCompilationEnvironment(*CollectionName, ParameterCollections[CollectionIndex]->GetUniformBufferStruct(), InPlatform, OutEnvironment);
// 		}
		OutEnvironment.SetDefine(("IS_MATERIAL_SHADER"), ("1"));
	}

	std::string GetMaterialShaderCode()
	{
		std::string MaterialSource;
		LoadShaderSourceFile("Material.dusf", MaterialSource);
		return MaterialSource;
	}
};