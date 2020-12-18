#pragma once

#include "StaticMesh.h"
#include "Camera.h"
#include "SceneView.h"
#include "MeshBach.h"
#include "StaticMeshDrawList.h"
#include "DepthOnlyRendering.h"
#include "SceneManagement.h"
#include "DeferredShading.h"
#include "ShadowRendering.h"
#include "BasePassRendering.h"

#include <memory>

enum EAntiAliasingMethod
{
	AAM_None ,
	AAM_FXAA,
	AAM_TemporalAA ,
	/** Only supported with forward shading.  MSAA sample count is controlled by r.MSAACount. */
	AAM_MSAA,
	AAM_MAX,
};

enum EAutoExposureMethod
{
	/** Not supported on mobile, requires compute shader to construct 64 bin histogram */
	AEM_Histogram,
	/** Not supported on mobile, faster method that computes single value by downsampling */
	AEM_Basic,
	/** Uses camera settings. */
	AEM_Manual,
	AEM_MAX,
};

enum EBasePassDrawListType
{
	EBasePass_Default = 0,
	EBasePass_Masked,
	EBasePass_MAX
};

Vector4 CreateInvDeviceZToWorldZTransform(const FMatrix& ProjMatrix);

class FSceneRenderTargets;
class UPrimitiveComponent;
class FLightSceneInfo;
class FLightSceneInfoCompact;
class UAtmosphericFogComponent;

class FCachedShadowMapData
{
public:
	FWholeSceneProjectedShadowInitializer Initializer;
	FShadowMapRenderTargetsRefCounted ShadowMap;
	float LastUsedTime;
	bool bCachedShadowMapHasPrimitives;

	FCachedShadowMapData(const FWholeSceneProjectedShadowInitializer& InInitializer, float InLastUsedTime) :
		Initializer(InInitializer),
		LastUsedTime(InLastUsedTime),
		bCachedShadowMapHasPrimitives(true)
	{}
};
class FIndirectLightingCache
{
public:
	void UpdateCache(FScene* Scene, FSceneRenderer& Renderer, bool bAllowUnbuiltPreview);
};
/**
* Bounding information used to cull primitives in the scene.
*/
struct FPrimitiveBounds
{
	FBoxSphereBounds BoxSphereBounds;
	/** Square of the minimum draw distance for the primitive. */
	float MinDrawDistanceSq;
	/** Maximum draw distance for the primitive. */
	float MaxDrawDistance;
	/** Maximum cull distance for the primitive. This is only different from the MaxDrawDistance for HLOD.*/
	float MaxCullDistance;
};

struct FUpdateLightTransformParameters
{
	FMatrix LightToWorld;
	Vector4 Position;
};
class FSceneViewState
{
public:
	// Previous frame's view info to use.
	FPreviousViewInfo PrevFrameViewInfo;

	// Pending previous frame's view info. When rendering a new view, this must be the PendingPrevFrame that
	// should be updated. This is the next frame that is only going to set PrevFrame = PendingPrevFrame if
	// the world is not pause.
	FPreviousViewInfo PendingPrevFrameViewInfo;

	bool bSequencerIsPaused;
	const std::shared_ptr<FD3D11Texture2D>* GetTonemappingLUTTexture() const {
		const std::shared_ptr<FD3D11Texture2D>* ShaderResourceTexture = NULL;

		if (CombinedLUTRenderTarget.Get()!= NULL) {
			ShaderResourceTexture = &CombinedLUTRenderTarget->ShaderResourceTexture;
		}
		return ShaderResourceTexture;
	}
	bool HasValidTonemappingLUT() const
	{
		return bValidTonemappingLUT;
	}
	void SetValidTonemappingLUT(bool bValid = true)
	{
		bValidTonemappingLUT = bValid;
	}
	PooledRenderTarget* GetCurrentEyeAdaptationRT()
	{
		return EyeAdaptationRTManager.GetCurrentRT().Get();
	}
	PooledRenderTarget* GetLastEyeAdaptationRT()
	{
		return EyeAdaptationRTManager.GetLastRT().Get();
	}
	/** Swaps the double-buffer targets used in eye adaptation */
	void SwapEyeAdaptationRTs()
	{
		EyeAdaptationRTManager.SwapRTs(bUpdateLastExposure);
	}

