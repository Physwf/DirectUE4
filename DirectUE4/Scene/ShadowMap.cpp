#include "ShadowMap.h"

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

