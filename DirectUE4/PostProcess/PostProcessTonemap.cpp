#include "PostProcessTonemap.h"
#include "Scene.h"
#include "SceneFilterRendering.h"
#include "PostProcessCombineLUTs.h"
#include "SceneFilterRendering.h"
#include "GPUProfiler.h"
#include "Viewport.h"

// Note: These values are directly referenced in code, please update all paths if changing
enum class FTonemapperOutputDevice
{
	sRGB,
	Rec709,
	ExplicitGammaMapping,
	ACES1000nitST2084,
	ACES2000nitST2084,
	ACES1000nitScRGB,
	ACES2000nitScRGB,
	LinearEXR,

	MAX
};

const int32 GTonemapComputeTileSizeX = 8;
const int32 GTonemapComputeTileSizeY = 8;

namespace
{

	namespace TonemapperPermutation
	{

		// Shared permutation dimensions between deferred and mobile renderer.
		class FTonemapperBloomDim : SHADER_PERMUTATION_BOOL("USE_BLOOM");
		class FTonemapperGammaOnlyDim : SHADER_PERMUTATION_BOOL("USE_GAMMA_ONLY");
		class FTonemapperGrainIntensityDim : SHADER_PERMUTATION_BOOL("USE_GRAIN_INTENSITY");
		class FTonemapperVignetteDim : SHADER_PERMUTATION_BOOL("USE_VIGNETTE");
		class FTonemapperSharpenDim : SHADER_PERMUTATION_BOOL("USE_SHARPEN");
		class FTonemapperGrainJitterDim : SHADER_PERMUTATION_BOOL("USE_GRAIN_JITTER");

		using FCommonDomain = TShaderPermutationDomain<
			FTonemapperBloomDim,
			FTonemapperGammaOnlyDim,
			FTonemapperGrainIntensityDim,
			FTonemapperVignetteDim,
			FTonemapperSharpenDim,
			FTonemapperGrainJitterDim>;

		inline bool ShouldCompileCommonPermutation(const FCommonDomain& PermutationVector)
		{
			// If GammaOnly, don't compile any other dimmension == true.
			if (PermutationVector.Get<FTonemapperGammaOnlyDim>())
			{
				return !PermutationVector.Get<FTonemapperBloomDim>() &&
					!PermutationVector.Get<FTonemapperGrainIntensityDim>() &&
					!PermutationVector.Get<FTonemapperVignetteDim>() &&
					!PermutationVector.Get<FTonemapperSharpenDim>() &&
					!PermutationVector.Get<FTonemapperGrainJitterDim>();
			}
			return true;
		}
		// Common conversion of engine settings into.
		FCommonDomain BuildCommonPermutationDomain(const FViewInfo& View, bool bGammaOnly)
		{
			const FSceneViewFamily* Family = View.Family;

			FCommonDomain PermutationVector;

// 			// Gamma
// 			if (bGammaOnly ||
// 				(Family->EngineShowFlags.Tonemapper == 0) ||
// 				(Family->EngineShowFlags.PostProcessing == 0))
// 			{
// 				PermutationVector.Set<FTonemapperGammaOnlyDim>(true);
// 				return PermutationVector;
// 			}

			//const FPostProcessSettings& Settings = View.FinalPostProcessSettings;
			PermutationVector.Set<FTonemapperGrainIntensityDim>(/*Settings.GrainIntensity*/0.0f > 0.0f);
			PermutationVector.Set<FTonemapperVignetteDim>(/*Settings.VignetteIntensity*/0.4f > 0.0f);
			PermutationVector.Set<FTonemapperBloomDim>(/*Settings.BloomIntensity*//*0.675f > 0.0*/false);
			PermutationVector.Set<FTonemapperGrainJitterDim>(/*Settings.GrainJitter*/0.f > 0.0f);
			PermutationVector.Set<FTonemapperSharpenDim>(/*CVarTonemapperSharpen.GetValueOnRenderThread()*/0.0f > 0.0f);

			return PermutationVector;
		}


