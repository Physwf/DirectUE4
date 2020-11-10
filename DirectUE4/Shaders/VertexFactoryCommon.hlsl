#include "Common.hlsl"

//本地坐标到加上平移的世界坐标,
float4 TransformLocalToTranslatedWorld(float3 LocalPosition)
{
	float3 RotatedPosition = Primitive.LocalToWorld[0].xyz * LocalPosition.xxx + Primitive.LocalToWorld[1].xyz * LocalPosition.yyy + Primitive.LocalToWorld[2].xyz * LocalPosition.zzz;
	float3 TranslationWorld = Primitive.LocalToWorld[3].xyz;
	return float4(RotatedPosition + (TranslationWorld + ResolvedView.PreViewTranslation.xyz),1);
}