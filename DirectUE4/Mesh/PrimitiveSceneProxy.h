#pragma once

#include "UnrealMath.h"
#include "PrimitiveUniformBufferParameters.h"
#include "PrimitiveRelevance.h"
#include "EngineTypes.h"

#include <vector>

class FLightSceneInfo;
class FLightSceneProxy;
class FPrimitiveSceneInfo;
class UPrimitiveComponent;
struct FMeshBatch;
class FScene;
class AActor;
class FSceneView;


class FPrimitiveComponentId
{
public:

	FPrimitiveComponentId() : PrimIDValue(0)
	{}

	inline bool IsValid() const
	{
		return PrimIDValue > 0;
	}

	inline bool operator==(FPrimitiveComponentId OtherId) const
	{
		return PrimIDValue == OtherId.PrimIDValue;
	}

	template<typename T> friend struct std::hash;

	uint32 PrimIDValue;

};

namespace std
{
	template<> struct hash<FPrimitiveComponentId>
	{
		std::size_t operator()(FPrimitiveComponentId const& ID) const noexcept
		{
			return ID.PrimIDValue;
		}
	};
}

class FPrimitiveSceneProxy
{
public:

	/** Initialization constructor. */
	FPrimitiveSceneProxy(const UPrimitiveComponent* InComponent);

	/** Virtual destructor. */
	virtual ~FPrimitiveSceneProxy();


	inline FScene& GetScene() const { return *Scene; }
	inline FPrimitiveComponentId GetPrimitiveComponentId() const { return PrimitiveComponentId; }
	inline FPrimitiveSceneInfo* GetPrimitiveSceneInfo() const { return PrimitiveSceneInfo; }
	inline const FMatrix& GetLocalToWorld() const { return LocalToWorld; }
	inline bool IsLocalToWorldDeterminantNegative() const { return bIsLocalToWorldDeterminantNegative; }
	inline const FBoxSphereBounds& GetBounds() const { return Bounds; }
	inline const FBoxSphereBounds& GetLocalBounds() const { return LocalBounds; }

	inline bool IsMeshShapeOftenMoving() const
	{
		//return Mobility == EComponentMobility::Movable || !bGoodCandidateForCachedShadowmap;
		return false;
	}
	inline bool IsMovable() const
	{
		// Note: primitives with EComponentMobility::Stationary can still move (as opposed to lights with EComponentMobility::Stationary)
		return Mobility == EComponentMobility::Movable || Mobility == EComponentMobility::Stationary;
	}
	inline EIndirectLightingCacheQuality GetIndirectLightingCacheQuality() const { return IndirectLightingCacheQuality; }

	inline uint8 GetLightingChannelMask() const { return LightingChannelMask; }
	inline uint8 GetLightingChannelStencilValue() const
	{
		// Flip the default channel bit so that the default value is 0, to align with the default stencil clear value and GBlackTexture value
		return (LightingChannelMask & 0x6) | (~LightingChannelMask & 0x1);
	}

	inline bool HasStaticLighting() const { return bStaticLighting; }

	inline bool CastsStaticShadow() const { return bCastStaticShadow; }
	inline bool CastsDynamicShadow() const { return bCastDynamicShadow; }
	inline bool CastsVolumetricTranslucentShadow() const { return bCastVolumetricTranslucentShadow; }

	inline bool CastsSelfShadowOnly() const { return bSelfShadowOnly; }
	inline bool CastsInsetShadow() const { return bCastInsetShadow; }
	inline bool CastsFarShadow() const { return bCastFarShadow; }
	inline bool HasValidSettingsForStaticLighting() const { return bHasValidSettingsForStaticLighting; }

	inline const bool ReceivesDecals() const { return bReceivesDecals; }

	virtual void DrawStaticElements(FPrimitiveSceneInfo* PrimitiveSceneInfo/*FStaticPrimitiveDrawInterface* PDI*/) {};
	
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const;

	virtual void GetLightRelevance(const FLightSceneProxy* LightSceneProxy, bool& bDynamic, bool& bRelevant, bool& bLightMapped, bool& bShadowMapped) const
	{
		// Determine the lights relevance to the primitive.
		bDynamic = true;
		bRelevant = true;
		bLightMapped = false;
		bShadowMapped = false;
	}

	inline const TUniformBuffer<FPrimitiveUniformShaderParameters>& GetUniformBuffer() const
	{
		return UniformBuffer;
	}
	void UpdateUniformBuffer();
private:
	uint32 bStaticLighting : 1;

	uint8 LightingChannelMask;
protected:
	uint32 bReceivesDecals : 1;

	uint32 bHasValidSettingsForStaticLighting : 1;

	uint32 bCastDynamicShadow : 1;

	uint32 bCastStaticShadow : 1;
	uint32 bCastVolumetricTranslucentShadow : 1;

	/** When enabled, the component will only cast a shadow on itself and not other components in the world.  This is especially useful for first person weapons, and forces bCastInsetShadow to be enabled. */
	uint32 bSelfShadowOnly : 1;
	/** Whether this component should create a per-object shadow that gives higher effective shadow resolution. true if bSelfShadowOnly is true. */
	uint32 bCastInsetShadow : 1;

	uint32 bCastFarShadow : 1;

	EComponentMobility::Type Mobility;

	EIndirectLightingCacheQuality IndirectLightingCacheQuality;

private:
	/** The primitive's local to world transform. */
	FMatrix LocalToWorld;

	/** The primitive's bounds. */
	FBoxSphereBounds Bounds;

	/** The primitive's local space bounds. */
	FBoxSphereBounds LocalBounds;

	/** The component's actor's position. */
	FVector ActorPosition;

	/** The hierarchy of owners of this primitive.  These must not be dereferenced on the rendering thread, but the pointer values can be used for identification.  */
	std::vector<const AActor*> Owners;

	/** The scene the primitive is in. */
	FScene* Scene;

	/**
	* Id for the component this proxy belongs to.
	* This will stay the same for the lifetime of the component, so it can be used to identify the component across re-registers.
	*/
	FPrimitiveComponentId PrimitiveComponentId;

	/** Pointer back to the PrimitiveSceneInfo that owns this Proxy. */
	FPrimitiveSceneInfo* PrimitiveSceneInfo;

	uint32 bIsLocalToWorldDeterminantNegative : 1;

	void SetTransform(const FMatrix& InLocalToWorld, const FBoxSphereBounds& InBounds, const FBoxSphereBounds& InLocalBounds, FVector InActorPosition);

	void UpdateUniformBufferMaybeLazy();

	TUniformBuffer<FPrimitiveUniformShaderParameters> UniformBuffer;


	friend class FScene;
};