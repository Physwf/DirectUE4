#include "LightRendering.h"
#include "Scene.h"
#include "RenderTargets.h"
#include "DeferredShading.h"
#include "GPUProfiler.h"
#include "SystemTextures.h"

FDeferredLightUniformStruct DeferredLightUniforms;

void StencilingGeometry::DrawSphere()
{
	CommitNonComputeShaderConstants();
	UINT Stride = sizeof(Vector4);
	UINT Offset = 0;
	D3D11DeviceContext->IASetVertexBuffers(0, 1, &StencilingGeometry::GStencilSphereVertexBuffer.VertexBufferRHI, &Stride, &Offset);
	D3D11DeviceContext->IASetIndexBuffer(StencilingGeometry::GStencilSphereIndexBuffer.IndexBufferRHI, DXGI_FORMAT_R32_UINT, 0);
	D3D11DeviceContext->DrawIndexed(StencilingGeometry::GStencilSphereIndexBuffer.GetIndexCount(), 0, 0);
}


void StencilingGeometry::DrawVectorSphere()
{

}

void StencilingGeometry::DrawCone()
{

}

StencilingGeometry::TStencilSphereVertexBuffer<18, 12, Vector4> 		StencilingGeometry::GStencilSphereVertexBuffer;
StencilingGeometry::TStencilSphereVertexBuffer<18, 12, FVector> 		StencilingGeometry::GStencilSphereVectorBuffer;
StencilingGeometry::TStencilSphereIndexBuffer<18, 12>					StencilingGeometry::GStencilSphereIndexBuffer;
StencilingGeometry::TStencilSphereVertexBuffer<4, 4, Vector4> 			StencilingGeometry::GLowPolyStencilSphereVertexBuffer;
StencilingGeometry::TStencilSphereIndexBuffer<4, 4> 					StencilingGeometry::GLowPolyStencilSphereIndexBuffer;
StencilingGeometry::FStencilConeVertexBuffer							StencilingGeometry::GStencilConeVertexBuffer;
StencilingGeometry::FStencilConeIndexBuffer								StencilingGeometry::GStencilConeIndexBuffer;

void InitStencilingGeometryResources()
{
	StencilingGeometry::GStencilSphereVertexBuffer.InitRHI();
	StencilingGeometry::GStencilSphereVectorBuffer.InitRHI();
	StencilingGeometry::GStencilSphereIndexBuffer.InitRHI();
	StencilingGeometry::GLowPolyStencilSphereVertexBuffer.InitRHI();
	StencilingGeometry::GLowPolyStencilSphereIndexBuffer.InitRHI();
	StencilingGeometry::GStencilConeVertexBuffer.InitRHI();
	StencilingGeometry::GStencilConeIndexBuffer.InitRHI();
}

