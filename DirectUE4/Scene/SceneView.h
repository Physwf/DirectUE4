#pragma once

#include "UnrealMath.h"
#include "D3D11RHI.h"
#include "ConvexVolume.h"

class FSceneViewFamily;

struct ViewInitOptions
{
	const FSceneViewFamily* ViewFamily;
	/** The view origin. */
	FVector ViewOrigin;
	/** Rotation matrix transforming from world space to view space. */
	FMatrix ViewRotationMatrix;
	/** UE4 projection matrix projects such that clip space Z=1 is the near plane, and Z=0 is the infinite far plane. */
	FMatrix ProjectionMatrix;

	const class AActor* ViewActor;

	float LODDistanceFactor;

	/** Actual field of view and that desired by the camera originally */
	float FOV;
	float DesiredFOV;

	//The unconstrained (no aspect ratio bars applied) view rectangle (also unscaled)
	FIntRect ViewRect;
	// The constrained view rectangle (identical to UnconstrainedUnscaledViewRect if aspect ratio is not constrained)
	FIntRect ConstrainedViewRect;

	float OverrideFarClippingPlaneDistance;

	void SetViewRectangle(const FIntRect& InViewRect)
	{
		ViewRect = InViewRect;
		ConstrainedViewRect = InViewRect;
	}
	const FIntRect& GetViewRect() const { return ViewRect; }
	const FIntRect& GetConstrainedViewRect() const { return ConstrainedViewRect; }

	ViewInitOptions()
		: ViewFamily(NULL)
		//, SceneViewStateInterface(NULL)
		, ViewActor(NULL)
		, OverrideFarClippingPlaneDistance(-1.0f)
		, FOV(90.f)
		, DesiredFOV(90.f)
		, LODDistanceFactor(1.0f)
	{
	}
};

struct FViewMatrices
{
	FViewMatrices()
	{
		ProjectionMatrix.SetIdentity();
		ViewMatrix.SetIdentity();
		TranslatedViewMatrix.SetIdentity();
		TranslatedViewProjectionMatrix.SetIdentity();
		InvTranslatedViewMatrix.SetIdentity();
		PreViewTranslation = FVector(0.0f, 0.0f, 0.0f);
		ViewOrigin = FVector(0.0f, 0.0f, 0.0f);
		ScreenScale = 1.f;
	}
	FViewMatrices(const ViewInitOptions& InitOptions);
private:
	FMatrix ProjectionMatrix;//ViewToClip
	FMatrix InvProjectionMatrix;//ClipToView
	FMatrix ViewMatrix;//WorldToView
	FMatrix InvViewMatrix;//ViewToWorld
	FMatrix ViewProjectionMatrix;//WorldToClip
	FMatrix InvViewProjectionMatrix;//ClipToWorld

	FMatrix TranslatedViewMatrix;//
	FMatrix InvTranslatedViewMatrix;//
	FMatrix OverriddenTranslatedViewMatrix;//
	FMatrix OverriddenInvTranslatedViewMatrix;//
	FMatrix TranslatedViewProjectionMatrix;//
	FMatrix InvTranslatedViewProjectionMatrix;//

	FVector PreViewTranslation;
	FVector ViewOrigin;

	Vector2	ProjectionScale;

	float ScreenScale;

public:
	inline const FMatrix& GetProjectionMatrix() const
	{
		return ProjectionMatrix;
	}
	inline const FMatrix& GetInvProjectionMatrix() const
	{
		return InvProjectionMatrix;
	}
	inline const FMatrix& GetViewMatrix() const
	{
		return ViewMatrix;
	}
	inline const FMatrix& GetInvViewMatrix() const
	{
		return InvViewMatrix;
	}
	inline const FMatrix& GetViewProjectionMatrix() const
	{
		return ViewProjectionMatrix;
	}
	inline const FMatrix& GetInvViewProjectionMatrix() const
	{
		return InvViewProjectionMatrix;
	}

	inline const FMatrix& GetTranslatedViewMatrix() const
	{
		return TranslatedViewMatrix;
	}
	inline const FMatrix& GetInvTranslatedViewMatrix() const
	{
		return InvTranslatedViewMatrix;
	}
	inline const FMatrix& GetOverriddenTranslatedViewMatrix() const
	{
		return OverriddenTranslatedViewMatrix;
	}
	inline const FMatrix& GetOverriddenInvTranslatedViewMatrix() const
	{
		return OverriddenInvTranslatedViewMatrix;
	}
	inline const FMatrix& GetTranslatedViewProjectionMatrix() const
	{
		return TranslatedViewProjectionMatrix;
	}
	inline const FMatrix& GetInvTranslatedViewProjectionMatrix() const
	{
		return InvTranslatedViewProjectionMatrix;
	}

