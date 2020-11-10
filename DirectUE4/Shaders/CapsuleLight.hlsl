
#ifndef __CapsuleLight_H__
#define __CapsuleLight_H__

struct CapsuleLight
{
	float3	LightPos[2];
	float	Length;
	float	Radius;
	float	SoftRadius;
	float	DistBiasSqr;
};


float3 LineIrradiace(float3 N, float3 Line0, float3 Line1, float DistanceBiasSqr, out float CosSubtended, out float BaseIrradiance, out float NoL)
{
    return float3(0,0,0);
}

float SphereHorizonCosWrap(float NoL, float SinAlphaSqr)
{
#if 1
	float SinAlpha = sqrt(SinAlphaSqr);

	if(NoL < SinAlpha)
	{
		NoL = max(NoL,-SinAlpha);
#if 0
#else
		NoL = Pow2(SinAlpha+NoL) / (4 * SinAlphaSqr);
#endif
	}
#else
	NoL = saturate((NoL,SinAlphaSqr) / (1 + SinAlphaSqr);)
#endif
	return NoL;
}

//
float3 ClosestPointLineToPoint(float3 Line0, float3 Line1, float Length)
{
	float3 Line01 = Line1 - Line0;
	return Line0 + Line01 * saturate( -dot( Line01, Line0 ) / Pow2( Length ) );
}

// Closest point on line segment to ray
float3 ClosestPointLineToRay( float3 Line0, float3 Line1, float Length, float3 R )
{
	float3 L0 = Line0;
	float3 L1 = Line1;
	float3 Line01 = Line1 - Line0;

	// Shortest distance
	float A = Square( Length );
	float B = dot( R, Line01 );
	float t = saturate( dot( Line0, B*R - Line01 ) / (A - B*B) );

	return Line0 + t * Line01;
}

#endif