		// Desktop renderer permutation dimensions.
		class FTonemapperColorFringeDim : SHADER_PERMUTATION_BOOL("USE_COLOR_FRINGE");
		class FTonemapperGrainQuantizationDim : SHADER_PERMUTATION_BOOL("USE_GRAIN_QUANTIZATION");
		class FTonemapperOutputDeviceDim : SHADER_PERMUTATION_ENUM_CLASS("DIM_OUTPUT_DEVICE", FTonemapperOutputDevice);

		using FDesktopDomain = TShaderPermutationDomain<
			FCommonDomain,
			FTonemapperColorFringeDim,
			FTonemapperGrainQuantizationDim,
			FTonemapperOutputDeviceDim>;

		FDesktopDomain RemapPermutation(FDesktopDomain PermutationVector)
		{
			FCommonDomain CommonPermutationVector = PermutationVector.Get<FCommonDomain>();

			// No remapping if gamma only.
			if (CommonPermutationVector.Get<FTonemapperGammaOnlyDim>())
			{
				return PermutationVector;
			}

			// Grain jitter or intensity looks bad anyway.
			bool bFallbackToSlowest = false;
			bFallbackToSlowest = bFallbackToSlowest || CommonPermutationVector.Get<FTonemapperGrainIntensityDim>();
			bFallbackToSlowest = bFallbackToSlowest || CommonPermutationVector.Get<FTonemapperGrainJitterDim>();

			if (bFallbackToSlowest)
			{
				CommonPermutationVector.Set<FTonemapperGrainIntensityDim>(true);
				CommonPermutationVector.Set<FTonemapperGrainJitterDim>(true);
				CommonPermutationVector.Set<FTonemapperSharpenDim>(true);

				PermutationVector.Set<FTonemapperColorFringeDim>(true);
			}

			// You most likely need Bloom anyway.
			CommonPermutationVector.Set<FTonemapperBloomDim>(true);

			// Grain quantization is pretty important anyway.
			PermutationVector.Set<FTonemapperGrainQuantizationDim>(true);

			PermutationVector.Set<FCommonDomain>(CommonPermutationVector);
			return PermutationVector;
		}

		bool ShouldCompileDesktopPermutation(FDesktopDomain PermutationVector)
		{
			auto CommonPermutationVector = PermutationVector.Get<FCommonDomain>();

			if (RemapPermutation(PermutationVector) != PermutationVector)
			{
				return false;
			}

			if (!ShouldCompileCommonPermutation(CommonPermutationVector))
			{
				return false;
			}

			if (CommonPermutationVector.Get<FTonemapperGammaOnlyDim>())
			{
				return !PermutationVector.Get<FTonemapperColorFringeDim>() &&
					!PermutationVector.Get<FTonemapperGrainQuantizationDim>();
			}

			return true;
		}


	} // namespace TonemapperPermutation


} // namespace


static FTonemapperOutputDevice GetOutputDeviceValue()
{
	int32 OutputDeviceValue = 0; //CVarDisplayOutputDevice.GetValueOnRenderThread();
	float Gamma = 0.0f;// CVarTonemapperGamma.GetValueOnRenderThread();

// 	if (PLATFORM_APPLE && Gamma == 0.0f)
// 	{
// 		Gamma = 2.2f;
// 	}

	if (Gamma > 0.0f)
	{
		// Enforce user-controlled ramp over sRGB or Rec709
		OutputDeviceValue = FMath::Max(OutputDeviceValue, 2);
	}
	return FTonemapperOutputDevice(FMath::Clamp(OutputDeviceValue, 0, int32(FTonemapperOutputDevice::MAX) - 1));
}

void GrainPostSettings(FVector* __restrict const Constant/*, const FPostProcessSettings* __restrict const Settings*/)
{
	float GrainJitter = 0.f;// Settings->GrainJitter;
	float GrainIntensity = 0.f;//Settings->GrainIntensity;
	Constant->X = GrainIntensity;
	Constant->Y = 1.0f + (-0.5f * GrainIntensity);
	Constant->Z = GrainJitter;
}