// Implement a version for directional lights, and a version for point / spot lights
IMPLEMENT_SHADER_TYPE(template<>, TDeferredLightVS<false>, ("DeferredLightVertexShaders.dusf"), ("DirectionalVertexMain"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(template<>, TDeferredLightVS<true>, ("DeferredLightVertexShaders.dusf"), ("RadialVertexMain"), SF_Vertex);


enum class ELightSourceShape
{
	Directional,
	Capsule,
	Rect,

	MAX
};

/** A pixel shader for rendering the light in a deferred pass. */
class FDeferredLightPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FDeferredLightPS)

	class FSourceShapeDim : SHADER_PERMUTATION_ENUM_CLASS("LIGHT_SOURCE_SHAPE", ELightSourceShape);
	class FIESProfileDim : SHADER_PERMUTATION_BOOL("USE_IES_PROFILE");
	class FInverseSquaredDim : SHADER_PERMUTATION_BOOL("INVERSE_SQUARED_FALLOFF");
	class FVisualizeCullingDim : SHADER_PERMUTATION_BOOL("VISUALIZE_LIGHT_CULLING");
	class FLightingChannelsDim : SHADER_PERMUTATION_BOOL("USE_LIGHTING_CHANNELS");
	class FTransmissionDim : SHADER_PERMUTATION_BOOL("USE_TRANSMISSION");

	using FPermutationDomain = TShaderPermutationDomain<
		FSourceShapeDim,
		FIESProfileDim,
		FInverseSquaredDim,
		FVisualizeCullingDim,
		FLightingChannelsDim,
		FTransmissionDim >;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		FPermutationDomain PermutationVector(Parameters.PermutationId);

		if (PermutationVector.Get< FSourceShapeDim >() == ELightSourceShape::Directional && (
			PermutationVector.Get< FIESProfileDim >() ||
			PermutationVector.Get< FInverseSquaredDim >()))
		{
			return false;
		}

		if (PermutationVector.Get< FSourceShapeDim >() == ELightSourceShape::Rect &&
			!PermutationVector.Get< FInverseSquaredDim >())
		{
			return false;
		}

		/*if( PermutationVector.Get< FVisualizeCullingDim >() && (
		PermutationVector.Get< FSourceShapeDim >() == ELightSourceShape::Rect ||
		PermutationVector.Get< FIESProfileDim >() ||
		PermutationVector.Get< FInverseSquaredDim >() ) )
		{
		return false;
		}*/

		//return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
		return true;
	}

	FDeferredLightPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		SceneTextureParameters.Bind(Initializer);
		LightAttenuationTexture.Bind(Initializer.ParameterMap, ("LightAttenuationTexture"));
		LightAttenuationTextureSampler.Bind(Initializer.ParameterMap, ("LightAttenuationTextureSampler"));
		LTCMatTexture.Bind(Initializer.ParameterMap, ("LTCMatTexture"));
		LTCMatSampler.Bind(Initializer.ParameterMap, ("LTCMatSampler"));
		LTCAmpTexture.Bind(Initializer.ParameterMap, ("LTCAmpTexture"));
		LTCAmpSampler.Bind(Initializer.ParameterMap, ("LTCAmpSampler"));
		IESTexture.Bind(Initializer.ParameterMap, ("IESTexture"));
		IESTextureSampler.Bind(Initializer.ParameterMap, ("IESTextureSampler"));
		LightingChannelsTexture.Bind(Initializer.ParameterMap, ("LightingChannelsTexture"));
		LightingChannelsSampler.Bind(Initializer.ParameterMap, ("LightingChannelsSampler"));
		TransmissionProfilesTexture.Bind(Initializer.ParameterMap, ("SSProfilesTexture"));
		TransmissionProfilesLinearSampler.Bind(Initializer.ParameterMap, ("TransmissionProfilesLinearSampler"));
	}

	FDeferredLightPS()
	{}

public:
	void SetParameters(const FSceneView& View, const FLightSceneInfo* LightSceneInfo, PooledRenderTarget* ScreenShadowMaskTexture)
	{
		ID3D11PixelShader* const ShaderRHI = GetPixelShader();
		SetParametersBase(ShaderRHI, View, ScreenShadowMaskTexture, /*LightSceneInfo->Proxy->GetIESTextureResource()*/nullptr);
		SetDeferredLightParameters(ShaderRHI, GetUniformBufferParameter<FDeferredLightUniformStruct>(), LightSceneInfo, View);
	}

// 	void SetParametersSimpleLight(const FSceneView& View, const FSimpleLightEntry& SimpleLight, const FSimpleLightPerViewEntry& SimpleLightPerViewData)
// 	{
// 		ID3D11PixelShader* const ShaderRHI = GetPixelShader();
// 		SetParametersBase(ShaderRHI, View, NULL, NULL);
// 		SetSimpleDeferredLightParameters(ShaderRHI, GetUniformBufferParameter<FDeferredLightUniformStruct>(), SimpleLight, SimpleLightPerViewData, View);
// 	}

