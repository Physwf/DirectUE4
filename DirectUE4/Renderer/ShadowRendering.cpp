#include "ShadowRendering.h"
#include "Scene.h"
#include "DeferredShading.h"
#include "StaticMesh.h"
#include "RenderTargets.h"


void FShadowVolumeBoundProjectionVS::SetParameters(const FSceneView& View, const FProjectedShadowInfo* ShadowInfo)
{
	FGlobalShader::SetParameters<FViewUniformShaderParameters>(GetVertexShader(), View.ViewUniformBuffer.get());

	if (ShadowInfo->IsWholeSceneDirectionalShadow())
	{
		// Calculate bounding geometry transform for whole scene directional shadow.
		// Use a pair of pre-transformed planes for stenciling.
		StencilingGeometryParameters.Set(this, Vector4(0, 0, 0, 1));
	}
	else if (ShadowInfo->IsWholeScenePointLightShadow())
	{
		// Handle stenciling sphere for point light.
		StencilingGeometryParameters.Set(this, View, ShadowInfo->LightSceneInfo);
	}
	else
	{
		// Other bounding geometry types are pre-transformed.
		StencilingGeometryParameters.Set(this, Vector4(0, 0, 0, 1));
	}
}

//IMPLEMENT_SHADER_TYPE(, FShadowProjectionNoTransformVS, TEXT("ShadowProjectionVertexShader.usf"), TEXT("Main"), SF_Vertex);

IMPLEMENT_SHADER_TYPE(, FShadowVolumeBoundProjectionVS, ("ShadowProjectionVertexShader.dusf"), ("Main"), SF_Vertex);