	inline const FVector& GetPreViewTranslation() const
	{
		return PreViewTranslation;
	}
	inline const FVector& GetViewOrigin() const
	{
		return ViewOrigin;
	}
	inline const Vector2& GetProjectionScale() const
	{
		return ProjectionScale;
	}
	inline float GetScreenScale() const
	{
		return ScreenScale;
	}
	inline bool IsPerspectiveProjection() const
	{
		return ProjectionMatrix.M[3][3] < 1.0f;
	}
	static const FMatrix InvertProjectionMatrix(const FMatrix& M)
	{
		if (M.M[1][0] == 0.0f &&
			M.M[3][0] == 0.0f &&
			M.M[0][1] == 0.0f &&
			M.M[3][1] == 0.0f &&
			M.M[0][2] == 0.0f &&
			M.M[1][2] == 0.0f &&
			M.M[0][3] == 0.0f &&
			M.M[1][3] == 0.0f &&
			M.M[2][3] == 1.0f &&
			M.M[3][3] == 0.0f)
		{
			// Solve the common case directly with very high precision.
			/*
			M =
			| a | 0 | 0 | 0 |
			| 0 | b | 0 | 0 |
			| s | t | c | 1 |
			| 0 | 0 | d | 0 |
			*/

			float a = M.M[0][0];
			float b = M.M[1][1];
			float c = M.M[2][2];
			float d = M.M[3][2];
			float s = M.M[2][0];
			float t = M.M[2][1];

			return FMatrix(
				FPlane(1.0f / a, 0.0f, 0.0f, 0.0f),
				FPlane(0.0f, 1.0f / b, 0.0f, 0.0f),
				FPlane(0.0f, 0.0f, 0.0f, 1.0f / d),
				FPlane(-s / a, -t / b, 1.0f, -c / d)
			);
		}
		else
		{
			return M.Inverse();
		}
	}
};

enum ETranslucencyVolumeCascade
{
	TVC_Inner,
	TVC_Outer,

	TVC_MAX,
};
const int32 GMaxGlobalDistanceFieldClipmaps = 4;

struct alignas(16) FViewUniformShaderParameters
{
	FViewUniformShaderParameters()
	{
		ConstructUniformBufferInfo(*this);
	}

	struct ConstantStruct
	{
		/*
		Matrix TranslatedWorldToClip;
		Matrix WorldToClip;
		Matrix TranslatedWorldToView;
		Matrix ViewToTranslatedWorld;
		Matrix TranslatedWorldToCameraView;
		Matrix CameraViewToTranslatedWorld;
		Matrix ViewToClip;
		Matrix ViewToClipNoAA;
		Matrix ClipToView;
		Matrix ClipToTranslatedWorld;
		Matrix SVPositionToTranslatedWorld;
		Matrix ScreenToWorld;
		Matrix ScreenToTranslatedWorld;
		// half3 ViewForward;
		// half3 ViewUp;
		// half3 ViewRight;
		// half3 HMDViewNoRollUp;
		// half3 HMDViewNoRollRight;
		Vector4 InvDeviceZToWorldZTransform;
		Vector4 ScreenPositionScaleBias;
		Vector WorldCameraOrigin;
		float ViewPading01;
		Vector TranslatedWorldCameraOrigin;
		float ViewPading02;
		Vector WorldViewOrigin;
		float ViewPading03;

		Vector PreViewTranslation;
		float ViewPading04;
		Vector4 ViewRectMin;
		Vector4 ViewSizeAndInvSize;
		Vector4 BufferSizeAndInvSize;
		Vector4 BufferBilinearUVMinMax;

		uint32 Random;
		uint32 FrameNumber;
		uint32 StateFrameIndexMod8;
		uint32 ViewPading05;

		float DemosaicVposOffset;
		Vector IndirectLightingColorScale;

		Vector AtmosphericFogSunDirection;
		float AtmosphericFogSunPower;
		float AtmosphericFogPower;
		float AtmosphericFogDensityScale;
		float AtmosphericFogDensityOffset;
		float AtmosphericFogGroundOffset;
		float AtmosphericFogDistanceScale;
		float AtmosphericFogAltitudeScale;
		float AtmosphericFogHeightScaleRayleigh;
		float AtmosphericFogStartDistance;
		float AtmosphericFogDistanceOffset;
		float AtmosphericFogSunDiscScale;
		uint32 AtmosphericFogRenderMask;
		uint32 AtmosphericFogInscatterAltitudeSampleNum;
		FLinearColor AtmosphericFogSunColor;

		float AmbientCubemapIntensity;
		float SkyLightParameters;
		float PrePadding_View_2472;
		float PrePadding_View_2476;
		Vector4 SkyLightColor;
		Vector4 SkyIrradianceEnvironmentMap[7];

		float PrePadding_View_2862;
		Vector VolumetricLightmapWorldToUVScale;
		float PrePadding_View_2861;
		Vector VolumetricLightmapWorldToUVAdd;
		float PrePadding_View_2876;
		Vector VolumetricLightmapIndirectionTextureSize;

		float VolumetricLightmapBrickSize;
		Vector VolumetricLightmapBrickTexelSize;
		*/

