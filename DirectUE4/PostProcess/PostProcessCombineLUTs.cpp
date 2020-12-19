#include "PostProcessCombineLUTs.h"
#include "VolumeRendering.h"
#include "GPUProfiler.h"

const uint32 GMaxLUTBlendCount = 5;

const int32 GCombineLUTsComputeTileSize = 8;

int32 GLUTSize = 32;

struct FColorTransform
{
	FColorTransform()
	{
		Reset();
	}

	float			MinValue;
	float			MidValue;
	float			MaxValue;

	void Reset()
	{
		MinValue = 0.0f;
		MidValue = 0.5f;
		MaxValue = 1.0f;
	}
};

template <uint32 BlendCount>
class FCombineLUTsShaderParameters
{
public:
	FCombineLUTsShaderParameters() {}

	FCombineLUTsShaderParameters(const FShaderParameterMap& ParameterMap)
		: ColorRemapShaderParameters(ParameterMap)
	{
		// Suppress static code analysis warnings about a potentially ill-defined loop. BlendCount > 0 is valid.
		//CA_SUPPRESS(6294)

			// starts as 1 as 0 is the neutral one
			for (uint32 i = 1; i < BlendCount; ++i)
			{
				std::string Name = std::string("Texture")+ std::to_string(i);
				std::string NameSampler = Name + ("Sampler");
				TextureParameter[i].Bind(ParameterMap, Name.c_str());
				TextureParameterSampler[i].Bind(ParameterMap, NameSampler.c_str());
			}

		WeightsParameter.Bind(ParameterMap, ("LUTWeights"));
		ColorScale.Bind(ParameterMap, ("ColorScale"));
		OverlayColor.Bind(ParameterMap, ("OverlayColor"));
		InverseGamma.Bind(ParameterMap, ("InverseGamma"));

		WhiteTemp.Bind(ParameterMap, ("WhiteTemp"));
		WhiteTint.Bind(ParameterMap, ("WhiteTint"));

		ColorSaturation.Bind(ParameterMap, ("ColorSaturation"));
		ColorContrast.Bind(ParameterMap, ("ColorContrast"));
		ColorGamma.Bind(ParameterMap, ("ColorGamma"));
		ColorGain.Bind(ParameterMap, ("ColorGain"));
		ColorOffset.Bind(ParameterMap, ("ColorOffset"));

		ColorSaturationShadows.Bind(ParameterMap, ("ColorSaturationShadows"));
		ColorContrastShadows.Bind(ParameterMap, ("ColorContrastShadows"));
		ColorGammaShadows.Bind(ParameterMap, ("ColorGammaShadows"));
		ColorGainShadows.Bind(ParameterMap, ("ColorGainShadows"));
		ColorOffsetShadows.Bind(ParameterMap, ("ColorOffsetShadows"));

		ColorSaturationMidtones.Bind(ParameterMap, ("ColorSaturationMidtones"));
		ColorContrastMidtones.Bind(ParameterMap, ("ColorContrastMidtones"));
		ColorGammaMidtones.Bind(ParameterMap, ("ColorGammaMidtones"));
		ColorGainMidtones.Bind(ParameterMap, ("ColorGainMidtones"));
		ColorOffsetMidtones.Bind(ParameterMap, ("ColorOffsetMidtones"));

		ColorSaturationHighlights.Bind(ParameterMap, ("ColorSaturationHighlights"));
		ColorContrastHighlights.Bind(ParameterMap, ("ColorContrastHighlights"));
		ColorGammaHighlights.Bind(ParameterMap, ("ColorGammaHighlights"));
		ColorGainHighlights.Bind(ParameterMap, ("ColorGainHighlights"));
		ColorOffsetHighlights.Bind(ParameterMap, ("ColorOffsetHighlights"));

		ColorCorrectionShadowsMax.Bind(ParameterMap, ("ColorCorrectionShadowsMax"));
		ColorCorrectionHighlightsMin.Bind(ParameterMap, ("ColorCorrectionHighlightsMin"));

		BlueCorrection.Bind(ParameterMap, ("BlueCorrection"));
		ExpandGamut.Bind(ParameterMap, ("ExpandGamut"));

		FilmSlope.Bind(ParameterMap, ("FilmSlope"));
		FilmToe.Bind(ParameterMap, ("FilmToe"));
		FilmShoulder.Bind(ParameterMap, ("FilmShoulder"));
		FilmBlackClip.Bind(ParameterMap, ("FilmBlackClip"));
		FilmWhiteClip.Bind(ParameterMap, ("FilmWhiteClip"));

		OutputDevice.Bind(ParameterMap, ("OutputDevice"));
		OutputGamut.Bind(ParameterMap, ("OutputGamut"));

		ColorMatrixR_ColorCurveCd1.Bind(ParameterMap, ("ColorMatrixR_ColorCurveCd1"));
		ColorMatrixG_ColorCurveCd3Cm3.Bind(ParameterMap, ("ColorMatrixG_ColorCurveCd3Cm3"));
		ColorMatrixB_ColorCurveCm2.Bind(ParameterMap, ("ColorMatrixB_ColorCurveCm2"));
		ColorCurve_Cm0Cd0_Cd2_Ch0Cm1_Ch3.Bind(ParameterMap, ("ColorCurve_Cm0Cd0_Cd2_Ch0Cm1_Ch3"));
		ColorCurve_Ch1_Ch2.Bind(ParameterMap, ("ColorCurve_Ch1_Ch2"));
		ColorShadow_Luma.Bind(ParameterMap, ("ColorShadow_Luma"));
		ColorShadow_Tint1.Bind(ParameterMap, ("ColorShadow_Tint1"));
		ColorShadow_Tint2.Bind(ParameterMap, ("ColorShadow_Tint2"));
	}