// Implement a pixel shader for rendering one pass point light shadows with different quality levels
#define IMPLEMENT_ONEPASS_POINT_SHADOW_PROJECTION_PIXEL_SHADER(Quality,UseTransmission) \
	typedef TOnePassPointShadowProjectionPS<Quality,  UseTransmission> FOnePassPointShadowProjectionPS##Quality##UseTransmission; \
	IMPLEMENT_SHADER_TYPE(template<>,FOnePassPointShadowProjectionPS##Quality##UseTransmission,("ShadowProjectionPixelShader.dusf"),("MainOnePassPointLightPS"),SF_Pixel);

IMPLEMENT_ONEPASS_POINT_SHADOW_PROJECTION_PIXEL_SHADER(1, false);
IMPLEMENT_ONEPASS_POINT_SHADOW_PROJECTION_PIXEL_SHADER(2, false);
IMPLEMENT_ONEPASS_POINT_SHADOW_PROJECTION_PIXEL_SHADER(3, false);
IMPLEMENT_ONEPASS_POINT_SHADOW_PROJECTION_PIXEL_SHADER(4, false);
IMPLEMENT_ONEPASS_POINT_SHADOW_PROJECTION_PIXEL_SHADER(5, false);

IMPLEMENT_ONEPASS_POINT_SHADOW_PROJECTION_PIXEL_SHADER(1, true);
IMPLEMENT_ONEPASS_POINT_SHADOW_PROJECTION_PIXEL_SHADER(2, true);
IMPLEMENT_ONEPASS_POINT_SHADOW_PROJECTION_PIXEL_SHADER(3, true);
IMPLEMENT_ONEPASS_POINT_SHADOW_PROJECTION_PIXEL_SHADER(4, true);
IMPLEMENT_ONEPASS_POINT_SHADOW_PROJECTION_PIXEL_SHADER(5, true);


void FProjectedShadowInfo::SetBlendStateForProjection(
	int32 ShadowMapChannel, 
	bool bIsWholeSceneDirectionalShadow, 
	bool bUseFadePlane, 
	bool bProjectingForForwardShading, 
	bool bMobileModulatedProjections)
{
	ID3D11BlendState* BlendState;
	{
		if (bIsWholeSceneDirectionalShadow)
		{
			// Note: blend logic has to match ordering in FCompareFProjectedShadowInfoBySplitIndex.  For example the fade plane blend mode requires that shadow to be rendered first.
			// use R and G in Light Attenuation
			if (bUseFadePlane)
			{
				// alpha is used to fade between cascades, we don't don't need to do BO_Min as we leave B and A untouched which has translucency shadow
				BlendState = TStaticBlendState<false,false, D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN, D3D11_BLEND_OP_ADD, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA>::GetRHI();
			}
			else
			{
				// first cascade rendered doesn't require fading (CO_Min is needed to combine multiple shadow passes)
				// RTDF shadows: CO_Min is needed to combine with far shadows which overlap the same depth range
				BlendState = TStaticBlendState<false, false, D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN, D3D11_BLEND_OP_MIN, D3D11_BLEND_ONE, D3D11_BLEND_ONE>::GetRHI();
			}
		}
		else
		{
// 			if (bMobileModulatedProjections)
// 			{
// 				bool bEncodedHDR = GetMobileHDRMode() == EMobileHDRMode::EnabledRGBE;
// 				if (bEncodedHDR)
// 				{
// 					GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
// 				}
// 				else
// 				{
// 					// Color modulate shadows, ignore alpha.
// 					GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGB, BO_Add, BF_Zero, BF_SourceColor, BO_Add, BF_Zero, BF_One>::GetRHI();
// 				}
// 			}
// 			else
			{
				// use B and A in Light Attenuation
				// CO_Min is needed to combine multiple shadow passes
				BlendState = TStaticBlendState<false, false, D3D11_COLOR_WRITE_ENABLE_BLUE | D3D11_COLOR_WRITE_ENABLE_ALPHA, D3D11_BLEND_OP_MIN, D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_MIN, D3D11_BLEND_ONE, D3D11_BLEND_ONE>::GetRHI();
			}
		}
	}
	D3D11DeviceContext->OMSetBlendState(BlendState, NULL, 0xffffffff);
}

void FProjectedShadowInfo::SetBlendStateForProjection(bool bProjectingForForwardShading, bool bMobileModulatedProjections) const
{
	SetBlendStateForProjection(
		GetLightSceneInfo().GetDynamicShadowMapChannel(),
		IsWholeSceneDirectionalShadow(),
		CascadeSettings.FadePlaneLength > 0 && !bRayTracedDistanceField,
		bProjectingForForwardShading,
		bMobileModulatedProjections);
}

void FProjectedShadowInfo::RenderProjection(int32 ViewIndex, const class FViewInfo* View, const class FSceneRenderer* SceneRender, bool bProjectingForForwardShading, bool bMobile) const
{

}

template <uint32 Quality>
static void SetPointLightShaderTempl(int32 ViewIndex, const FViewInfo& View, const FProjectedShadowInfo* ShadowInfo)
{
	TShaderMapRef<FShadowVolumeBoundProjectionVS> VertexShader(View.ShaderMap);
	TShaderMapRef<TOnePassPointShadowProjectionPS<Quality> > PixelShader(View.ShaderMap);

	//GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
	//GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	//GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);

	ID3D11InputLayout* InputLayout = GetInputLayout(GetVertexDeclarationFVector4().get(), VertexShader->GetCode().Get());
	D3D11DeviceContext->IASetInputLayout(InputLayout);
	ID3D11VertexShader* VertexShaderRHI = VertexShader->GetVertexShader();
	ID3D11PixelShader* PixelShaderRHI = PixelShader->GetPixelShader();
	D3D11DeviceContext->VSSetShader(VertexShaderRHI, 0, 0);
	D3D11DeviceContext->PSSetShader(PixelShaderRHI, 0, 0);

	//SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	VertexShader->SetParameters(View, ShadowInfo);
	PixelShader->SetParameters(ViewIndex, View, ShadowInfo);
}

void FProjectedShadowInfo::RenderOnePassPointLightProjection(int32 ViewIndex, const FViewInfo& View, bool bProjectingForForwardShading) const
{
	const FSphere LightBounds = LightSceneInfo->Proxy->GetBoundingSphere();

	//FGraphicsPipelineStateInitializer GraphicsPSOInit;
	//RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	SetBlendStateForProjection(bProjectingForForwardShading, false);
	//GraphicsPSOInit.PrimitiveType = PT_TriangleList;
	D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const bool bCameraInsideLightGeometry = ((FVector)View.ViewMatrices.GetViewOrigin() - LightBounds.Center).SizeSquared() < FMath::Square(LightBounds.W * 1.05f + View.NearClippingDistance * 2.0f);

	ID3D11DepthStencilState* DepthStencilState;
	ID3D11RasterizerState* RasterizerState;
	if (bCameraInsideLightGeometry)
	{
		DepthStencilState = TStaticDepthStencilState<false, D3D11_COMPARISON_ALWAYS>::GetRHI();
		// Render backfaces with depth tests disabled since the camera is inside (or close to inside) the light geometry
		RasterizerState = View.bReverseCulling ? TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_FRONT>::GetRHI() : TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_BACK>::GetRHI();
	}
	else
	{
		// Render frontfaces with depth tests on to get the speedup from HiZ since the camera is outside the light geometry
		DepthStencilState = TStaticDepthStencilState<false, D3D11_COMPARISON_GREATER_EQUAL>::GetRHI();
		RasterizerState = View.bReverseCulling ? TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_BACK>::GetRHI() : TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_FRONT>::GetRHI();
	}
	D3D11DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);
	D3D11DeviceContext->RSSetState(RasterizerState);

	{
		uint32 LocalQuality = 5;// GetShadowQuality();

		if (LocalQuality > 1)
		{
			// adjust kernel size so that the penumbra size of distant splits will better match up with the closer ones
			//const float SizeScale = ShadowInfo->ResolutionX;
			int32 Reduce = 0;

			{
				int32 Res = ResolutionX;

				while (Res < 512)
				{
					Res *= 2;
					++Reduce;
				}
			}
		}

		switch (LocalQuality)
		{
		case 1: SetPointLightShaderTempl<1>(ViewIndex, View, this); break;
		case 2: SetPointLightShaderTempl<2>(ViewIndex, View, this); break;
		case 3: SetPointLightShaderTempl<3>(ViewIndex, View, this); break;
		case 4: SetPointLightShaderTempl<4>(ViewIndex, View, this); break;
		case 5: SetPointLightShaderTempl<5>(ViewIndex, View, this); break;
		default:
			assert(0);
		}
	}
}


