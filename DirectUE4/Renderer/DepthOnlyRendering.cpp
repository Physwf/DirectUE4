#include "DepthOnlyRendering.h"
#include "Scene.h"
#include "RenderTargets.h"
#include "DeferredShading.h"
#include "GPUProfiler.h"
#include "SceneRenderTargetParameters.h"
#include "VertexFactory.h"
#include "SceneView.h"
#include "MeshBach.h"
#include "MeshMaterialShader.h"
#include "Material.h"
#include "PrimitiveSceneProxy.h"


/**
* A vertex shader for rendering the depth of a mesh.
*/
template <bool bUsePositionOnlyStream>
class TDepthOnlyVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(TDepthOnlyVS, MeshMaterial);
protected:

	TDepthOnlyVS() {}
	TDepthOnlyVS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) :
		FMeshMaterialShader(Initializer)
	{
// 		InstancedEyeIndexParameter.Bind(Initializer.ParameterMap, TEXT("InstancedEyeIndex"));
// 		IsInstancedStereoParameter.Bind(Initializer.ParameterMap, TEXT("bIsInstancedStereo"));
// 		IsInstancedStereoEmulatedParameter.Bind(Initializer.ParameterMap, TEXT("bIsInstancedStereoEmulated"));
		BindSceneTextureUniformBufferDependentOnShadingPath(Initializer, PassUniformBuffer/*, PassUniformBuffer*/);
	}

private:

	FShaderParameter InstancedEyeIndexParameter;
	FShaderParameter IsInstancedStereoParameter;
	FShaderParameter IsInstancedStereoEmulatedParameter;

public:

	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		// Only the local vertex factory supports the position-only stream
		if (bUsePositionOnlyStream)
		{
			return VertexFactoryType->SupportsPositionOnly() && Material->IsSpecialEngineMaterial();
		}

		// Only compile for the default material and masked materials
		return (Material->IsSpecialEngineMaterial() || !Material->WritesEveryPixel() || Material->MaterialMayModifyMeshPosition() || Material->IsTranslucencyWritingCustomDepth());
	}

	void SetParameters(
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FMaterial& MaterialResource,
		const FSceneView& View,
		const FDrawingPolicyRenderState& DrawRenderState,
		const bool bIsInstancedStereo,
		const bool bIsInstancedStereoEmulated
	)
	{
		FMeshMaterialShader::SetParameters(GetVertexShader(), MaterialRenderProxy, MaterialResource, View, DrawRenderState.GetViewUniformBuffer(), DrawRenderState.GetPassUniformBuffer());

// 		if (IsInstancedStereoParameter.IsBound())
// 		{
// 			SetShaderValue(RHICmdList, GetVertexShader(), IsInstancedStereoParameter, bIsInstancedStereo);
// 		}
// 
// 		if (IsInstancedStereoEmulatedParameter.IsBound())
// 		{
// 			SetShaderValue(RHICmdList, GetVertexShader(), IsInstancedStereoEmulatedParameter, bIsInstancedStereoEmulated);
// 		}
// 
// 		if (InstancedEyeIndexParameter.IsBound())
// 		{
// 			SetShaderValue(RHICmdList, GetVertexShader(), InstancedEyeIndexParameter, 0);
// 		}
	}

	void SetMesh(const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatchElement& BatchElement, const FDrawingPolicyRenderState& DrawRenderState)
	{
		FMeshMaterialShader::SetMesh(GetVertexShader(), VertexFactory, View, Proxy, BatchElement, DrawRenderState);
	}

	void SetInstancedEyeIndex(const uint32 EyeIndex)
	{
// 		if (InstancedEyeIndexParameter.IsBound())
// 		{
// 			SetShaderValue(RHICmdList, GetVertexShader(), InstancedEyeIndexParameter, EyeIndex);
// 		}
	}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(template<>, TDepthOnlyVS<true>, ("PositionOnlyDepthShader.dusf"), ("VS_Main"), SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>, TDepthOnlyVS<false>, ("DepthOnlyVertexShader.dusf"), ("Main"), SF_Vertex);

