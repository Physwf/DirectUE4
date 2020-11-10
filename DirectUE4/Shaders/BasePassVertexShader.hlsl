#include "BasePassVertexCommon.hlsl"
// #include "SHCommon.ush"
// #include "VolumetricLightmapShared.ush"

// #if TRANSLUCENCY_PERVERTEX_FORWARD_SHADING
// #include "ReflectionEnvironmentShared.ush"
// #include "ForwardLightingCommon.ush"
// #endif

void VS_Main(VertexFactoryInput Input, out BasePassVSOutput Output)
{
    ResolvedView = ResolveView();

    VertexFactoryIntermediates VFIntermediates = GetVertexFactoryIntermediates(Input);

    //WPO==World Position Offset
    float4 WorldPositionExcludingWPO = VertexFactoryGetWorldPosition(Input, VFIntermediates);
    float4 WorldPosition = WorldPositionExcludingWPO;
    float4 ClipSpacePosition;

    float3x3 TangentToLocal = VertexFactoryGetTangentToLocal(Input, VFIntermediates);	
    MaterialVertexParameters VertexParameters = GetMaterialVertexParameters(Input, VFIntermediates, WorldPosition.xyz, TangentToLocal);
    
    //ISOLATE
	{
		//WorldPosition.xyz += GetMaterialWorldPositionOffset(VertexParameters);
	}

    float4 RasterizedWorldPosition = VertexFactoryGetRasterizedWorldPosition(Input, VFIntermediates, WorldPosition);
    ClipSpacePosition = mul(WorldPositionExcludingWPO, ResolvedView.TranslatedWorldToClip);
    
    Output.Position = ClipSpacePosition; //INVARIANT(ClipSpacePosition);

    Output.FactoryInterpolants = VertexFactoryGetInterpolants(Input, VFIntermediates, VertexParameters);
    
// #if NEEDS_BASEPASS_VERTEX_FOGGING
// #endif

// #if TRANSLUCENCY_ANY_PERVERTEX_LIGHTING
// #endif

// #if TRANSLUCENCY_PERVERTEX_LIGHTING_VOLUME
// #elif TRANSLUCENCY_PERVERTEX_FORWARD_SHADING
// #endif

// #if WRITES_VELOCITY_TO_GBUFFER
// #endif
}