bool FSceneRenderer::RenderShadowProjections(const FLightSceneInfo* LightSceneInfo, PooledRenderTarget* ScreenShadowMaskTexture, bool bProjectingForForwardShading, bool bMobileModulatedProjections)
{
	FVisibleLightInfo& VisibleLightInfo = VisibleLightInfos[LightSceneInfo->Id];
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();

	SetRenderTarget( ScreenShadowMaskTexture->TargetableTexture.get(), SceneContext.GetSceneDepthSurface().get(), false, false, true);

	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
	{
		const FViewInfo& View = Views[ViewIndex];

		RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

		// Set the light's scissor rectangle.
		LightSceneInfo->Proxy->SetScissorRect(View, View.ViewRect);

		// Project the shadow depth buffers onto the scene.
		for (uint32 ShadowIndex = 0; ShadowIndex < VisibleLightInfo.ShadowsToProject.size(); ShadowIndex++)
		{
			FProjectedShadowInfo* ProjectedShadowInfo = VisibleLightInfo.ShadowsToProject[ShadowIndex];

// 			if (ProjectedShadowInfo->bRayTracedDistanceField)
// 			{
// 				ProjectedShadowInfo->RenderRayTracedDistanceFieldProjection(RHICmdList, View, ScreenShadowMaskTexture, bProjectingForForwardShading);
// 			}
// 			else if (ProjectedShadowInfo->bAllocated)
			{
				// Only project the shadow if it's large enough in this particular view (split screen, etc... may have shadows that are large in one view but irrelevantly small in others)
				if (ProjectedShadowInfo->FadeAlphas[ViewIndex] > 1.0f / 256.0f)
				{
					if (ProjectedShadowInfo->bOnePassPointLightShadow)
					{
						ProjectedShadowInfo->RenderOnePassPointLightProjection(ViewIndex, View, bProjectingForForwardShading);
					}
					else
					{
						ProjectedShadowInfo->RenderProjection(ViewIndex, &View, this, bProjectingForForwardShading, bMobileModulatedProjections);
					}
				}
			}
		}

		// Reset the scissor rectangle.
		RHISetScissorRect(false, 0, 0, 0, 0);
	}
	return true;
}

bool FSceneRenderer::RenderShadowProjections(const FLightSceneInfo* LightSceneInfo, PooledRenderTarget* ScreenShadowMaskTexture, bool& bInjectedTranslucentVolume)
{
	RenderShadowProjections(LightSceneInfo, ScreenShadowMaskTexture, false, false);

	return true;
}