	bool HasValidEyeAdaptation() const
	{
		return bValidEyeAdaptation;
	}

	void SetValidEyeAdaptation()
	{
		bValidEyeAdaptation = true;
	}

	float GetLastEyeAdaptationExposure() const
	{
		return EyeAdaptationRTManager.GetLastExposure();
	}
	virtual uint32 GetFrameIndexMod8() const;

	// Returns a reference to the render target used for the LUT.  Allocated on the first request.
	PooledRenderTarget& GetTonemappingLUTRenderTarget(const int32 LUTSize, const bool bUseVolumeLUT, const bool bNeedUAV)
	{
		if (CombinedLUTRenderTarget.Get() == NULL ||
			CombinedLUTRenderTarget->GetDesc().Extent.Y != LUTSize ||
			((CombinedLUTRenderTarget->GetDesc().Depth != 0) != bUseVolumeLUT) ||
			!!(CombinedLUTRenderTarget->GetDesc().TargetableFlags & TexCreate_UAV) != bNeedUAV)
		{
			// Create the texture needed for the tonemapping LUT
			EPixelFormat LUTPixelFormat = PF_A2B10G10R10;
			if (!GPixelFormats[LUTPixelFormat].Supported)
			{
				LUTPixelFormat = PF_R8G8B8A8;
			}

			PooledRenderTargetDesc Desc = PooledRenderTargetDesc::Create2DDesc(FIntPoint(LUTSize * LUTSize, LUTSize), LUTPixelFormat, FClearValueBinding::Transparent, TexCreate_None, TexCreate_ShaderResource, false);
			Desc.TargetableFlags |= bNeedUAV ? TexCreate_UAV : TexCreate_RenderTargetable;

			if (bUseVolumeLUT)
			{
				Desc.Extent = FIntPoint(LUTSize, LUTSize);
				Desc.Depth = LUTSize;
			}

			//Desc.DebugName = TEXT("CombineLUTs");

			GRenderTargetPool.FindFreeElement(Desc, CombinedLUTRenderTarget, TEXT("CombineLUTs")/*Desc.DebugName*/);
		}

		PooledRenderTarget& RenderTarget = *CombinedLUTRenderTarget.Get();
		return RenderTarget;
	}
private:
	/** The current frame PreExposure */
	float PreExposure;

	bool bValidEyeAdaptation;

	/** Whether to get the last exposure from GPU */
	bool bUpdateLastExposure;
	// to implement eye adaptation / auto exposure changes over time
	class FEyeAdaptationRTManager
	{
	public:

		FEyeAdaptationRTManager() : CurrentBuffer(0), LastExposure(0.f), CurrentStagingBuffer(0) {}

		void SafeRelease();

		ComPtr<PooledRenderTarget>& GetCurrentRT()
		{
			return GetRTRef(CurrentBuffer);
		}

		/** Return old Render Target*/
		ComPtr<PooledRenderTarget>& GetLastRT()
		{
			return GetRTRef(1 - CurrentBuffer);
		}

		/** Reverse the current/last order of the targets */
		void SwapRTs(bool bUpdateLastExposure);

		/** Get the last frame exposure value (used to compute pre-exposure) */
		float GetLastExposure() const { return LastExposure; }

	private:

		/** Return one of two two render targets */
		ComPtr<PooledRenderTarget>&  GetRTRef(const int BufferNumber);

	private:

		int32 CurrentBuffer;

		float LastExposure;
		int32 CurrentStagingBuffer;
		static const int32 NUM_STAGING_BUFFERS = 3;

