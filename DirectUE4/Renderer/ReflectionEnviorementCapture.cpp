#include "ReflectionEnviorementCapture.h"
#include "Scene.h"
#include "GPUProfiler.h"
#include "ScreenRendering.h"
#include "SceneFilterRendering.h"
#include "PostProcessing.h"
#include "OneColorShader.h"
#include "World.h"
#include "SkyLight.h"

/** Near plane to use when capturing the scene. */
float GReflectionCaptureNearPlane = 5;

int32 GSupersampleCaptureFactor = 1;

int32 GDiffuseIrradianceCubemapSize = 32;

/** Vertex shader used when writing to a cubemap. */
class FCopyToCubeFaceVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCopyToCubeFaceVS, Global);
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	FCopyToCubeFaceVS() {}
	FCopyToCubeFaceVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
	}

	void SetParameters(const FViewInfo& View)
	{
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(GetVertexShader(), View.ViewUniformBuffer.get());
	}

};

IMPLEMENT_SHADER_TYPE(, FCopyToCubeFaceVS, ("ReflectionEnvironmentShaders.dusf"), ("CopyToCubeFaceVS"), SF_Vertex);

/** Pixel shader used when copying scene color from a scene render into a face of a reflection capture cubemap. */
class FCopySceneColorToCubeFacePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCopySceneColorToCubeFacePS, Global);
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	FCopySceneColorToCubeFacePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		SceneTextureParameters.Bind(Initializer);
		InTexture.Bind(Initializer.ParameterMap, ("InTexture"));
		InTextureSampler.Bind(Initializer.ParameterMap, ("InTextureSampler"));
		SkyLightCaptureParameters.Bind(Initializer.ParameterMap, ("SkyLightCaptureParameters"));
		LowerHemisphereColor.Bind(Initializer.ParameterMap, ("LowerHemisphereColor"));
	}
	FCopySceneColorToCubeFacePS() {}

	void SetParameters(const FViewInfo& View, bool bCapturingForSkyLight, bool bLowerHemisphereIsBlack, const FLinearColor& LowerHemisphereColorValue)
	{
		ID3D11PixelShader* const ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(ShaderRHI, View.ViewUniformBuffer.get());
		SceneTextureParameters.Set(ShaderRHI,ESceneTextureSetupMode::All);

		SetTextureParameter(
			ShaderRHI,
			InTexture,
			InTextureSampler,
			TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI(),
			FSceneRenderTargets::Get().GetSceneColor()->ShaderResourceTexture->GetShaderResourceView());

		FVector SkyLightParametersValue = FVector::ZeroVector;
		FScene* Scene = (FScene*)View.Family->Scene;

		if (bCapturingForSkyLight)
		{
			// When capturing reflection captures, support forcing all low hemisphere lighting to be black
			SkyLightParametersValue = FVector(0, 0, bLowerHemisphereIsBlack ? 1.0f : 0.0f);
		}
		else if (Scene->SkyLight && !Scene->SkyLight->bHasStaticLighting)
		{
			// When capturing reflection captures and there's a stationary sky light, mask out any pixels whose depth classify it as part of the sky
			// This will allow changing the stationary sky light at runtime
			SkyLightParametersValue = FVector(1, Scene->SkyLight->SkyDistanceThreshold, 0);
		}
		else
		{
			// When capturing reflection captures and there's no sky light, or only a static sky light, capture all depth ranges
			SkyLightParametersValue = FVector(2, 0, 0);
		}

		SetShaderValue(ShaderRHI, SkyLightCaptureParameters, SkyLightParametersValue);
		SetShaderValue(ShaderRHI, LowerHemisphereColor, LowerHemisphereColorValue);
	}

private:
	FSceneTextureShaderParameters SceneTextureParameters;
	FShaderResourceParameter InTexture;
	FShaderResourceParameter InTextureSampler;
	FShaderParameter SkyLightCaptureParameters;
	FShaderParameter LowerHemisphereColor;
};

IMPLEMENT_SHADER_TYPE(, FCopySceneColorToCubeFacePS, ("ReflectionEnvironmentShaders.dusf"), ("CopySceneColorToCubeFaceColorPS"), SF_Pixel);