		FMatrix TranslatedWorldToClip;
		FMatrix WorldToClip;
		FMatrix TranslatedWorldToView;
		FMatrix ViewToTranslatedWorld;
		FMatrix TranslatedWorldToCameraView;
		FMatrix CameraViewToTranslatedWorld;
		FMatrix ViewToClip;
		FMatrix ViewToClipNoAA;
		FMatrix ClipToView;
		FMatrix ClipToTranslatedWorld;
		FMatrix SVPositionToTranslatedWorld;
		FMatrix ScreenToWorld;
		FMatrix ScreenToTranslatedWorld;
		FVector ViewForward;//EShaderPrecisionModifier::Half;;
		float pading001;
		FVector ViewUp;//EShaderPrecisionModifier::Half;;
		float pading002;
		FVector ViewRight;//EShaderPrecisionModifier::Half;;
		float pading003;
		FVector HMDViewNoRollUp;//EShaderPrecisionModifier::Half;;
		float pading004;
		FVector HMDViewNoRollRight;//EShaderPrecisionModifier::Half;;
		float pading005;
		Vector4 InvDeviceZToWorldZTransform;
		Vector4 ScreenPositionScaleBias;//EShaderPrecisionModifier::Half;;
		FVector WorldCameraOrigin;
		float pading006;
		FVector TranslatedWorldCameraOrigin;
		float pading007;
		FVector WorldViewOrigin;
		float pading008;
		FVector PreViewTranslation;
		float pading009;
		FMatrix PrevProjection;
		FMatrix PrevViewProj;
		FMatrix PrevViewRotationProj;
		FMatrix PrevViewToClip;
		FMatrix PrevClipToView;
		FMatrix PrevTranslatedWorldToClip;
		FMatrix PrevTranslatedWorldToView;
		FMatrix PrevViewToTranslatedWorld;
		FMatrix PrevTranslatedWorldToCameraView;
		FMatrix PrevCameraViewToTranslatedWorld;
		FVector PrevWorldCameraOrigin;
		float pading010;
		FVector PrevWorldViewOrigin;
		float pading011;
		FVector PrevPreViewTranslation;
		float pading012;
		FMatrix PrevInvViewProj;
		FMatrix PrevScreenToTranslatedWorld;
		FMatrix ClipToPrevClip;
		Vector4 TemporalAAJitter;
		Vector4 GlobalClippingPlane;
		Vector2 FieldOfViewWideAngles;
		Vector2 PrevFieldOfViewWideAngles;
		Vector4 ViewRectMin;//EShaderPrecisionModifier::Half;;
		Vector4 ViewSizeAndInvSize;
		Vector4 BufferSizeAndInvSize;
		Vector4 BufferBilinearUVMinMax;
		int32 NumSceneColorMSAASamples;
		float PreExposure;//EShaderPrecisionModifier::Half;;
		float OneOverPreExposure;//EShaderPrecisionModifier::Half;;
		float pading013;
		Vector4 DiffuseOverrideParameter;//EShaderPrecisionModifier::Half;;
		Vector4 SpecularOverrideParameter;//;//EShaderPrecisionModifier::Half;;;
		Vector4 NormalOverrideParameter;//;//EShaderPrecisionModifier::Half;;;
		Vector2 RoughnessOverrideParameter;//EShaderPrecisionModifier::Half;;
		float PrevFrameGameTime;
		float PrevFrameRealTime;
		float OutOfBoundsMask;//EShaderPrecisionModifier::Half;;
		float pading014;
		float pading015;
		FVector WorldCameraMovementSinceLastFrame;
		float CullingSign;
		float NearPlane;//EShaderPrecisionModifier::Half;;
		float AdaptiveTessellationFactor;
		float GameTime;
		float RealTime;
		float MaterialTextureMipBias;
		float MaterialTextureDerivativeMultiply;
		uint32 Random;
		uint32 FrameNumber;
		uint32 StateFrameIndexMod8;
		float CameraCut;//EShaderPrecisionModifier::Half;;
		float UnlitViewmodeMask;//EShaderPrecisionModifier::Half;;
		float pading016;
		FLinearColor DirectionalLightColor;//EShaderPrecisionModifier::Half;;
		FVector DirectionalLightDirection;//EShaderPrecisionModifier::Half;;
		float pading017;
		Vector4 TranslucencyLightingVolumeMin[TVC_MAX];
		Vector4 TranslucencyLightingVolumeInvSize[TVC_MAX];
		Vector4 TemporalAAParams;
		Vector4 CircleDOFParams;
		float DepthOfFieldSensorWidth;
		float DepthOfFieldFocalDistance;
		float DepthOfFieldScale;
		float DepthOfFieldFocalLength;
		float DepthOfFieldFocalRegion;
		float DepthOfFieldNearTransitionRegion;
		float DepthOfFieldFarTransitionRegion;
		float MotionBlurNormalizedToPixel;
		float bSubsurfacePostprocessEnabled;
		float GeneralPurposeTweak;
		float DemosaicVposOffset;//EShaderPrecisionModifier::Half;;
		float pading018;
		FVector IndirectLightingColorScale;
		float HDR32bppEncodingMode;//EShaderPrecisionModifier::Half;;
		FVector AtmosphericFogSunDirection;
		float AtmosphericFogSunPower;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogPower;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogDensityScale;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogDensityOffset;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogGroundOffset;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogDistanceScale;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogAltitudeScale;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogHeightScaleRayleigh;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogStartDistance;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogDistanceOffset;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogSunDiscScale;//EShaderPrecisionModifier::Half;;
		uint32 AtmosphericFogRenderMask;
		uint32 AtmosphericFogInscatterAltitudeSampleNum;
		FLinearColor AtmosphericFogSunColor;
		FVector NormalCurvatureToRoughnessScaleBias;
		float RenderingReflectionCaptureMask;
		FLinearColor AmbientCubemapTint;
		float AmbientCubemapIntensity;
		float SkyLightParameters;
		float pading019;
		float pading020;
		FLinearColor SkyLightColor;
		Vector4 SkyIrradianceEnvironmentMap[7];
		float MobilePreviewMode;
		float HMDEyePaddingOffset;
		float ReflectionCubemapMaxMip;//EShaderPrecisionModifier::Half;;
		float ShowDecalsMask;
		uint32 DistanceFieldAOSpecularOcclusionMode;
		float IndirectCapsuleSelfShadowingIntensity;
		float pading021;
		float pading022;
		FVector ReflectionEnvironmentRoughnessMixingScaleBiasAndLargestWeight;
		int32 StereoPassIndex;
		Vector4 GlobalVolumeCenterAndExtent[GMaxGlobalDistanceFieldClipmaps];
		Vector4 GlobalVolumeWorldToUVAddAndMul[GMaxGlobalDistanceFieldClipmaps];
		float GlobalVolumeDimension;
		float GlobalVolumeTexelSize;
		float MaxGlobalDistance;
		float bCheckerboardSubsurfaceProfileRendering;
		FVector VolumetricFogInvGridSize;
		float pading023;
		FVector VolumetricFogGridZParams;
		float pading024;
		Vector2 VolumetricFogSVPosToVolumeUV;
		float VolumetricFogMaxDistance;
		float pading025;
		FVector VolumetricLightmapWorldToUVScale;
		float pading026;
		FVector VolumetricLightmapWorldToUVAdd;
		float pading027;
		FVector VolumetricLightmapIndirectionTextureSize;
		float VolumetricLightmapBrickSize;
		FVector VolumetricLightmapBrickTexelSize;
		float StereoIPD;
	} Constants;


