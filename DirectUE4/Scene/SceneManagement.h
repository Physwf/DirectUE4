#pragma once

#include "UnrealMath.h"
#include "ConvexVolume.h"
#include "MeshBach.h"
#include "PrimitiveSceneProxy.h"


#define WORLD_MAX					2097152.0				/* Maximum size of the world */
#define HALF_WORLD_MAX				(WORLD_MAX * 0.5)		/* Half the maximum size of the world */
#define HALF_WORLD_MAX1				(HALF_WORLD_MAX - 1.0)	/* Half the maximum size of the world minus one */

class ULightComponent;
class FLightSceneInfo;
struct FViewMatrices;
class FSceneView;
class FSceneViewFamily;


/**
* The types of interactions between a light and a primitive.
*/
enum ELightInteractionType
{
	LIT_CachedIrrelevant,
	LIT_CachedLightMap,
	LIT_Dynamic,
	LIT_CachedSignedDistanceFieldShadowMap2D,

	LIT_MAX
};

/**
* Information about an interaction between a light and a mesh.
*/
class FLightInteraction
{
public:

	// Factory functions.
	static FLightInteraction Dynamic() { return FLightInteraction(LIT_Dynamic); }
	static FLightInteraction LightMap() { return FLightInteraction(LIT_CachedLightMap); }
	static FLightInteraction Irrelevant() { return FLightInteraction(LIT_CachedIrrelevant); }
	static FLightInteraction ShadowMap2D() { return FLightInteraction(LIT_CachedSignedDistanceFieldShadowMap2D); }

	// Accessors.
	ELightInteractionType GetType() const { return Type; }

	/**
	* Minimal initialization constructor.
	*/
	FLightInteraction(ELightInteractionType InType)
		: Type(InType)
	{}

private:
	ELightInteractionType Type;
};

/** The number of coefficients that are stored for each light sample. */
static const int32 NUM_STORED_LIGHTMAP_COEF = 4;

/** The number of directional coefficients which the lightmap stores for each light sample. */
static const int32 NUM_HQ_LIGHTMAP_COEF = 2;

/** The number of simple coefficients which the lightmap stores for each light sample. */
static const int32 NUM_LQ_LIGHTMAP_COEF = 2;

/** The index at which simple coefficients are stored in any array containing all NUM_STORED_LIGHTMAP_COEF coefficients. */
static const int32 LQ_LIGHTMAP_COEF_INDEX = 2;

/** The maximum value between NUM_LQ_LIGHTMAP_COEF and NUM_HQ_LIGHTMAP_COEF. */
static const int32 MAX_NUM_LIGHTMAP_COEF = 2;

/** Compile out low quality lightmaps to save memory */
// @todo-mobile: Need to fix this!
#define ALLOW_LQ_LIGHTMAPS 1

/** Compile out high quality lightmaps to save memory */
#define ALLOW_HQ_LIGHTMAPS 1

/** Make sure at least one is defined */
#if !ALLOW_LQ_LIGHTMAPS && !ALLOW_HQ_LIGHTMAPS
#error At least one of ALLOW_LQ_LIGHTMAPS and ALLOW_HQ_LIGHTMAPS needs to be defined!
#endif

enum ELightMapInteractionType
{
	LMIT_None = 0,
	LMIT_GlobalVolume = 1,
	LMIT_Texture = 2,

	LMIT_NumBits = 3
};

/**
* Information about an interaction between a light and a mesh.
*/
class FLightMapInteraction
{
public:

	// Factory functions.
	static FLightMapInteraction None()
	{
		FLightMapInteraction Result;
		Result.Type = LMIT_None;
		return Result;
	}

	static FLightMapInteraction GlobalVolume()
	{
		FLightMapInteraction Result;
		Result.Type = LMIT_GlobalVolume;
		return Result;
	}

