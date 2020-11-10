#ifndef __UniformBuffer_Material_Definition__
#define __UniformBuffer_Material_Definition__

cbuffer Material
{
	half4 Material_VectorExpressions[5];
}
Texture2D<float4> Material_BaseColor;
Texture2D<float4> Material_Normal;
Texture2D<float> Material_Gloss;
Texture2D<float> Material_Specular;
SamplerState Material_BaseColorSampler;
SamplerState Material_NormalSampler;
SamplerState Material_GlossSampler;
SamplerState Material_SpecularSampler;
static const struct
{
    half4 VectorExpressions[5];
	Texture2D<float4> BaseColor;
    Texture2D<float4> Normal;
    Texture2D<float> Gloss;
    Texture2D<float> Specular;
    SamplerState BaseColorSampler;
    SamplerState NormalSampler;
    SamplerState GlossSampler;
    SamplerState SpecularSampler;
} Material = 
{ 
    Material_VectorExpressions,
    Material_BaseColor,
    Material_Normal,
    Material_Gloss,
    Material_Specular,
    Material_BaseColorSampler,
    Material_NormalSampler,
    Material_GlossSampler,
    Material_SpecularSampler,
};

#endif