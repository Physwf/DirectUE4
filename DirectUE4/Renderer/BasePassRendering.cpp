#include "BasePassRendering.h"
#include "Scene.h"
#include "StaticMesh.h"
#include "RenderTargets.h"
#include "DeferredShading.h"
#include "GPUProfiler.h"
#include "DirectXTex.h"
using namespace DirectX;

FSharedBasePassUniformParameters BasePass;
FOpaqueBasePassUniformParameters OpaqueBasePass;

#define IMPLEMENT_BASEPASS_VERTEXSHADER_TYPE(LightMapPolicyType,LightMapPolicyName) \
	typedef TBasePassVS< LightMapPolicyType, false > TBasePassVS##LightMapPolicyName ; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassVS##LightMapPolicyName,TEXT("BasePassVertexShader.dusf"),("Main"),SF_Vertex); 

#define IMPLEMENT_BASEPASS_VERTEXSHADER_ONLY_TYPE(LightMapPolicyType,LightMapPolicyName,AtmosphericFogShaderName) \
	typedef TBasePassVS<LightMapPolicyType,true> TBasePassVS##LightMapPolicyName##AtmosphericFogShaderName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassVS##LightMapPolicyName##AtmosphericFogShaderName,TEXT("BasePassVertexShader.dusf"),("Main"),SF_Vertex)

#define IMPLEMENT_BASEPASS_PIXELSHADER_TYPE(LightMapPolicyType,LightMapPolicyName,bEnableSkyLight,SkyLightName) \
	typedef TBasePassPS<LightMapPolicyType, bEnableSkyLight> TBasePassPS##LightMapPolicyName##SkyLightName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassPS##LightMapPolicyName##SkyLightName,TEXT("BasePassPixelShader.dusf"),("MainPS"),SF_Pixel);

// Implement a pixel shader type for skylights and one without, and one vertex shader that will be shared between them
#define IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(LightMapPolicyType,LightMapPolicyName) \
	IMPLEMENT_BASEPASS_VERTEXSHADER_TYPE(LightMapPolicyType,LightMapPolicyName) \
	IMPLEMENT_BASEPASS_VERTEXSHADER_ONLY_TYPE(LightMapPolicyType,LightMapPolicyName,AtmosphericFog) \
	IMPLEMENT_BASEPASS_PIXELSHADER_TYPE(LightMapPolicyType,LightMapPolicyName,true,Skylight) \
	IMPLEMENT_BASEPASS_PIXELSHADER_TYPE(LightMapPolicyType,LightMapPolicyName,false,);

IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(TUniformLightMapPolicy<LMP_NO_LIGHTMAP>, FNoLightMapPolicy);
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(TUniformLightMapPolicy<LMP_PRECOMPUTED_IRRADIANCE_VOLUME_INDIRECT_LIGHTING>, FPrecomputedVolumetricLightmapLightingPolicy);
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(TUniformLightMapPolicy<LMP_CACHED_VOLUME_INDIRECT_LIGHTING>, FCachedVolumeIndirectLightingPolicy);
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(TUniformLightMapPolicy<LMP_CACHED_POINT_INDIRECT_LIGHTING>, FCachedPointIndirectLightingPolicy);
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(TUniformLightMapPolicy<LMP_LQ_LIGHTMAP>, TLightMapPolicyLQ);
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(TUniformLightMapPolicy<LMP_HQ_LIGHTMAP>, TLightMapPolicyHQ);
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(TUniformLightMapPolicy<LMP_DISTANCE_FIELD_SHADOWS_AND_HQ_LIGHTMAP>, TDistanceFieldShadowsAndLightMapPolicyHQ);


void FBasePassReflectionParameters::SetMesh(ID3D11PixelShader* PixelShaderRHI, const FSceneView& View, const FPrimitiveSceneProxy* Proxy)
{

}