	template <typename TRHIShader>
	void Set(TRHIShader const ShaderRHI, const FSceneView& View, ID3D11ShaderResourceView* Textures[BlendCount], float Weights[BlendCount])
	{
		//const FPostProcessSettings& Settings = View.FinalPostProcessSettings;
		const FSceneViewFamily& ViewFamily = *(View.Family);

		for (uint32 i = 0; i < BlendCount; ++i)
		{
			// we don't need to set the neutral one
			if (i != 0)
			{
				// don't use texture asset sampler as it might have anisotropic filtering enabled
				ID3D11SamplerState* Sampler = TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, 0, 1>::GetRHI();

				SetTextureParameter(ShaderRHI, TextureParameter[i], TextureParameterSampler[i], Sampler, Textures[i]/*->TextureRHI*/);
			}

			SetShaderValue(ShaderRHI, WeightsParameter, Weights[i], i);
		}

		SetShaderValue(ShaderRHI, ColorScale, View.ColorScale);
		SetShaderValue(ShaderRHI, OverlayColor, View.OverlayColor);

		ColorRemapShaderParameters.Set(ShaderRHI);

		// White balance
		SetShaderValue(ShaderRHI, WhiteTemp, 6500.f/*Settings.WhiteTemp*/);
		SetShaderValue(ShaderRHI, WhiteTint, 0/*Settings.WhiteTint*/);

		// Color grade
		SetShaderValue(ShaderRHI, ColorSaturation, Vector4(1.0f) /*Settings.ColorSaturation*/);
		SetShaderValue(ShaderRHI, ColorContrast, Vector4(1.0f) /*Settings.ColorContrast*/);
		SetShaderValue(ShaderRHI, ColorGamma, Vector4(1.0f)/*Settings.ColorGamma*/);
		SetShaderValue(ShaderRHI, ColorGain, Vector4(1.0f)/*Settings.ColorGain*/);
		SetShaderValue(ShaderRHI, ColorOffset, Vector4(0.0f)/*Settings.ColorOffset*/);

		SetShaderValue(ShaderRHI, ColorSaturationShadows, Vector4(1.0f)/*Settings.ColorSaturationShadows*/);
		SetShaderValue(ShaderRHI, ColorContrastShadows, Vector4(1.0f)/*Settings.ColorContrastShadows*/);
		SetShaderValue(ShaderRHI, ColorGammaShadows, Vector4(1.0f)/*Settings.ColorGammaShadows*/);
		SetShaderValue(ShaderRHI, ColorGainShadows, Vector4(1.0f)/*Settings.ColorGainShadows*/);
		SetShaderValue(ShaderRHI, ColorOffsetShadows, Vector4(0.0f)/*Settings.ColorOffsetShadows*/);

		SetShaderValue(ShaderRHI, ColorSaturationMidtones, Vector4(1.0f) /*Settings.ColorSaturationMidtones*/);
		SetShaderValue(ShaderRHI, ColorContrastMidtones, Vector4(1.0f)/*Settings.ColorContrastMidtones*/);
		SetShaderValue(ShaderRHI, ColorGammaMidtones, Vector4(1.0f)/* Settings.ColorGammaMidtones*/);
		SetShaderValue(ShaderRHI, ColorGainMidtones, Vector4(1.0f)/*Settings.ColorGainMidtones*/);
		SetShaderValue(ShaderRHI, ColorOffsetMidtones, Vector4(0.0f)/*Settings.ColorOffsetMidtones*/);

		SetShaderValue(ShaderRHI, ColorSaturationHighlights, Vector4(1.0f)/* Settings.ColorSaturationHighlights*/);
		SetShaderValue(ShaderRHI, ColorContrastHighlights, Vector4(1.0f)/*Settings.ColorContrastHighlights*/);
		SetShaderValue(ShaderRHI, ColorGammaHighlights, Vector4(1.0f)/*Settings.ColorGammaHighlights*/);
		SetShaderValue(ShaderRHI, ColorGainHighlights, Vector4(1.0f)/*Settings.ColorGainHighlights*/);
		SetShaderValue(ShaderRHI, ColorOffsetHighlights, Vector4(0.0f)/*Settings.ColorOffsetHighlights*/);

		SetShaderValue(ShaderRHI, ColorCorrectionShadowsMax, 0.9f/* Settings.ColorCorrectionShadowsMax*/);
		SetShaderValue(ShaderRHI, ColorCorrectionHighlightsMin, 0.5f /*Settings.ColorCorrectionHighlightsMin*/);

		SetShaderValue(ShaderRHI, BlueCorrection, 0.6f/*Settings.BlueCorrection*/);
		SetShaderValue(ShaderRHI, ExpandGamut, 1.0f/*Settings.ExpandGamut*/);

		// Film
		SetShaderValue(ShaderRHI, FilmSlope, 0.88f /*Settings.FilmSlope*/);
		SetShaderValue(ShaderRHI, FilmToe, 0.55/* Settings.FilmToe*/);
		SetShaderValue(ShaderRHI, FilmShoulder,0.26 /*Settings.FilmShoulder*/);
		SetShaderValue(ShaderRHI, FilmBlackClip, 0.0/* Settings.FilmBlackClip*/);
		SetShaderValue(ShaderRHI, FilmWhiteClip, 0.04f/*Settings.FilmWhiteClip*/);

		{
			//static TConsoleVariableData<int32>* CVarOutputDevice = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.HDR.Display.OutputDevice"));
			//static TConsoleVariableData<float>* CVarOutputGamma = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.TonemapperGamma"));

			int32 OutputDeviceValue = 0;// CVarOutputDevice->GetValueOnRenderThread();
			float Gamma = 0.f;//CVarOutputGamma->GetValueOnRenderThread();

// 			if (PLATFORM_APPLE && Gamma == 0.0f)
// 			{
// 				Gamma = 2.2f;
// 			}

			if (Gamma > 0.0f)
			{
				// Enforce user-controlled ramp over sRGB or Rec709
				OutputDeviceValue = FMath::Max(OutputDeviceValue, 2);
			}

			SetShaderValue(ShaderRHI, OutputDevice, OutputDeviceValue);

			//static TConsoleVariableData<int32>* CVarOutputGamut = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.HDR.Display.ColorGamut"));
			int32 OutputGamutValue = 0;//CVarOutputGamut->GetValueOnRenderThread();
			SetShaderValue(ShaderRHI, OutputGamut, OutputGamutValue);

			FVector InvDisplayGammaValue;
			InvDisplayGammaValue.X = 1.0f / 2.2f;// ViewFamily.RenderTarget->GetDisplayGamma();
			InvDisplayGammaValue.Y = 2.2f / 2.2f;//  ViewFamily.RenderTarget->GetDisplayGamma();
			InvDisplayGammaValue.Z = 1.0f / FMath::Max(Gamma, 1.0f);
			SetShaderValue(ShaderRHI, InverseGamma, InvDisplayGammaValue);
		}

		{
			// Legacy tone mapper
			// TODO remove

			// Must insure inputs are in correct range (else possible generation of NaNs).
			float InExposure = 1.0f;
			FVector InWhitePoint(1.0f/*Settings.FilmWhitePoint*/);
			float InSaturation = FMath::Clamp(1.0f/*Settings.FilmSaturation*/, 0.0f, 2.0f);
			FVector InLuma = FVector(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f);
			FVector InMatrixR(FLinearColor::Red /*Settings.FilmChannelMixerRed*/);
			FVector InMatrixG(FLinearColor::Green/* Settings.FilmChannelMixerGreen*/);
			FVector InMatrixB(FLinearColor::Blue /*Settings.FilmChannelMixerBlue*/);
			float InContrast = FMath::Clamp(/*Settings.FilmContrast*/0.03f, 0.0f, 1.0f) + 1.0f;
			float InDynamicRange = powf(2.0f, FMath::Clamp(4.0f/*Settings.FilmDynamicRange*/, 1.0f, 4.0f));
			float InToe = (1.0f - FMath::Clamp(1.0f/*Settings.FilmToeAmount*/, 0.0f, 1.0f)) * 0.18f;
			InToe = FMath::Clamp(InToe, 0.18f / 8.0f, 0.18f * (15.0f / 16.0f));
			float InHeal = 1.0f - (FMath::Max(1.0f / 32.0f, 1.0f - FMath::Clamp(1.0f/*Settings.FilmHealAmount*/, 0.0f, 1.0f)) * (1.0f - 0.18f));
			FVector InShadowTint(FLinearColor::White /*Settings.FilmShadowTint*/);
			float InShadowTintBlend = FMath::Clamp(0.5f/* Settings.FilmShadowTintBlend*/, 0.0f, 1.0f) * 64.0f;

			// Shadow tint amount enables turning off shadow tinting.
			float InShadowTintAmount = FMath::Clamp(0.0f/* Settings.FilmShadowTintAmount*/, 0.0f, 1.0f);
			InShadowTint = InWhitePoint + (InShadowTint - InWhitePoint) * InShadowTintAmount;

			// Make sure channel mixer inputs sum to 1 (+ smart dealing with all zeros).
			InMatrixR.X += 1.0f / (256.0f*256.0f*32.0f);
			InMatrixG.Y += 1.0f / (256.0f*256.0f*32.0f);
			InMatrixB.Z += 1.0f / (256.0f*256.0f*32.0f);
			InMatrixR *= 1.0f / FVector::DotProduct(InMatrixR, FVector(1.0f));
			InMatrixG *= 1.0f / FVector::DotProduct(InMatrixG, FVector(1.0f));
			InMatrixB *= 1.0f / FVector::DotProduct(InMatrixB, FVector(1.0f));

			// Conversion from linear rgb to luma (using HDTV coef).
			FVector LumaWeights = FVector(0.2126f, 0.7152f, 0.0722f);

			// Make sure white point has 1.0 as luma (so adjusting white point doesn't change exposure).
			// Make sure {0.0,0.0,0.0} inputs do something sane (default to white).
			InWhitePoint += FVector(1.0f / (256.0f*256.0f*32.0f));
			InWhitePoint *= 1.0f / FVector::DotProduct(InWhitePoint, LumaWeights);
			InShadowTint += FVector(1.0f / (256.0f*256.0f*32.0f));
			InShadowTint *= 1.0f / FVector::DotProduct(InShadowTint, LumaWeights);

			// Grey after color matrix is applied.
			FVector ColorMatrixLuma = FVector(
				FVector::DotProduct(InLuma.X * FVector(InMatrixR.X, InMatrixG.X, InMatrixB.X), FVector(1.0f)),
				FVector::DotProduct(InLuma.Y * FVector(InMatrixR.Y, InMatrixG.Y, InMatrixB.Y), FVector(1.0f)),
				FVector::DotProduct(InLuma.Z * FVector(InMatrixR.Z, InMatrixG.Z, InMatrixB.Z), FVector(1.0f)));

			FVector OutMatrixR = FVector(0.0f);
			FVector OutMatrixG = FVector(0.0f);
			FVector OutMatrixB = FVector(0.0f);
			FVector OutColorShadow_Luma = LumaWeights * InShadowTintBlend;
			FVector OutColorShadow_Tint1 = InWhitePoint;
			FVector OutColorShadow_Tint2 = InShadowTint - InWhitePoint;

			// Final color matrix effected by saturation and exposure.
			OutMatrixR = (ColorMatrixLuma + ((InMatrixR - ColorMatrixLuma) * InSaturation)) * InExposure;
			OutMatrixG = (ColorMatrixLuma + ((InMatrixG - ColorMatrixLuma) * InSaturation)) * InExposure;
			OutMatrixB = (ColorMatrixLuma + ((InMatrixB - ColorMatrixLuma) * InSaturation)) * InExposure;

			// Line for linear section.
			float FilmLineOffset = 0.18f - 0.18f*InContrast;
			float FilmXAtY0 = -FilmLineOffset / InContrast;
			float FilmXAtY1 = (1.0f - FilmLineOffset) / InContrast;
			float FilmXS = FilmXAtY1 - FilmXAtY0;

			// Coordinates of linear section.
			float FilmHiX = FilmXAtY0 + InHeal * FilmXS;
			float FilmHiY = FilmHiX * InContrast + FilmLineOffset;
			float FilmLoX = FilmXAtY0 + InToe * FilmXS;
			float FilmLoY = FilmLoX * InContrast + FilmLineOffset;
			// Supported exposure range before clipping.
			float FilmHeal = InDynamicRange - FilmHiX;
			// Intermediates.
			float FilmMidXS = FilmHiX - FilmLoX;
			float FilmMidYS = FilmHiY - FilmLoY;
			float FilmSlopeS = FilmMidYS / (FilmMidXS);
			float FilmHiYS = 1.0f - FilmHiY;
			float FilmLoYS = FilmLoY;
			float FilmToeVal = FilmLoX;
			float FilmHiG = (-FilmHiYS + (FilmSlopeS*FilmHeal)) / (FilmSlopeS*FilmHeal);
			float FilmLoG = (-FilmLoYS + (FilmSlopeS*FilmToeVal)) / (FilmSlopeS*FilmToeVal);

			// Constants.
			float OutColorCurveCh1 = FilmHiYS / FilmHiG;
			float OutColorCurveCh2 = -FilmHiX * (FilmHiYS / FilmHiG);
			float OutColorCurveCh3 = FilmHiYS / (FilmSlopeS*FilmHiG) - FilmHiX;
			float OutColorCurveCh0Cm1 = FilmHiX;
			float OutColorCurveCm2 = FilmSlopeS;
			float OutColorCurveCm0Cd0 = FilmLoX;
			float OutColorCurveCd3Cm3 = FilmLoY - FilmLoX * FilmSlopeS;
			float OutColorCurveCd1 = 0.0f;
			float OutColorCurveCd2 = 1.0f;
			// Handle these separate in case of FilmLoG being 0.
			if (FilmLoG != 0.0f)
			{
				OutColorCurveCd1 = -FilmLoYS / FilmLoG;
				OutColorCurveCd2 = FilmLoYS / (FilmSlopeS*FilmLoG);
			}
			else
			{
				// FilmLoG being zero means dark region is a linear segment (so just continue the middle section).
				OutColorCurveCm0Cd0 = 0.0f;
				OutColorCurveCd3Cm3 = 0.0f;
			}

			Vector4 Constants[8];
			Constants[0] = Vector4(OutMatrixR, OutColorCurveCd1);
			Constants[1] = Vector4(OutMatrixG, OutColorCurveCd3Cm3);
			Constants[2] = Vector4(OutMatrixB, OutColorCurveCm2);
			Constants[3] = Vector4(OutColorCurveCm0Cd0, OutColorCurveCd2, OutColorCurveCh0Cm1, OutColorCurveCh3);
			Constants[4] = Vector4(OutColorCurveCh1, OutColorCurveCh2, 0.0f, 0.0f);
			Constants[5] = Vector4(OutColorShadow_Luma, 0.0f);
			Constants[6] = Vector4(OutColorShadow_Tint1, 0.0f);
			Constants[7] = Vector4(OutColorShadow_Tint2, 1.0f/*CVarTonemapperFilm.GetValueOnRenderThread()*/);

			SetShaderValue(ShaderRHI, ColorMatrixR_ColorCurveCd1, Constants[0]);
			SetShaderValue(ShaderRHI, ColorMatrixG_ColorCurveCd3Cm3, Constants[1]);
			SetShaderValue(ShaderRHI, ColorMatrixB_ColorCurveCm2, Constants[2]);
			SetShaderValue(ShaderRHI, ColorCurve_Cm0Cd0_Cd2_Ch0Cm1_Ch3, Constants[3]);
			SetShaderValue(ShaderRHI, ColorCurve_Ch1_Ch2, Constants[4]);
			SetShaderValue(ShaderRHI, ColorShadow_Luma, Constants[5]);
			SetShaderValue(ShaderRHI, ColorShadow_Tint1, Constants[6]);
			SetShaderValue(ShaderRHI, ColorShadow_Tint2, Constants[7]);
		}
	}

