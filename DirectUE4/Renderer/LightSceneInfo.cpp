#include "LightSceneInfo.h"
#include "UnrealMathFPU.h"

void FLightSceneInfoCompact::Init(FLightSceneInfo* InLightSceneInfo)
{
	LightSceneInfo = InLightSceneInfo;
	FSphere BoundingSphere = InLightSceneInfo->Proxy->GetBoundingSphere();
	BoundingSphere.W = BoundingSphere.W > 0.0f ? BoundingSphere.W : FLT_MAX;
	memcpy(&BoundingSphereVector, &BoundingSphere, sizeof(BoundingSphereVector));
	Color = InLightSceneInfo->Proxy->GetColor();
	LightType = InLightSceneInfo->Proxy->GetLightType();

	bCastDynamicShadow = InLightSceneInfo->Proxy->CastsDynamicShadow();
	bCastStaticShadow = InLightSceneInfo->Proxy->CastsStaticShadow();
	bStaticLighting = InLightSceneInfo->Proxy->HasStaticLighting();
}
bool AreSpheresNotIntersecting(
	const VectorRegister& A_XYZ,
	const VectorRegister& A_Radius,
	const VectorRegister& B_XYZ,
	const VectorRegister& B_Radius
)
{
	const VectorRegister DeltaVector = VectorSubtract(A_XYZ, B_XYZ);
	const VectorRegister DistanceSquared = VectorDot3(DeltaVector, DeltaVector);
	const VectorRegister MaxDistance = VectorAdd(A_Radius, B_Radius);
	const VectorRegister MaxDistanceSquared = VectorMultiply(MaxDistance, MaxDistance);
	return !!VectorAnyGreaterThan(DistanceSquared, MaxDistanceSquared);
}

bool FLightSceneInfoCompact::AffectsPrimitive(const FBoxSphereBounds& PrimitiveBounds, const FPrimitiveSceneProxy* PrimitiveSceneProxy) const
{
	// Check if the light's bounds intersect the primitive's bounds.
	if (AreSpheresNotIntersecting(
		BoundingSphereVector,
		VectorReplicate(BoundingSphereVector, 3),
		VectorLoadFloat3(&PrimitiveBounds.Origin),
		VectorLoadFloat1(&PrimitiveBounds.SphereRadius)
	))
	{
		return false;
	}

	// Cull based on information in the full scene infos.

	if (!LightSceneInfo->Proxy->AffectsBounds(PrimitiveBounds))
	{
		return false;
	}

// 	if (LightSceneInfo->Proxy->CastsShadowsFromCinematicObjectsOnly() && !PrimitiveSceneProxy->CastsCinematicShadow())
// 	{
// 		return false;
// 	}
// 
// 	if (!(LightSceneInfo->Proxy->GetLightingChannelMask() & PrimitiveSceneProxy->GetLightingChannelMask()))
// 	{
// 		return false;
// 	}

	return true;
}

FLightSceneInfo::FLightSceneInfo(FLightSceneProxy* InProxy)
	:Proxy(InProxy)
{

}

FLightSceneInfo::~FLightSceneInfo()
{

}

void FLightSceneInfo::AddToScene()
{

}

void FLightSceneInfo::RemoveFromScene()
{

}