class FDepthOnlyPS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FDepthOnlyPS, MeshMaterial);
public:

	static bool ShouldCompilePermutation(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		return
			// Compile for materials that are masked.
			(!Material->WritesEveryPixel() || Material->HasPixelDepthOffsetConnected() || Material->IsTranslucencyWritingCustomDepth())
			// Mobile uses material pixel shader to write custom stencil to color target
			|| (/*IsMobilePlatform(Platform)*/false && (Material->IsDefaultMaterial() || Material->MaterialMayModifyMeshPosition()));
	}

	FDepthOnlyPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FMeshMaterialShader(Initializer)
	{
		ApplyDepthOffsetParameter.Bind(Initializer.ParameterMap, ("bApplyDepthOffset"));
		MobileColorValue.Bind(Initializer.ParameterMap, ("MobileColorValue"));
		BindSceneTextureUniformBufferDependentOnShadingPath(Initializer, PassUniformBuffer/*, PassUniformBuffer*/);
	}

	FDepthOnlyPS() {}

	void SetParameters(const FMaterialRenderProxy* MaterialRenderProxy, const FMaterial& MaterialResource, const FSceneView* View, const FDrawingPolicyRenderState& DrawRenderState, float InMobileColorValue)
	{
		FMeshMaterialShader::SetParameters(GetPixelShader(), MaterialRenderProxy,MaterialResource, *View, DrawRenderState.GetViewUniformBuffer(), DrawRenderState.GetPassUniformBuffer());

		// For debug view shaders, don't apply the depth offset as their base pass PS are using global shaders with depth equal.
		//SetShaderValue(RHICmdList, GetPixelShader(), ApplyDepthOffsetParameter, !View->Family->UseDebugViewPS());
		//SetShaderValue(RHICmdList, GetPixelShader(), MobileColorValue, InMobileColorValue);
	}

	void SetMesh( const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatchElement& BatchElement, const FDrawingPolicyRenderState& DrawRenderState)
	{
		FMeshMaterialShader::SetMesh(GetPixelShader(), VertexFactory, View, Proxy, BatchElement, DrawRenderState);
	}

	FShaderParameter ApplyDepthOffsetParameter;
	FShaderParameter MobileColorValue;
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FDepthOnlyPS, ("DepthOnlyPixelShader.dusf"), "Main", SF_Pixel);


bool FSceneRenderer::RenderPrePassViewDynamic(const FViewInfo& View, const FDrawingPolicyRenderState& DrawRenderState)
{
	FDepthDrawingPolicyFactory::ContextType Context(EarlyZPassMode, true);

	for (uint32 MeshBatchIndex = 0; MeshBatchIndex < View.DynamicMeshElements.size(); MeshBatchIndex++)
	{
		const FMeshBatchAndRelevance& MeshBatchAndRelevance = View.DynamicMeshElements[MeshBatchIndex];

		if (MeshBatchAndRelevance.GetHasOpaqueOrMaskedMaterial() && MeshBatchAndRelevance.GetRenderInMainPass())
		{
			const FMeshBatch& MeshBatch = *MeshBatchAndRelevance.Mesh;
			const FPrimitiveSceneProxy* PrimitiveSceneProxy = MeshBatchAndRelevance.PrimitiveSceneProxy;
			bool bShouldUseAsOccluder = true;

			if (EarlyZPassMode < DDM_AllOpaque)
			{
				extern float GMinScreenRadiusForDepthPrepass;
				//@todo - move these proxy properties into FMeshBatchAndRelevance so we don't have to dereference the proxy in order to reject a mesh
				const float LODFactorDistanceSquared = (PrimitiveSceneProxy->GetBounds().Origin - View.ViewMatrices.GetViewOrigin()).SizeSquared() * FMath::Square(View.LODDistanceFactor);

				// Only render primitives marked as occluders
				bShouldUseAsOccluder = PrimitiveSceneProxy->ShouldUseAsOccluder()
					// Only render static objects unless movable are requested
					&& (!PrimitiveSceneProxy->IsMovable() || bEarlyZPassMovable)
					&& (FMath::Square(PrimitiveSceneProxy->GetBounds().SphereRadius) > GMinScreenRadiusForDepthPrepass * GMinScreenRadiusForDepthPrepass * LODFactorDistanceSquared);
			}

			if (bShouldUseAsOccluder)
			{
				FDepthDrawingPolicyFactory::DrawDynamicMesh(D3D11DeviceContext, View, Context, MeshBatch, true, DrawRenderState, PrimitiveSceneProxy, /*MeshBatch.BatchHitProxyId,*/false/* View.IsInstancedStereoPass()*/);
			}
		}
	}
	return true;
}