struct alignas(16) FBloomDirtMaskParameters
{
	FBloomDirtMaskParameters()
	{
		ConstructUniformBufferInfo(*this);
	}
	struct ConstantStruct
	{
		Vector4 Tint;
	} Constants;

	ID3D11ShaderResourceView* Mask;
	ID3D11SamplerState* MaskSampler;

	static std::string GetConstantBufferName()
	{
		return "BloomDirtMask";
	}
#define ADD_RES(StructName, MemberName) List.insert(std::make_pair(std::string(#StructName) + "_" + std::string(#MemberName),StructName.MemberName))
	static std::map<std::string, ID3D11ShaderResourceView*> GetSRVs(const FBloomDirtMaskParameters& BloomDirtMask)
	{
		std::map<std::string, ID3D11ShaderResourceView*> List;
		ADD_RES(BloomDirtMask, Mask);
		return List;
	}
	static std::map<std::string, ID3D11SamplerState*> GetSamplers(const FBloomDirtMaskParameters& BloomDirtMask)
	{
		std::map<std::string, ID3D11SamplerState*> List;
		ADD_RES(BloomDirtMask, MaskSampler);
		return List;
	}
	static std::map<std::string, ID3D11UnorderedAccessView*> GetUAVs(const FBloomDirtMaskParameters& BloomDirtMask)
	{
		std::map<std::string, ID3D11UnorderedAccessView*> List;
		return List;
	}
#undef ADD_RES
};

FBloomDirtMaskParameters BloomDirtMask;

class FPostProcessTonemapShaderParameters
{
public:
	FPostProcessTonemapShaderParameters() {}

	FPostProcessTonemapShaderParameters(const FShaderParameterMap& ParameterMap)
	{
		ColorScale0.Bind(ParameterMap, ("ColorScale0"));
		ColorScale1.Bind(ParameterMap, ("ColorScale1"));
		NoiseTexture.Bind(ParameterMap, ("NoiseTexture"));
		NoiseTextureSampler.Bind(ParameterMap, ("NoiseTextureSampler"));
		TonemapperParams.Bind(ParameterMap, ("TonemapperParams"));
		GrainScaleBiasJitter.Bind(ParameterMap, ("GrainScaleBiasJitter"));
		ColorGradingLUT.Bind(ParameterMap, ("ColorGradingLUT"));
		ColorGradingLUTSampler.Bind(ParameterMap, ("ColorGradingLUTSampler"));
		InverseGamma.Bind(ParameterMap, ("InverseGamma"));
		ChromaticAberrationParams.Bind(ParameterMap, ("ChromaticAberrationParams"));
		ScreenPosToScenePixel.Bind(ParameterMap, ("ScreenPosToScenePixel"));
		SceneUVMinMax.Bind(ParameterMap, ("SceneUVMinMax"));
		SceneBloomUVMinMax.Bind(ParameterMap, ("SceneBloomUVMinMax"));
	}