static void SetDepthStencilStateForBasePass(
	FDrawingPolicyRenderState& DrawRenderState, 
	const FSceneView& View, 
	const FMeshBatch& Mesh, 
	const FPrimitiveSceneProxy* PrimitiveSceneProxy, 
	bool bEnableReceiveDecalOutput, 
	/*bool bUseDebugViewPS, */
	ID3D11DepthStencilState* LodFadeOverrideDepthStencilState)
{
	//static IConsoleVariable* EarlyZPassOnlyMaterialMaskingCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EarlyZPassOnlyMaterialMasking"));
	bool bMaskInEarlyPass = false;//(EarlyZPassOnlyMaterialMaskingCVar && Mesh.MaterialRenderProxy->GetMaterial(View.GetFeatureLevel())->IsMasked() && EarlyZPassOnlyMaterialMaskingCVar->GetInt());

	if (bEnableReceiveDecalOutput /*&& !bUseDebugViewPS*/)
	{
		// Set stencil value for this draw call
		// This is effectively extending the GBuffer using the stencil bits
		const uint8 StencilValue = GET_STENCIL_BIT_MASK(RECEIVE_DECAL, PrimitiveSceneProxy ? !!PrimitiveSceneProxy->ReceivesDecals() : 0x00)
			| STENCIL_LIGHTING_CHANNELS_MASK(PrimitiveSceneProxy ? PrimitiveSceneProxy->GetLightingChannelStencilValue() : 0x00);

		if (LodFadeOverrideDepthStencilState != nullptr)
		{
			//@TODO: Handle bMaskInEarlyPass in this case (used when a LODTransition is specified)
			DrawRenderState.SetDepthStencilState(LodFadeOverrideDepthStencilState);
			DrawRenderState.SetStencilRef(StencilValue);
		}
		else if (bMaskInEarlyPass)
		{
			DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<
				false, D3D11_COMPARISON_EQUAL,
				true, D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_REPLACE,
				false, D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP,
				0xFF, GET_STENCIL_BIT_MASK(RECEIVE_DECAL, 1) | STENCIL_LIGHTING_CHANNELS_MASK(0x7)
			>::GetRHI());
			DrawRenderState.SetStencilRef(StencilValue);
		}
		else if (DrawRenderState.GetDepthStencilAccess() & FExclusiveDepthStencil::DepthWrite)
		{
			DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<
				true, D3D11_COMPARISON_GREATER_EQUAL,
				true, D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_REPLACE,
				false, D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP,
				0xFF, GET_STENCIL_BIT_MASK(RECEIVE_DECAL, 1) | STENCIL_LIGHTING_CHANNELS_MASK(0x7)
			>::GetRHI());
			DrawRenderState.SetStencilRef(StencilValue);
		}
		else
		{
			DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<
				false, D3D11_COMPARISON_GREATER_EQUAL,
				true, D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_REPLACE,
				false, D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP,
				0xFF, GET_STENCIL_BIT_MASK(RECEIVE_DECAL, 1) | STENCIL_LIGHTING_CHANNELS_MASK(0x7)
			>::GetRHI());
			DrawRenderState.SetStencilRef(StencilValue);
		}
	}
	else if (bMaskInEarlyPass)
	{
		DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, D3D11_COMPARISON_EQUAL>::GetRHI());
	}
}

void FBasePassDrawingPolicy::ApplyDitheredLODTransitionState(FDrawingPolicyRenderState& DrawRenderState, const FViewInfo& ViewInfo, const FStaticMesh& Mesh, const bool InAllowStencilDither)
{

}
/** The action used to draw a base pass static mesh element. */
class FDrawBasePassStaticMeshAction
{
public:

	FScene * Scene;
	FStaticMesh* StaticMesh;

	/** Initialization constructor. */
	FDrawBasePassStaticMeshAction(FScene* InScene, FStaticMesh* InStaticMesh) :
		Scene(InScene),
		StaticMesh(InStaticMesh)
	{}

	bool UseTranslucentSelfShadowing() const { return false; }
	const FProjectedShadowInfo* GetTranslucentSelfShadow() const { return NULL; }

	bool AllowIndirectLightingCache() const
	{
		// Note: can't disallow based on presence of PrecomputedLightVolumes in the scene as this is registration time
		// Unless extra handling is added to recreate static draw lists when new volumes are added
		return true;
	}

	bool AllowIndirectLightingCacheVolumeTexture() const
	{
		return true;
	}

	bool UseVolumetricLightmap() const
	{
		return Scene->VolumetricLightmapSceneData.HasData();
	}

