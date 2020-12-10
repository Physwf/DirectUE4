//#include "SubsurfaceProfileCommon.ush"


void SetGBufferForShadingModel(
	in out FGBufferData GBuffer, 
	in const FMaterialPixelParameters MaterialParameters,
	const float Opacity,
	const half3 BaseColor,
	const half  Metallic,
	const half  Specular,
	const float Roughness,
	const float3 SubsurfaceColor,
	const float SubsurfaceProfile)
{
	GBuffer.WorldNormal = MaterialParameters.WorldNormal;
	GBuffer.BaseColor = BaseColor;
	GBuffer.Metallic = Metallic;
	GBuffer.Specular = Specular;
	GBuffer.Roughness = Roughness;

    GBuffer.ShadingModelID = SHADINGMODELID_DEFAULT_LIT;
}