	template <typename TRHIShader>
	void Set(const TRHIShader ShaderRHI, const FRenderingCompositePassContext& Context, const TShaderUniformBufferParameter<FBloomDirtMaskParameters>& BloomDirtMaskParam)
	{
		//const FPostProcessSettings& Settings = Context.View.FinalPostProcessSettings;
		const FSceneViewFamily& ViewFamily = *(Context.View.Family);

		{
			FLinearColor Col = FLinearColor::White;// Settings.SceneColorTint;
			Vector4 ColorScale(Col.R, Col.G, Col.B, 0);
			SetShaderValue(ShaderRHI, ColorScale0, ColorScale);
		}

		{
			FLinearColor Col = FLinearColor::White * 0.675f;// Settings.BloomIntensity;
			Vector4 ColorScale(Col.R, Col.G, Col.B, 0);
			SetShaderValue(ShaderRHI, ColorScale1, ColorScale);
		}

		{
			//UTexture2D* NoiseTextureValue = GEngine->HighFrequencyNoiseTexture;
			ID3D11ShaderResourceView* NoiseTextureValue = HighFrequencyNoiseTexture;
			SetTextureParameter(ShaderRHI, NoiseTexture, NoiseTextureSampler, TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::GetRHI(), NoiseTextureValue);
		}

		{
			float Sharpen = FMath::Clamp(/*CVarTonemapperSharpen.GetValueOnRenderThread()*/0.f, 0.0f, 10.0f);

			// /6.0 is to save one shader instruction
			Vector2 Value(/*Settings.VignetteIntensity*/0.4f, Sharpen / 6.0f);

			SetShaderValue(ShaderRHI, TonemapperParams, Value);
		}

		FVector GrainValue;
		GrainPostSettings(&GrainValue/*, &Settings*/);
		SetShaderValue(ShaderRHI, GrainScaleBiasJitter, GrainValue);

		if (BloomDirtMaskParam.IsBound())
		{
			FBloomDirtMaskParameters BloomDirtMaskParams;

			FLinearColor Col = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f) * 0.0f;//  Settings.BloomDirtMaskTint * Settings.BloomDirtMaskIntensity;
			BloomDirtMaskParams.Constants.Tint = Vector4(Col.R, Col.G, Col.B, 0.f /*unused*/);

			BloomDirtMaskParams.Mask = GSystemTextures.BlackDummy->TargetableTexture->GetShaderResourceView();
// 			if (Settings.BloomDirtMask && Settings.BloomDirtMask->Resource)
// 			{
// 				BloomDirtMaskParams.Mask = Settings.BloomDirtMask->Resource->TextureRHI;
// 			}
			BloomDirtMaskParams.MaskSampler = TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI();

			std::shared_ptr<FUniformBuffer> BloomDirtMaskUB = TUniformBufferPtr<FBloomDirtMaskParameters>::CreateUniformBufferImmediate(BloomDirtMaskParams);
			SetUniformBufferParameter(ShaderRHI, BloomDirtMaskParam, BloomDirtMaskUB.get());
		}

		{
			FRenderingCompositeOutputRef* OutputRef = Context.Pass->GetInput(ePId_Input3);

			const std::shared_ptr<FD3D11Texture2D>* SrcTexture = Context.View.GetTonemappingLUTTexture();
			bool bShowErrorLog = false;
			// Use a provided tonemaping LUT (provided by a CombineLUTs pass). 
			if (!SrcTexture)
			{
				if (OutputRef && OutputRef->IsValid())
				{
					FRenderingCompositeOutput* Input = OutputRef->GetOutput();

					if (Input)
					{
						ComPtr<PooledRenderTarget> InputPooledElement = Input->RequestInput();

						if (InputPooledElement)
						{
							assert(!InputPooledElement->IsFree());

							SrcTexture = &InputPooledElement->ShaderResourceTexture;
						}
					}

					// Indicates the Tonemapper combined LUT node was nominally in the network, so error if it's not found
					bShowErrorLog = true;
				}
			}

			if (SrcTexture && *SrcTexture)
			{
				SetTextureParameter(ShaderRHI, ColorGradingLUT, ColorGradingLUTSampler, TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::GetRHI(), (*SrcTexture)->GetShaderResourceView());
			}
			else if (bShowErrorLog)
			{
				//UE_LOG(LogRenderer, Error, TEXT("No Color LUT texture to sample: output will be invalid."));
			}
		}

		{
			FVector InvDisplayGammaValue;
			InvDisplayGammaValue.X = 1.0f / ViewFamily.RenderTarget->GetDisplayGamma();
			InvDisplayGammaValue.Y = 2.2f / ViewFamily.RenderTarget->GetDisplayGamma();
			{
				//static TConsoleVariableData<float>* CVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.TonemapperGamma"));
				float Value = 0.f;// CVar->GetValueOnRenderThread();
				if (Value < 1.0f)
				{
					Value = 1.0f;
				}
				InvDisplayGammaValue.Z = 1.0f / Value;
			}
			SetShaderValue(ShaderRHI, InverseGamma, InvDisplayGammaValue);
		}