	static FLightMapInteraction Texture(
		ID3D11ShaderResourceView* const* InTextures,
		ID3D11ShaderResourceView* InSkyOcclusionTexture,
		ID3D11ShaderResourceView* InAOMaterialMaskTexture,
		const Vector4* InCoefficientScales,
		const Vector4* InCoefficientAdds,
		const Vector2& InCoordinateScale,
		const Vector2& InCoordinateBias,
		bool bAllowHighQualityLightMaps);

	/** Default constructor. */
	FLightMapInteraction() :
		SkyOcclusionTexture(NULL),
		AOMaterialMaskTexture(NULL),
		Type(LMIT_None)
	{}

	// Accessors.
	ELightMapInteractionType GetType() const { return Type; }

	ID3D11ShaderResourceView* GetTexture(bool bHighQuality) const
	{
		assert(Type == LMIT_Texture);
#if ALLOW_LQ_LIGHTMAPS && ALLOW_HQ_LIGHTMAPS
		return bHighQuality ? HighQualityTexture : LowQualityTexture;
#elif ALLOW_HQ_LIGHTMAPS
		return HighQualityTexture;
#else
		return LowQualityTexture;
#endif
	}

	ID3D11ShaderResourceView* GetSkyOcclusionTexture() const
	{
		assert(Type == LMIT_Texture);
#if ALLOW_HQ_LIGHTMAPS
		return SkyOcclusionTexture;
#else
		return NULL;
#endif
	}

	ID3D11ShaderResourceView* GetAOMaterialMaskTexture() const
	{
		assert(Type == LMIT_Texture);
#if ALLOW_HQ_LIGHTMAPS
		return AOMaterialMaskTexture;
#else
		return NULL;
#endif
	}

	const Vector4* GetScaleArray() const
	{
#if ALLOW_LQ_LIGHTMAPS && ALLOW_HQ_LIGHTMAPS
		return AllowsHighQualityLightmaps() ? HighQualityCoefficientScales : LowQualityCoefficientScales;
#elif ALLOW_HQ_LIGHTMAPS
		return HighQualityCoefficientScales;
#else
		return LowQualityCoefficientScales;
#endif
	}

	const Vector4* GetAddArray() const
	{
#if ALLOW_LQ_LIGHTMAPS && ALLOW_HQ_LIGHTMAPS
		return AllowsHighQualityLightmaps() ? HighQualityCoefficientAdds : LowQualityCoefficientAdds;
#elif ALLOW_HQ_LIGHTMAPS
		return HighQualityCoefficientAdds;
#else
		return LowQualityCoefficientAdds;
#endif
	}

	const Vector2& GetCoordinateScale() const
	{
		assert(Type == LMIT_Texture);
		return CoordinateScale;
	}
	const Vector2& GetCoordinateBias() const
	{
		assert(Type == LMIT_Texture);
		return CoordinateBias;
	}

	uint32 GetNumLightmapCoefficients() const
	{
#if ALLOW_LQ_LIGHTMAPS && ALLOW_HQ_LIGHTMAPS
		return NumLightmapCoefficients;
#elif ALLOW_HQ_LIGHTMAPS
		return NUM_HQ_LIGHTMAP_COEF;
#else
		return NUM_LQ_LIGHTMAP_COEF;
#endif
	}

	/**
	* @return true if high quality lightmaps are allowed
	*/
	inline bool AllowsHighQualityLightmaps() const
	{
#if ALLOW_LQ_LIGHTMAPS && ALLOW_HQ_LIGHTMAPS
		return bAllowHighQualityLightMaps;
#elif ALLOW_HQ_LIGHTMAPS
		return true;
#else
		return false;
#endif
	}

