#include "AtmosphereRendering.h"
#include "RenderTargets.h"
#include "Scene.h"
#include "DeferredShading.h"
#include "GPUProfiler.h"
#include "GlobalShader.h"
#include "AtmosphereTextureParameters.h"
#include "AtmosphereFog.h"

void FAtmosphereShaderTextureParameters::Bind(const FShaderParameterMap& ParameterMap)
{
	TransmittanceTexture.Bind(ParameterMap, ("AtmosphereTransmittanceTexture"));
	TransmittanceTextureSampler.Bind(ParameterMap, ("AtmosphereTransmittanceTextureSampler"));
	IrradianceTexture.Bind(ParameterMap, ("AtmosphereIrradianceTexture"));
	IrradianceTextureSampler.Bind(ParameterMap, ("AtmosphereIrradianceTextureSampler"));
	InscatterTexture.Bind(ParameterMap, ("AtmosphereInscatterTexture"));
	InscatterTextureSampler.Bind(ParameterMap, ("AtmosphereInscatterTextureSampler"));
}



/** A pixel shader for rendering atmospheric fog. */
class FAtmosphericFogPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FAtmosphericFogPS, Global);
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		//return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
		return true;
	}

	FAtmosphericFogPS() {}
	FAtmosphericFogPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		SceneTextureParameters.Bind(Initializer);
		AtmosphereTextureParameters.Bind(Initializer.ParameterMap);
		OcclusionTextureParameter.Bind(Initializer.ParameterMap, ("OcclusionTexture"));
		OcclusionTextureSamplerParameter.Bind(Initializer.ParameterMap, ("OcclusionTextureSampler"));
	}

	void SetParameters(const FSceneView& View, const ComPtr<PooledRenderTarget>& LightShaftOcclusion)
	{
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(GetPixelShader(), View.ViewUniformBuffer.get());
		SceneTextureParameters.Set(GetPixelShader(), ESceneTextureSetupMode::All);
		AtmosphereTextureParameters.Set(GetPixelShader(), View);

		if (LightShaftOcclusion)
		{
			SetTextureParameter(
				GetPixelShader(),
				OcclusionTextureParameter, OcclusionTextureSamplerParameter,
				TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI(),
				LightShaftOcclusion->ShaderResourceTexture->GetShaderResourceView()
			);
		}
		else
		{
			SetTextureParameter(
				GetPixelShader(),
				OcclusionTextureParameter, OcclusionTextureSamplerParameter,
				TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI(),
				GWhiteTextureSRV
			);
		}
	}

private:
	FSceneTextureShaderParameters SceneTextureParameters;
	FAtmosphereShaderTextureParameters AtmosphereTextureParameters;
	FShaderResourceParameter OcclusionTextureParameter;
	FShaderResourceParameter OcclusionTextureSamplerParameter;
};

template<uint32 RenderFlag>
class TAtmosphericFogPS : public FAtmosphericFogPS
{
	DECLARE_SHADER_TYPE(TAtmosphericFogPS, Global);

	/** Default constructor. */
	TAtmosphericFogPS() {}
public:
	TAtmosphericFogPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FAtmosphericFogPS(Initializer)
	{
	}

	/**
	* Add any compiler flags/defines required by the shader
	* @param OutEnvironment - shader environment to modify
	*/
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FAtmosphericFogPS::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(("ATMOSPHERIC_NO_SUN_DISK"), (RenderFlag & EAtmosphereRenderFlag::E_DisableSunDisk));
		OutEnvironment.SetDefine(("ATMOSPHERIC_NO_GROUND_SCATTERING"), (RenderFlag & EAtmosphereRenderFlag::E_DisableGroundScattering));
		OutEnvironment.SetDefine(("ATMOSPHERIC_NO_LIGHT_SHAFT"), (RenderFlag & EAtmosphereRenderFlag::E_DisableLightShaft));
	}
};

#define SHADER_VARIATION(RenderFlag) IMPLEMENT_SHADER_TYPE(template<>,TAtmosphericFogPS<RenderFlag>, ("AtmosphericFogShader.dusf"), ("AtmosphericPixelMain"), SF_Pixel);
SHADER_VARIATION(EAtmosphereRenderFlag::E_EnableAll)
SHADER_VARIATION(EAtmosphereRenderFlag::E_DisableSunDisk)
SHADER_VARIATION(EAtmosphereRenderFlag::E_DisableGroundScattering)
SHADER_VARIATION(EAtmosphereRenderFlag::E_DisableSunAndGround)
SHADER_VARIATION(EAtmosphereRenderFlag::E_DisableLightShaft)
SHADER_VARIATION(EAtmosphereRenderFlag::E_DisableSunAndLightShaft)
SHADER_VARIATION(EAtmosphereRenderFlag::E_DisableGroundAndLightShaft)
SHADER_VARIATION(EAtmosphereRenderFlag::E_DisableAll)
#undef SHADER_VARIATION