		{
			// for scene color fringe
			// from percent to fraction
			float Offset = 0.f;
			float StartOffset = 0.f;
			float Multiplier = 1.f;

			if (/*Context.View.FinalPostProcessSettings.ChromaticAberrationStartOffset*/0.f < 1.f - KINDA_SMALL_NUMBER)
			{
				Offset = 0; //Context.View.FinalPostProcessSettings.SceneFringeIntensity * 0.01f;
				StartOffset = 0;// Context.View.FinalPostProcessSettings.ChromaticAberrationStartOffset;
				Multiplier = 1.f / (1.f - StartOffset);
			}

			// Wavelength of primaries in nm
			const float PrimaryR = 611.3f;
			const float PrimaryG = 549.1f;
			const float PrimaryB = 464.3f;

			// Simple lens chromatic aberration is roughly linear in wavelength
			float ScaleR = 0.007f * (PrimaryR - PrimaryB);
			float ScaleG = 0.007f * (PrimaryG - PrimaryB);
			Vector4 Value(Offset * ScaleR * Multiplier, Offset * ScaleG * Multiplier, StartOffset, 0.f);

			// we only get bigger to not leak in content from outside
			SetShaderValue(ShaderRHI, ChromaticAberrationParams, Value);
		}

		{
			float InvBufferSizeX = 1.f / float(Context.ReferenceBufferSize.X);
			float InvBufferSizeY = 1.f / float(Context.ReferenceBufferSize.Y);
			Vector4 SceneUVMinMaxValue(
				(Context.SceneColorViewRect.Min.X + 0.5f) * InvBufferSizeX,
				(Context.SceneColorViewRect.Min.Y + 0.5f) * InvBufferSizeY,
				(Context.SceneColorViewRect.Max.X - 0.5f) * InvBufferSizeX,
				(Context.SceneColorViewRect.Max.Y - 0.5f) * InvBufferSizeY);
			SetShaderValue(ShaderRHI, SceneUVMinMax, SceneUVMinMaxValue);
		}

		{
			float InvBufferSizeX = 1.f / float(Context.ReferenceBufferSize.X);
			float InvBufferSizeY = 1.f / float(Context.ReferenceBufferSize.Y);
			Vector4 SceneBloomUVMinMaxValue(
				(Context.SceneColorViewRect.Min.X + 1.5) * InvBufferSizeX,
				(Context.SceneColorViewRect.Min.Y + 1.5f) * InvBufferSizeY,
				(Context.SceneColorViewRect.Max.X - 1.5f) * InvBufferSizeX,
				(Context.SceneColorViewRect.Max.Y - 1.5f) * InvBufferSizeY);
			SetShaderValue(ShaderRHI, SceneBloomUVMinMax, SceneBloomUVMinMaxValue);
		}

		{
			FIntPoint ViewportOffset = Context.SceneColorViewRect.Min;
			FIntPoint ViewportExtent = Context.SceneColorViewRect.Size();
			Vector4 ScreenPosToScenePixelValue(
				ViewportExtent.X * 0.5f,
				-ViewportExtent.Y * 0.5f,
				ViewportExtent.X * 0.5f - 0.5f + ViewportOffset.X,
				ViewportExtent.Y * 0.5f - 0.5f + ViewportOffset.Y);
			SetShaderValue(ShaderRHI, ScreenPosToScenePixel, ScreenPosToScenePixelValue);
		}
	}


	FShaderParameter ColorScale0;
	FShaderParameter ColorScale1;
	FShaderResourceParameter NoiseTexture;
	FShaderResourceParameter NoiseTextureSampler;
	FShaderParameter TonemapperParams;
	FShaderParameter GrainScaleBiasJitter;
	FShaderResourceParameter ColorGradingLUT;
	FShaderResourceParameter ColorGradingLUTSampler;
	FShaderParameter InverseGamma;
	FShaderParameter ChromaticAberrationParams;
	FShaderParameter ScreenPosToScenePixel;

	FShaderParameter SceneUVMinMax;
	FShaderParameter SceneBloomUVMinMax;
};


