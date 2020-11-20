#include "VertexFactoryCommon.hlsl"
#include "LocalVertexFactoryCommon.hlsl"

#include "/Generated/PrecomputedLightingBuffer.hlsl"

#ifndef MANUAL_VERTEX_FETCH
#define MANUAL_VERTEX_FETCH 0
#endif

#if MANUAL_VERTEX_FETCH
	Buffer<float4> VertexFetch_InstanceOriginBuffer;
	Buffer<float4> VertexFetch_InstanceTransformBuffer;
	Buffer<float4> VertexFetch_InstanceLightmapBuffer;

	uint InstanceOffset;
	uint VertexOffset;

	#define VF_ColorIndexMask_Index 0
	#define VF_NumTexcoords_Index 1
	#define FV_LightMapIndex_Index 2
#endif

struct VertexFactoryInput
{
    float4 Position 	        : ATTRIBUTE0;
#if !MANUAL_VERTEX_FETCH
	#if METAL_PROFILE
		float3	TangentX	: ATTRIBUTE1;
		// TangentZ.w contains sign of tangent basis determinant
		float4	TangentZ	: ATTRIBUTE2;

		float4	Color		: ATTRIBUTE3;
	#else
		half3	TangentX	: ATTRIBUTE1;
		// TangentZ.w contains sign of tangent basis determinant
		half4	TangentZ	: ATTRIBUTE2;

		half4	Color		: ATTRIBUTE3;
	#endif
#endif//!MANUAL_VERTEX_FETCH

#if NUM_MATERIAL_TEXCOORDS_VERTEX
	#if !MANUAL_VERTEX_FETCH
		#if GPUSKIN_PASS_THROUGH
			// These must match GPUSkinVertexFactory.usf
			float2	TexCoords[NUM_MATERIAL_TEXCOORDS_VERTEX] : ATTRIBUTE4;
			#if NUM_MATERIAL_TEXCOORDS_VERTEX > 4
				#error Too many texture coordinate sets defined on GPUSkin vertex input. Max: 4.
			#endif
		#else
			#if NUM_MATERIAL_TEXCOORDS_VERTEX > 1
				float4	PackedTexCoords4[NUM_MATERIAL_TEXCOORDS_VERTEX/2] : ATTRIBUTE4;
			#endif
			#if NUM_MATERIAL_TEXCOORDS_VERTEX == 1
				float2	PackedTexCoords2 : ATTRIBUTE4;
			#elif NUM_MATERIAL_TEXCOORDS_VERTEX == 3
				float2	PackedTexCoords2 : ATTRIBUTE5;
			#elif NUM_MATERIAL_TEXCOORDS_VERTEX == 5
				float2	PackedTexCoords2 : ATTRIBUTE6;
			#elif NUM_MATERIAL_TEXCOORDS_VERTEX == 7
				float2	PackedTexCoords2 : ATTRIBUTE7;
			#endif
		#endif
	#endif
#elif USE_PARTICLE_SUBUVS && !MANUAL_VERTEX_FETCH
	float2	TexCoords[1] : ATTRIBUTE4;
#endif

#if NEEDS_LIGHTMAP_COORDINATE && !MANUAL_VERTEX_FETCH
	float2	LightMapCoordinate : ATTRIBUTE15;
#endif

};

struct PositionOnlyVertexFactoryInput
{
    float4 Position 	        : ATTRIBUTE0;
};

struct VertexFactoryIntermediates
{
    half3x3 TangentToLocal;
	half3x3 TangentToWorld;
	half TangentToWorldSign;

    half4 Color;
};

half3x3 CalcTangentToLocal(VertexFactoryInput Input,out float TangentSign)
{
    half3x3 Result;

#if MANUAL_VERTEX_FETCH
	half3 TangentInputX = LocalVF.VertexFetch_PackedTangentsBuffer[2 * (VertexOffset + Input.VertexId) + 0].xyz;
	half4 TangentInputZ = LocalVF.VertexFetch_PackedTangentsBuffer[2 * (VertexOffset + Input.VertexId) + 1].xyzw;
#else
	half3 TangentInputX = Input.TangentX;
	half4 TangentInputZ = Input.TangentZ;
#endif

    //half3 TangentX = TangentBias(TangentInputX);
	//half4 TangentZ = TangentBias(TangentInputZ);

    half3 TangentX = TangentInputX;
	half4 TangentZ = TangentInputZ;

    TangentSign = TangentZ.w;

    half3 TangentY = cross(TangentZ.xyz,TangentX) * TangentZ.w;

    Result[0] = cross(TangentY,TangentZ.xyz) * TangentZ.w;
    Result[1] = TangentY;
    Result[2] = TangentZ.xyz;

    return Result;
}