	// [0] is not used as it's the neutral one we do in the shader
	FShaderResourceParameter TextureParameter[GMaxLUTBlendCount];
	FShaderResourceParameter TextureParameterSampler[GMaxLUTBlendCount];
	FShaderParameter WeightsParameter;
	FShaderParameter ColorScale;
	FShaderParameter OverlayColor;
	FShaderParameter InverseGamma;
	FColorRemapShaderParameters ColorRemapShaderParameters;

	FShaderParameter WhiteTemp;
	FShaderParameter WhiteTint;

	FShaderParameter ColorSaturation;
	FShaderParameter ColorContrast;
	FShaderParameter ColorGamma;
	FShaderParameter ColorGain;
	FShaderParameter ColorOffset;

	FShaderParameter ColorSaturationShadows;
	FShaderParameter ColorContrastShadows;
	FShaderParameter ColorGammaShadows;
	FShaderParameter ColorGainShadows;
	FShaderParameter ColorOffsetShadows;

	FShaderParameter ColorSaturationMidtones;
	FShaderParameter ColorContrastMidtones;
	FShaderParameter ColorGammaMidtones;
	FShaderParameter ColorGainMidtones;
	FShaderParameter ColorOffsetMidtones;

	FShaderParameter ColorSaturationHighlights;
	FShaderParameter ColorContrastHighlights;
	FShaderParameter ColorGammaHighlights;
	FShaderParameter ColorGainHighlights;
	FShaderParameter ColorOffsetHighlights;