	/** Draws the mesh with a specific light-map type */
	template<typename LightMapPolicyType>
	void Process(
		const FProcessBasePassMeshParameters& Parameters,
		const LightMapPolicyType& LightMapPolicy,
		const typename LightMapPolicyType::ElementDataType& LightMapElementData
	) const
	{
		EBasePassDrawListType DrawType = EBasePass_Default;

		if (StaticMesh->IsMasked())
		{
			DrawType = EBasePass_Masked;
		}

		if (Scene)
		{
			// Find the appropriate draw list for the static mesh based on the light-map policy type.
			TStaticMeshDrawList<TBasePassDrawingPolicy<LightMapPolicyType> >& DrawList =
				Scene->GetBasePassDrawList<LightMapPolicyType>(DrawType);

			const bool bRenderSkylight = Scene->ShouldRenderSkylightInBasePass(Parameters.BlendMode) && Parameters.ShadingModel != MSM_Unlit;
			const bool bRenderAtmosphericFog = IsTranslucentBlendMode(Parameters.BlendMode) && Scene->HasAtmosphericFog() && true/*Scene->ReadOnlyCVARCache.bEnableAtmosphericFog*/;

			// Add the static mesh to the draw list.
			DrawList.AddMesh(
				StaticMesh,
				typename TBasePassDrawingPolicy<LightMapPolicyType>::ElementDataType(LightMapElementData),
				TBasePassDrawingPolicy<LightMapPolicyType>(
					StaticMesh->VertexFactory,
					StaticMesh->MaterialRenderProxy,
					*Parameters.Material,
					LightMapPolicy,
					Parameters.BlendMode,
					bRenderSkylight,
					bRenderAtmosphericFog,
					ComputeMeshOverrideSettings(*StaticMesh),
					/*DVSM_None,*/
					/* bInEnableReceiveDecalOutput = */ true
					)
			);
		}
	}
};

void FBasePassOpaqueDrawingPolicyFactory::AddStaticMesh(FScene* Scene, FStaticMesh* StaticMesh)
{
	// Determine the mesh's material and blend mode.
	const FMaterial* Material = StaticMesh->MaterialRenderProxy->GetMaterial();
	const EBlendMode BlendMode = Material->GetBlendMode();

	// Only draw opaque materials.
	if (!IsTranslucentBlendMode(BlendMode) && ShouldIncludeDomainInMeshPass(Material->GetMaterialDomain()))
	{
		ProcessBasePassMesh(
			FProcessBasePassMeshParameters(
				*StaticMesh,
				Material,
				StaticMesh->PrimitiveSceneInfo->Proxy,
				false),
			FDrawBasePassStaticMeshAction(Scene, StaticMesh)
		);
	}
}
/** The action used to draw a base pass dynamic mesh element. */
class FDrawBasePassDynamicMeshAction
{
public:

	const FViewInfo& View;
	FDrawingPolicyRenderState DrawRenderState;
	/*FHitProxyId HitProxyId;*/

	/** Initialization constructor. */
	FDrawBasePassDynamicMeshAction(
		const FViewInfo& InView,
		float InDitheredLODTransitionAlpha,
		const FDrawingPolicyRenderState& InDrawRenderState//,
		/*const FHitProxyId InHitProxyId*/
	)
		: View(InView)
		, DrawRenderState(InDrawRenderState)
		/*, HitProxyId(InHitProxyId)*/
	{
		DrawRenderState.SetDitheredLODTransitionAlpha(InDitheredLODTransitionAlpha);
	}

	bool UseTranslucentSelfShadowing() const { return false; }
	const FProjectedShadowInfo* GetTranslucentSelfShadow() const { return NULL; }

	bool AllowIndirectLightingCache() const
	{
		const FScene* Scene = (const FScene*)View.Family->Scene;
		return /*View.Family->EngineShowFlags.IndirectLightingCache*/true && Scene && Scene->PrecomputedLightVolumes.Num() > 0;
	}

	bool AllowIndirectLightingCacheVolumeTexture() const
	{
		return true;
	}

	bool UseVolumetricLightmap() const
	{
		const FScene* Scene = (const FScene*)View.Family->Scene;
		return /*View.Family->EngineShowFlags.VolumetricLightmap*/true
			&& Scene
			&& Scene->VolumetricLightmapSceneData.HasData();
	}

