struct VertexIn
{
	float3 Position 	: POSITION;
	float3 Normal 		: NORMAL;
	float2 TexCoord0	: TEXCOORD;
	float2 TexCoord1	: TEXCOORD;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
};

cbuffer LightViewUniform : register(b0)
{
	float4x4 WorldToView;
	float4x4 ViewToClip;
	float4x4 WorldToClip;
};

VertexOut VS_Main(VertexIn vin)
{
	VertexOut vout;
	vout.PosH = mul(WorldToClip, WorldPostion);
	return vout;
}

float4 PS_Main(VertexOut pin) : SV_Target
{
	return 0;
}