// Vertex Shader permutations based on bool AutoExposure.
IMPLEMENT_SHADER_TYPE(template<>, TPostProcessTonemapVS<true>, ("PostProcessTonemap.dusf"), ("MainVS"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(template<>, TPostProcessTonemapVS<false>, ("PostProcessTonemap.dusf"), ("MainVS"), SF_Vertex);

class FPostProcessTonemapPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FPostProcessTonemapPS);

	using FPermutationDomain = TonemapperPermutation::FDesktopDomain;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
// 		if (!IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES2))
// 		{
// 			return false;
// 		}
		return TonemapperPermutation::ShouldCompileDesktopPermutation(FPermutationDomain(Parameters.PermutationId));
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(("USE_VOLUME_LUT"), true/* UseVolumeTextureLUT(Parameters.Platform)*/);
	}

	/** Default constructor. */
	FPostProcessTonemapPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FPostProcessTonemapShaderParameters PostProcessTonemapShaderParameters;

	/** Initialization constructor. */
	FPostProcessTonemapPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
		, PostProcessTonemapShaderParameters(Initializer.ParameterMap)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
	}

	void SetPS(const FRenderingCompositePassContext& Context)
	{
		ID3D11PixelShader* const ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(ShaderRHI, Context.View.ViewUniformBuffer.get());

		{
			// filtering can cost performance so we use point where possible, we don't want anisotropic sampling
			ID3D11SamplerState* Filters[] =
			{
				TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, 0, 1>::GetRHI(),		// todo: could be SF_Point if fringe is disabled
				TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, 0, 1>::GetRHI(),
				TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, 0, 1>::GetRHI(),
				TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, 0, 1>::GetRHI(),
			};

			PostprocessParameter.SetPS(ShaderRHI, Context, 0, eFC_0000, Filters);
		}

		PostProcessTonemapShaderParameters.Set(ShaderRHI, Context, GetUniformBufferParameter<FBloomDirtMaskParameters>());
	}
};

IMPLEMENT_GLOBAL_SHADER(FPostProcessTonemapPS, "PostProcessTonemap.dusf", "MainPS", SF_Pixel);


FRCPassPostProcessTonemap::FRCPassPostProcessTonemap(const FViewInfo& InView, bool bInDoGammaOnly, bool bInDoEyeAdaptation, bool bInHDROutput, bool bInIsComputePass)
	: bDoGammaOnly(bInDoGammaOnly)
	, bDoScreenPercentageInTonemapper(false)
	, bDoEyeAdaptation(bInDoEyeAdaptation)
	, bHDROutput(bInHDROutput)
	, View(InView)
{
	bIsComputePass = bInIsComputePass;
	bPreferAsyncCompute = false;
}
namespace PostProcessTonemapUtil
{

