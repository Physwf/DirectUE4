struct VSIn
{
    float4 Position : POSITION;
    float4 Color : COLOR;
};

struct VSOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

struct GSOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

VSOut VS_Main(VSIn vin)
{
    VSOut vout;

    vout.PosH = vin.Position;
    vout.Color = vin.Color;

    return vout;
}

[maxvertexcount(128)]
void GS_Main(point VSOut input[1], inout TriangleStream<VSOut> OutputStream)
{
    VSOut p1,p2,p3;
    p1.PosH    =  input[0].PosH    + float4(0.0f,0.1f,0.0f,0.0f);
    p1.Color   =  input[0].Color;
    OutputStream.Append(p1);
    p2.PosH    =  input[0].PosH    + float4(0.1f,0.0f,0.0f,0.0f);
    p2.Color   =  input[0].Color;
    OutputStream.Append(p2);
    p3.PosH    =  input[0].PosH    + float4(-0.1f,0.0f,0.0f,0.0f);
    p3.Color   =  input[0].Color;
    OutputStream.Append(p3);
}

float4 PS_Main(VSOut pin) : SV_Target
{
    return pin.Color;
}


