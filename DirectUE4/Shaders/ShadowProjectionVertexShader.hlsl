#include "Common.hlsl"

void Main(in float4 InPosition : ATTRIBUTE0, out float4 OutPosition : SV_Position )
{
    OutPosition = float4(InPosition.xyz,1);
}