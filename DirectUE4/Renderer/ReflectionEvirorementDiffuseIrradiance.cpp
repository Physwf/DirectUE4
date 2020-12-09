#include "ReflectionEnviorementCapture.h"
#include "GlobalShader.h"
#include "RenderTargets.h"
#include "ScreenRendering.h"
#include "SceneFilterRendering.h"
#include "GPUProfiler.h"

extern int32 GDiffuseIrradianceCubemapSize;

PooledRenderTarget& GetEffectiveDiffuseIrradianceRenderTarget(FSceneRenderTargets& SceneContext, int32 TargetMipIndex)
{
	const int32 ScratchTextureIndex = TargetMipIndex % 2;

	return *SceneContext.DiffuseIrradianceScratchCubemap[ScratchTextureIndex].Get();
}

PooledRenderTarget& GetEffectiveDiffuseIrradianceSourceTexture(FSceneRenderTargets& SceneContext, int32 TargetMipIndex)
{
	const int32 ScratchTextureIndex = 1 - TargetMipIndex % 2;

	return *SceneContext.DiffuseIrradianceScratchCubemap[ScratchTextureIndex].Get();
}

/** Pixel shader used for copying to diffuse irradiance texture. */
class FCopyDiffuseIrradiancePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCopyDiffuseIrradiancePS, Global);
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	FCopyDiffuseIrradiancePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		CubeFace.Bind(Initializer.ParameterMap, ("CubeFace"));
		SourceMipIndex.Bind(Initializer.ParameterMap, ("SourceMipIndex"));
		SourceTexture.Bind(Initializer.ParameterMap, ("SourceTexture"));
		SourceTextureSampler.Bind(Initializer.ParameterMap, ("SourceTextureSampler"));
		CoefficientMask0.Bind(Initializer.ParameterMap, ("CoefficientMask0"));
		CoefficientMask1.Bind(Initializer.ParameterMap, ("CoefficientMask1"));
		CoefficientMask2.Bind(Initializer.ParameterMap, ("CoefficientMask2"));
		NumSamples.Bind(Initializer.ParameterMap, ("NumSamples"));
	}
	FCopyDiffuseIrradiancePS() {}

	void SetParameters(int32 CubeFaceValue, int32 SourceMipIndexValue, int32 CoefficientIndex, int32 FaceResolution, ID3D11ShaderResourceView* SourceTextureValue)
	{
		SetShaderValue(GetPixelShader(), CubeFace, CubeFaceValue);
		SetShaderValue(GetPixelShader(), SourceMipIndex, SourceMipIndexValue);

		SetTextureParameter(
			GetPixelShader(),
			SourceTexture,
			SourceTextureSampler,
			TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI(),
			SourceTextureValue);

		const Vector4 Mask0(CoefficientIndex == 0, CoefficientIndex == 1, CoefficientIndex == 2, CoefficientIndex == 3);
		const Vector4 Mask1(CoefficientIndex == 4, CoefficientIndex == 5, CoefficientIndex == 6, CoefficientIndex == 7);
		const float Mask2 = CoefficientIndex == 8;
		SetShaderValue(GetPixelShader(), CoefficientMask0, Mask0);
		SetShaderValue(GetPixelShader(), CoefficientMask1, Mask1);
		SetShaderValue(GetPixelShader(), CoefficientMask2, Mask2);

		SetShaderValue(GetPixelShader(), NumSamples, FaceResolution * FaceResolution * 6);
	}

private:
	FShaderParameter CubeFace;
	FShaderParameter SourceMipIndex;
	FShaderResourceParameter SourceTexture;
	FShaderResourceParameter SourceTextureSampler;
	FShaderParameter CoefficientMask0;
	FShaderParameter CoefficientMask1;
	FShaderParameter CoefficientMask2;
	FShaderParameter NumSamples;
};

IMPLEMENT_SHADER_TYPE(, FCopyDiffuseIrradiancePS, ("ReflectionEnvironmentShaders.dusf"), ("DiffuseIrradianceCopyPS"), SF_Pixel)

