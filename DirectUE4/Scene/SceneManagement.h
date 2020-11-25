#pragma once

#include "UnrealMath.h"

#define WORLD_MAX					2097152.0				/* Maximum size of the world */
#define HALF_WORLD_MAX				(WORLD_MAX * 0.5)		/* Half the maximum size of the world */
#define HALF_WORLD_MAX1				(HALF_WORLD_MAX - 1.0)	/* Half the maximum size of the world minus one */

class ULightComponent;
class FLightSceneInfo;

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

	void SetTransform(const FMatrix& InLightToWorld, const Vector4& InPosition);
};