	FShaderParameter ColorCorrectionShadowsMax;
	FShaderParameter ColorCorrectionHighlightsMin;

	FShaderParameter BlueCorrection;
	FShaderParameter ExpandGamut;

	FShaderParameter FilmSlope;
	FShaderParameter FilmToe;
	FShaderParameter FilmShoulder;
	FShaderParameter FilmBlackClip;
	FShaderParameter FilmWhiteClip;

	FShaderParameter OutputDevice;
	FShaderParameter OutputGamut;

	// Legacy
	FShaderParameter ColorMatrixR_ColorCurveCd1;
	FShaderParameter ColorMatrixG_ColorCurveCd3Cm3;
	FShaderParameter ColorMatrixB_ColorCurveCm2;
	FShaderParameter ColorCurve_Cm0Cd0_Cd2_Ch0Cm1_Ch3;
	FShaderParameter ColorCurve_Ch1_Ch2;
	FShaderParameter ColorShadow_Luma;
	FShaderParameter ColorShadow_Tint1;
	FShaderParameter ColorShadow_Tint2;
};

FColorRemapShaderParameters::FColorRemapShaderParameters(const FShaderParameterMap& ParameterMap)
{
	MappingPolynomial.Bind(ParameterMap, ("MappingPolynomial"));
}

