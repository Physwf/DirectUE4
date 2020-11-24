#pragma once

#include "SceneComponent.h"

enum ELightComponentType
{
	LightType_Directional = 0,
	LightType_Point,
	LightType_Spot,
	LightType_Rect,
	LightType_MAX,
	LightType_NumBits = 2
};

#define WORLD_MAX					2097152.0				/* Maximum size of the world */
#define HALF_WORLD_MAX				(WORLD_MAX * 0.5)		/* Half the maximum size of the world */
#define HALF_WORLD_MAX1				(HALF_WORLD_MAX - 1.0)	/* Half the maximum size of the world minus one */

class Actor;
class UPrimitiveComponent;

class ULightComponent : public USceneComponent
{
public:
	ULightComponent(Actor* InOwner);
	~ULightComponent();
/************************************************************************/
/* Component                                                            */
/************************************************************************/
	/**
	* Total energy that the light emits.
	*/
	float Intensity;
	/**
	* Filter color of the light.
	* Note that this can change the light's effective intensity.
	*/
	FColor LightColor;
	/**
	* Whether the light should cast any shadows.
	**/
	uint32 CastShadows : 1;
	/**
	* Whether the light should cast shadows from static objects.  Also requires Cast Shadows to be set to True.
	*/
	uint32 CastStaticShadows : 1;
	/**
	* Whether the light should cast shadows from dynamic objects.  Also requires Cast Shadows to be set to True.
	**/
	uint32 CastDynamicShadows : 1;
	/** Whether the light affects translucency or not.  Disabling this can save GPU time when there are many small lights. */
	uint32 bAffectTranslucentLighting : 1;
	/**
	* Multiplier on specular highlights. Use only with great care! Any value besides 1 is not physical!
	* Can be used to artistically remove highlights mimicking polarizing filters or photo touch up.
	*/
	float SpecularScale;
	/**
	* Controls how accurate self shadowing of whole scene shadows from this light are.
	* At 0, shadows will start at the their caster surface, but there will be many self shadowing artifacts.
	* larger values, shadows will start further from their caster, and there won't be self shadowing artifacts but object might appear to fly.
	* around 0.5 seems to be a good tradeoff. This also affects the soft transition of shadows
	*/
	float ShadowBias;
	/** Amount to sharpen shadow filtering */
	float ShadowSharpen;
	/** Whether the light is allowed to cast dynamic shadows from translucency. */
	uint32 CastTranslucentShadows : 1;
	bool AffectsPrimitive(const UPrimitiveComponent* Primitive) const;
	virtual bool AffectsBounds(const FBoxSphereBounds& InBounds) const;
	/**
	* Return the world-space bounding box of the light's influence.
	*/
	virtual FBox GetBoundingBox() const { return FBox(FVector(-HALF_WORLD_MAX, -HALF_WORLD_MAX, -HALF_WORLD_MAX), FVector(HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX)); }
	virtual FSphere GetBoundingSphere() const
	{
		// Directional lights will have a radius of WORLD_MAX
		return FSphere(FVector::ZeroVector, WORLD_MAX);
	}
	/**
	* Return the homogenous position of the light.
	*/
	virtual Vector4 GetLightPosition() const = 0;
	/**
	* @return ELightComponentType for the light component class
	*/
	virtual ELightComponentType GetLightType() const = 0;
	FVector GetDirection() const;

	virtual void Register() override;
	virtual void Unregister() override;
/************************************************************************/
/* Proxy                                                                */
/************************************************************************/
protected:
	friend class FScene;
	class FLightSceneInfo* LightSceneInfo;
	/** A transform from world space into light space. */
	FMatrix WorldToLight;
	/** A transform from light space into world space. */
	FMatrix LightToWorld;
	/** The homogenous position of the light. */
	Vector4 Position;
	/** The light color. */
	FLinearColor Color;
	/** User setting from light component, 0:no bias, 0.5:reasonable, larger object might appear to float */
	//float ShadowBias;
	/** Sharpen shadow filtering */
	//float ShadowSharpen;

// 	const uint32 bMovable : 1;
// 	/**
// 	* Return True if a light's parameters as well as its position is static during gameplay, and can thus use static lighting.
// 	* A light with HasStaticLighting() == true will always have HasStaticShadowing() == true as well.
// 	*/
// 	const uint32 bStaticLighting : 1;
// 	/**
// 	* Whether the light has static direct shadowing.
// 	* The light may still have dynamic brightness and color.
// 	* The light may or may not also have static lighting.
// 	*/
// 	const uint32 bStaticShadowing : 1;
// 	/** True if the light casts dynamic shadows. */
// 	const uint32 bCastDynamicShadow : 1;
// 	/** True if the light casts static shadows. */
// 	const uint32 bCastStaticShadow : 1;
// 	/** Whether the light is allowed to cast dynamic shadows from translucency. */
// 	const uint32 bCastTranslucentShadows : 1;