	/** These functions are used for the Dummy lightmap policy used in LightMap density view mode. */
	/**
	*	Set the type.
	*
	*	@param	InType				The type to set it to.
	*/
	void SetLightMapInteractionType(ELightMapInteractionType InType)
	{
		Type = InType;
	}
	/**
	*	Set the coordinate scale.
	*
	*	@param	InCoordinateScale	The scale to set it to.
	*/
	void SetCoordinateScale(const Vector2& InCoordinateScale)
	{
		CoordinateScale = InCoordinateScale;
	}
	/**
	*	Set the coordinate bias.
	*
	*	@param	InCoordinateBias	The bias to set it to.
	*/
	void SetCoordinateBias(const Vector2& InCoordinateBias)
	{
		CoordinateBias = InCoordinateBias;
	}

private:

#if ALLOW_HQ_LIGHTMAPS
	Vector4 HighQualityCoefficientScales[NUM_HQ_LIGHTMAP_COEF];
	Vector4 HighQualityCoefficientAdds[NUM_HQ_LIGHTMAP_COEF];
	ID3D11ShaderResourceView* HighQualityTexture;
	ID3D11ShaderResourceView* SkyOcclusionTexture;
	ID3D11ShaderResourceView* AOMaterialMaskTexture;
#endif

#if ALLOW_LQ_LIGHTMAPS
	Vector4 LowQualityCoefficientScales[NUM_LQ_LIGHTMAP_COEF];
	Vector4 LowQualityCoefficientAdds[NUM_LQ_LIGHTMAP_COEF];
	ID3D11ShaderResourceView* LowQualityTexture;
#endif

#if ALLOW_LQ_LIGHTMAPS && ALLOW_HQ_LIGHTMAPS
	bool bAllowHighQualityLightMaps;
	uint32 NumLightmapCoefficients;
#endif

	ELightMapInteractionType Type;

	Vector2 CoordinateScale;
	Vector2 CoordinateBias;
};

enum EShadowMapInteractionType
{
	SMIT_None = 0,
	SMIT_GlobalVolume = 1,
	SMIT_Texture = 2,

	SMIT_NumBits = 3
};

/** Information about the static shadowing information for a primitive. */
class FShadowMapInteraction
{
public:

	// Factory functions.
	static FShadowMapInteraction None()
	{
		FShadowMapInteraction Result;
		Result.Type = SMIT_None;
		return Result;
	}

	static FShadowMapInteraction GlobalVolume()
	{
		FShadowMapInteraction Result;
		Result.Type = SMIT_GlobalVolume;
		return Result;
	}

	static FShadowMapInteraction Texture(
		ID3D11ShaderResourceView* InTexture,
		const Vector2& InCoordinateScale,
		const Vector2& InCoordinateBias,
		const bool* InChannelValid,
		const Vector4& InInvUniformPenumbraSize)
	{
		FShadowMapInteraction Result;
		Result.Type = SMIT_Texture;
		Result.ShadowTexture = InTexture;
		Result.CoordinateScale = InCoordinateScale;
		Result.CoordinateBias = InCoordinateBias;
		Result.InvUniformPenumbraSize = InInvUniformPenumbraSize;

		for (int Channel = 0; Channel < 4; Channel++)
		{
			Result.bChannelValid[Channel] = InChannelValid[Channel];
		}

		return Result;
	}

	/** Default constructor. */
	FShadowMapInteraction() :
		ShadowTexture(nullptr),
		InvUniformPenumbraSize(Vector4(0, 0, 0, 0)),
		Type(SMIT_None)
	{
		for (int Channel = 0; Channel < 4/*ARRAY_COUNT(bChannelValid)*/; Channel++)
		{
			bChannelValid[Channel] = false;
		}
	}

	// Accessors.
	EShadowMapInteractionType GetType() const { return Type; }

	ID3D11ShaderResourceView* GetTexture() const
	{
		assert(Type == SMIT_Texture);
		return ShadowTexture;
	}

	const Vector2& GetCoordinateScale() const
	{
		assert(Type == SMIT_Texture);
		return CoordinateScale;
	}

	const Vector2& GetCoordinateBias() const
	{
		assert(Type == SMIT_Texture);
		return CoordinateBias;
	}