void FColorRemapShaderParameters::Set(ID3D11PixelShader* const ShaderRHI)
{
	FColorTransform ColorTransform;
	ColorTransform.MinValue = FMath::Clamp(0.f/* CVarColorMin.GetValueOnRenderThread()*/, -10.0f, 10.0f);
	ColorTransform.MidValue = FMath::Clamp(0.5f/*CVarColorMid.GetValueOnRenderThread()*/, -10.0f, 10.0f);
	ColorTransform.MaxValue = FMath::Clamp(1.f/*CVarColorMax.GetValueOnRenderThread()*/, -10.0f, 10.0f);

	{
		// x is the input value, y the output value
		// RGB = a, b, c where y = a * x*x + b * x + c

		float c = ColorTransform.MinValue;
		float b = 4 * ColorTransform.MidValue - 3 * ColorTransform.MinValue - ColorTransform.MaxValue;
		float a = ColorTransform.MaxValue - ColorTransform.MinValue - b;

		SetShaderValue(ShaderRHI, MappingPolynomial, FVector(a, b, c));
	}
}

template<uint32 BlendCount>
class FLUTBlenderPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FLUTBlenderPS, Global);
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	FLUTBlenderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
		, CombineLUTsShaderParameters(Initializer.ParameterMap)
	{
	}

	void SetParameters(const FSceneView& View, ID3D11ShaderResourceView* Textures[BlendCount], float Weights[BlendCount])
	{
		ID3D11PixelShader* const ShaderRHI = GetPixelShader();
		CombineLUTsShaderParameters.Set(ShaderRHI, View, Textures, Weights);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(("BLENDCOUNT"), BlendCount);
		OutEnvironment.SetDefine(("USE_VOLUME_LUT"), true /*UseVolumeTextureLUT(Parameters.Platform)*/);
	}

