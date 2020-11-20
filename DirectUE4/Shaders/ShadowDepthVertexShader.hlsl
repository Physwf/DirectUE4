#include "Common.hlsl"
#include "/Generated/Material.hlsl"
#include "LocalVertexFactory.hlsl"
#include "ShadowDepthCommon.hlsl"

float4x4 ProjectionMatrix;

void SetShadowDepthOutputs(float4 WorldPosition, out float4 OutPosition, out float ShadowDepth)
{
    OutPosition = mul(WorldPosition, ProjectionMatrix);
    ShadowDepth = 0.f;
}


void VS_Main(PositionOnlyVertexFactoryInput Input, /*out FShadowDepthVSToPS OutParameters,*/ out float4 OutPosition : SV_POSITION)
{
    ResolvedView = ResolveView();

    float4 WorldPostion =   VertexFactoryGetWorldPosition(Input);
    // VertexFactoryIntermediates VFIntermediates = GetVertexFactoryIntermediates(Input);

    // float4 WorldPos = VertexFactoryGetWorldPosition(Input, VFIntermediates);
	// float3x3 TangentToLocal = VertexFactoryGetTangentToLocal(Input, VFIntermediates);

	// MaterialVertexParameters VertexParameters = GetMaterialVertexParameters(Input, VFIntermediates, WorldPos.xyz, TangentToLocal);
	// WorldPos.xyz += GetMaterialWorldPositionOffset(VertexParameters);
    float Dummy;
    SetShadowDepthOutputs(WorldPostion,OutPosition,Dummy);
}