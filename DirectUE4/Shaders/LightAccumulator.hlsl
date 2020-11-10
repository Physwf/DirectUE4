#ifndef __LIGHTACCUMULATOR_COMMON__
#define __LIGHTACCUMULATOR_COMMON__

// set by c++, not set for LPV
// 0 / 1
#ifndef VISUALIZE_LIGHT_CULLING
	#define VISUALIZE_LIGHT_CULLING 0
#endif

#define SUBSURFACE_CHANNEL_MODE 1


struct LightAccumulator
{
    float3 TotalLight;

    float ScatterableLightLuma;

    float3 ScatterableLight;

    float EstimatedCost;
};

void LightAccumulator_Add(inout LightAccumulator In, float3 TotalLight, float3 ScatterableLight, float3 CommonMultiplier, const bool bNeedsSeparateSubsurfaceLightAccumulation)
{
    In.TotalLight += TotalLight * CommonMultiplier;

    if (bNeedsSeparateSubsurfaceLightAccumulation)
	{
		if (SUBSURFACE_CHANNEL_MODE == 1)
		{
			// if (View.bCheckerboardSubsurfaceProfileRendering == 0)
			// {
			// 	In.ScatterableLightLuma += Luminance(ScatterableLight * CommonMultiplier);
			// }
		}
		else if (SUBSURFACE_CHANNEL_MODE == 2)
		{
			// 3 mad
			In.ScatterableLight += ScatterableLight * CommonMultiplier;
		}
    }
}

float4 LightAccumulator_GetResult(LightAccumulator In)
{
    float4 Ret;

	if (VISUALIZE_LIGHT_CULLING == 1)
	{
		// a soft gradient from dark red to bright white, can be changed to be different
		Ret = 0.1f * float4(1.0f, 0.25f, 0.075f, 0) * In.EstimatedCost;
	}
	else
	{
		Ret = float4(In.TotalLight, 0);

		if (SUBSURFACE_CHANNEL_MODE == 1 )
		{
			// if (View.bCheckerboardSubsurfaceProfileRendering == 0)
			// {
			// 	// RGB accumulated RGB HDR color, A: specular luminance for screenspace subsurface scattering
			// 	Ret.a = In.ScatterableLightLuma;
			// }
		}
		else if (SUBSURFACE_CHANNEL_MODE == 2)
		{
			// RGB accumulated RGB HDR color, A: view independent (diffuse) luminance for screenspace subsurface scattering
			// 3 add,  1 mul, 2 mad, can be optimized to use 2 less temporary during accumulation and remove the 3 add
			Ret.a = Luminance(In.ScatterableLight);
			// todo, need second MRT for SUBSURFACE_CHANNEL_MODE==2
		}
	}

	return Ret;
}
#endif