	/** Draws the translucent mesh with a specific light-map type, and shader complexity predicate. */
	template<typename LightMapPolicyType>
	void Process(
		const FProcessBasePassMeshParameters& Parameters,
		const LightMapPolicyType& LightMapPolicy,
		const typename LightMapPolicyType::ElementDataType& LightMapElementData
	)
	{
		const FScene* Scene = Parameters.PrimitiveSceneProxy ? Parameters.PrimitiveSceneProxy->GetPrimitiveSceneInfo()->Scene : NULL;

		const bool bRenderSkylight = Scene && Scene->ShouldRenderSkylightInBasePass(Parameters.BlendMode) && Parameters.ShadingModel != MSM_Unlit;
		const bool bRenderAtmosphericFog = IsTranslucentBlendMode(Parameters.BlendMode) && (Scene && Scene->HasAtmosphericFog() && true /*Scene->ReadOnlyCVARCache.bEnableAtmosphericFog*/) && /*View.Family->EngineShowFlags.AtmosphericFog*/true;

		bool bEnableReceiveDecalOutput = Scene != nullptr;
		TBasePassDrawingPolicy<LightMapPolicyType> DrawingPolicy(
			Parameters.Mesh.VertexFactory,
			Parameters.Mesh.MaterialRenderProxy,
			*Parameters.Material,
			LightMapPolicy,
			Parameters.BlendMode,
			bRenderSkylight,
			bRenderAtmosphericFog,
			ComputeMeshOverrideSettings(Parameters.Mesh),
			/*View.Family->GetDebugViewShaderMode(),*/
			bEnableReceiveDecalOutput
		);

		SetDepthStencilStateForBasePass(DrawRenderState, View, Parameters.Mesh, Parameters.PrimitiveSceneProxy, bEnableReceiveDecalOutput, /*DrawingPolicy.UseDebugViewPS(),*/ nullptr);
		DrawingPolicy.SetupPipelineState(DrawRenderState, View);
		CommitGraphicsPipelineState(DrawingPolicy, DrawRenderState, DrawingPolicy.GetBoundShaderStateInput());
		DrawingPolicy.SetSharedState(DrawRenderState, &View, typename TBasePassDrawingPolicy<LightMapPolicyType>::ContextDataType(Parameters.bIsInstancedStereo));

		for (int32 BatchElementIndex = 0, Num = Parameters.Mesh.Elements.Num(); BatchElementIndex < Num; BatchElementIndex++)
		{
			// We draw instanced static meshes twice when rendering with instanced stereo. Once for each eye.
			const bool bIsInstancedMesh = Parameters.Mesh.Elements[BatchElementIndex].bIsInstancedMesh;
			const uint32 InstancedStereoDrawCount = (Parameters.bIsInstancedStereo && bIsInstancedMesh) ? 2 : 1;
			for (uint32 DrawCountIter = 0; DrawCountIter < InstancedStereoDrawCount; ++DrawCountIter)
			{
				DrawingPolicy.SetInstancedEyeIndex(DrawCountIter);

				//TDrawEvent<FRHICommandList> MeshEvent;
				//BeginMeshDrawEvent(RHICmdList, Parameters.PrimitiveSceneProxy, Parameters.Mesh, MeshEvent, EnumHasAnyFlags(EShowMaterialDrawEventTypes(GShowMaterialDrawEventTypes), EShowMaterialDrawEventTypes::BasePass));

				DrawingPolicy.SetMeshRenderState(
					View,
					Parameters.PrimitiveSceneProxy,
					Parameters.Mesh,
					BatchElementIndex,
					DrawRenderState,
					typename TBasePassDrawingPolicy<LightMapPolicyType>::ElementDataType(LightMapElementData),
					typename TBasePassDrawingPolicy<LightMapPolicyType>::ContextDataType()
				);
				DrawingPolicy.DrawMesh(View, Parameters.Mesh, BatchElementIndex, Parameters.bIsInstancedStereo);
			}
		}
	}
};
bool FBasePassOpaqueDrawingPolicyFactory::DrawDynamicMesh(const FViewInfo& View, ContextType DrawingContext, const FMeshBatch& Mesh, bool bPreFog, const FDrawingPolicyRenderState& DrawRenderState, const FPrimitiveSceneProxy* PrimitiveSceneProxy, /*FHitProxyId HitProxyId,*/ const bool bIsInstancedStereo /*= false */)
{

}