	//const uint8 LightType;

	void SetTransform(const FMatrix& InLightToWorld, const Vector4& InPosition);
public:
// 	inline FLightSceneInfo* GetLightSceneInfo() const { return LightSceneInfo; }
// 	inline const FMatrix& GetWorldToLight() const { return WorldToLight; }
// 	inline const FMatrix& GetLightToWorld() const { return LightToWorld; }
// 	//inline FVector GetDirection() const { return FVector(WorldToLight.M[0][0], WorldToLight.M[1][0], WorldToLight.M[2][0]); }
// 	inline FVector GetOrigin() const { return LightToWorld.GetOrigin(); }
// 	inline Vector4 GetPosition() const { return Position; }
// 	inline const FLinearColor& GetColor() const { return Color; }
// 
// 	inline bool HasStaticLighting() const { return bStaticLighting; }
// 	inline bool HasStaticShadowing() const { return bStaticShadowing; }
// 	inline bool CastsDynamicShadow() const { return bCastDynamicShadow; }
// 	inline bool CastsStaticShadow() const { return bCastStaticShadow; }
// 	inline bool CastsTranslucentShadows() const { return bCastTranslucentShadows; }

	//inline uint8 GetLightType() const { return LightType; }
};

class UDirectionalLightComponent : public ULightComponent
{
public:
	UDirectionalLightComponent(Actor* InOwner);
	~UDirectionalLightComponent();
	/**
	* How far Cascaded Shadow Map dynamic shadows will cover for a movable light, measured from the camera.
	* A value of 0 disables the dynamic shadow.
	*/
	float DynamicShadowDistanceMovableLight;
	/**
	* How far Cascaded Shadow Map dynamic shadows will cover for a stationary light, measured from the camera.
	* A value of 0 disables the dynamic shadow.
	*/
	float DynamicShadowDistanceStationaryLight;
	/**
	* Number of cascades to split the view frustum into for the whole scene dynamic shadow.
	* More cascades result in better shadow resolution, but adds significant rendering cost.
	*/
	int32 DynamicShadowCascades;
	/**
	* Angle subtended by light source in degrees (also known as angular diameter).
	* Defaults to 0.5357 which is the angle for our sun.
	*/
	float LightSourceAngle;
	/**
	* Angle subtended by soft light source in degrees.
	*/
	float LightSourceSoftAngle;
public:
	virtual Vector4 GetLightPosition() const override;
	virtual ELightComponentType GetLightType() const override;
};

class ULocalLightComponent : public ULightComponent
{
public:
	ULocalLightComponent(Actor* InOwner);
	~ULocalLightComponent();

	float AttenuationRadius;
	virtual bool AffectsBounds(const FBoxSphereBounds& InBounds) const override;
	virtual Vector4 GetLightPosition() const override;
	virtual FBox GetBoundingBox() const override;
	virtual FSphere GetBoundingSphere() const override;
};

class UPointLightComponent : public ULocalLightComponent
{
public:
	UPointLightComponent(Actor* InOwner);
	~UPointLightComponent();
	/**
	* Whether to use physically based inverse squared distance falloff, where AttenuationRadius is only clamping the light's contribution.
	* Disabling inverse squared falloff can be useful when placing fill lights (don't want a super bright spot near the light).
	* When enabled, the light's Intensity is in units of lumens, where 1700 lumens is a 100W lightbulb.
	* When disabled, the light's Intensity is a brightness scale.
	*/
	uint32 bUseInverseSquaredFalloff : 1;
	/**
	* Controls the radial falloff of the light when UseInverseSquaredFalloff is disabled.
	* 2 is almost linear and very unrealistic and around 8 it looks reasonable.
	* With large exponents, the light has contribution to only a small area of its influence radius but still costs the same as low exponents.
	*/
	float LightFalloffExponent;
	/**
	* Radius of light source shape.
	* Note that light sources shapes which intersect shadow casting geometry can cause shadowing artifacts.
	*/
	float SourceRadius;
	/**
	* Soft radius of light source shape.
	* Note that light sources shapes which intersect shadow casting geometry can cause shadowing artifacts.
	*/
	float SoftSourceRadius;
	/**
	* Length of light source shape.
	* Note that light sources shapes which intersect shadow casting geometry can cause shadowing artifacts.
	*/
	float SourceLength;
public:
	virtual ELightComponentType GetLightType() const override;
};