std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>> GetAtmopshereVertexDeclaration()
{
	static std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>> AtmopshereVertexDeclaration = std::make_shared<std::vector<D3D11_INPUT_ELEMENT_DESC>,size_t, D3D11_INPUT_ELEMENT_DESC>
		(
			1, { "ATTRIBUTE",0,DXGI_FORMAT_R32G32_FLOAT,0,0, D3D11_INPUT_PER_VERTEX_DATA,0 }
		);
	return AtmopshereVertexDeclaration;
}

/** A vertex shader for rendering height fog. */
class FAtmosphericVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FAtmosphericVS, Global);
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		//return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
		return true;
	}

	FAtmosphericVS() { }
	FAtmosphericVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
	}

	void SetParameters(const FViewInfo& View)
	{
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(GetVertexShader(), View.ViewUniformBuffer.get());
	}
};

IMPLEMENT_SHADER_TYPE(, FAtmosphericVS, ("AtmosphericFogShader.dusf"), ("VSMain"), SF_Vertex);

bool ShouldRenderAtmosphere(const FSceneViewFamily& Family)
{
	return true;
}

void InitAtmosphereConstantsInView(FViewInfo& View)
{
	bool bInitTextures = false;
	if (ShouldRenderAtmosphere(*View.Family))
	{
		if (View.Family->Scene)
		{
			FScene* Scene = (FScene*)View.Family->Scene;
			if (Scene->AtmosphericFog)
			{
				const FAtmosphericFogSceneInfo& FogInfo = *Scene->AtmosphericFog;

				View.AtmosphereTransmittanceTexture = (FogInfo.TransmittanceResource) ? FogInfo.TransmittanceResource: GBlackTextureSRV;
				View.AtmosphereIrradianceTexture = (FogInfo.IrradianceResource) ? FogInfo.IrradianceResource : GBlackTextureSRV;
				View.AtmosphereInscatterTexture = (FogInfo.InscatterResource) ? FogInfo.InscatterResource : GBlackVolumeTextureSRV;
				bInitTextures = true;
			}
		}
	}

	if (!bInitTextures)
	{
		View.AtmosphereTransmittanceTexture = GBlackTextureSRV;
		View.AtmosphereIrradianceTexture = GBlackTextureSRV;
		View.AtmosphereInscatterTexture = GBlackVolumeTextureSRV;
	}
}

void SetAtmosphericFogShaders(FScene* Scene, const FViewInfo& View, const ComPtr<PooledRenderTarget>& LightShaftOcclusion)
{
	uint32 RenderFlag = Scene->AtmosphericFog->RenderFlag;

	auto ShaderMap = View.ShaderMap;

	TShaderMapRef<FAtmosphericVS> VertexShader(ShaderMap);
	FAtmosphericFogPS* PixelShader = NULL;
	switch (RenderFlag)
	{
	default:
		assert(false);
	case EAtmosphereRenderFlag::E_EnableAll:
		PixelShader = *TShaderMapRef<TAtmosphericFogPS<EAtmosphereRenderFlag::E_EnableAll> >(ShaderMap);
		break;
	case EAtmosphereRenderFlag::E_DisableSunDisk:
		PixelShader = *TShaderMapRef<TAtmosphericFogPS<EAtmosphereRenderFlag::E_DisableSunDisk> >(ShaderMap);
		break;
	case EAtmosphereRenderFlag::E_DisableGroundScattering:
		PixelShader = *TShaderMapRef<TAtmosphericFogPS<EAtmosphereRenderFlag::E_DisableGroundScattering> >(ShaderMap);
		break;
	case EAtmosphereRenderFlag::E_DisableSunAndGround:
		PixelShader = *TShaderMapRef<TAtmosphericFogPS<EAtmosphereRenderFlag::E_DisableSunAndGround> >(ShaderMap);
		break;
	case EAtmosphereRenderFlag::E_DisableLightShaft:
		PixelShader = *TShaderMapRef<TAtmosphericFogPS<EAtmosphereRenderFlag::E_DisableLightShaft> >(ShaderMap);
		break;
	case EAtmosphereRenderFlag::E_DisableSunAndLightShaft:
		PixelShader = *TShaderMapRef<TAtmosphericFogPS<EAtmosphereRenderFlag::E_DisableSunAndLightShaft> >(ShaderMap);
		break;
	case EAtmosphereRenderFlag::E_DisableGroundAndLightShaft:
		PixelShader = *TShaderMapRef<TAtmosphericFogPS<EAtmosphereRenderFlag::E_DisableGroundAndLightShaft> >(ShaderMap);
		break;
	case EAtmosphereRenderFlag::E_DisableAll:
		PixelShader = *TShaderMapRef<TAtmosphericFogPS<EAtmosphereRenderFlag::E_DisableAll> >(ShaderMap);
		break;
	}

	ID3D11InputLayout* InputLayout = GetInputLayout(GetAtmopshereVertexDeclaration().get(), VertexShader->GetCode().Get());
	ID3D11VertexShader* VertexShaderRHI = VertexShader->GetVertexShader();
	ID3D11PixelShader* PixelShaderRHI = PixelShader->GetPixelShader();
	D3D11DeviceContext->IASetInputLayout(InputLayout);
	D3D11DeviceContext->VSSetShader(VertexShaderRHI, 0, 0);
	D3D11DeviceContext->PSSetShader(PixelShaderRHI, 0, 0);
	//GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GAtmophereVertexDeclaration.VertexDeclarationRHI;
	//GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	//GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(PixelShader);
	//GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	//SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	VertexShader->SetParameters(View);
	PixelShader->SetParameters(View, LightShaftOcclusion);
}


