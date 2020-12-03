#include "LightMap.h"
#include "DirectXTex.h"

using namespace DirectX;

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

std::shared_ptr<FLightMap2D> FLightMap2D::AllocateLightMap()
{
	std::shared_ptr<FLightMap2D> Result = std::make_shared<FLightMap2D>();
	{
		TexMetadata Metadata;
		ScratchImage sImage;
		if (S_OK == LoadFromDDSFile(TEXT("./dx11demo/PrecomputedLightingBuffer_LightMapTexture.dds"), DDS_FLAGS_NONE, &Metadata, sImage))
		{
			CreateShaderResourceView(D3D11Device, sImage.GetImages(), sImage.GetImageCount(), Metadata, &Result->Textures[0]);
		}
	}
	{
		TexMetadata Metadata;
		ScratchImage sImage;
		if (S_OK == LoadFromDDSFile(TEXT("./dx11demo/PrecomputedLightingBuffer_LightMapTexture.dds"), DDS_FLAGS_NONE, &Metadata, sImage))
		{
			CreateShaderResourceView(D3D11Device, sImage.GetImages(), sImage.GetImageCount(), Metadata, &Result->Textures[1]);
		}
	}
	{
		TexMetadata Metadata;
		ScratchImage sImage;
		if (S_OK == LoadFromDDSFile(TEXT("./dx11demo/PrecomputedLightingBuffer_SkyOcclusionTexture.dds"), DDS_FLAGS_NONE, &Metadata, sImage))
		{
			CreateShaderResourceView(D3D11Device, sImage.GetImages(), sImage.GetImageCount(), Metadata, &Result->SkyOcclusionTexture);
		}
	}
	Result->ScaleVectors[0] =  { 9.99999975e-06f, 9.99999975e-06f , 9.99999975e-06f, 9.99999975e-06f};
	Result->ScaleVectors[1] = { 9.99999975e-06f, 9.99999975e-06f , 9.99999975e-06f , 0};
	Result->AddVectors[0] = { 1.f, 1.f , 1.f ,-5.75f };
	Result->AddVectors[1] = { 0.f, 0.f , 0.f ,0.282094985f };
	Result->CoordinateScale = { 0.48438f, 0.96875f };
	Result->CoordinateBias = { 0.00781f, 0.01563f };
	return Result;
}
