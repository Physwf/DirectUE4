#pragma once

#include "SceneComponent.h"
#include "SceneManagement.h"

enum ELightComponentType
{
	LightType_Directional = 0,
	LightType_Point,
	LightType_Spot,
	LightType_Rect,
	LightType_MAX,
	LightType_NumBits = 2
};



class Actor;
class UPrimitiveComponent;



class ULightComponent : public USceneComponent
{
public:
	ULightComponent(Actor* InOwner);
	~ULightComponent();

	float MaxDrawDistance;
	float MaxDistanceFadeRange;
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

	uint32 bMoveable : 1;
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
	/**
	* Return True if a light's parameters as well as its position is static during gameplay, and can thus use static lighting.
	* A light with HasStaticLighting() == true will always have HasStaticShadowing() == true as well.
	*/
	bool HasStaticLighting() const;
	/**
	* Whether the light has static direct shadowing.
	* The light may still have dynamic brightness and color.
	* The light may or may not also have static lighting.
	*/
	bool HasStaticShadowing() const;

	virtual class FLightSceneProxy* CreateSceneProxy() const
	{
		return NULL;
	}

	virtual void Register() override;
	virtual void Unregister() override;
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

	virtual FLightSceneProxy* CreateSceneProxy() const override;
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

	virtual FLightSceneProxy* CreateSceneProxy() const override;

};