void CreateCubeMips(int32 NumMips, ComPtr<PooledRenderTarget>& Cubemap)
{
	SCOPED_DRAW_EVENT(CreateCubeMips);

	FD3D11Texture2D* CubeRef = Cubemap->TargetableTexture.get();

	auto* ShaderMap = GetGlobalShaderMap();

// 	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	ID3D11RasterizerState* RasterizerState = TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_NONE>::GetRHI();
	ID3D11DepthStencilState* DepthStencilState = TStaticDepthStencilState<false, D3D11_COMPARISON_ALWAYS>::GetRHI();
	ID3D11BlendState* BlendState = TStaticBlendState<>::GetRHI();

	D3D11DeviceContext->RSSetState(RasterizerState);
	D3D11DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);
	D3D11DeviceContext->OMSetBlendState(BlendState, nullptr, 0xffffffff);

	// Downsample all the mips, each one reads from the mip above it
	for (int32 MipIndex = 1; MipIndex < NumMips; MipIndex++)
	{
		const int32 MipSize = 1 << (NumMips - MipIndex - 1);
		SCOPED_DRAW_EVENT(CreateCubeMipsPerFace);
		for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
		{
			//FRHIRenderPassInfo RPInfo(Cubemap.TargetableTexture, ERenderTargetActions::DontLoad_Store, nullptr, MipIndex, CubeFace);
			//RPInfo.bGeneratingMips = true;
			//RHICmdList.BeginRenderPass(RPInfo, TEXT("CreateCubeMips"));
			//RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

			SetRenderTarget(Cubemap->TargetableTexture.get(), nullptr, false, false, false, MipIndex, CubeFace);

			const FIntRect ViewRect(0, 0, MipSize, MipSize);
			RHISetViewport(0, 0, 0.0f, MipSize, MipSize, 1.0f);

			TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
			TShaderMapRef<FCubeFilterPS> PixelShader(ShaderMap);

			//GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
			//GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
			//GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
			//GraphicsPSOInit.PrimitiveType = PT_TriangleList;

			ID3D11InputLayout* InputLayout = GetInputLayout(GetFilterInputDelcaration().get(), VertexShader->GetCode().Get());
			D3D11DeviceContext->IASetInputLayout(InputLayout);
			D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
			D3D11DeviceContext->PSSetShader(PixelShader->GetPixelShader(), 0, 0);
			D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			//SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

			{
				ID3D11PixelShader* const ShaderRHI = PixelShader->GetPixelShader();

				SetShaderValue(ShaderRHI, PixelShader->CubeFace, CubeFace);
				SetShaderValue(ShaderRHI, PixelShader->MipIndex, MipIndex);

				SetShaderValue(ShaderRHI, PixelShader->NumMips, NumMips);

				SetSRVParameter(ShaderRHI, PixelShader->SourceTexture, Cubemap->MipSRVs[MipIndex - 1].Get());
				SetSamplerParameter(ShaderRHI, PixelShader->SourceTextureSampler, TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI());
			}

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

	//RHICmdList.TransitionResources(EResourceTransitionAccess::EReadable, &CubeRef, 1);
}

/** Pixel shader used when copying a cubemap into a face of a reflection capture cubemap. */
class FCopyCubemapToCubeFacePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCopyCubemapToCubeFacePS, Global);
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	FCopyCubemapToCubeFacePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		CubeFace.Bind(Initializer.ParameterMap, ("CubeFace"));
		SourceTexture.Bind(Initializer.ParameterMap, ("SourceTexture"));
		SourceTextureSampler.Bind(Initializer.ParameterMap, ("SourceTextureSampler"));
		SkyLightCaptureParameters.Bind(Initializer.ParameterMap, ("SkyLightCaptureParameters"));
		LowerHemisphereColor.Bind(Initializer.ParameterMap, ("LowerHemisphereColor"));
		SinCosSourceCubemapRotation.Bind(Initializer.ParameterMap, ("SinCosSourceCubemapRotation"));
	}
	FCopyCubemapToCubeFacePS() {}

	void SetParameters(ID3D11ShaderResourceView* const SourceCubemap, uint32 CubeFaceValue, bool bIsSkyLight, bool bLowerHemisphereIsBlack, float SourceCubemapRotation, const FLinearColor& LowerHemisphereColorValue)
	{
		ID3D11PixelShader* const ShaderRHI = GetPixelShader();

		SetShaderValue(ShaderRHI, CubeFace, CubeFaceValue);

		SetTextureParameter(
			ShaderRHI,
			SourceTexture,
			SourceTextureSampler,
			TStaticSamplerState<>::GetRHI(),
			SourceCubemap);

		SetShaderValue(ShaderRHI, SkyLightCaptureParameters, FVector(bIsSkyLight ? 1.0f : 0.0f, 0.0f, bLowerHemisphereIsBlack ? 1.0f : 0.0f));
		SetShaderValue(ShaderRHI, LowerHemisphereColor, LowerHemisphereColorValue);

		SetShaderValue(ShaderRHI, SinCosSourceCubemapRotation, Vector2(FMath::Sin(SourceCubemapRotation), FMath::Cos(SourceCubemapRotation)));
	}

private:
	FShaderParameter CubeFace;
	FShaderResourceParameter SourceTexture;
	FShaderResourceParameter SourceTextureSampler;
	FShaderParameter SkyLightCaptureParameters;
	FShaderParameter LowerHemisphereColor;
	FShaderParameter SinCosSourceCubemapRotation;
};

IMPLEMENT_SHADER_TYPE(, FCopyCubemapToCubeFacePS, ("ReflectionEnvironmentShaders.dusf"), ("CopyCubemapToCubeFaceColorPS"), SF_Pixel);