	ID3D11SamplerState* MaterialTextureBilinearWrapedSampler;
	ID3D11SamplerState* MaterialTextureBilinearClampedSampler;

	ID3D11ShaderResourceView* VolumetricLightmapIndirectionTexture; // FPrecomputedVolumetricLightmapLightingPolicy
	ID3D11ShaderResourceView* VolumetricLightmapBrickAmbientVector; // FPrecomputedVolumetricLightmapLightingPolicy
	ID3D11ShaderResourceView* VolumetricLightmapBrickSHCoefficients0; // FPrecomputedVolumetricLightmapLightingPolicy
	ID3D11ShaderResourceView* VolumetricLightmapBrickSHCoefficients1; // FPrecomputedVolumetricLightmapLightingPolicy
	ID3D11ShaderResourceView* VolumetricLightmapBrickSHCoefficients2; // FPrecomputedVolumetricLightmapLightingPolicy
	ID3D11ShaderResourceView* VolumetricLightmapBrickSHCoefficients3; // FPrecomputedVolumetricLightmapLightingPolicy
	ID3D11ShaderResourceView* VolumetricLightmapBrickSHCoefficients4; // FPrecomputedVolumetricLightmapLightingPolicy
	ID3D11ShaderResourceView* VolumetricLightmapBrickSHCoefficients5; // FPrecomputedVolumetricLightmapLightingPolicy
	ID3D11ShaderResourceView* SkyBentNormalBrickTexture; // FPrecomputedVolumetricLightmapLightingPolicy
	ID3D11ShaderResourceView* DirectionalLightShadowingBrickTexture; // FPrecomputedVolumetricLightmapLightingPolicy