void FSceneRenderer::RenderPrePassView(FViewInfo& View, const FDrawingPolicyRenderState& DrawRenderState)
{
	bool bDirty = false;

	SCOPED_DRAW_EVENT_FORMAT(EventPrePass, TEXT("PrePass"));

	FSceneRenderTargets& SceneContex = FSceneRenderTargets::Get();
	SceneContex.BeginRenderingPrePass(true);

	D3D11DeviceContext->RSSetScissorRects(0, NULL);
	RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

	{
		SCOPED_DRAW_EVENT(PosOnlyOpaque);
		Scene->PositionOnlyDepthDrawList.DrawVisible(D3D11DeviceContext, View, DrawRenderState);
	}

	{
		SCOPED_DRAW_EVENT(Dynamic);
		bDirty |= RenderPrePassViewDynamic(View, DrawRenderState);
	}
}

void FSceneRenderer::RenderPrePass()
{
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get();

	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ++ViewIndex)
	{
		FViewInfo& View = Views[ViewIndex];
		FSceneTexturesUniformParameters SceneTextureParameters;
		SetupSceneTextureUniformParameters(SceneContext, ESceneTextureSetupMode::None, SceneTextureParameters);
		TUniformBufferPtr<FSceneTexturesUniformParameters> PassUniformBuffer = TUniformBufferPtr<FSceneTexturesUniformParameters>::CreateUniformBufferImmediate(SceneTextureParameters);//todo cache in case of gc
		FDrawingPolicyRenderState DrawRenderState(View, PassUniformBuffer.get());

		DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<true, D3D11_COMPARISON_GREATER>::GetRHI());
		DrawRenderState.SetBlendState(TStaticBlendState<>::GetRHI());

		RenderPrePassView(View, DrawRenderState);

	}
}

FDepthDrawingPolicy::FDepthDrawingPolicy(
	const FVertexFactory* InVertexFactory, 
	const FMaterialRenderProxy* InMaterialRenderProxy,
	const FMaterial& InMaterialResource, 
	const FMeshDrawingPolicyOverrideSettings& InOverrideSettings, 
	/*ERHIFeatureLevel::Type InFeatureLevel, */ 
	float MobileColorValue)
	: FMeshDrawingPolicy(InVertexFactory, InMaterialRenderProxy, InMaterialResource, InOverrideSettings)
{
	bNeedsPixelShader = /*bUsesMobileColorValue ||*/ (!InMaterialResource.WritesEveryPixel() || InMaterialResource.MaterialUsesPixelDepthOffset() || InMaterialResource.IsTranslucencyWritingCustomDepth());

	if (!bNeedsPixelShader)
	{
		PixelShader = nullptr;
	}

	VertexShader = InMaterialResource.GetShader<TDepthOnlyVS<false> >(VertexFactory->GetType());
	if (bNeedsPixelShader)
	{
		PixelShader = InMaterialResource.GetShader<FDepthOnlyPS>(InVertexFactory->GetType());
	}

	BaseVertexShader = VertexShader;
}