half3x3 CalcTangentToWorldNoScale(in half3x3 TangentToLocal)
{
    half3x3 LocalToWorld = GetLocalToWorld3x3();
	half3 InvScale = Primitive.InvNonUniformScale.xyz;
	// LocalToWorld[0] *= InvScale.x;
	// LocalToWorld[1] *= InvScale.y;
	// LocalToWorld[2] *= InvScale.z;
    return mul(TangentToLocal,LocalToWorld);
}

half3x3 CalcTangentToWorld(VertexFactoryIntermediates Intermediates, half3x3 TangentToLocal)
{
    half3x3 TangentToWorld = CalcTangentToWorldNoScale(TangentToLocal);
    return TangentToWorld;
}
//颜色,切线空间坐标系
VertexFactoryIntermediates GetVertexFactoryIntermediates(VertexFactoryInput Input)
{
    VertexFactoryIntermediates Intermediates;
    Intermediates = (VertexFactoryIntermediates)0;

    Intermediates.Color = Input.Color;

    float TangentSign;
    Intermediates.TangentToLocal = CalcTangentToLocal(Input,TangentSign);
    Intermediates.TangentToWorld = CalcTangentToWorld(Intermediates,Intermediates.TangentToLocal);

    return Intermediates;
};

float4 CalcWorldPosition(float4 Position)
{
    return TransformLocalToTranslatedWorld(Position.xyz);
}

// @return translated world position
float4 VertexFactoryGetWorldPosition(VertexFactoryInput Input, VertexFactoryIntermediates Intermediates)
{
    return CalcWorldPosition(Input.Position);
}

/** for depth-only pass */
float4 VertexFactoryGetWorldPosition(PositionOnlyVertexFactoryInput Input)
{
	float4 Position = Input.Position;

    return CalcWorldPosition(Position);
}

/**
* Get the 3x3 tangent basis vectors for this vertex factory
* this vertex factory will calculate the binormal on-the-fly
*
* @param Input - vertex input stream structure
* @return 3x3 matrix
*/
half3x3 VertexFactoryGetTangentToLocal(VertexFactoryInput Input, VertexFactoryIntermediates Intermediates )
{
	return Intermediates.TangentToLocal;
}

/** Converts from vertex factory specific input to a FMaterialVertexParameters, which is used by vertex shader material inputs. */
MaterialVertexParameters GetMaterialVertexParameters(VertexFactoryInput Input, VertexFactoryIntermediates Intermediates, float3 WorldPosition, half3x3 TangentToLocal)
{
	MaterialVertexParameters Result = (MaterialVertexParameters)0;
	Result.WorldPosition = WorldPosition;
	Result.VertexColor = Intermediates.Color;

	// does not handle instancing!
	Result.TangentToWorld = Intermediates.TangentToWorld;

    Result.PreSkinnedPosition = Input.Position.xyz;
	Result.PreSkinnedNormal = TangentToLocal[2]; //TangentBias(Input.TangentZ.xyz);

#if MANUAL_VERTEX_FETCH && NUM_MATERIAL_TEXCOORDS_VERTEX
		const uint NumFetchTexCoords = LocalVF.VertexFetch_Parameters[VF_NumTexcoords_Index];
		[unroll]
		for (uint CoordinateIndex = 0; CoordinateIndex < NUM_MATERIAL_TEXCOORDS_VERTEX; CoordinateIndex++)
		{
			// Clamp coordinates to mesh's maximum as materials can request more than are available
			uint ClampedCoordinateIndex = min(CoordinateIndex, NumFetchTexCoords-1);
			Result.TexCoords[CoordinateIndex] = LocalVF.VertexFetch_TexCoordBuffer[NumFetchTexCoords * (VertexOffset + Input.VertexId) + ClampedCoordinateIndex];
		}
#elif NUM_MATERIAL_TEXCOORDS_VERTEX
		#if GPUSKIN_PASS_THROUGH
			[unroll] 
			for (int CoordinateIndex = 0; CoordinateIndex < NUM_MATERIAL_TEXCOORDS_VERTEX; CoordinateIndex++)
			{
				Result.TexCoords[CoordinateIndex] = Input.TexCoords[CoordinateIndex].xy;
			}
		#else
			#if (NUM_MATERIAL_TEXCOORDS_VERTEX > 1)
				[unroll]
				for(int CoordinateIndex = 0; CoordinateIndex < NUM_MATERIAL_TEXCOORDS_VERTEX-1; CoordinateIndex+=2)
				{
					Result.TexCoords[CoordinateIndex] = Input.PackedTexCoords4[CoordinateIndex/2].xy;
					if( CoordinateIndex+1 < NUM_MATERIAL_TEXCOORDS_VERTEX )
					{
						Result.TexCoords[CoordinateIndex+1] = Input.PackedTexCoords4[CoordinateIndex/2].zw;
					}
				}
			#endif	// NUM_MATERIAL_TEXCOORDS_VERTEX > 1
            #if 1//NUM_MATERIAL_TEXCOORDS_VERTEX % 2 == 1
				Result.TexCoords[NUM_MATERIAL_TEXCOORDS_VERTEX-1] = Input.PackedTexCoords2;
            #endif	// NUM_MATERIAL_TEXCOORDS_VERTEX % 2 == 1
		#endif
#endif  //MANUAL_VERTEX_FETCH && NUM_MATERIAL_TEXCOORDS_VERTEX
    return Result;
}