	ID3D11SamplerState* VolumetricLightmapBrickAmbientVectorSampler; // FPrecomputedVolumetricLightmapLightingPolicy
	ID3D11SamplerState* VolumetricLightmapTextureSampler0; // FPrecomputedVolumetricLightmapLightingPolicy
	ID3D11SamplerState* VolumetricLightmapTextureSampler1; // FPrecomputedVolumetricLightmapLightingPolicy
	ID3D11SamplerState* VolumetricLightmapTextureSampler2; // FPrecomputedVolumetricLightmapLightingPolicy
	ID3D11SamplerState* VolumetricLightmapTextureSampler3; // FPrecomputedVolumetricLightmapLightingPolicy
	ID3D11SamplerState* VolumetricLightmapTextureSampler4; // FPrecomputedVolumetricLightmapLightingPolicy
	ID3D11SamplerState* VolumetricLightmapTextureSampler5; // FPrecomputedVolumetricLightmapLightingPolicy
	ID3D11SamplerState* SkyBentNormalTextureSampler; // FPrecomputedVolumetricLightmapLightingPolicy
	ID3D11SamplerState* DirectionalLightShadowingTextureSampler; // FPrecomputedVolumetricLightmapLightingPolicy

	ID3D11ShaderResourceView* GlobalDistanceFieldTexture0;
	ID3D11SamplerState* GlobalDistanceFieldSampler0;
	ID3D11ShaderResourceView* GlobalDistanceFieldTexture1;
	ID3D11SamplerState* GlobalDistanceFieldSampler1;
	ID3D11ShaderResourceView* GlobalDistanceFieldTexture2;
	ID3D11SamplerState* GlobalDistanceFieldSampler2;
	ID3D11ShaderResourceView* GlobalDistanceFieldTexture3;
	ID3D11SamplerState* GlobalDistanceFieldSampler3;

	ID3D11ShaderResourceView* AtmosphereTransmittanceTexture;
	ID3D11SamplerState* AtmosphereTransmittanceTextureSampler;
	ID3D11ShaderResourceView* AtmosphereIrradianceTexture;
	ID3D11SamplerState* AtmosphereIrradianceTextureSampler;
	ID3D11ShaderResourceView* AtmosphereInscatterTexture;
	ID3D11SamplerState* AtmosphereInscatterTextureSampler;
	ID3D11ShaderResourceView* PerlinNoiseGradientTexture;
	ID3D11SamplerState* PerlinNoiseGradientTextureSampler;
	ID3D11ShaderResourceView* PerlinNoise3DTexture;
	ID3D11SamplerState* PerlinNoise3DTextureSampler;
	ID3D11ShaderResourceView* SobolSamplingTexture;
	ID3D11SamplerState* SharedPointWrappedSampler;
	ID3D11SamplerState* SharedPointClampedSampler;
	ID3D11SamplerState* SharedBilinearWrappedSampler;
	ID3D11SamplerState* SharedBilinearClampedSampler;
	ID3D11SamplerState* SharedTrilinearWrappedSampler;
	ID3D11SamplerState* SharedTrilinearClampedSampler;
	ID3D11ShaderResourceView* PreIntegratedBRDF;
	ID3D11SamplerState* PreIntegratedBRDFSampler;

	static std::string GetConstantBufferName()
	{
		return "View";
	}
#define ADD_RES(StructName, MemberName) List.insert(std::make_pair(std::string(#StructName) + "_" + std::string(#MemberName),StructName.MemberName))
	static std::map<std::string, ID3D11ShaderResourceView*> GetSRVs(const FViewUniformShaderParameters& View)
	{
		std::map<std::string, ID3D11ShaderResourceView*> List;
		ADD_RES(View, VolumetricLightmapIndirectionTexture);
		ADD_RES(View, VolumetricLightmapBrickAmbientVector);
		ADD_RES(View, VolumetricLightmapBrickSHCoefficients0);
		ADD_RES(View, VolumetricLightmapBrickSHCoefficients1);
		ADD_RES(View, VolumetricLightmapBrickSHCoefficients2);
		ADD_RES(View, VolumetricLightmapBrickSHCoefficients3);
		ADD_RES(View, VolumetricLightmapBrickSHCoefficients4);
		ADD_RES(View, VolumetricLightmapBrickSHCoefficients5);
		ADD_RES(View, SkyBentNormalBrickTexture);
		ADD_RES(View, DirectionalLightShadowingBrickTexture);
		ADD_RES(View, GlobalDistanceFieldTexture0);
		ADD_RES(View, GlobalDistanceFieldTexture1);
		ADD_RES(View, GlobalDistanceFieldTexture2);
		ADD_RES(View, GlobalDistanceFieldTexture3);
		ADD_RES(View, AtmosphereTransmittanceTexture);
		ADD_RES(View, AtmosphereIrradianceTexture);
		ADD_RES(View, AtmosphereInscatterTexture);
		ADD_RES(View, PerlinNoiseGradientTexture);
		ADD_RES(View, PerlinNoise3DTexture);
		ADD_RES(View, SobolSamplingTexture);
		ADD_RES(View, PreIntegratedBRDF);
		return List;
	}
	static std::map<std::string, ID3D11SamplerState*> GetSamplers(const FViewUniformShaderParameters& View)
	{
		std::map<std::string, ID3D11SamplerState*> List;
		ADD_RES(View, MaterialTextureBilinearWrapedSampler);
		ADD_RES(View, MaterialTextureBilinearClampedSampler);
		ADD_RES(View, VolumetricLightmapBrickAmbientVectorSampler);
		ADD_RES(View, VolumetricLightmapTextureSampler0);
		ADD_RES(View, VolumetricLightmapTextureSampler1);
		ADD_RES(View, VolumetricLightmapTextureSampler2);
		ADD_RES(View, VolumetricLightmapTextureSampler3);
		ADD_RES(View, VolumetricLightmapTextureSampler4);
		ADD_RES(View, VolumetricLightmapTextureSampler5);
		ADD_RES(View, SkyBentNormalTextureSampler);
		ADD_RES(View, GlobalDistanceFieldSampler0);
		ADD_RES(View, GlobalDistanceFieldSampler1);
		ADD_RES(View, GlobalDistanceFieldSampler2);
		ADD_RES(View, GlobalDistanceFieldSampler3);
		ADD_RES(View, AtmosphereTransmittanceTextureSampler);
		ADD_RES(View, AtmosphereIrradianceTextureSampler);
		ADD_RES(View, AtmosphereInscatterTextureSampler);
		ADD_RES(View, PerlinNoiseGradientTextureSampler);
		ADD_RES(View, PerlinNoise3DTextureSampler);
		ADD_RES(View, SharedPointWrappedSampler);
		ADD_RES(View, SharedPointClampedSampler);
		ADD_RES(View, SharedBilinearWrappedSampler);
		ADD_RES(View, SharedBilinearClampedSampler);
		ADD_RES(View, SharedTrilinearWrappedSampler);
		ADD_RES(View, SharedTrilinearClampedSampler);
		ADD_RES(View, PreIntegratedBRDFSampler);
		return List;
	}
	static std::map<std::string, ID3D11UnorderedAccessView*> GetUAVs(const FViewUniformShaderParameters& View)
	{
		std::map<std::string, ID3D11UnorderedAccessView*> List;
		return List;
	}
#undef ADD_RES