void FSceneRenderer::RenderAtmosphere(const FLightShaftsOutput& LightShaftsOutput)
{
	if (/*Scene->GetFeatureLevel() >= ERHIFeatureLevel::SM4 &&*/ Scene->HasAtmosphericFog())
	{
		static const Vector2 Vertices[4] =
		{
			Vector2(-1,-1),
			Vector2(-1,+1),
			Vector2(+1,+1),
			Vector2(+1,-1),
		};
		static const uint16 Indices[6] =
		{
			0, 1, 2,
			0, 2, 3
		};

		FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();

		SceneContext.BeginRenderingSceneColor(false,false,true);

		//FGraphicsPipelineStateInitializer GraphicsPSOInit;
		//RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

		ID3D11RasterizerState* RasterizerState = TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_NONE>::GetRHI();
		// disable alpha writes in order to preserve scene depth values on PC
		ID3D11BlendState* BlendState = TStaticBlendState<false,true, D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_SRC_ALPHA>::GetRHI();
		ID3D11DepthStencilState* DepthStencilState = TStaticDepthStencilState<false, D3D11_COMPARISON_ALWAYS>::GetRHI();
		D3D11DeviceContext->RSSetState(RasterizerState);
		D3D11DeviceContext->OMSetBlendState(BlendState, NULL, 0xffffffff);
		D3D11DeviceContext->OMSetDepthStencilState(DepthStencilState,0);

		for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
		{
			const FViewInfo& View = Views[ViewIndex];

			SCOPED_DRAW_EVENT_FORMAT(Atmosphere, TEXT("Atmosphere %dx%d"), View.ViewRect.Width(), View.ViewRect.Height());

			// Set the device viewport for the view.
			RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

			SetAtmosphericFogShaders(Scene, View, LightShaftsOutput.LightShaftOcclusion);

			// Draw a quad covering the view.
			DrawIndexedPrimitiveUP(
				D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
				0,
				sizeof(Vertices)/sizeof(Vertices[0]),
				2,
				Indices,
				sizeof(Indices[0]),
				Vertices,
				sizeof(Vertices[0])
			);
		}
	}
}

FAtmosphericFogSceneInfo::FAtmosphericFogSceneInfo(class UAtmosphericFogComponent* InComponent, const FScene* InScene)
	: Component(InComponent)
	, SunMultiplier(InComponent->SunMultiplier)
	, FogMultiplier(InComponent->FogMultiplier)
	, InvDensityMultiplier(InComponent->DensityMultiplier > 0.f ? 1.f / InComponent->DensityMultiplier : 1.f)
	, DensityOffset(InComponent->DensityOffset)
	, GroundOffset(InComponent->GroundOffset)
	, DistanceScale(InComponent->DistanceScale)
	, AltitudeScale(InComponent->AltitudeScale)
	, RHeight(InComponent->PrecomputeParams.DensityHeight * InComponent->PrecomputeParams.DensityHeight * InComponent->PrecomputeParams.DensityHeight * 64.f)
	, StartDistance(InComponent->StartDistance)
	, DistanceOffset(InComponent->DistanceOffset)
	, SunDiscScale(InComponent->SunDiscScale)
	, RenderFlag(EAtmosphereRenderFlag::E_EnableAll)
	, InscatterAltitudeSampleNum(InComponent->PrecomputeParams.InscatterAltitudeSampleNum)

{
	StartDistance *= DistanceScale * 0.00001f; // Convert to km in Atmospheric fog shader
											   // DistanceOffset is in km, no need to change...
	DefaultSunColor = FLinearColor(InComponent->DefaultLightColor) * InComponent->DefaultBrightness;
	RenderFlag |= (InComponent->bDisableSunDisk) ? EAtmosphereRenderFlag::E_DisableSunDisk : EAtmosphereRenderFlag::E_EnableAll;
	RenderFlag |= (InComponent->bDisableGroundScattering) ? EAtmosphereRenderFlag::E_DisableGroundScattering : EAtmosphereRenderFlag::E_EnableAll;
	// Should be same as UpdateAtmosphericFogTransform
	GroundOffset += InComponent->GetComponentLocation().Z;
	FMatrix WorldToLight = InComponent->GetComponentTransform().ToMatrixNoScale().InverseFast();
	DefaultSunDirection = FVector(WorldToLight.M[0][0], WorldToLight.M[1][0], WorldToLight.M[2][0]);

	TransmittanceResource = Component->TransmittanceResource;
	IrradianceResource = Component->IrradianceResource;
	InscatterResource = Component->InscatterResource;
}

FAtmosphericFogSceneInfo::~FAtmosphericFogSceneInfo()
{

}