private:

	void SetParametersBase(ID3D11PixelShader* const ShaderRHI, const FSceneView& View, PooledRenderTarget* ScreenShadowMaskTexture, ID3D11ShaderResourceView* IESTextureResource)
	{
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(ShaderRHI, View.ViewUniformBuffer.get());
		SceneTextureParameters.Set(ShaderRHI, ESceneTextureSetupMode::All);

		FSceneRenderTargets& SceneRenderTargets = FSceneRenderTargets::Get();

		if (LightAttenuationTexture.IsBound())
		{
			SetTextureParameter(
				ShaderRHI,
				LightAttenuationTexture,
				LightAttenuationTextureSampler,
				TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::GetRHI(),
				ScreenShadowMaskTexture ? ScreenShadowMaskTexture->ShaderResourceTexture->GetShaderResourceView() : GWhiteTextureSRV
			);
		}

		SetTextureParameter(
			ShaderRHI,
			LTCMatTexture,
			LTCMatSampler,
			TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI(),
			GSystemTextures.LTCMat->ShaderResourceTexture->GetShaderResourceView()
		);

		SetTextureParameter(
			ShaderRHI,
			LTCAmpTexture,
			LTCAmpSampler,
			TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI(),
			GSystemTextures.LTCAmp->ShaderResourceTexture->GetShaderResourceView()
		);

		{
			ID3D11ShaderResourceView* TextureRHI = IESTextureResource ? IESTextureResource : GSystemTextures.WhiteDummy->TargetableTexture->GetShaderResourceView();

			SetTextureParameter(
				ShaderRHI,
				IESTexture,
				IESTextureSampler,
				TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI(),
				TextureRHI
			);
		}

		if (LightingChannelsTexture.IsBound())
		{
			std::shared_ptr<FD3D11Texture> LightingChannelsTextureRHI = SceneRenderTargets.LightingChannels ? SceneRenderTargets.LightingChannels->ShaderResourceTexture : GSystemTextures.WhiteDummy->TargetableTexture;

			SetTextureParameter(
				ShaderRHI,
				LightingChannelsTexture,
				LightingChannelsSampler,
				TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI(),
				LightingChannelsTextureRHI->GetShaderResourceView()
			);
		}

		if (TransmissionProfilesTexture.IsBound())
		{
			FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();
			const PooledRenderTarget* PooledRT = nullptr;// GetSubsufaceProfileTexture_RT((FRHICommandListImmediate&)RHICmdList);

			if (!PooledRT)
			{
				// no subsurface profile was used yet
				PooledRT = GSystemTextures.BlackDummy.Get();
			}

			//const FSceneRenderTargetItem& Item = PooledRT->GetRenderTargetItem();

			SetTextureParameter(
				ShaderRHI,
				TransmissionProfilesTexture,
				TransmissionProfilesLinearSampler,
				TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI(),
				PooledRT->ShaderResourceTexture->GetShaderResourceView());
		}
	}

	FSceneTextureShaderParameters SceneTextureParameters;
	FShaderResourceParameter LightAttenuationTexture;
	FShaderResourceParameter LightAttenuationTextureSampler;
	FShaderResourceParameter LTCMatTexture;
	FShaderResourceParameter LTCMatSampler;
	FShaderResourceParameter LTCAmpTexture;
	FShaderResourceParameter LTCAmpSampler;
	FShaderResourceParameter IESTexture;
	FShaderResourceParameter IESTextureSampler;
	FShaderResourceParameter LightingChannelsTexture;
	FShaderResourceParameter LightingChannelsSampler;
	FShaderResourceParameter TransmissionProfilesTexture;
	FShaderResourceParameter TransmissionProfilesLinearSampler;
};

IMPLEMENT_GLOBAL_SHADER(FDeferredLightPS, "DeferredLightPixelShaders.dusf", "DeferredLightPixelMain", SF_Pixel);
/** Shader used to visualize stationary light overlap. */
template<bool bRadialAttenuation>
class TDeferredLightOverlapPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TDeferredLightOverlapPS, Global)
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		//return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(("RADIAL_ATTENUATION"), (uint32)bRadialAttenuation);
	}

	TDeferredLightOverlapPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		HasValidChannel.Bind(Initializer.ParameterMap, ("HasValidChannel"));
		SceneTextureParameters.Bind(Initializer);
	}

	TDeferredLightOverlapPS()
	{
	}

	void SetParameters(const FSceneView& View, const FLightSceneInfo* LightSceneInfo)
	{
		ID3D11PixelShader* const ShaderRHI = GetPixelShader();
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(ShaderRHI, View.ViewUniformBuffer.get());
		const float HasValidChannelValue = LightSceneInfo->Proxy->GetPreviewShadowMapChannel() == INDEX_NONE ? 0.0f : 1.0f;
		SetShaderValue(ShaderRHI, HasValidChannel, HasValidChannelValue);
		SceneTextureParameters.Set(ShaderRHI, ESceneTextureSetupMode::All);
		SetDeferredLightParameters(ShaderRHI, GetUniformBufferParameter<FDeferredLightUniformStruct>(), LightSceneInfo, View);
	}

