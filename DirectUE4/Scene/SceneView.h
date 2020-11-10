#pragma once

#include "UnrealMath.h"
#include "D3D11RHI.h"

struct ViewInitOptions
{
	/** The view origin. */
	Vector ViewOrigin;
	/** Rotation matrix transforming from world space to view space. */
	Matrix ViewRotationMatrix;
	/** UE4 projection matrix projects such that clip space Z=1 is the near plane, and Z=0 is the infinite far plane. */
	Matrix ProjectionMatrix;

	const class Actor* ViewActor;

	/** Actual field of view and that desired by the camera originally */
	float FOV;
	float DesiredFOV;

	//The unconstrained (no aspect ratio bars applied) view rectangle (also unscaled)
	IntRect ViewRect;
	// The constrained view rectangle (identical to UnconstrainedUnscaledViewRect if aspect ratio is not constrained)
	IntRect ConstrainedViewRect;

	void SetViewRectangle(const IntRect& InViewRect)
	{
		ViewRect = InViewRect;
		ConstrainedViewRect = InViewRect;
	}
	const IntRect& GetViewRect() const { return ViewRect; }
	const IntRect& GetConstrainedViewRect() const { return ConstrainedViewRect; }
};

struct ViewMatrices
{
	ViewMatrices()
	{
		ProjectionMatrix.SetIndentity();
		ViewMatrix.SetIndentity();
		TranslatedViewMatrix.SetIndentity();
		TranslatedViewProjectionMatrix.SetIndentity();
		InvTranslatedViewMatrix.SetIndentity();
		PreViewTranslation = Vector(0.0f, 0.0f, 0.0f);
		ViewOrigin = Vector(0.0f, 0.0f, 0.0f);
	}
	ViewMatrices(const ViewInitOptions& InitOptions);
private:
	Matrix ProjectionMatrix;//ViewToClip
	Matrix InvProjectionMatrix;//ClipToView
	Matrix ViewMatrix;//WorldToView
	Matrix InvViewMatrix;//ViewToWorld
	Matrix ViewProjectionMatrix;//WorldToClip
	Matrix InvViewProjectionMatrix;//ClipToWorld

	Matrix TranslatedViewMatrix;//
	Matrix InvTranslatedViewMatrix;//
	Matrix OverriddenTranslatedViewMatrix;//
	Matrix OverriddenInvTranslatedViewMatrix;//
	Matrix TranslatedViewProjectionMatrix;//
	Matrix InvTranslatedViewProjectionMatrix;//

	Vector PreViewTranslation;
	Vector ViewOrigin;

public:
	inline const Matrix& GetProjectionMatrix() const
	{
		return ProjectionMatrix;
	}
	inline const Matrix& GetInvProjectionMatrix() const
	{
		return InvProjectionMatrix;
	}
	inline const Matrix& GetViewMatrix() const
	{
		return ViewMatrix;
	}
	inline const Matrix& GetInvViewMatrix() const
	{
		return InvViewMatrix;
	}
	inline const Matrix& GetViewProjectionMatrix() const
	{
		return ViewProjectionMatrix;
	}
	inline const Matrix& GetInvViewProjectionMatrix() const
	{
		return InvViewProjectionMatrix;
	}

	inline const Matrix& GetTranslatedViewMatrix() const
	{
		return TranslatedViewMatrix;
	}
	inline const Matrix& GetInvTranslatedViewMatrix() const
	{
		return InvTranslatedViewMatrix;
	}
	inline const Matrix& GetOverriddenTranslatedViewMatrix() const
	{
		return OverriddenTranslatedViewMatrix;
	}
	inline const Matrix& GetOverriddenInvTranslatedViewMatrix() const
	{
		return OverriddenInvTranslatedViewMatrix;
	}
	inline const Matrix& GetTranslatedViewProjectionMatrix() const
	{
		return TranslatedViewProjectionMatrix;
	}
	inline const Matrix& GetInvTranslatedViewProjectionMatrix() const
	{
		return InvTranslatedViewProjectionMatrix;
	}

	inline const Vector& GetPreViewTranslation() const
	{
		return PreViewTranslation;
	}
	inline const Vector& GetViewOrigin() const
	{
		return ViewOrigin;
	}

