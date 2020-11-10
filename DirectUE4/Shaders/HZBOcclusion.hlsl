#include "Common.hlsl"

float2 InvSize;
float4 InputUvFactorAndOffset;
float2 InputViewportMaxBound;

Texture2D		Texture;
SamplerState	TextureSampler;

void HZBBuildPS(noperspective float4 RenderTargetUVAndScreenPos : TEXCOORD0, out float4 OutColor : SV_Target0)
{
	float4 Depth;
	float2 UV[4];

#if STAGE == 0
	const float2 ViewUV = RenderTargetUVAndScreenPos.zw * float2(0.5f, -0.5f) + 0.5f;
	const float2 InUV = ViewUV * InputUvFactorAndOffset.xy + InputUvFactorAndOffset.zw;
	
	// min(..., InputViewportMaxBound) because we don't want to sample outside of the viewport
	// when the view size has odd dimensions on X/Y axis.
	UV[0] = min(InUV + float2(-0.25f, -0.25f) * InvSize, InputViewportMaxBound);
	UV[1] = min(InUV + float2( 0.25f, -0.25f) * InvSize, InputViewportMaxBound);
	UV[2] = min(InUV + float2(-0.25f,  0.25f) * InvSize, InputViewportMaxBound);
	UV[3] = min(InUV + float2( 0.25f,  0.25f) * InvSize, InputViewportMaxBound);

	Depth.x = SceneTexturesStruct.SceneDepthTexture.SampleLevel( SceneTexturesStruct.SceneDepthTextureSampler, UV[0], 0 ).r;
	Depth.y = SceneTexturesStruct.SceneDepthTexture.SampleLevel( SceneTexturesStruct.SceneDepthTextureSampler, UV[1], 0 ).r;
	Depth.z = SceneTexturesStruct.SceneDepthTexture.SampleLevel( SceneTexturesStruct.SceneDepthTextureSampler, UV[2], 0 ).r;
	Depth.w = SceneTexturesStruct.SceneDepthTexture.SampleLevel( SceneTexturesStruct.SceneDepthTextureSampler, UV[3], 0 ).r;
#else
	const float2 InUV = RenderTargetUVAndScreenPos.xy;
	
	UV[0] = InUV + float2(-0.25f, -0.25f) * InvSize;
	UV[1] = InUV + float2( 0.25f, -0.25f) * InvSize;
	UV[2] = InUV + float2(-0.25f,  0.25f) * InvSize;
	UV[3] = InUV + float2( 0.25f,  0.25f) * InvSize;

	Depth.x = Texture.SampleLevel( TextureSampler, UV[0], 0 ).r;
	Depth.y = Texture.SampleLevel( TextureSampler, UV[1], 0 ).r;
	Depth.z = Texture.SampleLevel( TextureSampler, UV[2], 0 ).r;
	Depth.w = Texture.SampleLevel( TextureSampler, UV[3], 0 ).r;
#endif

	OutColor = min( min(Depth.x, Depth.y), min(Depth.z, Depth.w) );
}