/**  */
class FAccumulateDiffuseIrradiancePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FAccumulateDiffuseIrradiancePS, Global);
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	FAccumulateDiffuseIrradiancePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		CubeFace.Bind(Initializer.ParameterMap, ("CubeFace"));
		SourceMipIndex.Bind(Initializer.ParameterMap, ("SourceMipIndex"));
		SourceTexture.Bind(Initializer.ParameterMap, ("SourceTexture"));
		SourceTextureSampler.Bind(Initializer.ParameterMap, ("SourceTextureSampler"));
		Sample01.Bind(Initializer.ParameterMap, ("Sample01"));
		Sample23.Bind(Initializer.ParameterMap, ("Sample23"));
	}
	FAccumulateDiffuseIrradiancePS() {}

	void SetParameters(int32 CubeFaceValue, int32 NumMips, int32 SourceMipIndexValue, int32 CoefficientIndex, ID3D11ShaderResourceView* SourceTextureValue)
	{
		SetShaderValue(GetPixelShader(), CubeFace, CubeFaceValue);
		SetShaderValue(GetPixelShader(), SourceMipIndex, SourceMipIndexValue);

		SetTextureParameter(
			GetPixelShader(),
			SourceTexture,
			SourceTextureSampler,
			TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI(),
			SourceTextureValue);

		const int32 MipSize = 1 << (NumMips - SourceMipIndexValue - 1);
		const float HalfSourceTexelSize = .5f / MipSize;
		const Vector4 Sample01Value(-HalfSourceTexelSize, -HalfSourceTexelSize, HalfSourceTexelSize, -HalfSourceTexelSize);
		const Vector4 Sample23Value(-HalfSourceTexelSize, HalfSourceTexelSize, HalfSourceTexelSize, HalfSourceTexelSize);
		SetShaderValue(GetPixelShader(), Sample01, Sample01Value);
		SetShaderValue(GetPixelShader(), Sample23, Sample23Value);
	}

private:
	FShaderParameter CubeFace;
	FShaderParameter SourceMipIndex;
	FShaderResourceParameter SourceTexture;
	FShaderResourceParameter SourceTextureSampler;
	FShaderParameter Sample01;
	FShaderParameter Sample23;
};

IMPLEMENT_SHADER_TYPE(, FAccumulateDiffuseIrradiancePS, ("ReflectionEnvironmentShaders.dusf"), ("DiffuseIrradianceAccumulatePS"), SF_Pixel)

/**  */
class FAccumulateCubeFacesPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FAccumulateCubeFacesPS, Global);
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	FAccumulateCubeFacesPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		SourceMipIndex.Bind(Initializer.ParameterMap, ("SourceMipIndex"));
		SourceTexture.Bind(Initializer.ParameterMap, ("SourceTexture"));
		SourceTextureSampler.Bind(Initializer.ParameterMap, ("SourceTextureSampler"));
	}
	FAccumulateCubeFacesPS() {}

	void SetParameters(int32 SourceMipIndexValue, ID3D11ShaderResourceView* SourceTextureValue)
	{
		SetShaderValue(GetPixelShader(), SourceMipIndex, SourceMipIndexValue);

		SetTextureParameter(
			GetPixelShader(),
			SourceTexture,
			SourceTextureSampler,
			TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI(),
			SourceTextureValue);
	}


private:
	FShaderParameter SourceMipIndex;
	FShaderResourceParameter SourceTexture;
	FShaderResourceParameter SourceTextureSampler;
};

IMPLEMENT_SHADER_TYPE(, FAccumulateCubeFacesPS, ("ReflectionEnvironmentShaders.dusf"), ("AccumulateCubeFacesPS"), SF_Pixel)