	bool GetChannelValid(int32 ChannelIndex) const
	{
		assert(Type == SMIT_Texture);
		return bChannelValid[ChannelIndex];
	}

	inline Vector4 GetInvUniformPenumbraSize() const
	{
		return InvUniformPenumbraSize;
	}

private:
	ID3D11ShaderResourceView* ShadowTexture;
	Vector2 CoordinateScale;
	Vector2 CoordinateBias;
	bool bChannelValid[4];
	Vector4 InvUniformPenumbraSize;
	EShadowMapInteractionType Type;
};

class FLightMap;
class FShadowMap;
/**
* An interface to cached lighting for a specific mesh.
*/
class FLightCacheInterface
{
public:
	FLightCacheInterface(const FLightMap* InLightMap, const FShadowMap* InShadowMap)
		: bGlobalVolumeLightmap(false)
		, LightMap(InLightMap)
		, ShadowMap(InShadowMap)
	{
	}

	virtual ~FLightCacheInterface()
	{
	}

	// @param LightSceneProxy must not be 0
	virtual FLightInteraction GetInteraction(const class FLightSceneProxy* LightSceneProxy) const = 0;

	// helper function to implement GetInteraction(), call after checking for this: if(LightSceneProxy->HasStaticShadowing())
	// @param LightSceneProxy same as in GetInteraction(), must not be 0
	ELightInteractionType GetStaticInteraction(const FLightSceneProxy* LightSceneProxy, const std::vector<uint32>& IrrelevantLights) const;

	// @param InLightMap may be 0
	void SetLightMap(const FLightMap* InLightMap)
	{
		LightMap = InLightMap;
	}

	// @return may be 0
	const FLightMap* GetLightMap() const
	{
		return LightMap;
	}

	// @param InShadowMap may be 0
	void SetShadowMap(const FShadowMap* InShadowMap)
	{
		ShadowMap = InShadowMap;
	}

	// @return may be 0
	const FShadowMap* GetShadowMap() const
	{
		return ShadowMap;
	}

	void SetGlobalVolumeLightmap(bool bInGlobalVolumeLightmap)
	{
		bGlobalVolumeLightmap = bInGlobalVolumeLightmap;
	}

	// WARNING : This can be called with buffers valid for a single frame only, don't cache anywhere. See FPrimitiveSceneInfo::UpdatePrecomputedLightingBuffer()
	void SetPrecomputedLightingBuffer(FUniformBuffer* InPrecomputedLightingUniformBuffer)
	{
		PrecomputedLightingUniformBuffer = InPrecomputedLightingUniformBuffer;
	}

	FUniformBuffer* GetPrecomputedLightingBuffer() const
	{
		return PrecomputedLightingUniformBuffer;
	}

	FLightMapInteraction GetLightMapInteraction() const;

	FShadowMapInteraction GetShadowMapInteraction() const;

private:

	bool bGlobalVolumeLightmap;

	// The light-map used by the element. may be 0
	const FLightMap* LightMap;

	// The shadowmap used by the element, may be 0
	const FShadowMap* ShadowMap;

	/** The uniform buffer holding mapping the lightmap policy resources. */
	FUniformBuffer* PrecomputedLightingUniformBuffer;
};

// Information about a single shadow cascade.
class FShadowCascadeSettings
{
public:
	// The following 3 floats represent the view space depth of the split planes for this cascade.
	// SplitNear <= FadePlane <= SplitFar

	// The distance from the camera to the near split plane, in world units (linear).
	float SplitNear;

	// The distance from the camera to the far split plane, in world units (linear).
	float SplitFar;

	// in world units (linear).
	float SplitNearFadeRegion;

	// in world units (linear).
	float SplitFarFadeRegion;

	// ??
	// The distance from the camera to the start of the fade region, in world units (linear).
	// The area between the fade plane and the far split plane is blended to smooth between cascades.
	float FadePlaneOffset;