	template <bool bVSDoEyeAdaptation>
	static inline void ShaderTransitionResources(const FRenderingCompositePassContext& Context)
	{
		typedef TPostProcessTonemapVS<bVSDoEyeAdaptation>			VertexShaderType;
		TShaderMapRef<VertexShaderType> VertexShader(Context.GetShaderMap());
		VertexShader->TransitionResources(Context);
	}

} // PostProcessTonemapUtil
void FRCPassPostProcessTonemap::Process(FRenderingCompositePassContext& Context)
{
	const PooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	const PooledRenderTarget& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	const FSceneViewFamily& ViewFamily = *(View.Family);
	FIntRect SrcRect = Context.SceneColorViewRect;
	FIntRect DestRect = Context.GetSceneColorDestRect(DestRenderTarget);

	if (bDoScreenPercentageInTonemapper)
	{
		assert(Context.IsViewFamilyRenderTarget(DestRenderTarget));// , TEXT("Doing screen percentage in tonemapper should only be when tonemapper is actually the last pass."));
		assert(Context.View.PrimaryScreenPercentageMethod == EPrimaryScreenPercentageMethod::SpatialUpscale);// , TEXT("Tonemapper should only do screen percentage upscale if UpscalePass method should be used."));
	}

	FIntPoint SrcSize = InputDesc->Extent;
	FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;

	SCOPED_DRAW_EVENT_FORMAT(PostProcessTonemap, TEXT("Tonemapper(%s GammaOnly=%d HandleScreenPercentage=%d) %dx%d"),
		bIsComputePass ? TEXT("CS") : TEXT("PS"), bDoGammaOnly, bDoScreenPercentageInTonemapper, DestRect.Width(), DestRect.Height());

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();

	// Generate permutation vector for the desktop tonemapper.
	TonemapperPermutation::FDesktopDomain DesktopPermutationVector;
	{
		TonemapperPermutation::FCommonDomain CommonDomain = TonemapperPermutation::BuildCommonPermutationDomain(View, bDoGammaOnly);
		DesktopPermutationVector.Set<TonemapperPermutation::FCommonDomain>(CommonDomain);

		if (!CommonDomain.Get<TonemapperPermutation::FTonemapperGammaOnlyDim>())
		{
			// Grain Quantization
			{
				//static TConsoleVariableData<int32>* CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.Tonemapper.GrainQuantization"));
				int32 Value = 1;// CVar->GetValueOnRenderThread();
				DesktopPermutationVector.Set<TonemapperPermutation::FTonemapperGrainQuantizationDim>(Value > 0);
			}

			DesktopPermutationVector.Set<TonemapperPermutation::FTonemapperColorFringeDim>(/*View.FinalPostProcessSettings.SceneFringeIntensity*/0.f > 0.01f);
		}

		DesktopPermutationVector.Set<TonemapperPermutation::FTonemapperOutputDeviceDim>(GetOutputDeviceValue());

		DesktopPermutationVector = TonemapperPermutation::RemapPermutation(DesktopPermutationVector);
	}

	{
		//WaitForInputPassComputeFences(Context.RHICmdList);

		//const EShaderPlatform ShaderPlatform = GShaderPlatformForFeatureLevel[Context.GetFeatureLevel()];
		if (bDoEyeAdaptation)
		{
			PostProcessTonemapUtil::ShaderTransitionResources<true>(Context);
		}
		else
		{
			PostProcessTonemapUtil::ShaderTransitionResources<false>(Context);
		}

// 		if (IsMobilePlatform(ShaderPlatform))
// 		{
// 			// clear target when processing first view in case of splitscreen
// 			const bool bFirstView = (&View == View.Family->Views[0]);
// 
// 			// Full clear to avoid restore
// 			if ((View.StereoPass == eSSP_FULL && bFirstView) || View.StereoPass == eSSP_LEFT_EYE)
// 			{
// 				SetRenderTarget(Context.RHICmdList, DestRenderTarget.TargetableTexture, FTextureRHIParamRef(), ESimpleRenderTargetMode::EClearColorAndDepth);
// 			}
// 			else
// 			{
// 				SetRenderTarget(Context.RHICmdList, DestRenderTarget.TargetableTexture, FTextureRHIParamRef());
// 			}
// 		}
// 		else
		{
			ERenderTargetLoadAction LoadAction = Context.GetLoadActionForRenderTarget(DestRenderTarget);
			if (Context.View.AntiAliasingMethod == AAM_FXAA)
			{
				assert(LoadAction != ERenderTargetLoadAction::ELoad);
				// needed to not have PostProcessAA leaking in content (e.g. Matinee black borders).
				LoadAction = ERenderTargetLoadAction::EClear;
			}

			//FRHIRenderTargetView RtView = FRHIRenderTargetView(DestRenderTarget.TargetableTexture, LoadAction);
			//FRHISetRenderTargetsInfo Info(1, &RtView, FRHIDepthRenderTargetView());
			//Context.RHICmdList.SetRenderTargetsAndClear(Info);

			SetRenderTarget(DestRenderTarget.TargetableTexture.get(), nullptr, LoadAction == ERenderTargetLoadAction::EClear, false, false);
		}

		Context.SetViewportAndCallRHI(DestRect, 0.0f, 1.0f);

		FShader* VertexShader;
		{
			//FGraphicsPipelineStateInitializer GraphicsPSOInit;
			//Context.RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
			ID3D11BlendState* BlendState = TStaticBlendState<>::GetRHI();
			ID3D11RasterizerState* RasterizerState = TStaticRasterizerState<>::GetRHI();
			ID3D11DepthStencilState* DepthStencilState = TStaticDepthStencilState<false, D3D11_COMPARISON_ALWAYS>::GetRHI();

			D3D11DeviceContext->OMSetBlendState(BlendState, NULL, 0xffffffff);
			D3D11DeviceContext->RSSetState(RasterizerState);
			D3D11DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);

			if (bDoEyeAdaptation)
			{
				VertexShader = Context.GetShaderMap()->GetShader<TPostProcessTonemapVS<true>>();
			}
			else
			{
				VertexShader = Context.GetShaderMap()->GetShader<TPostProcessTonemapVS<false>>();
			}

			TShaderMapRef<FPostProcessTonemapPS> PixelShader(Context.GetShaderMap(), DesktopPermutationVector);

// 			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
// 			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader->GetVertexShader();
// 			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
// 			GraphicsPSOInit.PrimitiveType = PT_TriangleList;

			D3D11DeviceContext->IASetInputLayout(GetInputLayout(GetFilterInputDelcaration().get(), VertexShader->GetCode().Get()));
			D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
			D3D11DeviceContext->PSSetShader(PixelShader->GetPixelShader(), 0, 0);
			D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			//SetGraphicsPipelineState(Context.RHICmdList, GraphicsPSOInit);

			if (bDoEyeAdaptation)
			{
				TShaderMapRef<TPostProcessTonemapVS<true>> VertexShaderMapRef(Context.GetShaderMap());
				VertexShaderMapRef->SetVS(Context);
			}
			else
			{
				TShaderMapRef<TPostProcessTonemapVS<false>> VertexShaderMapRef(Context.GetShaderMap());
				VertexShaderMapRef->SetVS(Context);
			}

			PixelShader->SetPS(Context);
		}

		DrawPostProcessPass(
			0, 0,
			DestRect.Width(), DestRect.Height(),
			SrcRect.Min.X, SrcRect.Min.Y,
			SrcRect.Width(), SrcRect.Height(),
			DestRect.Size(),
			SrcSize,
			VertexShader//,
			/*View.StereoPass,*/
			//Context.HasHmdMesh(),
			/*EDRF_UseTriangleOptimization*/);

		RHICopyToResolveTarget(DestRenderTarget.TargetableTexture.get(), DestRenderTarget.ShaderResourceTexture.get(), FResolveParams());

		// We only release the SceneColor after the last view was processed (SplitScreen)
		if (Context.View.Family->Views[Context.View.Family->Views.size() - 1] == &Context.View/* && !GIsEditor*/)
		{
			// The RT should be released as early as possible to allow sharing of that memory for other purposes.
			// This becomes even more important with some limited VRam (XBoxOne).
			SceneContext.SetSceneColor(0);
		}
	}
}

