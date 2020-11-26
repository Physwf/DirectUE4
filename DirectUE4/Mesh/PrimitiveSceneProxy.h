#pragma once

#include "UnrealMath.h"
#include "PrimitiveUniformBufferParameters.h"

#include <vector>

class FLightSceneInfo;
class FLightSceneProxy;
class FPrimitiveSceneInfo;
class UPrimitiveComponent;
struct FMeshBatch;
class FScene;
class AActor;

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

	virtual void DrawStaticElements(FPrimitiveSceneInfo* PrimitiveSceneInfo/*FStaticPrimitiveDrawInterface* PDI*/) {};
	
	inline const TUniformBuffer<FPrimitiveUniformShaderParameters>& GetUniformBuffer() const
	{
		return UniformBuffer;
	}
	void UpdateUniformBuffer();
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