void FDepthDrawingPolicy::SetSharedState(ID3D11DeviceContext* Context, const FDrawingPolicyRenderState& DrawRenderState, const FSceneView* View, const ContextDataType PolicyContext) const
{
	// Set the depth-only shader parameters for the material.
	VertexShader->SetParameters(MaterialRenderProxy, *MaterialResource, *View, DrawRenderState, PolicyContext.bIsInstancedStereo, PolicyContext.bIsInstancedStereoEmulated);
// 	if (HullShader && DomainShader)
// 	{
// 		HullShader->SetParameters(/*MaterialRenderProxy,*/ *View, DrawRenderState.GetViewUniformBuffer(), DrawRenderState.GetPassUniformBuffer());
// 		DomainShader->SetParameters(/*MaterialRenderProxy,*/ *View, DrawRenderState.GetViewUniformBuffer(), DrawRenderState.GetPassUniformBuffer());
// 	}

	if (bNeedsPixelShader)
	{
		PixelShader->SetParameters(MaterialRenderProxy, *MaterialResource, View, DrawRenderState,0/* MobileColorValue*/);
	}

	// Set the shared mesh resources.
	FMeshDrawingPolicy::SetSharedState(Context, DrawRenderState, View, PolicyContext);
}

FBoundShaderStateInput FDepthDrawingPolicy::GetBoundShaderStateInput() const
{
	return FBoundShaderStateInput(
		FMeshDrawingPolicy::GetVertexDeclaration(),
		VertexShader->GetCode().Get(),
		VertexShader->GetVertexShader(),
		NULL,
		NULL,
		bNeedsPixelShader ? PixelShader->GetPixelShader() : NULL,
		NULL);
}

void FDepthDrawingPolicy::SetMeshRenderState(
	ID3D11DeviceContext* Context, 
	const FSceneView& View, 
	const FPrimitiveSceneProxy* PrimitiveSceneProxy, 
	const FMeshBatch& Mesh, 
	int32 BatchElementIndex,  
	const FDrawingPolicyRenderState& DrawRenderState, 
	const ElementDataType& ElementData, 
	const ContextDataType PolicyContext ) const
{
	const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];
	VertexShader->SetMesh(VertexFactory, View, PrimitiveSceneProxy, BatchElement, DrawRenderState);
// 	if (HullShader && DomainShader)
// 	{
// 		HullShader->SetMesh(VertexFactory, View, /*PrimitiveSceneProxy,*/ BatchElement, DrawRenderState);
// 		DomainShader->SetMesh(VertexFactory, View, /*PrimitiveSceneProxy,*/ BatchElement, DrawRenderState);
// 	}
	if (bNeedsPixelShader)
	{
		PixelShader->SetMesh(VertexFactory, View, PrimitiveSceneProxy, BatchElement, DrawRenderState);
	}
}


FPositionOnlyDepthDrawingPolicy::FPositionOnlyDepthDrawingPolicy(
	const FVertexFactory* InVertexFactory,
	const FMaterialRenderProxy* InMaterialRenderProxy, 
	const FMaterial& InMaterialResource,
	const FMeshDrawingPolicyOverrideSettings& InOverrideSettings 
) : FMeshDrawingPolicy(InVertexFactory, InMaterialRenderProxy, InMaterialResource, InOverrideSettings/*, DVSM_None*/)
{
	VertexShader = InMaterialResource.GetShader<TDepthOnlyVS<true>>(InVertexFactory->GetType());
	bUsePositionOnlyVS = true;
	BaseVertexShader = VertexShader;
}

void FPositionOnlyDepthDrawingPolicy::SetSharedState(ID3D11DeviceContext* Context, const FDrawingPolicyRenderState& DrawRenderState, const FSceneView* View, const ContextDataType PolicyContext) const
{
	VertexShader->SetParameters(MaterialRenderProxy,*MaterialResource,*View,DrawRenderState,false,false);
	VertexFactory->SetPositionStream(Context);
}