		ComPtr<PooledRenderTarget> RenderTarget[2];
		ComPtr<PooledRenderTarget> StagingBuffers[NUM_STAGING_BUFFERS];
	} EyeAdaptationRTManager;

	ComPtr<PooledRenderTarget> CombinedLUTRenderTarget;
	bool bValidTonemappingLUT;
	// counts up by one each frame, warped in 0..7 range, ResetViewState() puts it back to 0
	uint32 FrameIndexMod8;

};
class FScene
{
public:
	class UWorld* World;

	FScene(UWorld* InWorld);

	void AddPrimitive(UPrimitiveComponent* Primitive);
	void RemovePrimitive(UPrimitiveComponent* Primitive);
	void UpdatePrimitiveTransform(UPrimitiveComponent* Component);
	void UpdateLightTransform(ULightComponent* Light);
	void AddLight(class ULightComponent* Light);
	void RemoveLight(class ULightComponent* Light);
	void AddAtmosphericFog(UAtmosphericFogComponent* FogComponent);
	void RemoveAtmosphericFog(UAtmosphericFogComponent* FogComponent);
	void SetSkyLight(FSkyLightSceneProxy* Light);
	void DisableSkyLight(FSkyLightSceneProxy* Light);
	void AddLightSceneInfo(FLightSceneInfo* LightSceneInfo);
	void AddPrimitiveSceneInfo_RenderThread(FPrimitiveSceneInfo* PrimitiveSceneInfo);

	void UpdateLightTransform_RenderThread(FLightSceneInfo* LightSceneInfo, const struct FUpdateLightTransformParameters& Parameters);
	void UpdatePrimitiveTransform_RenderThread(FPrimitiveSceneProxy* PrimitiveSceneProxy, const FBoxSphereBounds& WorldBounds, const FBoxSphereBounds& LocalBounds, const FMatrix& LocalToWorld, const FVector& OwnerPosition);


	void UpdateSkyCaptureContents(const USkyLightComponent* CaptureComponent, bool bCaptureEmissiveOnly, FD3D11Texture2D* SourceCubemap, FD3D11Texture2D* OutProcessedTexture, float& OutAverageBrightness, FSHVectorRGB3& OutIrradianceEnvironmentMap, std::vector<FFloat16Color>* OutRadianceMap);

	class FGPUSkinCache* GetGPUSkinCache()
	{
		return nullptr;
	}
public:


	TStaticMeshDrawList<FPositionOnlyDepthDrawingPolicy> PositionOnlyDepthDrawList;
	TStaticMeshDrawList<FDepthDrawingPolicy> DepthDrawList;

	TStaticMeshDrawList<FDepthDrawingPolicy> MaskedDepthDrawList;
	/** Base pass draw list - no light map */
	TStaticMeshDrawList<TBasePassDrawingPolicy<FUniformLightMapPolicy> > BasePassUniformLightMapPolicyDrawList[EBasePass_MAX];
	/** Base pass draw list - self shadowed translucency*/
	//TStaticMeshDrawList<TBasePassDrawingPolicy<FSelfShadowedTranslucencyPolicy> > BasePassSelfShadowedTranslucencyDrawList[EBasePass_MAX];
	//TStaticMeshDrawList<TBasePassDrawingPolicy<FSelfShadowedCachedPointIndirectLightingPolicy> > BasePassSelfShadowedCachedPointIndirectTranslucencyDrawList[EBasePass_MAX];
	//TStaticMeshDrawList<TBasePassDrawingPolicy<FSelfShadowedVolumetricLightmapPolicy> > BasePassSelfShadowedVolumetricLightmapTranslucencyDrawList[EBasePass_MAX];

	/** hit proxy draw list (includes both opaque and translucent objects) */
	//TStaticMeshDrawList<FHitProxyDrawingPolicy> HitProxyDrawList;

	/** hit proxy draw list, with only opaque objects */
	//TStaticMeshDrawList<FHitProxyDrawingPolicy> HitProxyDrawList_OpaqueOnly;