private: // ---------------------------------------------------
	FLUTBlenderPS() {}

	FCombineLUTsShaderParameters<BlendCount> CombineLUTsShaderParameters;
};

IMPLEMENT_SHADER_TYPE(template<>, FLUTBlenderPS<1>, ("PostProcessCombineLUTs.dusf"), ("MainPS"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>, FLUTBlenderPS<2>, ("PostProcessCombineLUTs.dusf"), ("MainPS"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>, FLUTBlenderPS<3>, ("PostProcessCombineLUTs.dusf"), ("MainPS"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>, FLUTBlenderPS<4>, ("PostProcessCombineLUTs.dusf"), ("MainPS"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>, FLUTBlenderPS<5>, ("PostProcessCombineLUTs.dusf"), ("MainPS"), SF_Pixel);

static void SetLUTBlenderShader(FRenderingCompositePassContext& Context, uint32 BlendCount, ID3D11ShaderResourceView* Texture[], float Weights[], const FVolumeBounds& VolumeBounds)
{
// 	FGraphicsPipelineStateInitializer GraphicsPSOInit;
// 	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
// 	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
// 	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
// 	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
// 	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	ID3D11BlendState* BlendState = TStaticBlendState<>::GetRHI();
	ID3D11RasterizerState* RasterizerState = TStaticRasterizerState<>::GetRHI();
	ID3D11DepthStencilState* DepthStencilState = TStaticDepthStencilState<false, D3D11_COMPARISON_ALWAYS>::GetRHI();

	D3D11DeviceContext->OMSetBlendState(BlendState, nullptr, 0xffffffff);
	D3D11DeviceContext->RSSetState(RasterizerState);
	D3D11DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);
	D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	assert(BlendCount > 0);

	FShader* LocalPixelShader = 0;
	const FViewInfo& View = Context.View;

	auto ShaderMap = Context.GetShaderMap();

	// A macro to handle setting the filter shader for a specific number of samples.
#define CASE_COUNT(BlendCount) \
	case BlendCount: \
	{ \
		TShaderMapRef<FLUTBlenderPS<BlendCount> > PixelShader(ShaderMap); \
		LocalPixelShader = *PixelShader;\
	}; \
	break;

	switch (BlendCount)
	{
		// starts at 1 as we always have at least the neutral one
		CASE_COUNT(1);
		CASE_COUNT(2);
		CASE_COUNT(3);
		CASE_COUNT(4);
		CASE_COUNT(5);
		//	default:
		//		UE_LOG(LogRenderer, Fatal,TEXT("Invalid number of samples: %u"),BlendCount);
	}
#undef CASE_COUNT
	//if (/*UseVolumeTextureLUT(Context.View.GetShaderPlatform())*/)
	{
		TShaderMapRef<FWriteToSliceVS> VertexShader(ShaderMap);
		TOptionalShaderMapRef<FWriteToSliceGS> GeometryShader(ShaderMap);

// 		GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;
// 		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GScreenVertexDeclaration.VertexDeclarationRHI;
// 		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
// 		GraphicsPSOInit.BoundShaderState.GeometryShaderRHI = GETSAFERHISHADER_GEOMETRY(*GeometryShader);
// 		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(LocalPixelShader);
// 		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

		D3D11DeviceContext->IASetInputLayout(GetInputLayout(GetScreenVertexDeclaration().get(),VertexShader->GetCode().Get()));
		D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
		D3D11DeviceContext->GSSetShader(GeometryShader->GetGeometryShader(), 0, 0);
		D3D11DeviceContext->PSSetShader(LocalPixelShader->GetPixelShader(),0,0);
		D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		VertexShader->SetParameters(VolumeBounds, FIntVector(VolumeBounds.MaxX - VolumeBounds.MinX));
		if (GeometryShader.IsValid())
		{
			GeometryShader->SetParameters(VolumeBounds.MinZ);
		}
	}
// 	else
// 	{
// 		TShaderMapRef<FPostProcessVS> VertexShader(ShaderMap);
// 
// 		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
// 		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
// 		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(LocalPixelShader);
// 		SetGraphicsPipelineState(GraphicsPSOInit);
// 
// 		VertexShader->SetParameters(Context);
// 	}
#define CASE_COUNT(BlendCount) \
	case BlendCount: \
	{ \
	TShaderMapRef<FLUTBlenderPS<BlendCount> > PixelShader(ShaderMap); \
	PixelShader->SetParameters(View, Texture, Weights); \
	}; \
	break;

	switch (BlendCount)
	{
		// starts at 1 as we always have at least the neutral one
		CASE_COUNT(1);
		CASE_COUNT(2);
		CASE_COUNT(3);
		CASE_COUNT(4);
		CASE_COUNT(5);
		//	default:
		//		UE_LOG(LogRenderer, Fatal,TEXT("Invalid number of samples: %u"),BlendCount);
	}
#undef CASE_COUNT
}



void FRCPassPostProcessCombineLUTs::Process(FRenderingCompositePassContext& Context)
{
	ID3D11ShaderResourceView* LocalTextures[GMaxLUTBlendCount];
	float LocalWeights[GMaxLUTBlendCount];

	const FViewInfo& View = Context.View;
	const FSceneViewFamily& ViewFamily = *(View.Family);

	uint32 LocalCount = 1;

	// set defaults for no LUT
	LocalTextures[0] = 0;
	LocalWeights[0] = 1.0f;

	//if (ViewFamily.EngineShowFlags.ColorGrading)
	{
		//LocalCount = GenerateFinalTable(/*Context.View.FinalPostProcessSettings, LocalTextures, */LocalWeights, GMaxLUTBlendCount);
	}

	SCOPED_DRAW_EVENT_FORMAT(PostProcessCombineLUTs, TEXT("PostProcessCombineLUTs%s [%d] %dx%dx%d"),
		bIsComputePass ? TEXT("Compute") : TEXT(""), LocalCount, GLUTSize, GLUTSize, GLUTSize);


	const bool bUseVolumeTextureLUT = true;// UseVolumeTextureLUT(ShaderPlatform);
	// for a 3D texture, the viewport is 16x16 (per slice), for a 2D texture, it's unwrapped to 256x16
	FIntPoint DestSize(bUseVolumeTextureLUT ? GLUTSize : GLUTSize * GLUTSize, GLUTSize);

	const PooledRenderTarget* DestRenderTarget = !bAllocateOutput ?
		Context.View.GetTonemappingLUTRenderTarget(GLUTSize, bUseVolumeTextureLUT, bIsComputePass) :
		&PassOutputs[0].RequestSurface(Context);

	assert(DestRenderTarget);
	//static auto* RenderPassCVar = 0;// IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.RHIRenderPasses"));

	{
		// Set the view family's render target/viewport.
// 		if (IsMobilePlatform(ShaderPlatform))
// 		{
// 			// Full clear to avoid restore
// 			SetRenderTarget(Context.RHICmdList, DestRenderTarget->TargetableTexture, FTextureRHIRef(), ESimpleRenderTargetMode::EClearColorAndDepth);
// 		}
//		else
		{
			SetRenderTarget(DestRenderTarget->TargetableTexture.get(), nullptr, false,false,false);
		}

		Context.SetViewportAndCallRHI(0, 0, 0.0f, DestSize.X, DestSize.Y, 1.0f);

		const FVolumeBounds VolumeBounds(GLUTSize);

		SetLUTBlenderShader(Context, LocalCount, LocalTextures, LocalWeights, VolumeBounds);

		if (bUseVolumeTextureLUT)
		{
			// use volume texture 16x16x16
			RasterizeToVolumeTexture(VolumeBounds);
		}
		else
		{
			// use unwrapped 2d texture 256x16
// 			TShaderMapRef<FPostProcessVS> VertexShader(Context.GetShaderMap());
// 
// 			DrawRectangle(
// 				Context.RHICmdList,
// 				0, 0,										// XY
// 				GLUTSize * GLUTSize, GLUTSize,				// SizeXY
// 				0, 0,										// UV
// 				GLUTSize * GLUTSize, GLUTSize,				// SizeUV
// 				FIntPoint(GLUTSize * GLUTSize, GLUTSize),	// TargetSize
// 				FIntPoint(GLUTSize * GLUTSize, GLUTSize),	// TextureSize
// 				*VertexShader,
// 				EDRF_UseTriangleOptimization);
		}

		RHICopyToResolveTarget(DestRenderTarget->TargetableTexture.get(), DestRenderTarget->ShaderResourceTexture.get(), FResolveParams());
	}

	Context.View.SetValidTonemappingLUT();
}

PooledRenderTargetDesc FRCPassPostProcessCombineLUTs::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	// Specify invalid description to avoid the creation of an intermediate rendertargets.
	// We want to use ViewState->GetTonemappingLUTRT instead.
	PooledRenderTargetDesc Ret;
	Ret.TargetableFlags &= ~(TexCreate_RenderTargetable | TexCreate_UAV);
	Ret.TargetableFlags |= bIsComputePass ? TexCreate_UAV : TexCreate_RenderTargetable;

	if (!bAllocateOutput)
	{
		Ret.DebugName = TEXT("DummyLUT");
	}
	else
	{
		EPixelFormat LUTPixelFormat = PF_A2B10G10R10;
		if (!GPixelFormats[LUTPixelFormat].Supported)
		{
			LUTPixelFormat = PF_R8G8B8A8;
		}

		Ret = PooledRenderTargetDesc::Create2DDesc(FIntPoint(GLUTSize * GLUTSize, GLUTSize), LUTPixelFormat, FClearValueBinding::Transparent, TexCreate_None, TexCreate_RenderTargetable | TexCreate_ShaderResource, false);

		if (true/*UseVolumeTextureLUT(ShaderPlatform)*/)
		{
			Ret.Extent = FIntPoint(GLUTSize, GLUTSize);
			Ret.Depth = GLUTSize;
		}
		//Ret.Flags |= GFastVRamConfig.CombineLUTs;
		Ret.DebugName = TEXT("CombineLUTs");
	}
	Ret.ClearValue = FClearValueBinding::Transparent;

	return Ret;
}