FBoundShaderStateInput FPositionOnlyDepthDrawingPolicy::GetBoundShaderStateInput() const
{
	std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>> VertexDeclaration;
	VertexDeclaration = VertexFactory->GetPositionDeclaration();

	assert(MaterialRenderProxy->GetMaterial()->GetBlendMode() == BLEND_Opaque);
	return FBoundShaderStateInput(VertexDeclaration, VertexShader->GetCode().Get(), VertexShader->GetVertexShader(), NULL, NULL, NULL, NULL);
}

void FPositionOnlyDepthDrawingPolicy::SetMeshRenderState(
	ID3D11DeviceContext* Context, 
	const FSceneView& View, 
	const FPrimitiveSceneProxy* PrimitiveSceneProxy, 
	const FMeshBatch& Mesh, 
	int32 BatchElementIndex,  
	const FDrawingPolicyRenderState& DrawRenderState, 
	const ElementDataType& ElementData, 
	const ContextDataType PolicyContext ) const
{
	VertexShader->SetMesh(VertexFactory, View, PrimitiveSceneProxy, Mesh.Elements[BatchElementIndex], DrawRenderState);
}

void FDepthDrawingPolicyFactory::AddStaticMesh(FScene* Scene, FStaticMesh* StaticMesh)
{
	const FMaterialRenderProxy* MaterialRenderProxy = StaticMesh->MaterialRenderProxy;
	const FMaterial* Material = MaterialRenderProxy->GetMaterial();
	const EBlendMode BlendMode = Material->GetBlendMode();

	FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(*StaticMesh);
	OverrideSettings.MeshOverrideFlags |= Material->IsTwoSided() ? EDrawingPolicyOverrideFlags::TwoSided : EDrawingPolicyOverrideFlags::None;

	const FMaterialRenderProxy* DefaultProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(false);
	FPositionOnlyDepthDrawingPolicy DrawingPolicy(StaticMesh->VertexFactory,
		DefaultProxy,
		*DefaultProxy->GetMaterial(),
		OverrideSettings
	);

	// Add the static mesh to the position-only depth draw list.
	Scene->PositionOnlyDepthDrawList.AddMesh(
		StaticMesh,
		FPositionOnlyDepthDrawingPolicy::ElementDataType(),
		DrawingPolicy//,
		/*FeatureLevel*/
	);
}

bool FDepthDrawingPolicyFactory::DrawDynamicMesh(
	ID3D11DeviceContext* Context, 
	const FViewInfo& View, 
	ContextType DrawingContext, 
	const FMeshBatch& Mesh, 
	bool bPreFog, 
	const FDrawingPolicyRenderState& DrawRenderState, 
	const FPrimitiveSceneProxy* PrimitiveSceneProxy, 
	/*FHitProxyId HitProxyId, */ 
	const bool bIsInstancedStereo /*= false*/, 
	const bool bIsInstancedStereoEmulated /*= false */)
{
	return DrawMesh(
		Context,
		View,
		DrawingContext,
		Mesh,
		Mesh.Elements.size() == 1 ? 1 : (1 << Mesh.Elements.size()) - 1,	// 1 bit set for each mesh element
		DrawRenderState,
		bPreFog,
		PrimitiveSceneProxy,
		//HitProxyId,
		bIsInstancedStereo,
		bIsInstancedStereoEmulated
	);
}

bool FDepthDrawingPolicyFactory::DrawStaticMesh(ID3D11DeviceContext* Context, const FViewInfo& View, /*ContextType DrawingContext, */ const FStaticMesh& StaticMesh, const uint64& BatchElementMask, /*bool bPreFog, */ /*const FDrawingPolicyRenderState& DrawRenderState, */ /*const FPrimitiveSceneProxy* PrimitiveSceneProxy, */ /*FHitProxyId HitProxyId, */ /*const bool bIsInstancedStereo = false, */ const bool bIsInstancedStereoEmulated /*= false */)
{
	return false;
}