	/** draw list for motion blur velocities */
	//TStaticMeshDrawList<FVelocityDrawingPolicy> VelocityDrawList;

	/** Draw list used for rendering whole scene shadow depths. */
	TStaticMeshDrawList<FShadowDepthDrawingPolicy<false> > WholeSceneShadowDepthDrawList;

	/** Draw list used for rendering whole scene reflective shadow maps.  */
	TStaticMeshDrawList<FShadowDepthDrawingPolicy<true> > WholeSceneReflectiveShadowMapDrawList;

	template<typename LightMapPolicyType>
	TStaticMeshDrawList<TBasePassDrawingPolicy<LightMapPolicyType> >& GetBasePassDrawList(EBasePassDrawListType DrawType);

	std::vector<FPrimitiveSceneInfo*> Primitives;
	std::vector<FPrimitiveSceneProxy*> PrimitiveSceneProxies;
	std::vector<FPrimitiveBounds> PrimitiveBounds;
	std::vector<FPrimitiveComponentId> PrimitiveComponentIds;

	std::vector<FLightSceneInfoCompact> Lights;


	std::vector<FPrimitiveSceneInfoCompact> PrimitiveOctree;

	class FAtmosphericFogSceneInfo* AtmosphericFog;

	FSkyLightSceneProxy* SkyLight;

	std::vector<FSkyLightSceneProxy*> SkyLightStack;

	FLightSceneInfo* SimpleDirectionalLight;
	/** The sun light for atmospheric effect, if any. */
	FLightSceneInfo* SunLight;

	//TSparseArray<FDeferredDecalProxy*> Decals;

	//TArray<const FPrecomputedLightVolume*> PrecomputedLightVolumes;

	std::vector<FStaticMesh*> StaticMeshes;

	/** Interpolates and caches indirect lighting for dynamic objects. */
	FIndirectLightingCache IndirectLightingCache;

	//FVolumetricLightmapSceneData VolumetricLightmapSceneData;

	std::map<int32, FCachedShadowMapData> CachedShadowMaps;


	bool ShouldRenderSkylightInBasePass(EBlendMode BlendMode) const
	{
		bool bRenderSkyLight = SkyLight && !SkyLight->bHasStaticLighting;

		if (IsTranslucentBlendMode(BlendMode))
		{
			// Both stationary and movable skylights are applied in base pass for translucent materials
			bRenderSkyLight = bRenderSkyLight
				&& (/*ReadOnlyCVARCache.bEnableStationarySkylight*/true || !SkyLight->bWantsStaticShadowing);
		}
		else
		{
			// For opaque materials, stationary skylight is applied in base pass but movable skylight
			// is applied in a separate render pass (bWantssStaticShadowing means stationary skylight)
			bRenderSkyLight = bRenderSkyLight
				&& ((/*ReadOnlyCVARCache.bEnableStationarySkylight*/true && SkyLight->bWantsStaticShadowing)
					/*|| (!SkyLight->bWantsStaticShadowing*/
						/*&& (IsAnyForwardShadingEnabled(GetShaderPlatform()) || IsMobilePlatform(GetShaderPlatform())))*/);
		}

		return bRenderSkyLight;
	}

	bool HasAtmosphericFog() const
	{
		return (AtmosphericFog != NULL); // Use default value when Sun Light is not existing
	}
	uint32 GetFrameNumber() const
	{
		return SceneFrameNumber;
	}
	void IncrementFrameNumber()
	{
		++SceneFrameNumber;
	}
	UWorld* GetWorld() const { return World; }
private:
	uint32 SceneFrameNumber;
};

inline bool ShouldIncludeDomainInMeshPass(EMaterialDomain Domain)
{
	// Non-Surface domains can be applied to static meshes for thumbnails or material editor preview
	// Volume domain materials however must only be rendered in the voxelization pass
	return Domain != MD_Volume;
}

#include "BasePassRendering.inl"