void ClearScratchCubemaps(int32 TargetSize)
{
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();
	SceneContext.AllocateReflectionTargets(TargetSize);

	ComPtr<PooledRenderTarget>& RT0 = SceneContext.ReflectionColorScratchCubemap[0];
	int32 NumMips = (int32)RT0->TargetableTexture->GetNumMips();

	{
		for (int32 MipIndex = 0; MipIndex < NumMips; MipIndex++)
		{
			for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
			{
// 				FRHIRenderTargetView RtView = FRHIRenderTargetView(RT0.TargetableTexture, ERenderTargetLoadAction::EClear, MipIndex, CubeFace);
// 				FRHISetRenderTargetsInfo Info(1, &RtView, FRHIDepthRenderTargetView());
// 				RHICmdList.SetRenderTargetsAndClear(Info);
				SetRenderTarget(RT0->TargetableTexture.get(), nullptr, true, false, false, MipIndex, CubeFace);
			}
		}
	}

	{
		ComPtr<PooledRenderTarget>& RT1 = SceneContext.ReflectionColorScratchCubemap[1];
		NumMips = (int32)RT1->TargetableTexture->GetNumMips();

		for (int32 MipIndex = 0; MipIndex < NumMips; MipIndex++)
		{
			for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
			{
// 				FRHIRenderTargetView RtView = FRHIRenderTargetView(RT1.TargetableTexture, ERenderTargetLoadAction::EClear, MipIndex, CubeFace);
// 				FRHISetRenderTargetsInfo Info(1, &RtView, FRHIDepthRenderTargetView());
// 				RHICmdList.SetRenderTargetsAndClear(Info);
				SetRenderTarget(RT1->TargetableTexture.get(), nullptr, true, false, false, MipIndex, CubeFace);
			}
		}
	}
}

void CaptureSceneToScratchCubemap(FSceneRenderer* SceneRenderer, ECubeFace CubeFace, int32 CubemapSize, bool bCapturingForSkyLight, bool bLowerHemisphereIsBlack, const FLinearColor& LowerHemisphereColor)
{
	SCOPED_DRAW_EVENT(CubeMapCapture);

	SceneRenderer->Render();

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();
	SceneContext.AllocateReflectionTargets(CubemapSize);

	auto ShaderMap = GetGlobalShaderMap();

	const int32 EffectiveSize = CubemapSize;
	ComPtr<PooledRenderTarget>& EffectiveColorRT = SceneContext.ReflectionColorScratchCubemap[0];

	{
		//FRHIRenderPassInfo RPInfo(EffectiveColorRT.TargetableTexture, ERenderTargetActions::DontLoad_Store, nullptr, 0, CubeFace);
		//RHICmdList.BeginRenderPass(RPInfo, TEXT("CubeMapCopySceneRP"));
		SetRenderTarget(EffectiveColorRT->TargetableTexture.get(), nullptr, true, false, false, 0, CubeFace);

		const FIntRect ViewRect(0, 0, EffectiveSize, EffectiveSize);
		RHISetViewport(0, 0, 0.0f, EffectiveSize, EffectiveSize, 1.0f);

		ID3D11RasterizerState* RasterizerState = TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_NONE>::GetRHI();
		ID3D11DepthStencilState* DepthStencilState = TStaticDepthStencilState<false, D3D11_COMPARISON_ALWAYS>::GetRHI();
		ID3D11BlendState* BlendState = TStaticBlendState<>::GetRHI();

		TShaderMapRef<FCopyToCubeFaceVS> VertexShader(GetGlobalShaderMap());
		TShaderMapRef<FCopySceneColorToCubeFacePS> PixelShader(GetGlobalShaderMap());

		ID3D11InputLayout* InputLayout = GetInputLayout(GetFilterInputDelcaration().get(), VertexShader->GetCode().Get());
		D3D11DeviceContext->IASetInputLayout(InputLayout);
		D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
		D3D11DeviceContext->PSSetShader(PixelShader->GetPixelShader(), 0, 0);
		D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		PixelShader->SetParameters(SceneRenderer->Views[0], bCapturingForSkyLight, bLowerHemisphereIsBlack, LowerHemisphereColor);
		VertexShader->SetParameters(SceneRenderer->Views[0]);

		DrawRectangle(
			ViewRect.Min.X, ViewRect.Min.Y,
			ViewRect.Width(), ViewRect.Height(),
			ViewRect.Min.X, ViewRect.Min.Y,
			ViewRect.Width() * GSupersampleCaptureFactor, ViewRect.Height() * GSupersampleCaptureFactor,
			FIntPoint(ViewRect.Width(), ViewRect.Height()),
			SceneContext.GetBufferSizeXY(),
			*VertexShader);
	}
}
/** Creates a transformation for a cubemap face, following the D3D cubemap layout. */
FMatrix CalcCubeFaceViewRotationMatrix(ECubeFace Face)
{
	FMatrix Result(FMatrix::Identity);

	static const FVector XAxis(1.f, 0.f, 0.f);
	static const FVector YAxis(0.f, 1.f, 0.f);
	static const FVector ZAxis(0.f, 0.f, 1.f);

	// vectors we will need for our basis
	FVector vUp(YAxis);
	FVector vDir;

	switch (Face)
	{
	case CubeFace_PosX:
		vDir = XAxis;
		break;
	case CubeFace_NegX:
		vDir = -XAxis;
		break;
	case CubeFace_PosY:
		vUp = -ZAxis;
		vDir = YAxis;
		break;
	case CubeFace_NegY:
		vUp = ZAxis;
		vDir = -YAxis;
		break;
	case CubeFace_PosZ:
		vDir = ZAxis;
		break;
	case CubeFace_NegZ:
		vDir = -ZAxis;
		break;
	}

	// derive right vector
	FVector vRight(vUp ^ vDir);
	// create matrix from the 3 axes
	Result = FBasisVectorMatrix(vRight, vUp, vDir, FVector::ZeroVector);

	return Result;
}