	void TransposeMatrices();
};

struct alignas(16) FInstancedViewUniformShaderParameters
{
	FInstancedViewUniformShaderParameters()
	{
		ConstructUniformBufferInfo(*this);
	}

	struct ConstantStruct
	{
		FMatrix TranslatedWorldToClip;
		FMatrix WorldToClip;
		FMatrix TranslatedWorldToView;
		FMatrix ViewToTranslatedWorld;
		FMatrix TranslatedWorldToCameraView;
		FMatrix CameraViewToTranslatedWorld;
		FMatrix ViewToClip;
		FMatrix ViewToClipNoAA;
		FMatrix ClipToView;
		FMatrix ClipToTranslatedWorld;
		FMatrix SVPositionToTranslatedWorld;
		FMatrix ScreenToWorld;
		FMatrix ScreenToTranslatedWorld;
		FVector ViewForward;//EShaderPrecisionModifier::Half;;
		float pading001;
		FVector ViewUp;//EShaderPrecisionModifier::Half;;
		float pading002;
		FVector ViewRight;//EShaderPrecisionModifier::Half;;
		float pading003;
		FVector HMDViewNoRollUp;//EShaderPrecisionModifier::Half;;
		float pading004;
		FVector HMDViewNoRollRight;//EShaderPrecisionModifier::Half;;
		float pading005;
		Vector4 InvDeviceZToWorldZTransform;
		Vector4 ScreenPositionScaleBias;//EShaderPrecisionModifier::Half;;
		FVector WorldCameraOrigin;
		float pading006;
		FVector TranslatedWorldCameraOrigin;
		float pading007;
		FVector WorldViewOrigin;
		float pading008;
		FVector PreViewTranslation;
		float pading009;
		FMatrix PrevProjection;
		FMatrix PrevViewProj;
		FMatrix PrevViewRotationProj;
		FMatrix PrevViewToClip;
		FMatrix PrevClipToView;
		FMatrix PrevTranslatedWorldToClip;
		FMatrix PrevTranslatedWorldToView;
		FMatrix PrevViewToTranslatedWorld;
		FMatrix PrevTranslatedWorldToCameraView;
		FMatrix PrevCameraViewToTranslatedWorld;
		FVector PrevWorldCameraOrigin;
		float pading010;
		FVector PrevWorldViewOrigin;
		float pading011;
		FVector PrevPreViewTranslation;
		float pading012;
		FMatrix PrevInvViewProj;
		FMatrix PrevScreenToTranslatedWorld;
		FMatrix ClipToPrevClip;
		Vector4 TemporalAAJitter;
		Vector4 GlobalClippingPlane;
		Vector2 FieldOfViewWideAngles;
		Vector2 PrevFieldOfViewWideAngles;
		Vector4 ViewRectMin;//EShaderPrecisionModifier::Half;;
		Vector4 ViewSizeAndInvSize;
		Vector4 BufferSizeAndInvSize;
		Vector4 BufferBilinearUVMinMax;
		int32 NumSceneColorMSAASamples;
		float PreExposure;//EShaderPrecisionModifier::Half;;
		float OneOverPreExposure;//EShaderPrecisionModifier::Half;;
		float pading013;
		Vector4 DiffuseOverrideParameter;//EShaderPrecisionModifier::Half;;
		Vector4 SpecularOverrideParameter;//;//EShaderPrecisionModifier::Half;;;
		Vector4 NormalOverrideParameter;//;//EShaderPrecisionModifier::Half;;;
		Vector2 RoughnessOverrideParameter;//EShaderPrecisionModifier::Half;;
		float PrevFrameGameTime;
		float PrevFrameRealTime;
		float OutOfBoundsMask;//EShaderPrecisionModifier::Half;;
		float pading014;
		float pading015;
		FVector WorldCameraMovementSinceLastFrame;
		float CullingSign;
		float NearPlane;//EShaderPrecisionModifier::Half;;
		float AdaptiveTessellationFactor;
		float GameTime;
		float RealTime;
		float MaterialTextureMipBias;
		float MaterialTextureDerivativeMultiply;
		uint32 Random;
		uint32 FrameNumber;
		uint32 StateFrameIndexMod8;
		float CameraCut;//EShaderPrecisionModifier::Half;;
		float UnlitViewmodeMask;//EShaderPrecisionModifier::Half;;
		float pading016;
		FLinearColor DirectionalLightColor;//EShaderPrecisionModifier::Half;;
		FVector DirectionalLightDirection;//EShaderPrecisionModifier::Half;;
		float pading017;
		Vector4 TranslucencyLightingVolumeMin[TVC_MAX];
		Vector4 TranslucencyLightingVolumeInvSize[TVC_MAX];
		Vector4 TemporalAAParams;
		Vector4 CircleDOFParams;
		float DepthOfFieldSensorWidth;
		float DepthOfFieldFocalDistance;
		float DepthOfFieldScale;
		float DepthOfFieldFocalLength;
		float DepthOfFieldFocalRegion;
		float DepthOfFieldNearTransitionRegion;
		float DepthOfFieldFarTransitionRegion;
		float MotionBlurNormalizedToPixel;
		float bSubsurfacePostprocessEnabled;
		float GeneralPurposeTweak;
		float DemosaicVposOffset;//EShaderPrecisionModifier::Half;;
		float pading018;
		FVector IndirectLightingColorScale;
		float HDR32bppEncodingMode;//EShaderPrecisionModifier::Half;;
		FVector AtmosphericFogSunDirection;
		float AtmosphericFogSunPower;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogPower;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogDensityScale;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogDensityOffset;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogGroundOffset;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogDistanceScale;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogAltitudeScale;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogHeightScaleRayleigh;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogStartDistance;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogDistanceOffset;//EShaderPrecisionModifier::Half;;
		float AtmosphericFogSunDiscScale;//EShaderPrecisionModifier::Half;;
		uint32 AtmosphericFogRenderMask;
		uint32 AtmosphericFogInscatterAltitudeSampleNum;
		FLinearColor AtmosphericFogSunColor;
		FVector NormalCurvatureToRoughnessScaleBias;
		float RenderingReflectionCaptureMask;
		FLinearColor AmbientCubemapTint;
		float AmbientCubemapIntensity;
		float SkyLightParameters;
		float pading019;
		float pading020;
		FLinearColor SkyLightColor;
		Vector4 SkyIrradianceEnvironmentMap[7];
		float MobilePreviewMode;
		float HMDEyePaddingOffset;
		float ReflectionCubemapMaxMip;//EShaderPrecisionModifier::Half;;
		float ShowDecalsMask;
		uint32 DistanceFieldAOSpecularOcclusionMode;
		float IndirectCapsuleSelfShadowingIntensity;
		float pading021;
		float pading022;
		FVector ReflectionEnvironmentRoughnessMixingScaleBiasAndLargestWeight;
		int32 StereoPassIndex;
		Vector4 GlobalVolumeCenterAndExtent[GMaxGlobalDistanceFieldClipmaps];
		Vector4 GlobalVolumeWorldToUVAddAndMul[GMaxGlobalDistanceFieldClipmaps];
		float GlobalVolumeDimension;
		float GlobalVolumeTexelSize;
		float MaxGlobalDistance;
		float bCheckerboardSubsurfaceProfileRendering;
		FVector VolumetricFogInvGridSize;
		float pading023;
		FVector VolumetricFogGridZParams;
		float pading024;
		Vector2 VolumetricFogSVPosToVolumeUV;
		float VolumetricFogMaxDistance;
		float pading025;
		FVector VolumetricLightmapWorldToUVScale;
		float pading026;
		FVector VolumetricLightmapWorldToUVAdd;
		float pading027;
		FVector VolumetricLightmapIndirectionTextureSize;
		float VolumetricLightmapBrickSize;
		FVector VolumetricLightmapBrickTexelSize;
		float StereoIPD;
	} Constants;

