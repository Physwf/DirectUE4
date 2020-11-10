#include "Common.hlsl"

#include "Material.hlsl"
#include "BasePassCommon.hlsl"
#include "LocalVertexFactory.hlsl"

struct BasePassVSToPS
{
	VertexFactoryInterpolantsVSToPS FactoryInterpolants;
	BasePassInterpolantsVSToPS BasePassInterpolants;
	float4 Position : SV_POSITION;
};

#define BasePassVSOutput BasePassVSToPS
#define VertexFactoryGetInterpolants VertexFactoryGetInterpolantsVSToPS