void CaptureSceneIntoScratchCubemap(
	FScene* Scene,
	FVector CapturePosition,
	int32 CubemapSize,
	bool bCapturingForSkyLight,
	bool bStaticSceneOnly,
	float SkyLightNearPlane,
	bool bLowerHemisphereIsBlack,
	bool bCaptureEmissiveOnly,
	const FLinearColor& LowerHemisphereColor
)
{
	for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
	{
		FSceneViewFamily ViewFamily;

		ViewInitOptions InitOptions;
		InitOptions.ViewFamily = &ViewFamily;
		//InitOptions.BackgroundColor = FLinearColor::Black;
		//InitOptions.OverlayColor = FLinearColor::Black;
		InitOptions.SetViewRectangle(FIntRect(0, 0, CubemapSize * GSupersampleCaptureFactor, CubemapSize * GSupersampleCaptureFactor));

		const float NearPlane = bCapturingForSkyLight ? SkyLightNearPlane : GReflectionCaptureNearPlane;

		// Projection matrix based on the fov, near / far clip settings
		// Each face always uses a 90 degree field of view
		if ((bool)ERHIZBuffer::IsInverted)
		{
			InitOptions.ProjectionMatrix = FReversedZPerspectiveMatrix(
				90.0f * (float)PI / 360.0f,
				(float)CubemapSize * GSupersampleCaptureFactor,
				(float)CubemapSize * GSupersampleCaptureFactor,
				NearPlane
			);
		}
		else
		{
			InitOptions.ProjectionMatrix = FPerspectiveMatrix(
				90.0f * (float)PI / 360.0f,
				(float)CubemapSize * GSupersampleCaptureFactor,
				(float)CubemapSize * GSupersampleCaptureFactor,
				NearPlane
			);
		}

		InitOptions.ViewOrigin = CapturePosition;
		InitOptions.ViewRotationMatrix = CalcCubeFaceViewRotationMatrix((ECubeFace)CubeFace);

		FSceneView* View = new FSceneView(InitOptions);

		// Force all surfaces diffuse
		//View->RoughnessOverrideParameter = Vector2(1.0f, 0.0f);

		if (bCaptureEmissiveOnly)
		{
			//View->DiffuseOverrideParameter = Vector4(0, 0, 0, 0);
			//View->SpecularOverrideParameter = Vector4(0, 0, 0, 0);
		}

		//View->bIsReflectionCapture = true;
		View->bStaticSceneOnly = bStaticSceneOnly;
		//View->StartFinalPostprocessSettings(CapturePosition);
		//View->EndFinalPostprocessSettings(InitOptions);

		ViewFamily.Views.push_back(View);
		ViewFamily.Scene = Scene;

		FSceneRenderer SceneRenderer(ViewFamily);

		CaptureSceneToScratchCubemap(&SceneRenderer, (ECubeFace)CubeFace, CubemapSize, bCapturingForSkyLight, bLowerHemisphereIsBlack, LowerHemisphereColor);
	}
}