static void SetupBasePassView(const FViewInfo& View, const FSceneRenderer* SceneRenderer, FDrawingPolicyRenderState& DrawRenderState, FExclusiveDepthStencil::Type BasePassDepthStencilAccess, const bool bShaderComplexity, const bool bIsEditorPrimitivePass = false)
{
	DrawRenderState.SetDepthStencilAccess(BasePassDepthStencilAccess);

	DrawRenderState.SetBlendState(TStaticBlendStateWriteMask<D3D11_COLOR_WRITE_ENABLE_ALL, D3D11_COLOR_WRITE_ENABLE_ALL, D3D11_COLOR_WRITE_ENABLE_ALL, D3D11_COLOR_WRITE_ENABLE_ALL>::GetRHI());

	if (DrawRenderState.GetDepthStencilAccess() & FExclusiveDepthStencil::DepthWrite)
	{
		DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<true, D3D11_COMPARISON_GREATER_EQUAL>::GetRHI());
	}
	else
	{
		DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, D3D11_COMPARISON_GREATER_EQUAL>::GetRHI());
	}

	RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);
}

void SetupSharedBasePassParameters(const FViewInfo& View, FSceneRenderTargets& SceneRenderTargets, FSharedBasePassUniformParameters& BasePassParameters)
{

}

void CreateOpaqueBasePassUniformBuffer(const FViewInfo& View, PooledRenderTarget* ForwardScreenSpaceShadowMask, TUniformBufferPtr<FOpaqueBasePassUniformParameters>& BasePassUniformBuffer)
{

}

bool FSceneRenderer::RenderBasePassStaticData(FViewInfo& View, const FDrawingPolicyRenderState& DrawRenderState)
{
	bool bDirty = false;

	// When using a depth-only pass, the default opaque geometry's depths are already
	// in the depth buffer at this point, so rendering masked next will already cull
	// as efficiently as it can, while also increasing the ZCull efficiency when
	// rendering the default opaque geometry afterward.
	if (EarlyZPassMode != DDM_None)
	{
		bDirty |= RenderBasePassStaticDataType(View, DrawRenderState, EBasePass_Masked);
		bDirty |= RenderBasePassStaticDataType(View, DrawRenderState, EBasePass_Default);
	}
	else
	{
		// Otherwise, in the case where we're not using a depth-only pre-pass, there
		// is an advantage to rendering default opaque first to help cull the more
		// expensive masked geometry.
		bDirty |= RenderBasePassStaticDataType(View, DrawRenderState, EBasePass_Default);
		bDirty |= RenderBasePassStaticDataType(View, DrawRenderState, EBasePass_Masked);
	}
	return bDirty;
}

bool FSceneRenderer::RenderBasePassStaticDataType(FViewInfo& View, const FDrawingPolicyRenderState& DrawRenderState, const EBasePassDrawListType DrawType)
{
	bool bDirty = false;
	bDirty |= Scene->BasePassUniformLightMapPolicyDrawList[DrawType].DrawVisible(View, DrawRenderState/*, View.StaticMeshVisibilityMap, View.StaticMeshBatchVisibility*/);
	return bDirty;
}

void FSceneRenderer::RenderBasePassDynamicData(const FViewInfo& View, const FDrawingPolicyRenderState& DrawRenderState, bool& bOutDirty)
{

}

bool FSceneRenderer::RenderBasePassView(FViewInfo& View, FExclusiveDepthStencil::Type BasePassDepthStencilAccess, const FDrawingPolicyRenderState& InDrawRenderState)
{
	bool bDirty = false;
	FDrawingPolicyRenderState DrawRenderState(InDrawRenderState);
	SetupBasePassView(View, this, DrawRenderState, BasePassDepthStencilAccess, false);
	bDirty |= RenderBasePassStaticData(View, DrawRenderState);
	RenderBasePassDynamicData(View, DrawRenderState, bDirty);

	return bDirty;
}

void FSceneRenderer::RenderBasePass(FExclusiveDepthStencil::Type BasePassDepthStencilAccess)
{
	SCOPED_DRAW_EVENT_FORMAT(RenderBasePass, TEXT("BasePass"));
	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ViewIndex++)
	{
		FViewInfo& View = Views[ViewIndex];

		TUniformBufferPtr<FOpaqueBasePassUniformParameters> BasePassUniformBuffer;
		CreateOpaqueBasePassUniformBuffer(View, ForwardScreenSpaceShadowMask, BasePassUniformBuffer);
		FDrawingPolicyRenderState DrawRenderState(View, BasePassUniformBuffer);
		RenderBasePassView(View, BasePassDepthStencilAccess, DrawRenderState);
	}
	
}