void ComputeDiffuseIrradiance(FD3D11Texture2D* LightingSource, int32 LightingSourceMipIndex, FSHVectorRGB3* OutIrradianceEnvironmentMap)
{
	auto ShaderMap = GetGlobalShaderMap();
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();

	//FGraphicsPipelineStateInitializer GraphicsPSOInit;
	ID3D11RasterizerState* RasterizerState = TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_NONE>::GetRHI();
	ID3D11DepthStencilState* DepthStencilState = TStaticDepthStencilState<false, D3D11_COMPARISON_ALWAYS>::GetRHI();
	ID3D11BlendState* BlendState = TStaticBlendState<>::GetRHI();

	D3D11DeviceContext->RSSetState(RasterizerState);
	D3D11DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);
	D3D11DeviceContext->OMSetBlendState(BlendState, nullptr, 0xffffffff);

	for (int32 CoefficientIndex = 0; CoefficientIndex < FSHVector3::MaxSHBasis; CoefficientIndex++)
	{
		// Copy the starting mip from the lighting texture, apply texel area weighting and appropriate SH coefficient
		{
			SCOPED_DRAW_EVENT(CopyDiffuseIrradiancePS);
			const int32 MipIndex = 0;
			const int32 MipSize = GDiffuseIrradianceCubemapSize;
			PooledRenderTarget& EffectiveRT = GetEffectiveDiffuseIrradianceRenderTarget(SceneContext, MipIndex);

			for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
			{
// 				FRHIRenderPassInfo RPInfo(EffectiveRT.TargetableTexture, ERenderTargetActions::DontLoad_Store, nullptr, 0, CubeFace);
// 				RHICmdList.BeginRenderPass(RPInfo, TEXT("CopyDiffuseIrradianceRP"));
// 				RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
				SetRenderTarget(EffectiveRT.TargetableTexture.get(), nullptr, false, false, false, 0, CubeFace);

				const FIntRect ViewRect(0, 0, MipSize, MipSize);
				RHISetViewport(0, 0, 0.0f, MipSize, MipSize, 1.0f);
				TShaderMapRef<FCopyDiffuseIrradiancePS> PixelShader(ShaderMap);

				TShaderMapRef<FScreenVS> VertexShader(GetGlobalShaderMap());

// 				GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
// 				GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
// 				GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
// 				GraphicsPSOInit.PrimitiveType = PT_TriangleList;

				ID3D11InputLayout* InputLayout = GetInputLayout(GetFilterInputDelcaration().get(), VertexShader->GetCode().Get());
				D3D11DeviceContext->IASetInputLayout(InputLayout);
				D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
				D3D11DeviceContext->PSSetShader(PixelShader->GetPixelShader(), 0, 0);
				D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				PixelShader->SetParameters(CubeFace, LightingSourceMipIndex, CoefficientIndex, MipSize, LightingSource->GetShaderResourceView());

				DrawRectangle(
					ViewRect.Min.X, ViewRect.Min.Y,
					ViewRect.Width(), ViewRect.Height(),
					ViewRect.Min.X, ViewRect.Min.Y,
					ViewRect.Width(), ViewRect.Height(),
					FIntPoint(ViewRect.Width(), ViewRect.Height()),
					FIntPoint(MipSize, MipSize),
					*VertexShader);
			}
		}

		const int32 NumMips = FMath::CeilLogTwo(GDiffuseIrradianceCubemapSize) + 1;

		{
			SCOPED_DRAW_EVENT(AccumulateDiffuseIrradiancePS);
			// Accumulate all the texel values through downsampling to 1x1 mip
			for (int32 MipIndex = 1; MipIndex < NumMips; MipIndex++)
			{
				const int32 SourceMipIndex = FMath::Max(MipIndex - 1, 0);
				const int32 MipSize = 1 << (NumMips - MipIndex - 1);

				PooledRenderTarget& EffectiveRT = GetEffectiveDiffuseIrradianceRenderTarget(SceneContext, MipIndex);
				PooledRenderTarget& EffectiveSource = GetEffectiveDiffuseIrradianceSourceTexture(SceneContext, MipIndex);
				assert(EffectiveRT.TargetableTexture.get() != EffectiveSource.ShaderResourceTexture.get());


				for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
				{
// 					FRHIRenderPassInfo RPInfo(EffectiveRT.TargetableTexture, ERenderTargetActions::Load_Store, nullptr, MipIndex, CubeFace);
// 					RHICmdList.BeginRenderPass(RPInfo, TEXT("AccumulateDiffuseIrradianceRP"));
// 					RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

					SetRenderTarget(EffectiveRT.TargetableTexture.get(), nullptr, false, false, false, MipIndex, CubeFace);

					const FIntRect ViewRect(0, 0, MipSize, MipSize);
					RHISetViewport(0, 0, 0.0f, MipSize, MipSize, 1.0f);
					TShaderMapRef<FAccumulateDiffuseIrradiancePS> PixelShader(ShaderMap);

					TShaderMapRef<FScreenVS> VertexShader(GetGlobalShaderMap());

// 					GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
// 					GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
// 					GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
// 					GraphicsPSOInit.PrimitiveType = PT_TriangleList;

					ID3D11InputLayout* InputLayout = GetInputLayout(GetFilterInputDelcaration().get(), VertexShader->GetCode().Get());
					D3D11DeviceContext->IASetInputLayout(InputLayout);
					D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
					D3D11DeviceContext->PSSetShader(PixelShader->GetPixelShader(), 0, 0);
					D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

					PixelShader->SetParameters(CubeFace, NumMips, SourceMipIndex, CoefficientIndex, EffectiveSource.ShaderResourceTexture->GetShaderResourceView());

					DrawRectangle(
						ViewRect.Min.X, ViewRect.Min.Y,
						ViewRect.Width(), ViewRect.Height(),
						ViewRect.Min.X, ViewRect.Min.Y,
						ViewRect.Width(), ViewRect.Height(),
						FIntPoint(ViewRect.Width(), ViewRect.Height()),
						FIntPoint(MipSize, MipSize),
						*VertexShader);

				}
			}
		}

		{
			SCOPED_DRAW_EVENT(AccumulateCubeFacesPS);
			// Gather the cubemap face results and normalize, copy this coefficient to FSceneRenderTargets::Get(RHICmdList).SkySHIrradianceMap
			PooledRenderTarget& EffectiveRT = *FSceneRenderTargets::Get().SkySHIrradianceMap.Get();

			//load/store actions so we don't lose results as we render one pixel at a time on tile renderers.
// 			RHICmdList.TransitionResource(EResourceTransitionAccess::EWritable, EffectiveRT.TargetableTexture);
// 			FRHIRenderPassInfo RPInfo(EffectiveRT.TargetableTexture, ERenderTargetActions::Load_Store, nullptr);
// 			RHICmdList.BeginRenderPass(RPInfo, TEXT("GatherCoeffRP"));
// 			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

			SetRenderTarget(EffectiveRT.TargetableTexture.get(), nullptr, false, false, false);

			const FIntRect ViewRect(CoefficientIndex, 0, CoefficientIndex + 1, 1);
			RHISetViewport(0, 0, 0.0f, FSHVector3::MaxSHBasis, 1, 1.0f);

			TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
			TShaderMapRef<FAccumulateCubeFacesPS> PixelShader(ShaderMap);

// 			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
// 			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
// 			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
// 			GraphicsPSOInit.PrimitiveType = PT_TriangleList;

			ID3D11InputLayout* InputLayout = GetInputLayout(GetFilterInputDelcaration().get(), VertexShader->GetCode().Get());
			D3D11DeviceContext->IASetInputLayout(InputLayout);
			D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
			D3D11DeviceContext->PSSetShader(PixelShader->GetPixelShader(), 0, 0);
			D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			const int32 SourceMipIndex = NumMips - 1;
			const int32 MipSize = 1;
			PooledRenderTarget& EffectiveSource = GetEffectiveDiffuseIrradianceRenderTarget(SceneContext, SourceMipIndex);
			PixelShader->SetParameters(SourceMipIndex, EffectiveSource.ShaderResourceTexture->GetShaderResourceView());

			DrawRectangle(
				ViewRect.Min.X, ViewRect.Min.Y,
				ViewRect.Width(), ViewRect.Height(),
				0, 0,
				MipSize, MipSize,
				FIntPoint(FSHVector3::MaxSHBasis, 1),
				FIntPoint(MipSize, MipSize),
				*VertexShader);
		}
	}

	{
		// Read back the completed SH environment map
		PooledRenderTarget& EffectiveRT = *FSceneRenderTargets::Get().SkySHIrradianceMap.Get();
		assert(EffectiveRT.ShaderResourceTexture->GetFormat() == PF_FloatRGBA);

		std::vector<FFloat16Color> SurfaceData;
		RHIReadSurfaceFloatData(EffectiveRT.ShaderResourceTexture.get(), FIntRect(0, 0, FSHVector3::MaxSHBasis, 1), SurfaceData, CubeFace_PosX, 0, 0);
		assert(SurfaceData.size() == FSHVector3::MaxSHBasis);

		for (int32 CoefficientIndex = 0; CoefficientIndex < FSHVector3::MaxSHBasis; CoefficientIndex++)
		{
			const FLinearColor CoefficientValue(SurfaceData[CoefficientIndex]);
			OutIrradianceEnvironmentMap->R.V[CoefficientIndex] = CoefficientValue.R;
			OutIrradianceEnvironmentMap->G.V[CoefficientIndex] = CoefficientValue.G;
			OutIrradianceEnvironmentMap->B.V[CoefficientIndex] = CoefficientValue.B;
		}
	}
}