void CopyCubemapToScratchCubemap(FD3D11Texture2D* SourceCubemap, int32 CubemapSize, bool bIsSkyLight, bool bLowerHemisphereIsBlack, float SourceCubemapRotation, const FLinearColor& LowerHemisphereColorValue)
{
	SCOPED_DRAW_EVENT(CopyCubemapToScratchCubemap);
	assert(SourceCubemap);

	const int32 EffectiveSize = CubemapSize;
	ComPtr<PooledRenderTarget>& EffectiveColorRT = FSceneRenderTargets::Get().ReflectionColorScratchCubemap[0];

	for (uint32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
	{
		// Copy the captured scene into the cubemap face
		//FRHIRenderPassInfo RPInfo(EffectiveColorRT.TargetableTexture, ERenderTargetActions::DontLoad_Store, nullptr, 0, CubeFace);
		//RHICmdList.BeginRenderPass(RPInfo, TEXT("CopyCubemapToScratchCubemapRP"));

		SetRenderTarget(EffectiveColorRT->TargetableTexture.get(),nullptr,true,false,false, 0, CubeFace);

		ID3D11ShaderResourceView* const SourceCubemapResource = SourceCubemap->GetShaderResourceView();
		const FIntPoint SourceDimensions(SourceCubemap->GetSizeX(), SourceCubemap->GetSizeY());
		const FIntRect ViewRect(0, 0, EffectiveSize, EffectiveSize);
		RHISetViewport(0, 0, 0.0f, EffectiveSize, EffectiveSize, 1.0f);

		//FGraphicsPipelineStateInitializer GraphicsPSOInit;
		//RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		ID3D11RasterizerState* RasterizerState = TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_NONE>::GetRHI();
		ID3D11DepthStencilState* DepthStencilState = TStaticDepthStencilState<false, D3D11_COMPARISON_ALWAYS>::GetRHI();
		ID3D11BlendState* BlendState = TStaticBlendState<>::GetRHI();

		TShaderMapRef<FScreenVS> VertexShader(GetGlobalShaderMap());
		TShaderMapRef<FCopyCubemapToCubeFacePS> PixelShader(GetGlobalShaderMap());

		//GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		//GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
		//GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		//GraphicsPSOInit.PrimitiveType = PT_TriangleList;

		ID3D11InputLayout* InputLayout = GetInputLayout(GetFilterInputDelcaration().get(), VertexShader->GetCode().Get());
		D3D11DeviceContext->IASetInputLayout(InputLayout);
		D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
		D3D11DeviceContext->PSSetShader(PixelShader->GetPixelShader(), 0, 0);
		D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
		PixelShader->SetParameters(SourceCubemapResource, CubeFace, bIsSkyLight, bLowerHemisphereIsBlack, SourceCubemapRotation, LowerHemisphereColorValue);

		DrawRectangle(
			ViewRect.Min.X, ViewRect.Min.Y,
			ViewRect.Width(), ViewRect.Height(),
			0, 0,
			SourceDimensions.X, SourceDimensions.Y,
			FIntPoint(ViewRect.Width(), ViewRect.Height()),
			SourceDimensions,
			*VertexShader);
	}
}

void ReadbackRadianceMap(int32 CubmapSize, std::vector<FFloat16Color>& OutRadianceMap)
{
	OutRadianceMap.clear();
	OutRadianceMap.resize(CubmapSize * CubmapSize * 6);

	const int32 MipIndex = 0;

	ComPtr<PooledRenderTarget>& SourceCube = FSceneRenderTargets::Get().ReflectionColorScratchCubemap[0];
	assert(SourceCube->ShaderResourceTexture->GetFormat() == PF_FloatRGBA);
	const int32 CubeFaceBytes = CubmapSize * CubmapSize * sizeof(FFloat16Color);

	for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
	{
		std::vector<FFloat16Color> SurfaceData;

		// Read each mip face
		RHIReadSurfaceFloatData(SourceCube->ShaderResourceTexture.get(), FIntRect(0, 0, CubmapSize, CubmapSize), SurfaceData, (ECubeFace)CubeFace, 0, MipIndex);
		const int32 DestIndex = CubeFace * CubmapSize * CubmapSize;
		FFloat16Color* FaceData = &OutRadianceMap[DestIndex];
		assert(SurfaceData.size() * sizeof(FFloat16Color) == CubeFaceBytes);
		memcpy(FaceData, SurfaceData.data(), CubeFaceBytes);
	}
}

void FullyResolveReflectionScratchCubes()
{
	SCOPED_DRAW_EVENT(FullyResolveReflectionScratchCubes);
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();
	std::shared_ptr<FD3D11Texture2D>& Scratch0 = SceneContext.ReflectionColorScratchCubemap[0]->TargetableTexture;
	std::shared_ptr<FD3D11Texture2D>& Scratch1 = SceneContext.ReflectionColorScratchCubemap[1]->TargetableTexture;
	FResolveParams ResolveParams(FResolveRect(), CubeFace_PosX, -1, -1, -1);
	RHICopyToResolveTarget(Scratch0.get(), Scratch0.get(), ResolveParams);
	RHICopyToResolveTarget(Scratch1.get(), Scratch1.get(), ResolveParams);
}

IMPLEMENT_SHADER_TYPE(, FCubeFilterPS, ("ReflectionEnvironmentShaders.dusf"), ("DownsamplePS"), SF_Pixel);

IMPLEMENT_SHADER_TYPE(template<>, TCubeFilterPS<0>, ("ReflectionEnvironmentShaders.dusf"), ("FilterPS"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>, TCubeFilterPS<1>, ("ReflectionEnvironmentShaders.dusf"), ("FilterPS"), SF_Pixel);

/** Computes the average brightness of a 1x1 mip of a cubemap. */
class FComputeBrightnessPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FComputeBrightnessPS, Global)
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(("COMPUTEBRIGHTNESS_PIXELSHADER"), 1);
	}

	FComputeBrightnessPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		ReflectionEnvironmentColorTexture.Bind(Initializer.ParameterMap, ("ReflectionEnvironmentColorTexture"));
		ReflectionEnvironmentColorSampler.Bind(Initializer.ParameterMap, ("ReflectionEnvironmentColorSampler"));
		NumCaptureArrayMips.Bind(Initializer.ParameterMap, ("NumCaptureArrayMips"));
	}

	FComputeBrightnessPS()
	{
	}

	void SetParameters(int32 TargetSize, ComPtr<PooledRenderTarget>& Cubemap)
	{
		const int32 EffectiveTopMipSize = TargetSize;
		const int32 NumMips = FMath::CeilLogTwo(EffectiveTopMipSize) + 1;
		// Read from the smallest mip that was downsampled to

		if (Cubemap.Get())
		{
			SetTextureParameter(
				GetPixelShader(),
				ReflectionEnvironmentColorTexture,
				ReflectionEnvironmentColorSampler,
				TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI(),
				Cubemap->ShaderResourceTexture->GetShaderResourceView());
		}

		SetShaderValue(GetPixelShader(), NumCaptureArrayMips, FMath::CeilLogTwo(TargetSize) + 1);
	}

