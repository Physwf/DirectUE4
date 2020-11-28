#ifndef __SceneTexturesCommon_H__
#define __SceneTexturesCommon_H__


float3 CalcSceneColor(float2 ScreenUV)
{
// #if SCENE_TEXTURES_DISABLED
// 	return float3(0.0f,0.0f,0.0f);
// #else
	return Texture2DSampleLevel(SceneTexturesStruct.SceneColorTexture, SceneTexturesStruct.SceneColorTextureSampler, ScreenUV, 0).rgb;
//#endif
}

float CalcSceneDepth(float2 ScreenUV)
{
   return ConvertFromDeviceZ(Texture2DSampleLevel(SceneTexturesStruct.SceneDepthTexture, SceneTexturesStruct.SceneDepthTextureSampler, ScreenUV, 0).r);
}

/** Returns DeviceZ which is the z value stored in the depth buffer. */
float LookupDeviceZ( float2 ScreenUV )
{
	// native Depth buffer lookup
	return Texture2DSampleLevel(SceneTexturesStruct.SceneDepthTexture, SceneTexturesStruct.SceneDepthTextureSampler, ScreenUV, 0).r;
}

#endif