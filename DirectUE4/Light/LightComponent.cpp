#include "LightComponent.h"
#include "World.h"
#include "Scene.h"
#include "SceneManagement.h"


ULightComponentBase::ULightComponentBase(class AActor* InOwner)
	:USceneComponent(InOwner)
{

}

ULightComponentBase::~ULightComponentBase()
{

}

bool ULightComponentBase::HasStaticLighting() const
{
	AActor* Owner = GetOwner();

	return Owner && (Mobility == EComponentMobility::Static);
}

bool ULightComponentBase::HasStaticShadowing() const
{
	AActor* Owner = GetOwner();

	return Owner && (Mobility != EComponentMobility::Movable);
}

ULightComponent::ULightComponent(class AActor* InOwner)
	: ULightComponentBase(InOwner)
{
	bMoveable = true;

	MaxDrawDistance = 0.f;
	MaxDistanceFadeRange = 0.f;
	Intensity = 100.f;
	LightColor = FColor::White;
	CastShadows = true;
	CastStaticShadows = true;
	CastDynamicShadows = true;
	bAffectTranslucentLighting = false;

	bMoveable = true;
	ShadowBias = 0.25f;
	ShadowSharpen = 1.f;
	CastTranslucentShadows = false;
	SpecularScale = 1.f;
}

ULightComponent::~ULightComponent()
{

}

bool ULightComponent::AffectsPrimitive(const UPrimitiveComponent* Primitive) const
{
	//return AffectsBounds(Primitive->Bounds);
	return true;
}

bool ULightComponent::AffectsBounds(const FBoxSphereBounds& InBounds) const
{
	return true;
}

FVector ULightComponent::GetDirection() const
{
	return GetComponentTransform().GetUnitAxis(EAxis::X);
}

void ULightComponent::CreateRenderState_Concurrent()
{
	GetWorld()->Scene->AddLight(this);
	OnRegister();
}

void ULightComponent::SendRenderTransform_Concurrent()
{
	GetWorld()->Scene->UpdateLightTransform(this);
	UActorComponent::SendRenderTransform_Concurrent();
}

void ULightComponent::DestroyRenderState_Concurrent()
{
	OnUnregister();
	GetWorld()->Scene->RemoveLight(this);
}

float ULightComponent::ComputeLightBrightness() const
{
	float LightBrightness = Intensity;

// 	if (IESTexture)
// 	{
// 		if (bUseIESBrightness)
// 		{
// 			LightBrightness = IESTexture->Brightness * IESBrightnessScale;
// 		}
// 
// 		LightBrightness *= IESTexture->TextureMultiplier;
// 	}

	return LightBrightness;
}

static float GMaxCSMRadiusToAllowPerObjectShadows = 8000;
class FDirectionalLightSceneProxy : public FLightSceneProxy
{
public:
	/**
	* Radius of the whole scene dynamic shadow centered on the viewer, which replaces the precomputed shadows based on distance from the camera.
	* A Radius of 0 disables the dynamic shadow.
	*/
	float WholeSceneDynamicShadowRadius;
	/**
	* Number of cascades to split the view frustum into for the whole scene dynamic shadow.
	* More cascades result in better shadow resolution and allow WholeSceneDynamicShadowRadius to be further, but add rendering cost.
	*/
	uint32 DynamicShadowCascades;

	bool bUseInsetShadowsForMovableObjects;

	FDirectionalLightSceneProxy(const UDirectionalLightComponent* Component) :
		FLightSceneProxy(Component)
	{

	}

};

UDirectionalLightComponent::UDirectionalLightComponent(AActor* InOwner)
	: ULightComponent(InOwner)
{

}

UDirectionalLightComponent::~UDirectionalLightComponent()
{

}

Vector4 UDirectionalLightComponent::GetLightPosition() const
{
	return Vector4(-GetDirection() * WORLD_MAX, 0);
}

ELightComponentType UDirectionalLightComponent::GetLightType() const
{
	return LightType_Directional;
}

FLightSceneProxy* UDirectionalLightComponent::CreateSceneProxy() const
{
	return new FDirectionalLightSceneProxy(this);
}

ULocalLightComponent::ULocalLightComponent(AActor* InOwner)
	: ULightComponent(InOwner)
{
	AttenuationRadius = 1000.f;
	IntensityUnits = ELightUnits::Lumens;
}

ULocalLightComponent::~ULocalLightComponent()
{

}

bool ULocalLightComponent::AffectsBounds(const FBoxSphereBounds& InBounds) const
{
	if ((InBounds.Origin - GetComponentTransform().GetLocation()).SizeSquared() > FMath::Square(AttenuationRadius + InBounds.SphereRadius))
	{
		return false;
	}

	if (!ULightComponent::AffectsBounds(InBounds))
	{
		return false;
	}

	return true;
}

Vector4 ULocalLightComponent::GetLightPosition() const
{
	return Vector4(GetComponentTransform().GetLocation(), 1);
}

FBox ULocalLightComponent::GetBoundingBox() const
{
	return FBox(GetComponentLocation() - FVector(AttenuationRadius, AttenuationRadius, AttenuationRadius), GetComponentLocation() + FVector(AttenuationRadius, AttenuationRadius, AttenuationRadius));
}

FSphere ULocalLightComponent::GetBoundingSphere() const
{
	return FSphere(GetComponentTransform().GetLocation(), AttenuationRadius);
}

void ULocalLightComponent::SendRenderTransform_Concurrent()
{
	ULightComponent::SendRenderTransform_Concurrent();
}