private:
	FShaderParameter HasValidChannel;
	FSceneTextureShaderParameters SceneTextureParameters;
};

IMPLEMENT_SHADER_TYPE(template<>, TDeferredLightOverlapPS<true>, ("StationaryLightOverlapShaders.dusf"), ("OverlapRadialPixelMain"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>, TDeferredLightOverlapPS<false>, ("StationaryLightOverlapShaders.dusf"), ("OverlapDirectionalPixelMain"), SF_Pixel);

/** Sets up rasterizer and depth state for rendering bounding geometry in a deferred pass. */
void SetBoundingGeometryRasterizerAndDepthState(const FViewInfo& View, const FSphere& LightBounds)
{
	const bool bCameraInsideLightGeometry = ((FVector)View.ViewMatrices.GetViewOrigin() - LightBounds.Center).SizeSquared() < FMath::Square(LightBounds.W * 1.05f + View.NearClippingDistance * 2.0f)
		// Always draw backfaces in ortho
		//@todo - accurate ortho camera / light intersection
		|| !View.IsPerspectiveProjection();

	ID3D11RasterizerState* RasterizerState;
	if (bCameraInsideLightGeometry)
	{
		// Render backfaces with depth tests disabled since the camera is inside (or close to inside) the light geometry
		RasterizerState = View.bReverseCulling ? TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_FRONT>::GetRHI() : TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_BACK>::GetRHI();
	}
	else
	{
		// Render frontfaces with depth tests on to get the speedup from HiZ since the camera is outside the light geometry
		RasterizerState = View.bReverseCulling ? TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_BACK>::GetRHI() : TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_FRONT>::GetRHI();
	}

	ID3D11DepthStencilState* DepthStencilState =
		bCameraInsideLightGeometry
		? TStaticDepthStencilState<false, D3D11_COMPARISON_ALWAYS>::GetRHI()
		: TStaticDepthStencilState<false, D3D11_COMPARISON_GREATER_EQUAL>::GetRHI();

	D3D11DeviceContext->RSSetState(RasterizerState);
	D3D11DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);
}

float GetLightFadeFactor(const FSceneView& View, const FLightSceneProxy* Proxy)
{
	// Distance fade
	FSphere Bounds = Proxy->GetBoundingSphere();

	const float DistanceSquared = (Bounds.Center - View.ViewMatrices.GetViewOrigin()).SizeSquared();
	extern float GMinScreenRadiusForLights;
	float SizeFade = FMath::Square(FMath::Min(0.0002f, GMinScreenRadiusForLights / Bounds.W) * View.LODDistanceFactor) * DistanceSquared;
	SizeFade = FMath::Clamp(6.0f - 6.0f * SizeFade, 0.0f, 1.0f);

	extern float GLightMaxDrawDistanceScale;
	float MaxDist = Proxy->GetMaxDrawDistance() * GLightMaxDrawDistanceScale;
	float Range = Proxy->GetFadeRange();
	float DistanceFade = MaxDist ? (MaxDist - FMath::Sqrt(DistanceSquared)) / Range : 1.0f;
	DistanceFade = FMath::Clamp(DistanceFade, 0.0f, 1.0f);
	return SizeFade * DistanceFade;
}