	static std::string GetConstantBufferName()
	{
		return "InstancedView";
	}
	static std::map<std::string, ID3D11ShaderResourceView*> GetSRVs(const FInstancedViewUniformShaderParameters& InstancedView)
	{
		std::map<std::string, ID3D11ShaderResourceView*> List;
		return List;
	}
	static std::map<std::string, ID3D11SamplerState*> GetSamplers(const FInstancedViewUniformShaderParameters& InstancedView)
	{
		std::map<std::string, ID3D11SamplerState*> List;
		return List;
	}
	static std::map<std::string, ID3D11UnorderedAccessView*> GetUAVs(const FInstancedViewUniformShaderParameters& InstancedView)
	{
		std::map<std::string, ID3D11UnorderedAccessView*> List;
		return List;
	}
};
class FSceneViewFamily;

class FSceneView
{
public:
	const FSceneViewFamily* Family;

	TUniformBufferPtr<FViewUniformShaderParameters> ViewUniformBuffer;
	FViewMatrices ViewMatrices;
private:
	/** During GetDynamicMeshElements this will be the correct cull volume for shadow stuff */
	//const FConvexVolume* DynamicMeshElementsShadowCullFrustum;
	/** If the above is non-null, a translation that is applied to world-space before transforming by one of the shadow matrices. */
	//FVector		PreShadowTranslation;

public:

	FSceneView(const ViewInitOptions& InitOptions);

	/** The actor which is being viewed from. */
	const class AActor* ViewActor;
	/* Final position of the view in the final render target (in pixels), potentially constrained by an aspect ratio requirement (black bars) */
	const FIntRect UnscaledViewRect;
	/* Raw view size (in pixels), used for screen space calculations */
	FIntRect UnconstrainedViewRect;

	FVector	ViewLocation;
	FRotator	ViewRotation;

	FViewMatrices ShadowViewMatrices;

	FMatrix ProjectionMatrixUnadjustedForRHI;
	/** Maximum number of shadow cascades to render with. */
	int32 MaxShadowCascades;

	float LODDistanceFactor;
	float LODDistanceFactorSquared;
	/** Actual field of view and that desired by the camera originally */
	float FOV;
	float DesiredFOV;

	FConvexVolume ViewFrustum;

	bool bHasNearClippingPlane;

	//FPlane NearClippingPlane;

	float NearClippingDistance;

	/** true if ViewMatrix.Determinant() is negative. */
	bool bReverseCulling;

	/* Vector used by shaders to convert depth buffer samples into z coordinates in world space */
	Vector4 InvDeviceZToWorldZTransform;

	inline bool IsPerspectiveProjection() const { return ViewMatrices.IsPerspectiveProjection(); }
	/**
	* The final settings for the current viewer position (blended together from many volumes).
	* Setup by the main thread, passed to the render thread and never touched again by the main thread.
	*/
	//FFinalPostProcessSettings FinalPostProcessSettings;
	inline FVector GetViewRight() const { return ViewMatrices.GetViewMatrix().GetColumn(0); }
	inline FVector GetViewUp() const { return ViewMatrices.GetViewMatrix().GetColumn(1); }
	inline FVector GetViewDirection() const { return ViewMatrices.GetViewMatrix().GetColumn(2); }

	/** Whether to force two sided rendering for this view. */
	bool bRenderSceneTwoSided;
	// The antialiasing method.
	//EAntiAliasingMethod AntiAliasingMethod;

	// Primary screen percentage method to use.
	//EPrimaryScreenPercentageMethod PrimaryScreenPercentageMethod;

	/** Parameters for atmospheric fog. */
	//FTextureRHIRef AtmosphereTransmittanceTexture;
	//FTextureRHIRef AtmosphereIrradianceTexture;
	//FTextureRHIRef AtmosphereInscatterTexture;
	bool bStaticSceneOnly;

	/** Sets up the view rect parameters in the view's uniform shader parameters */
	void SetupViewRectUniformBufferParameters(FViewUniformShaderParameters& ViewUniformParameters,
		const FIntPoint& InBufferSize,
		const FIntRect& InEffectiveViewRect,
		const FViewMatrices& InViewMatrices,
		const FViewMatrices& InPrevViewMatrice) const;

	/**
	* Populates the uniform buffer prameters common to all scene view use cases
	* View parameters should be set up in this method if they are required for the view to render properly.
	* This is to avoid code duplication and uninitialized parameters in other places that create view uniform parameters (e.g Slate)
	*/
	void SetupCommonViewUniformBufferParameters(FViewUniformShaderParameters& ViewUniformParameters,
		const FIntPoint& InBufferSize,
		int32 NumMSAASamples,
		const FIntRect& InEffectiveViewRect,
		const FViewMatrices& InViewMatrices,
		const FViewMatrices& InPrevViewMatrices) const;
};

class FSceneViewFamily
{
public:
	class FScene* Scene;
	std::vector<const FSceneView*> Views;
};