UPointLightComponent::UPointLightComponent(AActor* InOwner)
	: ULocalLightComponent(InOwner)
{
	bUseInverseSquaredFalloff = true;
	LightFalloffExponent = 5.f;
	SourceRadius = 1000.f;
	SoftSourceRadius = 100.f;
	SourceLength = 10.f;
	LightColor = FColor::White;

}

UPointLightComponent::~UPointLightComponent()
{

}

ELightComponentType UPointLightComponent::GetLightType() const
{
	return LightType_Point;
}

FLightSceneProxy* UPointLightComponent::CreateSceneProxy() const
{
	return new FPointLightSceneProxy(this);
}

float UPointLightComponent::ComputeLightBrightness() const
{
	float LightBrightness = ULocalLightComponent::ComputeLightBrightness();

	if (bUseInverseSquaredFalloff)
	{
		if (IntensityUnits == ELightUnits::Candelas)
		{
			LightBrightness *= (100.f * 100.f); // Conversion from cm2 to m2
		}
		else if (IntensityUnits == ELightUnits::Lumens)
		{
			LightBrightness *= (100.f * 100.f / 4 / PI); // Conversion from cm2 to m2 and 4PI from the sphere area in the 1/r2 attenuation
		}
		else
		{
			LightBrightness *= 16; // Legacy scale of 16
		}
	}
	return LightBrightness;
}

bool FPointLightSceneProxy::GetWholeSceneProjectedShadowInitializer(const FSceneViewFamily& ViewFamily, std::vector<FWholeSceneProjectedShadowInitializer>& OutInitializers) const
{
	OutInitializers.push_back(FWholeSceneProjectedShadowInitializer());
	FWholeSceneProjectedShadowInitializer& OutInitializer = OutInitializers.back();
	OutInitializer.PreShadowTranslation = -GetLightToWorld().GetOrigin();
	OutInitializer.WorldToLight = GetWorldToLight().RemoveTranslation();
	OutInitializer.Scales = FVector(1, 1, 1);
	OutInitializer.FaceDirection = FVector(0, 0, 1);
	OutInitializer.SubjectBounds = FBoxSphereBounds(FVector(0, 0, 0), FVector(Radius, Radius, Radius), Radius);
	OutInitializer.WAxis = Vector4(0, 0, 1, 0);
	OutInitializer.MinLightW = 0.1f;
	OutInitializer.MaxDistanceToCastInLightW = Radius;
	OutInitializer.bOnePassPointLightShadow = true;
	//OutInitializer.bRayTracedDistanceField = UseRayTracedDistanceFieldShadows() && DoesPlatformSupportDistanceFieldShadowing(ViewFamily.GetShaderPlatform());
	return true;
}

void FPointLightSceneProxy::GetParameters(FLightParameters& LightParameters) const
{
	LightParameters.LightPositionAndInvRadius = Vector4(
		GetOrigin(),
		InvRadius);

	LightParameters.LightColorAndFalloffExponent = Vector4(
		GetColor().R,
		GetColor().G,
		GetColor().B,
		FalloffExponent);

	const FVector ZAxis(WorldToLight.M[0][2], WorldToLight.M[1][2], WorldToLight.M[2][2]);

	LightParameters.NormalizedLightDirection = -GetDirection();
	LightParameters.NormalizedLightTangent = ZAxis;
	LightParameters.SpotAngles = Vector2(-2.0f, 1.0f);
	LightParameters.SpecularScale = SpecularScale;
	LightParameters.LightSourceRadius = SourceRadius;
	LightParameters.LightSoftSourceRadius = SoftSourceRadius;
	LightParameters.LightSourceLength = SourceLength;
	LightParameters.SourceTexture = GWhiteTextureSRV;
}

class FSkyLightSceneProxy* USkyLightComponent::CreateSceneProxy() const
{
	//if (ProcessedSkyTexture)
	{
		return new FSkyLightSceneProxy(this);
	}

	return NULL;
}


void FSkyLightSceneProxy::Initialize(
	float InBlendFraction,
	/*const FSHVectorRGB3* InIrradianceEnvironmentMap, */
	/*const FSHVectorRGB3* BlendDestinationIrradianceEnvironmentMap, */
	const float* InAverageBrightness,
	const float* BlendDestinationAverageBrightness)
{

}

FSkyLightSceneProxy::FSkyLightSceneProxy(const class USkyLightComponent* InLightComponent)
	: LightComponent(InLightComponent)
	//, ProcessedTexture(InLightComponent->ProcessedSkyTexture)
	//, BlendDestinationProcessedTexture(InLightComponent->BlendDestinationProcessedSkyTexture)
	, SkyDistanceThreshold(InLightComponent->SkyDistanceThreshold)
	, bCastShadows(InLightComponent->CastShadows)
	, bWantsStaticShadowing(InLightComponent->Mobility == EComponentMobility::Stationary)
	, bHasStaticLighting(InLightComponent->HasStaticLighting())
	, bCastVolumetricShadow(InLightComponent->bCastVolumetricShadow)
	, LightColor(FLinearColor(InLightComponent->LightColor) * InLightComponent->Intensity)
	, IndirectLightingIntensity(InLightComponent->IndirectLightingIntensity)
	, VolumetricScatteringIntensity(FMath::Max(InLightComponent->VolumetricScatteringIntensity, 0.0f))
	, OcclusionMaxDistance(InLightComponent->OcclusionMaxDistance)
	, Contrast(InLightComponent->Contrast)
	, OcclusionExponent(FMath::Clamp(InLightComponent->OcclusionExponent, .1f, 10.0f))
	, MinOcclusion(FMath::Clamp(InLightComponent->MinOcclusion, 0.0f, 1.0f))
	, OcclusionTint(InLightComponent->OcclusionTint)
	, OcclusionCombineMode(InLightComponent->OcclusionCombineMode)
{

}
