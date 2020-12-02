#include "LightMap.h"

FLightMap::FLightMap()
	: bAllowHighQualityLightMaps(true)
{

}

void FLightMap::Cleanup()
{

}

FLightMap2D::FLightMap2D()
{
	Textures[0] = NULL;
	Textures[1] = NULL;
	SkyOcclusionTexture = NULL;
	AOMaterialMaskTexture = NULL;
}

FLightMap2D::FLightMap2D(const std::vector<uint32>& InLightGuids)
{
	LightGuids = InLightGuids;
	Textures[0] = NULL;
	Textures[1] = NULL;
	SkyOcclusionTexture = NULL;
	AOMaterialMaskTexture = NULL;
}

ID3D11ShaderResourceView* FLightMap2D::GetSkyOcclusionTexture() const
{
	return SkyOcclusionTexture;
}

ID3D11ShaderResourceView* FLightMap2D::GetAOMaterialMaskTexture() const
{
	return AOMaterialMaskTexture;
}

bool FLightMap2D::IsValid(uint32 BasisIndex) const
{
	return Textures[BasisIndex] != nullptr;
}

FLightMapInteraction FLightMap2D::GetInteraction() const
{
	bool bHighQuality = true;// AllowHighQualityLightmaps(InFeatureLevel);

	int32 LightmapIndex = bHighQuality ? 0 : 1;

	bool bValidTextures = Textures[LightmapIndex] /*&& Textures[LightmapIndex]->Resource*/;

	// When the FLightMap2D is first created, the textures aren't set, so that case needs to be handled.
	if (bValidTextures)
	{
		return FLightMapInteraction::Texture(Textures, SkyOcclusionTexture, AOMaterialMaskTexture, ScaleVectors, AddVectors, CoordinateScale, CoordinateBias, bHighQuality);
	}

	return FLightMapInteraction::None();
}
