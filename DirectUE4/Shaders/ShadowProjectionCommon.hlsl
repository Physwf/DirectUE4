

Texture2D ShadowDepthTexture;
SamplerState ShadowDepthTextureSampler;
// xy:unused, z:SoftTransitionScale
float3 SoftTransitionScale;
// xy:ShadowTexelSize.xy, zw:1/ShadowTexelSize.xy
float4 ShadowBufferSize;