private:

	FShaderResourceParameter ReflectionEnvironmentColorTexture;
	FShaderResourceParameter ReflectionEnvironmentColorSampler;
	FShaderParameter NumCaptureArrayMips;
};

IMPLEMENT_SHADER_TYPE(, FComputeBrightnessPS, ("ReflectionEnvironmentShaders.dusf"), ("ComputeBrightnessMain"), SF_Pixel);

float ComputeSingleAverageBrightnessFromCubemap(int32 TargetSize, ComPtr<PooledRenderTarget>& Cubemap)
{
	SCOPED_DRAW_EVENT(ComputeSingleAverageBrightnessFromCubemap);

	ComPtr<PooledRenderTarget> ReflectionBrightnessTarget;
	PooledRenderTargetDesc Desc(PooledRenderTargetDesc::Create2DDesc(FIntPoint(1, 1), PF_FloatRGBA, FClearValueBinding::None, TexCreate_None, TexCreate_RenderTargetable, false));
	GRenderTargetPool.FindFreeElement(Desc, ReflectionBrightnessTarget, TEXT("ReflectionBrightness"));

	std::shared_ptr<FD3D11Texture2D>& BrightnessTarget = ReflectionBrightnessTarget->TargetableTexture;
	SetRenderTarget(BrightnessTarget.get(), NULL, false);

	//FGraphicsPipelineStateInitializer GraphicsPSOInit;
	//RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	ID3D11RasterizerState* RasterizerState = TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_NONE>::GetRHI();
	ID3D11DepthStencilState* DepthStencilState = TStaticDepthStencilState<false, D3D11_COMPARISON_ALWAYS>::GetRHI();
	ID3D11BlendState* BlendState = TStaticBlendState<>::GetRHI();

	auto ShaderMap = GetGlobalShaderMap();
	TShaderMapRef<FPostProcessVS> VertexShader(ShaderMap);
	TShaderMapRef<FComputeBrightnessPS> PixelShader(ShaderMap);

	//GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	//GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	//GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	//GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	ID3D11InputLayout* InputLayout = GetInputLayout(GetFilterInputDelcaration().get(), VertexShader->GetCode().Get());
	D3D11DeviceContext->IASetInputLayout(InputLayout);
	D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
	D3D11DeviceContext->PSSetShader(PixelShader->GetPixelShader(), 0, 0);
	D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	PixelShader->SetParameters(TargetSize, Cubemap);

	DrawRectangle(
		0, 0,
		1, 1,
		0, 0,
		1, 1,
		FIntPoint(1, 1),
		FIntPoint(1, 1),
		*VertexShader);

	RHICopyToResolveTarget(BrightnessTarget.get(), BrightnessTarget.get(), FResolveParams());

	ComPtr<PooledRenderTarget>& EffectiveRT = ReflectionBrightnessTarget;
	assert(EffectiveRT->ShaderResourceTexture->GetFormat() == PF_FloatRGBA);

	std::vector<FFloat16Color> SurfaceData;
	RHIReadSurfaceFloatData(EffectiveRT->ShaderResourceTexture.get(), FIntRect(0, 0, 1, 1), SurfaceData, CubeFace_PosX, 0, 0);

	// Shader outputs luminance to R
	float AverageBrightness = SurfaceData[0].R.GetFloat();
	return AverageBrightness;
}

void ComputeAverageBrightness(int32 CubmapSize, float& OutAverageBrightness)
{
	const int32 EffectiveTopMipSize = CubmapSize;
	const int32 NumMips = FMath::CeilLogTwo(EffectiveTopMipSize) + 1;

	// necessary to resolve the clears which touched all the mips.  scene rendering only resolves mip 0.
	FullyResolveReflectionScratchCubes();

	ComPtr<PooledRenderTarget>& DownSampledCube = FSceneRenderTargets::Get().ReflectionColorScratchCubemap[0];
	CreateCubeMips(NumMips, DownSampledCube);

	OutAverageBrightness = ComputeSingleAverageBrightnessFromCubemap(CubmapSize, DownSampledCube);
}

