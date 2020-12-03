#include "ShadowMap.h"
#include "DirectXTex.h"

using namespace DirectX;

std::shared_ptr<FShadowMap2D> FShadowMap2D::AllocateShadowMap()
{
	std::shared_ptr<FShadowMap2D> Result = std::make_shared<FShadowMap2D>();
	{
		TexMetadata Metadata;
		ScratchImage sImage;
		if (S_OK == LoadFromHDRFile(TEXT("./dx11demo/PrecomputedLightingBuffer_StaticShadowTexture.hdr"), &Metadata, sImage))
		{
			CreateShaderResourceView(D3D11Device, sImage.GetImages(), 1, Metadata, &Result->Texture);
		}
	}
	return Result;
}

FShadowMap2D::FShadowMap2D() :
	Texture(NULL),
	CoordinateScale(Vector2(0, 0)),
	CoordinateBias(Vector2(0, 0))
{
	for (int Channel = 0; Channel < 4; Channel++)
	{
		bChannelValid[Channel] = false;
	}
}

FShadowMap2D::FShadowMap2D(std::vector<uint32> InLightGuids)
	: FShadowMap(std::move(LightGuids))
	, Texture(nullptr)
	, CoordinateScale(Vector2(0, 0))
	, CoordinateBias(Vector2(0, 0))
{
	for (int Channel = 0; Channel < 4; Channel++)
	{
		bChannelValid[Channel] = false;
	}
}

FShadowMapInteraction FShadowMap2D::GetInteraction() const
{
	if (Texture)
	{
		return FShadowMapInteraction::Texture(Texture, CoordinateScale, CoordinateBias, bChannelValid, InvUniformPenumbraSize);
	}
	return FShadowMapInteraction::None();
}

void FShadowMap::Cleanup()
{

}