	inline bool IsPerspectiveProjection() const
	{
		return ProjectionMatrix.M[3][3] < 1.0f;
	}
	static const Matrix InvertProjectionMatrix(const Matrix& M)
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

			return Matrix(
				Plane(1.0f / a, 0.0f, 0.0f, 0.0f),
				Plane(0.0f, 1.0f / b, 0.0f, 0.0f),
				Plane(0.0f, 0.0f, 0.0f, 1.0f / d),
				Plane(-s / a, -t / b, 1.0f, -c / d)
			);
		}
		else
		{
			return M.Inverse();
		}
	}
};

#pragma pack(push)
#pragma pack(1)
struct ViewUniformShaderParameters
{
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
	LinearColor AtmosphericFogSunColor;

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

	void TransposeMatrices();
};
#pragma pack(pop)

class SceneView
{
public:
	ID3D11Buffer* ViewUniformBuffer;
	ViewMatrices mViewMatrices;
private:
	/** During GetDynamicMeshElements this will be the correct cull volume for shadow stuff */
	//const FConvexVolume* DynamicMeshElementsShadowCullFrustum;
	/** If the above is non-null, a translation that is applied to world-space before transforming by one of the shadow matrices. */
	//FVector		PreShadowTranslation;

public:

	SceneView(const ViewInitOptions& InitOptions);

	/** The actor which is being viewed from. */
	const class Actor* ViewActor;
	/* Final position of the view in the final render target (in pixels), potentially constrained by an aspect ratio requirement (black bars) */
	const IntRect UnscaledViewRect;
	/* Raw view size (in pixels), used for screen space calculations */
	IntRect UnconstrainedViewRect;

	Vector	ViewLocation;
	Rotator	ViewRotation;

	ViewMatrices ShadowViewMatrices;

	Matrix ProjectionMatrixUnadjustedForRHI;

	/** Actual field of view and that desired by the camera originally */
	float FOV;
	float DesiredFOV;

	//FConvexVolume ViewFrustum;

	bool bHasNearClippingPlane;

	//FPlane NearClippingPlane;

	float NearClippingDistance;

	/** true if ViewMatrix.Determinant() is negative. */
	bool bReverseCulling;

	/* Vector used by shaders to convert depth buffer samples into z coordinates in world space */
	Vector4 InvDeviceZToWorldZTransform;
	/**
	* The final settings for the current viewer position (blended together from many volumes).
	* Setup by the main thread, passed to the render thread and never touched again by the main thread.
	*/
	//FFinalPostProcessSettings FinalPostProcessSettings;

	// The antialiasing method.
	//EAntiAliasingMethod AntiAliasingMethod;

	// Primary screen percentage method to use.
	//EPrimaryScreenPercentageMethod PrimaryScreenPercentageMethod;

	/** Parameters for atmospheric fog. */
	//FTextureRHIRef AtmosphereTransmittanceTexture;
	//FTextureRHIRef AtmosphereIrradianceTexture;
	//FTextureRHIRef AtmosphereInscatterTexture;


	/** Sets up the view rect parameters in the view's uniform shader parameters */
	void SetupViewRectUniformBufferParameters(ViewUniformShaderParameters& ViewUniformParameters,
		const IntPoint& InBufferSize,
		const IntRect& InEffectiveViewRect,
		const ViewMatrices& InViewMatrices,
		const ViewMatrices& InPrevViewMatrice) const;

	/**
	* Populates the uniform buffer prameters common to all scene view use cases
	* View parameters should be set up in this method if they are required for the view to render properly.
	* This is to avoid code duplication and uninitialized parameters in other places that create view uniform parameters (e.g Slate)
	*/
	void SetupCommonViewUniformBufferParameters(ViewUniformShaderParameters& ViewUniformParameters,
		const IntPoint& InBufferSize,
		int32 NumMSAASamples,
		const IntRect& InEffectiveViewRect,
		const ViewMatrices& InViewMatrices,
		const ViewMatrices& InPrevViewMatrices) const;
};

class SceneViewFamily
{
public:
	class Scene* mScene;
	std::vector<const SceneView*> Views;
};