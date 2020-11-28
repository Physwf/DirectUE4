

float RadialAttenuation(float3 WorldLightVector, half FalloffExponent)
{
    float3 NormalizeDistanceSquared = dot(WorldLightVector,WorldLightVector);

    return pow(1.0f - saturate(NormalizeDistanceSquared), FalloffExponent);
}

float SpotAttenuation(float3 L, float3 SpotDirection,float2 SpotAngles)
{
    float ConeAngleFalloff = Square(saturate((dot(L,-SpotDirection) - SpotAngles.x)*SpotAngles.y));
    return ConeAngleFalloff;
}