void FilterReflectionEnvironment(int32 CubmapSize, FSHVectorRGB3* OutIrradianceEnvironmentMap)
{
	const int32 EffectiveTopMipSize = CubmapSize;
	const int32 NumMips = FMath::CeilLogTwo(EffectiveTopMipSize) + 1;

	ComPtr<PooledRenderTarget>& EffectiveColorRT = FSceneRenderTargets::Get().ReflectionColorScratchCubemap[0];

	//FGraphicsPipelineStateInitializer GraphicsPSOInit;
	ID3D11RasterizerState* RasterizerState = TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_NONE>::GetRHI();
	ID3D11DepthStencilState* DepthStencilState = TStaticDepthStencilState<false, D3D11_COMPARISON_ALWAYS>::GetRHI();
	ID3D11BlendState* BlendState = TStaticBlendState<false,false, D3D11_COLOR_WRITE_ENABLE_ALL, D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_ONE>::GetRHI();
	
	D3D11DeviceContext->RSSetState(RasterizerState);
	D3D11DeviceContext->OMSetDepthStencilState(DepthStencilState,0);
	D3D11DeviceContext->OMSetBlendState(BlendState, NULL, 0xffffffff);

	//RHICmdList.TransitionResource(EResourceTransitionAccess::EWritable, EffectiveColorRT.TargetableTexture);

	// Premultiply alpha in-place using alpha blending
	for (uint32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
	{
		//FRHIRenderPassInfo RPInfo(EffectiveColorRT.TargetableTexture, ERenderTargetActions::Load_Store, nullptr, 0, CubeFace);
		//RHICmdList.BeginRenderPass(RPInfo, TEXT("FilterReflectionEnvironmentRP"));
		//RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

		SetRenderTarget(EffectiveColorRT->TargetableTexture.get(), nullptr, false, false, false, 0, CubeFace);

		const FIntPoint SourceDimensions(CubmapSize, CubmapSize);
		const FIntRect ViewRect(0, 0, EffectiveTopMipSize, EffectiveTopMipSize);
		RHISetViewport(0, 0, 0.0f, EffectiveTopMipSize, EffectiveTopMipSize, 1.0f);

		TShaderMapRef<FScreenVS> VertexShader(GetGlobalShaderMap());
		TShaderMapRef<FOneColorPS> PixelShader(GetGlobalShaderMap());

		//GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		//GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
		//GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		//GraphicsPSOInit.PrimitiveType = PT_TriangleList;

		ID3D11InputLayout* InputLayout = GetInputLayout(GetFilterInputDelcaration().get(), VertexShader->GetCode().Get());
		D3D11DeviceContext->IASetInputLayout(InputLayout);
		D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
		D3D11DeviceContext->PSSetShader(PixelShader->GetPixelShader(), 0, 0);
		D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

		FLinearColor UnusedColors[1] = { FLinearColor::Black };
		PixelShader->SetColors(UnusedColors, sizeof( UnusedColors)/sizeof(UnusedColors[0]));

		DrawRectangle(
			ViewRect.Min.X, ViewRect.Min.Y,
			ViewRect.Width(), ViewRect.Height(),
			0, 0,
			SourceDimensions.X, SourceDimensions.Y,
			FIntPoint(ViewRect.Width(), ViewRect.Height()),
			SourceDimensions,
			*VertexShader);
	}

	auto ShaderMap = GetGlobalShaderMap();
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();

	ComPtr<PooledRenderTarget>& DownSampledCube = FSceneRenderTargets::Get().ReflectionColorScratchCubemap[0];
	ComPtr<PooledRenderTarget>& FilteredCube = FSceneRenderTargets::Get().ReflectionColorScratchCubemap[1];

	CreateCubeMips(NumMips, DownSampledCube);

	if (OutIrradianceEnvironmentMap)
	{
		SCOPED_DRAW_EVENT(ComputeDiffuseIrradiance);

		const int32 NumDiffuseMips = FMath::CeilLogTwo(GDiffuseIrradianceCubemapSize) + 1;
		const int32 DiffuseConvolutionSourceMip = NumMips - NumDiffuseMips;

		ComputeDiffuseIrradiance(DownSampledCube->ShaderResourceTexture.get(), DiffuseConvolutionSourceMip, OutIrradianceEnvironmentMap);
	}

	{
		SCOPED_DRAW_EVENT(FilterCubeMap);

		ID3D11RasterizerState* RasterizerState = TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_NONE>::GetRHI();
		ID3D11DepthStencilState* DepthStencilState = TStaticDepthStencilState<false, D3D11_COMPARISON_ALWAYS>::GetRHI();
		ID3D11BlendState* BlendState = TStaticBlendState<>::GetRHI();

		D3D11DeviceContext->RSSetState(RasterizerState);
		D3D11DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);
		D3D11DeviceContext->OMSetBlendState(BlendState, NULL, 0xffffffff);

		//RHICmdList.TransitionResource(EResourceTransitionAccess::EWritable, FilteredCube.TargetableTexture);

		// Filter all the mips
		for (int32 MipIndex = 0; MipIndex < NumMips; MipIndex++)
		{
			const int32 MipSize = 1 << (NumMips - MipIndex - 1);

			for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
			{
				//FRHIRenderPassInfo RPInfo(FilteredCube.TargetableTexture, ERenderTargetActions::DontLoad_Store, nullptr, MipIndex, CubeFace);
				//RHICmdList.BeginRenderPass(RPInfo, TEXT("FilterCubeMapRP"));
				//RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

				SetRenderTarget(FilteredCube->TargetableTexture.get(),nullptr,false,false,false, MipIndex, CubeFace);

				const FIntRect ViewRect(0, 0, MipSize, MipSize);
				RHISetViewport(0, 0, 0.0f, MipSize, MipSize, 1.0f);

				TShaderMapRef<FScreenVS> VertexShader(GetGlobalShaderMap());
				TShaderMapRef< TCubeFilterPS<1> > CaptureCubemapArrayPixelShader(GetGlobalShaderMap());

				FCubeFilterPS* PixelShader;

				PixelShader = *TShaderMapRef< TCubeFilterPS<0> >(ShaderMap);
				assert(PixelShader);

				//GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
				//GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
				//GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(PixelShader);
				//GraphicsPSOInit.PrimitiveType = PT_TriangleList;

				ID3D11InputLayout* InputLayout = GetInputLayout(GetFilterInputDelcaration().get(), VertexShader->GetCode().Get());
				D3D11DeviceContext->IASetInputLayout(InputLayout);
				D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
				D3D11DeviceContext->PSSetShader(PixelShader->GetPixelShader(), 0, 0);
				D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				//SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

				{
					ID3D11PixelShader* const ShaderRHI = PixelShader->GetPixelShader();

					SetShaderValue(ShaderRHI, PixelShader->CubeFace, CubeFace);
					SetShaderValue(ShaderRHI, PixelShader->MipIndex, MipIndex);

					SetShaderValue(ShaderRHI, PixelShader->NumMips, NumMips);

					SetTextureParameter(
						ShaderRHI,
						PixelShader->SourceTexture,
						PixelShader->SourceTextureSampler,
						TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI(),
						DownSampledCube->ShaderResourceTexture->GetShaderResourceView());
				}

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

		//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, FilteredCube.TargetableTexture);
	}
}

void CopyToSkyTexture(FScene* Scene, FD3D11Texture2D* ProcessedTexture)
{
	SCOPED_DRAW_EVENT(CopyToSkyTexture);
	if (ProcessedTexture->GetResource())
	{
		const int32 EffectiveTopMipSize = ProcessedTexture->GetSizeX();
		const int32 NumMips = FMath::CeilLogTwo(EffectiveTopMipSize) + 1;
		FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();

		ComPtr<PooledRenderTarget>& FilteredCube = FSceneRenderTargets::Get().ReflectionColorScratchCubemap[1];

		// GPU copy back to the skylight's texture, which is not a render target
		FRHICopyTextureInfo CopyInfo(FilteredCube->ShaderResourceTexture->GetSizeXYZ());
		CopyInfo.NumArraySlices = 6;
		for (int32 MipIndex = 0; MipIndex < NumMips; MipIndex++)
		{
			RHICopyTexture(FilteredCube->ShaderResourceTexture.get(), ProcessedTexture, CopyInfo);
			CopyInfo.AdvanceMip();
		}

		//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, ProcessedTexture->TextureRHI);
	}
}

void FScene::UpdateSkyCaptureContents(
	const USkyLightComponent* CaptureComponent, 
	bool bCaptureEmissiveOnly, 
	FD3D11Texture2D* SourceCubemap,
	FD3D11Texture2D* OutProcessedTexture,
	float& OutAverageBrightness, 
	FSHVectorRGB3& OutIrradianceEnvironmentMap, 
	std::vector<FFloat16Color>* OutRadianceMap)
{
	//if (GSupportsRenderTargetFormat_PF_FloatRGBA || GetFeatureLevel() >= ERHIFeatureLevel::SM4)
	{
		World = GetWorld();
		if (World)
		{
			//guarantee that all render proxies are up to date before kicking off this render
			World->SendAllEndOfFrameUpdates();
		}

		ClearScratchCubemaps(CaptureComponent->CubemapResolution);

		if (CaptureComponent->SourceType == SLS_CapturedScene)
		{
			bool bStaticSceneOnly = CaptureComponent->Mobility == EComponentMobility::Static;
			CaptureSceneIntoScratchCubemap(this, CaptureComponent->GetComponentLocation(), CaptureComponent->CubemapResolution, true, bStaticSceneOnly, CaptureComponent->SkyDistanceThreshold, CaptureComponent->bLowerHemisphereIsBlack, bCaptureEmissiveOnly, CaptureComponent->LowerHemisphereColor);
		}
		else if (CaptureComponent->SourceType == SLS_SpecifiedCubemap)
		{
			CopyCubemapToScratchCubemap(SourceCubemap, CaptureComponent->CubemapResolution, true, CaptureComponent->bLowerHemisphereIsBlack, CaptureComponent->SourceCubemapAngle * (PI / 180.f), CaptureComponent->LowerHemisphereColor);
		}
		else
		{
			assert(0);
		}

		if (OutRadianceMap)
		{
			ReadbackRadianceMap(CaptureComponent->CubemapResolution, *OutRadianceMap);
		}

		{
			ComputeAverageBrightness(CaptureComponent->CubemapResolution, OutAverageBrightness);
			FilterReflectionEnvironment(CaptureComponent->CubemapResolution, &OutIrradianceEnvironmentMap);
		}

		// Optionally copy the filtered mip chain to the output texture
		if (OutProcessedTexture)
		{
			{
				CopyToSkyTexture(this, OutProcessedTexture);
			}
		}
	}
}
