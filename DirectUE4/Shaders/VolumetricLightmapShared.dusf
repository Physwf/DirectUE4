
#if SUPPORTS_INDEPENDENT_SAMPLERS
	#define PIVSharedAmbientSampler View.SharedBilinearClampedSampler
	#define PIVSharedSampler0 View.SharedBilinearClampedSampler
	#define PIVSharedSampler1 View.SharedBilinearClampedSampler
	#define PIVSharedSampler2 View.SharedBilinearClampedSampler
	#define PIVSharedSampler3 View.SharedBilinearClampedSampler
	#define PIVSharedSampler4 View.SharedBilinearClampedSampler
	#define PIVSharedSampler5 View.SharedBilinearClampedSampler
	#define VLDirectionalLightShadowingSampler View.SharedBilinearClampedSampler
	#define VLSkyBentNormalSharedSampler View.SharedBilinearClampedSampler
#else
	#define PIVSharedAmbientSampler View.VolumetricLightmapBrickAmbientVectorSampler
	#define PIVSharedSampler0 View.VolumetricLightmapTextureSampler0
	#define PIVSharedSampler1 View.VolumetricLightmapTextureSampler1
	#define PIVSharedSampler2 View.VolumetricLightmapTextureSampler2
	#define PIVSharedSampler3 View.VolumetricLightmapTextureSampler3
	#define PIVSharedSampler4 View.VolumetricLightmapTextureSampler4
	#define PIVSharedSampler5 View.VolumetricLightmapTextureSampler5
	#define VLDirectionalLightShadowingSampler View.DirectionalLightShadowingTextureSampler
	#define VLSkyBentNormalSharedSampler View.SkyBentNormalTextureSampler
#endif

float3 ComputeVolumetricLightmapBrickTextureUVs(float3 WorldPosition)
{
	// Compute indirection UVs from world position
	float3 IndirectionVolumeUVs = clamp(WorldPosition * View.VolumetricLightmapWorldToUVScale + View.VolumetricLightmapWorldToUVAdd, 0.0f, .99f);
	float3 IndirectionTextureTexelCoordinate = IndirectionVolumeUVs * View.VolumetricLightmapIndirectionTextureSize;
	float4 BrickOffsetAndSize = View.VolumetricLightmapIndirectionTexture.Load(int4(IndirectionTextureTexelCoordinate, 0));

	float PaddedBrickSize = View.VolumetricLightmapBrickSize + 1;
	return (BrickOffsetAndSize.xyz * PaddedBrickSize + frac(IndirectionTextureTexelCoordinate / BrickOffsetAndSize.w) * View.VolumetricLightmapBrickSize + .5f) * View.VolumetricLightmapBrickTexelSize;
}

float GetVolumetricLightmapDirectionalLightShadowing(float3 BrickTextureUVs)
{
	return Texture3DSampleLevel(View.DirectionalLightShadowingBrickTexture, VLDirectionalLightShadowingSampler, BrickTextureUVs, 0).x;
}