PooledRenderTargetDesc FRCPassPostProcessTonemap::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	PooledRenderTargetDesc Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;

	Ret.Reset();

	Ret.TargetableFlags &= ~(TexCreate_RenderTargetable | TexCreate_UAV);
	Ret.TargetableFlags |= bIsComputePass ? TexCreate_UAV : TexCreate_RenderTargetable;
	Ret.Format = bIsComputePass ? PF_R8G8B8A8 : PF_B8G8R8A8;

	// RGB is the color in LDR, A is the luminance for PostprocessAA
	Ret.Format = bHDROutput ? GRHIHDRDisplayOutputFormat : Ret.Format;
	//Ret.DebugName = TEXT("Tonemap");
	Ret.ClearValue = FClearValueBinding(FLinearColor(0, 0, 0, 0));
	//Ret.Flags |= GFastVRamConfig.Tonemap;

	if (0/*CVarDisplayOutputDevice.GetValueOnRenderThread()*/ == 7)
	{
		Ret.Format = PF_A32B32G32R32F;
	}


	// Mobile needs to override the extent
	if (bDoScreenPercentageInTonemapper && false/*View.GetFeatureLevel() <= ERHIFeatureLevel::ES3_1*/)
	{
		Ret.Extent = View.UnscaledViewRect.Max;
	}
	return Ret;
}