bool FDepthDrawingPolicyFactory::DrawMesh(
	ID3D11DeviceContext* Context, 
	const FViewInfo& View, 
	ContextType DrawingContext, 
	const FMeshBatch& Mesh, 
	const uint64& BatchElementMask, 
	const FDrawingPolicyRenderState& DrawRenderState, 
	bool bPreFog, 
	const FPrimitiveSceneProxy* PrimitiveSceneProxy, 
	/*FHitProxyId HitProxyId, */ 
	const bool bIsInstancedStereo /*= false*/, 
	const bool bIsInstancedStereoEmulated /*= false */)
{
	const FMaterialRenderProxy* MaterialRenderProxy = Mesh.MaterialRenderProxy;
	const FMaterial* Material = MaterialRenderProxy->GetMaterial();
	bool bDirty = false;

	if ((Mesh.bUseAsOccluder || !DrawingContext.bRespectUseAsOccluderFlag || DrawingContext.DepthDrawingMode == DDM_AllOpaque)
		&& ShouldIncludeDomainInMeshPass(Material->GetMaterialDomain()))
	{
		const EBlendMode BlendMode = Material->GetBlendMode();
		const bool bUsesMobileColorValue = (DrawingContext.MobileColorValue != 0.0f);

		// Check to see if the primitive is currently fading in or out using the screen door effect.  If it is,
		// then we can't assume the object is opaque as it may be forcibly masked.
		//const FSceneViewState* SceneViewState = static_cast<const FSceneViewState*>(View.State);

		FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(Mesh);
		OverrideSettings.MeshOverrideFlags |= Material->IsTwoSided() ? EDrawingPolicyOverrideFlags::TwoSided : EDrawingPolicyOverrideFlags::None;

		if (BlendMode == BLEND_Opaque
			&& Mesh.VertexFactory->SupportsPositionOnlyStream()
			//&& !Material->MaterialModifiesMeshPosition_RenderThread()
			&& Material->WritesEveryPixel()
			&& !bUsesMobileColorValue
			)
		{
			//render opaque primitives that support a separate position-only vertex buffer
			const FMaterialRenderProxy* DefaultProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(false);

			OverrideSettings.MeshOverrideFlags |= Material->IsWireframe() ? EDrawingPolicyOverrideFlags::Wireframe : EDrawingPolicyOverrideFlags::None;

			FPositionOnlyDepthDrawingPolicy DrawingPolicy(
				Mesh.VertexFactory,
				DefaultProxy,
				*DefaultProxy->GetMaterial(),
				OverrideSettings
			);

			FDrawingPolicyRenderState DrawRenderStateLocal(DrawRenderState);
			DrawingPolicy.SetupPipelineState(DrawRenderStateLocal, View);
			CommitGraphicsPipelineState(DrawingPolicy, DrawRenderStateLocal, DrawingPolicy.GetBoundShaderStateInput());
			DrawingPolicy.SetSharedState(Context,DrawRenderStateLocal, &View, FPositionOnlyDepthDrawingPolicy::ContextDataType(bIsInstancedStereo, bIsInstancedStereoEmulated));

			int32 BatchElementIndex = 0;
			uint64 Mask = BatchElementMask;
			do
			{
				if (Mask & 1)
				{
					// We draw instanced static meshes twice when rendering with instanced stereo. Once for each eye.
					const bool bIsInstancedMesh = Mesh.Elements[BatchElementIndex].bIsInstancedMesh;
					const uint32 InstancedStereoDrawCount = (bIsInstancedStereo && bIsInstancedMesh) ? 2 : 1;
					for (uint32 DrawCountIter = 0; DrawCountIter < InstancedStereoDrawCount; ++DrawCountIter)
					{
						//DrawingPolicy.SetInstancedEyeIndex(DrawCountIter);

						//TDrawEvent<FRHICommandList> MeshEvent;
						//BeginMeshDrawEvent(RHICmdList, PrimitiveSceneProxy, Mesh, MeshEvent, EnumHasAnyFlags(EShowMaterialDrawEventTypes(GShowMaterialDrawEventTypes), EShowMaterialDrawEventTypes::DepthPositionOnly));

						DrawingPolicy.SetMeshRenderState(Context, View, PrimitiveSceneProxy, Mesh, BatchElementIndex, DrawRenderStateLocal, FPositionOnlyDepthDrawingPolicy::ElementDataType(), FPositionOnlyDepthDrawingPolicy::ContextDataType());
						DrawingPolicy.DrawMesh(Context, View, Mesh, BatchElementIndex, bIsInstancedStereo);
					}
				}
				Mask >>= 1;
				BatchElementIndex++;
			} while (Mask);

			bDirty = true;
		}
		else if (!IsTranslucentBlendMode(BlendMode) || Material->IsTranslucencyWritingCustomDepth())
		{
			bool bDraw = true;

			const bool bMaterialMasked = !Material->WritesEveryPixel() || Material->IsTranslucencyWritingCustomDepth();

			switch (DrawingContext.DepthDrawingMode)
			{
			case DDM_AllOpaque:
				break;
			case DDM_AllOccluders:
				break;
			case DDM_NonMaskedOnly:
				bDraw = !bMaterialMasked;
				break;
			default:
				assert(!"Unrecognized DepthDrawingMode");
			}

			if (bDraw)
			{
				FDepthDrawingPolicy DrawingPolicy(
					Mesh.VertexFactory,
					MaterialRenderProxy,
					*MaterialRenderProxy->GetMaterial(),
					OverrideSettings,
					DrawingContext.MobileColorValue
				);

				FDrawingPolicyRenderState DrawRenderStateLocal(DrawRenderState);
				DrawingPolicy.SetupPipelineState(DrawRenderStateLocal, View);
				CommitGraphicsPipelineState(DrawingPolicy, DrawRenderStateLocal, DrawingPolicy.GetBoundShaderStateInput());
				DrawingPolicy.SetSharedState(Context, DrawRenderStateLocal, &View, FDepthDrawingPolicy::ContextDataType(bIsInstancedStereo, bIsInstancedStereoEmulated));

				int32 BatchElementIndex = 0;
				uint64 Mask = BatchElementMask;
				do
				{
					if (Mask & 1)
					{
						// We draw instanced static meshes twice when rendering with instanced stereo. Once for each eye.
						const bool bIsInstancedMesh = Mesh.Elements[BatchElementIndex].bIsInstancedMesh;
						const uint32 InstancedStereoDrawCount = (bIsInstancedStereo && bIsInstancedMesh) ? 2 : 1;
						for (uint32 DrawCountIter = 0; DrawCountIter < InstancedStereoDrawCount; ++DrawCountIter)
						{
							//DrawingPolicy.SetInstancedEyeIndex(DrawCountIter);

							//TDrawEvent<FRHICommandList> MeshEvent;
							//BeginMeshDrawEvent(RHICmdList, PrimitiveSceneProxy, Mesh, MeshEvent, EnumHasAnyFlags(EShowMaterialDrawEventTypes(GShowMaterialDrawEventTypes), EShowMaterialDrawEventTypes::Depth));

							DrawingPolicy.SetMeshRenderState(Context, View, PrimitiveSceneProxy, Mesh, BatchElementIndex, DrawRenderStateLocal, FMeshDrawingPolicy::ElementDataType(), FDepthDrawingPolicy::ContextDataType());
							DrawingPolicy.DrawMesh(Context, View, Mesh, BatchElementIndex, bIsInstancedStereo);
						}
					}
					Mask >>= 1;
					BatchElementIndex++;
				} while (Mask);

				bDirty = true;
			}
		}
	}
	return bDirty;
}