void FSceneRenderer::RenderLight(const FLightSceneInfo* LightSceneInfo, struct PooledRenderTarget* ScreenShadowMaskTexture, bool bRenderOverlap, bool bIssueDrawEvent)
{
	SCOPED_DRAW_EVENT(StandardDeferredLighting);

	ID3D11BlendState* BlendState = TStaticBlendState<false,false, D3D11_COLOR_WRITE_ENABLE_ALL, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ONE>::GetRHI();
	D3D11DeviceContext->OMSetBlendState(BlendState, NULL, 0xffffffff);
	D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const FSphere LightBounds = LightSceneInfo->Proxy->GetBoundingSphere();
	const bool bTransmission = LightSceneInfo->Proxy->Transmission();

	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
	{
		FViewInfo& View = Views[ViewIndex];

// 		// Ensure the light is valid for this view
// 		if (!LightSceneInfo->ShouldRenderLight(View))
// 		{
// 			continue;
// 		}

		bool bUseIESTexture = false;

// 		if (View.Family->EngineShowFlags.TexturedLightProfiles)
// 		{
// 			bUseIESTexture = (LightSceneInfo->Proxy->GetIESTextureResource() != 0);
// 		}

		RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

		if (LightSceneInfo->Proxy->GetLightType() == LightType_Directional)
		{

		}
		else
		{
			//GraphicsPSOInit.bDepthBounds = (GSupportsDepthBoundsTest && GAllowDepthBoundsTest != 0);

			TShaderMapRef<TDeferredLightVS<true> > VertexShader(View.ShaderMap);

			SetBoundingGeometryRasterizerAndDepthState(View, LightBounds);

			if (bRenderOverlap)
			{
				TShaderMapRef<TDeferredLightOverlapPS<true> > PixelShader(View.ShaderMap);

				ID3D11InputLayout* InputLayout = GetInputLayout(GetVertexDeclarationFVector4().get(), VertexShader->GetCode().Get());
				D3D11DeviceContext->IASetInputLayout(InputLayout);
				D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
				D3D11DeviceContext->PSSetShader(PixelShader->GetPixelShader(), 0, 0);

				//GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
				//GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
				//GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);

				//SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
				PixelShader->SetParameters(View, LightSceneInfo);
			}
			else
			{
				FDeferredLightPS::FPermutationDomain PermutationVector;
				PermutationVector.Set< FDeferredLightPS::FSourceShapeDim >(LightSceneInfo->Proxy->IsRectLight() ? ELightSourceShape::Rect : ELightSourceShape::Capsule);
				PermutationVector.Set< FDeferredLightPS::FIESProfileDim >(bUseIESTexture);
				PermutationVector.Set< FDeferredLightPS::FInverseSquaredDim >(LightSceneInfo->Proxy->IsInverseSquared());
				PermutationVector.Set< FDeferredLightPS::FVisualizeCullingDim >(/*View.Family->EngineShowFlags.VisualizeLightCulling*/false);
				PermutationVector.Set< FDeferredLightPS::FLightingChannelsDim >(View.bUsesLightingChannels);
				PermutationVector.Set< FDeferredLightPS::FTransmissionDim >(bTransmission);

				TShaderMapRef< FDeferredLightPS > PixelShader(View.ShaderMap, PermutationVector);
				//GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
				//GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
				//GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);

				ID3D11InputLayout* InputLayout = GetInputLayout(GetVertexDeclarationFVector4().get(), VertexShader->GetCode().Get());
				D3D11DeviceContext->IASetInputLayout(InputLayout);
				D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
				D3D11DeviceContext->PSSetShader(PixelShader->GetPixelShader(), 0, 0);

				//SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
				PixelShader->SetParameters(View, LightSceneInfo, ScreenShadowMaskTexture);
			}

			VertexShader->SetParameters(View, LightSceneInfo);
			// Use DBT to allow work culling on shadow lights
// 			if (GSupportsDepthBoundsTest && GAllowDepthBoundsTest != 0)
// 			{
// 				// Can use the depth bounds test to skip work for pixels which won't be touched by the light (i.e outside the depth range)
// 				float NearDepth = 1.f;
// 				float FarDepth = 0.f;
// 				CalculateLightNearFarDepthFromBounds(View, LightBounds, NearDepth, FarDepth);
// 
// 				if (NearDepth <= FarDepth)
// 				{
// 					NearDepth = 1.0f;
// 					FarDepth = 0.0f;
// 				}
// 
// 				// UE4 uses reversed depth, so far < near
// 				RHICmdList.SetDepthBounds(FarDepth, NearDepth);
// 			}

			if (LightSceneInfo->Proxy->GetLightType() == LightType_Point ||
				LightSceneInfo->Proxy->GetLightType() == LightType_Rect)
			{
				// Apply the point or spot light with some approximate bounding geometry, 
				// So we can get speedups from depth testing and not processing pixels outside of the light's influence.
				StencilingGeometry::DrawSphere();
			}
			else if (LightSceneInfo->Proxy->GetLightType() == LightType_Spot)
			{
				StencilingGeometry::DrawCone();
			}
		}
	}
}

