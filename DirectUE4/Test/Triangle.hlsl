struct VertexIn
{
    float4 Position : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

VertexOut VS_Main(VertexIn vin)
{
    VertexOut vout;

    vout.PosH = vin.Position;
    vout.Color = vin.Color;

    return vout;
}

float4 PS_Main(VertexOut pin) : SV_Target
{
    return pin.Color;
}