	// The length of the fade region (SplitFar - FadePlaneOffset), in world units (linear).
	float FadePlaneLength;

	// The accurate bounds of the cascade used for primitive culling.
	FConvexVolume ShadowBoundsAccurate;

	FPlane NearFrustumPlane;
	FPlane FarFrustumPlane;

	/** When enabled, the cascade only renders objects marked with bCastFarShadows enabled (e.g. Landscape). */
	bool bFarShadowCascade;

	/**
	* Index of the split if this is a whole scene shadow from a directional light,
	* Or index of the direction if this is a whole scene shadow from a point light, otherwise INDEX_NONE.
	*/
	int32 ShadowSplitIndex;

	FShadowCascadeSettings()
		: SplitNear(0.0f)
		, SplitFar(WORLD_MAX)
		, SplitNearFadeRegion(0.0f)
		, SplitFarFadeRegion(0.0f)
		, FadePlaneOffset(SplitFar)
		, FadePlaneLength(SplitFar - FadePlaneOffset)
		, bFarShadowCascade(false)
		, ShadowSplitIndex(-1)
	{
	}
};

/** A projected shadow transform. */
class FProjectedShadowInitializer
{
public:

	/** A translation that is applied to world-space before transforming by one of the shadow matrices. */
	FVector PreShadowTranslation;

	FMatrix WorldToLight;
	/** Non-uniform scale to be applied after WorldToLight. */
	FVector Scales;

	FVector FaceDirection;
	FBoxSphereBounds SubjectBounds;
	Vector4 WAxis;
	float MinLightW;
	float MaxDistanceToCastInLightW;

	/** Default constructor. */
	FProjectedShadowInitializer()
	{}

	bool IsCachedShadowValid(const FProjectedShadowInitializer& CachedShadow) const
	{
		return PreShadowTranslation == CachedShadow.PreShadowTranslation
			&& WorldToLight == CachedShadow.WorldToLight
			&& Scales == CachedShadow.Scales
			&& FaceDirection == CachedShadow.FaceDirection
			&& SubjectBounds.Origin == CachedShadow.SubjectBounds.Origin
			&& SubjectBounds.BoxExtent == CachedShadow.SubjectBounds.BoxExtent
			&& SubjectBounds.SphereRadius == CachedShadow.SubjectBounds.SphereRadius
			&& WAxis == CachedShadow.WAxis
			&& MinLightW == CachedShadow.MinLightW
			&& MaxDistanceToCastInLightW == CachedShadow.MaxDistanceToCastInLightW;
	}
};

/** Information needed to create a per-object projected shadow. */
class FPerObjectProjectedShadowInitializer : public FProjectedShadowInitializer
{
public:

};

/** Information needed to create a whole scene projected shadow. */
class FWholeSceneProjectedShadowInitializer : public FProjectedShadowInitializer
{
public:
	FShadowCascadeSettings CascadeSettings;
	bool bOnePassPointLightShadow;
	bool bRayTracedDistanceField;

	FWholeSceneProjectedShadowInitializer() :
		bOnePassPointLightShadow(false),
		bRayTracedDistanceField(false)
	{}

	bool IsCachedShadowValid(const FWholeSceneProjectedShadowInitializer& CachedShadow) const
	{
		return FProjectedShadowInitializer::IsCachedShadowValid((const FProjectedShadowInitializer&)CachedShadow)
			&& bOnePassPointLightShadow == CachedShadow.bOnePassPointLightShadow
			&& bRayTracedDistanceField == CachedShadow.bRayTracedDistanceField;
	}
};

/** Represents a USkyLightComponent to the rendering thread. */
class FSkyLightSceneProxy
{
public:

	/** Initialization constructor. */
	FSkyLightSceneProxy(const class USkyLightComponent* InLightComponent);

	void Initialize(
		float InBlendFraction,
		//const FSHVectorRGB3* InIrradianceEnvironmentMap,
		//const FSHVectorRGB3* BlendDestinationIrradianceEnvironmentMap,
		const float* InAverageBrightness,
		const float* BlendDestinationAverageBrightness);

