struct VertexFactoryInterpolantsVSToPS
{
    TANGENTTOWORLD_INTERPOLATOR_BLOCK

#if INTERPOLATE_VERTEX_COLOR
	half4	Color : COLOR0;
#endif

#if NUM_TEX_COORD_INTERPOLATORS
	float4	TexCoords[(NUM_TEX_COORD_INTERPOLATORS+1)/2]	: TEXCOORD0;
#elif USE_PARTICLE_SUBUVS
	float4	TexCoords[1] : TEXCOORD0;
#endif

#if NEEDS_LIGHTMAP_COORDINATE
	float4	LightMapCoordinate : TEXCOORD4;
#endif

};

#if NUM_TEX_COORD_INTERPOLATORS /*|| USE_PARTICLE_SUBUVS*/
float2 GetUV(VertexFactoryInterpolantsVSToPS Interpolants, int UVIndex)
{
	float4 UVVector = Interpolants.TexCoords[UVIndex / 2];
	return UVIndex % 2 ? UVVector.zw : UVVector.xy;
}

void SetUV(inout VertexFactoryInterpolantsVSToPS Interpolants, int UVIndex, float2 InValue)
{
	[flatten]//FLATTEN
	if (UVIndex % 2)
	{
		Interpolants.TexCoords[UVIndex / 2].zw = InValue;
	}
	else
	{
		Interpolants.TexCoords[UVIndex / 2].xy = InValue;
	}
}
#endif

float4 GetTangentToWorld2(VertexFactoryInterpolantsVSToPS Interpolants)
{
	return Interpolants.TangentToWorld2;
}

float4 GetTangentToWorld0(VertexFactoryInterpolantsVSToPS Interpolants)
{
	return Interpolants.TangentToWorld0;
}

float4 GetColor(VertexFactoryInterpolantsVSToPS Interpolants)
{
    return 0;
}

void SetTangents(inout VertexFactoryInterpolantsVSToPS Interpolants, float3 InTangentToWorld0, float3 InTangentToWorld2, float InTangentToWorldSign)
{
	Interpolants.TangentToWorld0 = float4(InTangentToWorld0,0);
	Interpolants.TangentToWorld2 = float4(InTangentToWorld2,InTangentToWorldSign);
#if USE_WORLDVERTEXNORMAL_CENTER_INTERPOLATION
	Interpolants.TangentToWorld2_Center = Interpolants.TangentToWorld2;
#endif
}

void SetColor(inout VertexFactoryInterpolantsVSToPS Interpolants, float4 InValue)
{
#if INTERPOLATE_VERTEX_COLOR
	Interpolants.Color = InValue;
#endif
}

#if NEEDS_LIGHTMAP_COORDINATE
void GetLightMapCoordinates(VertexFactoryInterpolantsVSToPS Interpolants, out float2 LightmapUV0, out float2 LightmapUV1)
{
	LightmapUV0 = Interpolants.LightMapCoordinate.xy * float2( 1, 0.5 );
	LightmapUV1 = LightmapUV0 + float2( 0, 0.5 );
}

float2 GetShadowMapCoordinate(VertexFactoryInterpolantsVSToPS Interpolants)
{
	return Interpolants.LightMapCoordinate.zw;
}

void SetLightMapCoordinate(inout VertexFactoryInterpolantsVSToPS Interpolants, float2 InLightMapCoordinate, float2 InShadowMapCoordinate)
{
	Interpolants.LightMapCoordinate.xy = InLightMapCoordinate;
	Interpolants.LightMapCoordinate.zw = InShadowMapCoordinate;
}
#endif