void FSceneRenderer::RenderLights()
{
	SCOPED_DRAW_EVENT_FORMAT(RenderLights, TEXT("Lights"));

	std::vector<FSortedLightSceneInfo> SortedLights;
	SortedLights.reserve(Scene->Lights.size());

	// Build a list of visible lights.
	for (auto LightIt = Scene->Lights.begin(); LightIt != Scene->Lights.end(); ++LightIt)
	{
		const FLightSceneInfoCompact& LightSceneInfoCompact = *LightIt;
		const FLightSceneInfo* const LightSceneInfo = LightSceneInfoCompact.LightSceneInfo;

		for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
		{
			//if (LightSceneInfo->ShouldRenderLight(Views[ViewIndex]))
			{
				SortedLights.emplace_back(FSortedLightSceneInfo(LightSceneInfo));
				FSortedLightSceneInfo* SortedLightInfo = &SortedLights.back();

				// Check for shadows and light functions.
				SortedLightInfo->SortKey.Fields.LightType = LightSceneInfoCompact.LightType;
				SortedLightInfo->SortKey.Fields.bTextureProfile = false;//ViewFamily.EngineShowFlags.TexturedLightProfiles && LightSceneInfo->Proxy->GetIESTextureResource();
				SortedLightInfo->SortKey.Fields.bShadowed = true;// bDynamicShadows && CheckForProjectedShadows(LightSceneInfo);
				SortedLightInfo->SortKey.Fields.bLightFunction = false;// ViewFamily.EngineShowFlags.LightFunctions && CheckForLightFunction(LightSceneInfo);
				SortedLightInfo->SortKey.Fields.bUsesLightingChannels = false;// Views[ViewIndex].bUsesLightingChannels && LightSceneInfo->Proxy->GetLightingChannelMask() != GetDefaultLightingChannelMask();
				break;
			}
		}
	}

	// Sort non-shadowed, non-light function lights first to avoid render target switches.
	struct FCompareFSortedLightSceneInfo
	{
		inline bool operator()(const FSortedLightSceneInfo& A, const FSortedLightSceneInfo& B) const
		{
			return A.SortKey.Packed < B.SortKey.Packed;
		}
	};
	std::sort(SortedLights.begin(), SortedLights.end(), FCompareFSortedLightSceneInfo());

	{
		FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();

		uint32 AttenuationLightStart = SortedLights.size();
		uint32 SupportedByTiledDeferredLightEnd = SortedLights.size();
		bool bAnyUnsupportedByTiledDeferred = false;

		// Iterate over all lights to be rendered and build ranges for tiled deferred and unshadowed lights
		for (uint32 LightIndex = 0; LightIndex < SortedLights.size(); LightIndex++)
		{
			const FSortedLightSceneInfo& SortedLightInfo = SortedLights[LightIndex];
			bool bDrawShadows = SortedLightInfo.SortKey.Fields.bShadowed;
			bool bDrawLightFunction = SortedLightInfo.SortKey.Fields.bLightFunction;
			bool bTextureLightProfile = SortedLightInfo.SortKey.Fields.bTextureProfile;
			bool bLightingChannels = SortedLightInfo.SortKey.Fields.bUsesLightingChannels;

			if (bTextureLightProfile && SupportedByTiledDeferredLightEnd == SortedLights.size())
			{
				// Mark the first index to not support tiled deferred due to texture light profile
				SupportedByTiledDeferredLightEnd = LightIndex;
			}

			if (bDrawShadows || bDrawLightFunction || bLightingChannels)
			{
				AttenuationLightStart = LightIndex;

				if (SupportedByTiledDeferredLightEnd == SortedLights.size())
				{
					// Mark the first index to not support tiled deferred due to shadowing
					SupportedByTiledDeferredLightEnd = LightIndex;
				}
				break;
			}

			if (LightIndex < SupportedByTiledDeferredLightEnd)
			{
				// Directional lights currently not supported by tiled deferred
				bAnyUnsupportedByTiledDeferred = bAnyUnsupportedByTiledDeferred
					|| (SortedLightInfo.SortKey.Fields.LightType != LightType_Point && SortedLightInfo.SortKey.Fields.LightType != LightType_Spot);
			}
		}

		int32 StandardDeferredStart = 0;

		{
			SCOPED_DRAW_EVENT(NonShadowedLights);

			SceneContext.BeginRenderingSceneColor(false,false,true);

			for (uint32 LightIndex = StandardDeferredStart; LightIndex < AttenuationLightStart; LightIndex++)
			{
				const FSortedLightSceneInfo& SortedLightInfo = SortedLights[LightIndex];
				const FLightSceneInfo* const LightSceneInfo = SortedLightInfo.LightSceneInfo;

				// Render the light to the scene color buffer, using a 1x1 white texture as input 
				RenderLight(LightSceneInfo, NULL, false, false);
			}
		}


		{
			SCOPED_DRAW_EVENT(ShadowedLights);

			ComPtr<PooledRenderTarget> ScreenShadowMaskTexture;

			for (uint32 LightIndex = AttenuationLightStart; LightIndex < SortedLights.size(); LightIndex++)
			{
				const FSortedLightSceneInfo& SortedLightInfo = SortedLights[LightIndex];
				const FLightSceneInfo& LightSceneInfo = *SortedLightInfo.LightSceneInfo;
				bool bDrawShadows = SortedLightInfo.SortKey.Fields.bShadowed;
				bool bDrawLightFunction = SortedLightInfo.SortKey.Fields.bLightFunction;
				bool bDrawPreviewIndicator = false;// ViewFamily.EngineShowFlags.PreviewShadowsIndicator && !LightSceneInfo.IsPrecomputedLightingValid() && LightSceneInfo.Proxy->HasStaticShadowing();
				bool bInjectedTranslucentVolume = false;
				bool bUsedShadowMaskTexture = false;

				if ((bDrawShadows || bDrawLightFunction || bDrawPreviewIndicator) && !ScreenShadowMaskTexture.Get())
				{
					SceneContext.AllocateScreenShadowMask(ScreenShadowMaskTexture);
				}

				if (bDrawShadows)
				{
					// Clear light attenuation for local lights with a quad covering their extents
					const bool bClearLightScreenExtentsOnly = SortedLightInfo.SortKey.Fields.LightType != LightType_Directional;
					// All shadows render with min blending
					bool bClearToWhite = !bClearLightScreenExtentsOnly;

					SetRenderTarget(ScreenShadowMaskTexture->TargetableTexture.get(), SceneContext.GetSceneDepthSurface().get(), bClearToWhite ,false, true);

					if (bClearLightScreenExtentsOnly)
					{
						SCOPED_DRAW_EVENT(ClearQuad);

						for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
						{
							const FViewInfo& View = Views[ViewIndex];
							FIntRect ScissorRect;

							if (!LightSceneInfo.Proxy->GetScissorRect(ScissorRect, View, View.ViewRect))
							{
								ScissorRect = View.ViewRect;
							}

							if (ScissorRect.Min.X < ScissorRect.Max.X && ScissorRect.Min.Y < ScissorRect.Max.Y)
							{
								RHISetViewport(ScissorRect.Min.X, ScissorRect.Min.Y, 0.0f, ScissorRect.Max.X, ScissorRect.Max.Y, 1.0f);
								DrawClearQuad(true, FLinearColor(1, 1, 1, 1), false, 0, false, 0);
							}
							else
							{
								LightSceneInfo.Proxy->GetScissorRect(ScissorRect, View, View.ViewRect);
							}
						}
					}

					RenderShadowProjections(&LightSceneInfo, ScreenShadowMaskTexture.Get(), bInjectedTranslucentVolume);

					bUsedShadowMaskTexture = true;
				}

				if (bUsedShadowMaskTexture)
				{
					RHICopyToResolveTarget(ScreenShadowMaskTexture->TargetableTexture.get(), ScreenShadowMaskTexture->ShaderResourceTexture.get(), FResolveParams(FResolveRect()));
				}

				SceneContext.BeginRenderingSceneColor(false, false, true);

				// Render the light to the scene color buffer, conditionally using the attenuation buffer or a 1x1 white texture as input 
				//if (bDirectLighting)
				{
					RenderLight(&LightSceneInfo, ScreenShadowMaskTexture.Get(), false, true);
				}
			}
		}
	}
}