	const USkyLightComponent* LightComponent;
	//FTexture* ProcessedTexture;
	float BlendFraction;
	//FTexture* BlendDestinationProcessedTexture;
	float SkyDistanceThreshold;
	bool bCastShadows;
	bool bWantsStaticShadowing;
	bool bHasStaticLighting;
	bool bCastVolumetricShadow;
	FLinearColor LightColor;
	//FSHVectorRGB3 IrradianceEnvironmentMap;
	float AverageBrightness;
	float IndirectLightingIntensity;
	float VolumetricScatteringIntensity;
	float OcclusionMaxDistance;
	float Contrast;
	float OcclusionExponent;
	float MinOcclusion;
	FLinearColor OcclusionTint;
	EOcclusionCombineMode OcclusionCombineMode;
};

class FLightSceneProxy
{
public:
	FLightSceneProxy(const ULightComponent* InLightComponent);

	virtual ~FLightSceneProxy()
	{

	}

	virtual bool AffectsBounds(const FBoxSphereBounds& Bounds) const
	{
		return true;
	}

	virtual FSphere GetBoundingSphere() const
	{
		// Directional lights will have a radius of WORLD_MAX
		return FSphere(FVector::ZeroVector, WORLD_MAX);
	}

	virtual float GetRadius() const { return FLT_MAX; }
	virtual float GetOuterConeAngle() const { return 0.0f; }
	virtual float GetSourceRadius() const { return 0.0f; }
	virtual bool IsInverseSquared() const { return true; }
	virtual bool IsRectLight() const { return false; }
	virtual float GetLightSourceAngle() const { return 0.0f; }
	virtual float GetEffectiveScreenRadius(const FViewMatrices& ShadowViewMatrices) const { return 0.0f; }
	/** Whether this light should create per object shadows for dynamic objects. */
	virtual bool ShouldCreatePerObjectShadowsForDynamicObjects() const;

	/** Whether this light should create CSM for dynamic objects only (forward renderer) */
	virtual bool UseCSMForDynamicObjects() const;

	/** Returns the number of view dependent shadows this light will create, not counting distance field shadow cascades. */
	virtual uint32 GetNumViewDependentWholeSceneShadows(const FSceneView& View, bool bPrecomputedLightingIsValid) const { return 0; }
	/**
	* Sets up a projected shadow initializer for shadows from the entire scene.
	* @return True if the whole-scene projected shadow should be used.
	*/
	virtual bool GetWholeSceneProjectedShadowInitializer(const FSceneViewFamily& ViewFamily, std::vector<class FWholeSceneProjectedShadowInitializer>& OutInitializers) const
	{
		return false;
	}
	virtual bool GetViewDependentWholeSceneProjectedShadowInitializer(
		const class FSceneView& View,
		int32 InCascadeIndex,
		bool bPrecomputedLightingIsValid,
		class FWholeSceneProjectedShadowInitializer& OutInitializer) const
	{
		return false;
	}
	/**
	* Sets up a projected shadow initializer for the given subject.
	* @param SubjectBounds - The bounding volume of the subject.
	* @param OutInitializer - Upon successful return, contains the initialization parameters for the shadow.
	* @return True if a projected shadow should be cast by this subject-light pair.
	*/
	virtual bool GetPerObjectProjectedShadowInitializer(const FBoxSphereBounds& SubjectBounds, class FPerObjectProjectedShadowInitializer& OutInitializer) const
	{
		return false;
	}
	virtual FSphere GetShadowSplitBounds(const class FSceneView& View, int32 InCascadeIndex, bool bPrecomputedLightingIsValid, FShadowCascadeSettings* OutCascadeSettings) const { return FSphere(FVector::ZeroVector, 0); }
	virtual FSphere GetShadowSplitBoundsDepthRange(const FSceneView& View, FVector ViewOrigin, float SplitNear, float SplitFar, FShadowCascadeSettings* OutCascadeSettings) const { return FSphere(FVector::ZeroVector, 0); }