float4 VertexFactoryGetRasterizedWorldPosition(VertexFactoryInput Input, VertexFactoryIntermediates Intermediates, float4 InWorldPosition)
{
	return InWorldPosition;
}

VertexFactoryInterpolantsVSToPS VertexFactoryGetInterpolantsVSToPS(VertexFactoryInput Input, VertexFactoryIntermediates Intermediates, MaterialVertexParameters VertexParameters)
{
	VertexFactoryInterpolantsVSToPS Interpolants;
    Interpolants = (VertexFactoryInterpolantsVSToPS)0;

#if NUM_TEX_COORD_INTERPOLATORS
    float2 CustomizedUVs[NUM_TEX_COORD_INTERPOLATORS];
	GetMaterialCustomizedUVs(VertexParameters, CustomizedUVs);
	GetCustomInterpolators(VertexParameters, CustomizedUVs);

    [unroll]//UNROLL
	for (int CoordinateIndex = 0; CoordinateIndex < NUM_TEX_COORD_INTERPOLATORS; CoordinateIndex++)
	{
		SetUV(Interpolants, CoordinateIndex, CustomizedUVs[CoordinateIndex]);
	}
    //Interpolants.TexCoords[0].xy = VertexParameters.TexCoords[0];

#elif NUM_MATERIAL_TEXCOORDS_VERTEX == 0 && USE_PARTICLE_SUBUVS

#endif

#if NEEDS_LIGHTMAP_COORDINATE
	float2 LightMapCoordinate = 0;
	float2 ShadowMapCoordinate = 0;
	#if MANUAL_VERTEX_FETCH
		float2 LightMapCoordinateInput = LocalVF.VertexFetch_TexCoordBuffer[LocalVF.VertexFetch_Parameters[VF_NumTexcoords_Index] * (VertexOffset + Input.VertexId) + LocalVF.VertexFetch_Parameters[FV_LightMapIndex_Index]];
	#else
		float2 LightMapCoordinateInput = Input.LightMapCoordinate;
	#endif

	#if USE_INSTANCING
		LightMapCoordinate = LightMapCoordinateInput * PrecomputedLightingBuffer.LightMapCoordinateScaleBias.xy + GetInstanceLightMapBias(Intermediates);
	#else
		LightMapCoordinate = LightMapCoordinateInput * PrecomputedLightingBuffer.LightMapCoordinateScaleBias.xy + PrecomputedLightingBuffer.LightMapCoordinateScaleBias.zw;
	#endif
	#if STATICLIGHTING_TEXTUREMASK
		#if USE_INSTANCING
			ShadowMapCoordinate = LightMapCoordinateInput * PrecomputedLightingBuffer.ShadowMapCoordinateScaleBias.xy + GetInstanceShadowMapBias(Intermediates);
		#else
			ShadowMapCoordinate = LightMapCoordinateInput * PrecomputedLightingBuffer.ShadowMapCoordinateScaleBias.xy + PrecomputedLightingBuffer.ShadowMapCoordinateScaleBias.zw;
		#endif
	#endif	// STATICLIGHTING_TEXTUREMASK
	SetLightMapCoordinate(Interpolants, LightMapCoordinate, ShadowMapCoordinate);
#endif	// NEEDS_LIGHTMAP_COORDINATE

	SetTangents(Interpolants, Intermediates.TangentToWorld[0], Intermediates.TangentToWorld[2], Intermediates.TangentToWorldSign);
	SetColor(Interpolants, Intermediates.Color);

    return Interpolants;
}

MaterialPixelParameters GetMaterialPixelParameters(VertexFactoryInterpolantsVSToPS Interpolants, float4 SvPosition)
{
    MaterialPixelParameters Result = MakeInitializedMaterialPixelParameters();

    #if NUM_TEX_COORD_INTERPOLATORS
	[unroll]//UNROLL
	for( int CoordinateIndex = 0; CoordinateIndex < NUM_TEX_COORD_INTERPOLATORS; CoordinateIndex++ )
	{
		Result.TexCoords[CoordinateIndex] = GetUV(Interpolants, CoordinateIndex);
	}
    #endif

    half3 TangentToWorld0 = GetTangentToWorld0(Interpolants).xyz;
	half4 TangentToWorld2 = GetTangentToWorld2(Interpolants);
	Result.UnMirrored = TangentToWorld2.w;

	Result.VertexColor = GetColor(Interpolants);

    Result.TangentToWorld = AssembleTangentToWorld( TangentToWorld0, TangentToWorld2 );
	// Required for previewing materials that use ParticleColor
	//Result.Particle.Color = half4(1,1,1,1);

    Result.TwoSidedSign = 1;
    return Result;
}