	inline const ULightComponent* GetLightComponent() const { return LightComponent; }
	inline FLightSceneInfo* GetLightSceneInfo() const { return LightSceneInfo; }
	inline const FMatrix& GetWorldToLight() const { return WorldToLight; }
	inline const FMatrix& GetLightToWorld() const { return LightToWorld; }
	inline FVector GetDirection() const { return FVector(WorldToLight.M[0][0], WorldToLight.M[1][0], WorldToLight.M[2][0]); }
	inline FVector GetOrigin() const { return LightToWorld.GetOrigin(); }
	inline Vector4 GetPosition() const { return Position; }
	inline const FLinearColor& GetColor() const { return Color; }
	inline bool HasStaticLighting() const { return bStaticLighting; }
	inline bool HasStaticShadowing() const { return bStaticShadowing; }
	inline bool CastsDynamicShadow() const { return bCastDynamicShadow; }
	inline bool CastsStaticShadow() const { return bCastStaticShadow; }
	inline bool CastsTranslucentShadows() const { return bCastTranslucentShadows; }
	inline uint8 GetLightType() const { return LightType; }
	inline uint32 GetLightGuid() const { return LightGuid; }
	virtual float GetMaxDrawDistance() const { return 0.0f; }
	virtual float GetFadeRange() const { return 0.0f; }
protected:
	friend class FScene;

	/** The light component. */
	const ULightComponent* LightComponent;

	/** The light's scene info. */
	class FLightSceneInfo* LightSceneInfo;

	/** A transform from world space into light space. */
	FMatrix WorldToLight;

	/** A transform from light space into world space. */
	FMatrix LightToWorld;

	/** The homogenous position of the light. */
	Vector4 Position;

	/** The light color. */
	FLinearColor Color;

	float ShadowBias;

	const uint32 bStaticLighting : 1;
	/**
	* Whether the light has static direct shadowing.
	* The light may still have dynamic brightness and color.
	* The light may or may not also have static lighting.
	*/
	const uint32 bStaticShadowing : 1;

	/** True if the light casts dynamic shadows. */
	const uint32 bCastDynamicShadow : 1;

	/** True if the light casts static shadows. */
	const uint32 bCastStaticShadow : 1;

	/** Whether the light is allowed to cast dynamic shadows from translucency. */
	const uint32 bCastTranslucentShadows : 1;

	const uint8 LightType;

	uint32 LightGuid;

	void SetTransform(const FMatrix& InLightToWorld, const Vector4& InPosition);
};

/**
* A reference to a mesh batch that is added to the collector, together with some cached relevance flags.
*/
struct FMeshBatchAndRelevance
{
	const FMeshBatch* Mesh;

	/** The render info for the primitive which created this mesh, required. */
	const FPrimitiveSceneProxy* PrimitiveSceneProxy;

private:
	/**
	* Cached usage information to speed up traversal in the most costly passes (depth-only, base pass, shadow depth),
	* This is done so the Mesh does not have to be dereferenced to determine pass relevance.
	*/
	uint32 bHasOpaqueMaterial : 1;
	uint32 bHasMaskedMaterial : 1;
	uint32 bRenderInMainPass : 1;

public:
	FMeshBatchAndRelevance(const FMeshBatch& InMesh, const FPrimitiveSceneProxy* InPrimitiveSceneProxy);

	bool GetHasOpaqueMaterial() const { return bHasOpaqueMaterial; }
	bool GetHasMaskedMaterial() const { return bHasMaskedMaterial; }
	bool GetHasOpaqueOrMaskedMaterial() const { return bHasOpaqueMaterial || bHasMaskedMaterial; }
	bool GetRenderInMainPass() const { return bRenderInMainPass; }
};