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

ID3D11InputLayout* PositionOnlyMeshInputLayout;

ID3DBlob* PrePassVSBytecode;
ID3DBlob* PrePassPSBytecode;
ID3D11VertexShader* PrePassVS;
ID3D11PixelShader* PrePassPS;
ID3D11RasterizerState* PrePassRasterizerState;
ID3D11BlendState* PrePassBlendState;
ID3D11DepthStencilState* PrePassDepthStencilState;

std::map<std::string, ParameterAllocation> PrePassVSParams;
std::map<std::string, ParameterAllocation> PrePassPSParams;


void InitPrePass()
{
	//Prepass
	PrePassVSBytecode = CompileVertexShader(TEXT("./Shaders/DepthOnlyPass.hlsl"), "VS_Main");
	PrePassPSBytecode = CompilePixelShader(TEXT("./Shaders/DepthOnlyPass.hlsl"), "PS_Main");
	GetShaderParameterAllocations(PrePassVSBytecode, PrePassVSParams);
	GetShaderParameterAllocations(PrePassPSBytecode, PrePassPSParams);

	D3D11_INPUT_ELEMENT_DESC PositionOnlyInputDesc[] =
	{
		{ "ATTRIBUTE",	0,	DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 0,  D3D11_INPUT_PER_VERTEX_DATA,0 },
	};
	PositionOnlyMeshInputLayout = CreateInputLayout(PositionOnlyInputDesc, sizeof(PositionOnlyInputDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC), PrePassVSBytecode);

	PrePassVS = CreateVertexShader(PrePassVSBytecode);
	PrePassPS = CreatePixelShader(PrePassPSBytecode);

	PrePassRasterizerState = TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_BACK, FALSE, FALSE>::GetRHI();
	PrePassDepthStencilState = TStaticDepthStencilState<true, D3D11_COMPARISON_GREATER>::GetRHI();
	PrePassBlendState = TStaticBlendState<>::GetRHI();
}

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
		FMeshMaterialShader::SetParameters(GetVertexShader(), MaterialRenderProxy, MaterialResource, View, DrawRenderState.GetViewUniformBuffer(), DrawRenderState.GetPassUniformBuffer().get());

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

	void SetMesh(const FVertexFactory* VertexFactory, const FSceneView& View, /*const FPrimitiveSceneProxy* Proxy, */const FMeshBatchElement& BatchElement, const FDrawingPolicyRenderState& DrawRenderState)
	{
		FMeshMaterialShader::SetMesh(GetVertexShader(), VertexFactory, View, /*Proxy,*/ BatchElement, DrawRenderState);
	}

	void SetInstancedEyeIndex(const uint32 EyeIndex)
	{
// 		if (InstancedEyeIndexParameter.IsBound())
// 		{
// 			SetShaderValue(RHICmdList, GetVertexShader(), InstancedEyeIndexParameter, EyeIndex);
// 		}
	}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(template<>, TDepthOnlyVS<true>, ("PositionOnlyDepthShader.hlsl"), ("VS_Main"), SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>, TDepthOnlyVS<false>, ("DepthOnlyVertexShader.hlsl"), ("Main"), SF_Vertex);

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
		FMeshMaterialShader::SetParameters(GetPixelShader(), MaterialRenderProxy,MaterialResource, *View, DrawRenderState.GetViewUniformBuffer(), DrawRenderState.GetPassUniformBuffer().get());

		// For debug view shaders, don't apply the depth offset as their base pass PS are using global shaders with depth equal.
		//SetShaderValue(RHICmdList, GetPixelShader(), ApplyDepthOffsetParameter, !View->Family->UseDebugViewPS());
		//SetShaderValue(RHICmdList, GetPixelShader(), MobileColorValue, InMobileColorValue);
	}

	void SetMesh( const FVertexFactory* VertexFactory, const FSceneView& View,/* const FPrimitiveSceneProxy* Proxy,*/ const FMeshBatchElement& BatchElement, const FDrawingPolicyRenderState& DrawRenderState)
	{
		FMeshMaterialShader::SetMesh(GetPixelShader(), VertexFactory, View, /*Proxy,*/ BatchElement, DrawRenderState);
	}

	FShaderParameter ApplyDepthOffsetParameter;
	FShaderParameter MobileColorValue;
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FDepthOnlyPS, ("DepthOnlyPixelShader.hlsl"), "Main", SF_Pixel);

void SceneRenderer::RenderPrePassView(FViewInfo& View, const FDrawingPolicyRenderState& DrawRenderState)
{
	SCOPED_DRAW_EVENT_FORMAT(EventPrePass, TEXT("PrePass"));

	RenderTargets& SceneContex = RenderTargets::Get();
	SceneContex.BeginRenderingPrePass(true);

	D3D11DeviceContext->RSSetScissorRects(0, NULL);
	D3D11_VIEWPORT VP = { 
		(float)View.ViewRect.Min.X, 
		(float)View.ViewRect.Min.Y, 
		(float)(View.ViewRect.Max.X - View.ViewRect.Min.X),
		(float)(View.ViewRect.Max.Y - View.ViewRect.Min.Y),
		0.f,1.f 
	};
	D3D11DeviceContext->RSSetViewports(1, &VP);

	Scene->PositionOnlyDepthDrawList.DrawVisible(D3D11DeviceContext, View, DrawRenderState);
	//Scene->DepthDrawList.DrawVisible(D3D11DeviceContext, View, DrawRenderState);
	/*
	

	const ParameterAllocation& ViewParams = PrePassVSParams.at("View");
	const ParameterAllocation& PrimitiveParams = PrePassVSParams.at("Primitive");

	D3D11DeviceContext->VSSetConstantBuffers(ViewParams.BufferIndex, 1, &View.ViewUniformBuffer);

	D3D11DeviceContext->RSSetState(PrePassRasterizerState);
	//D3D11DeviceContext->OMSetBlendState(PrePassBlendState,);
	D3D11DeviceContext->RSSetViewports(1, &GViewport);
	D3D11DeviceContext->OMSetDepthStencilState(PrePassDepthStencilState, 0);


	for (MeshBatch& MB : GScene->AllBatches)
	{
		D3D11DeviceContext->IASetInputLayout(PositionOnlyMeshInputLayout);
		D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		UINT Stride = sizeof(PositionOnlyLocalVertex);
		UINT Offset = 0;
		D3D11DeviceContext->IASetVertexBuffers(0, 1, &MB.PositionOnlyVertexBuffer, &Stride, &Offset);
		D3D11DeviceContext->IASetIndexBuffer(MB.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		D3D11DeviceContext->VSSetShader(PrePassVS, 0, 0);
		D3D11DeviceContext->PSSetShader(PrePassPS, 0, 0);

		for (size_t Element = 0; Element < MB.Elements.size(); ++Element)
		{
			D3D11DeviceContext->VSSetConstantBuffers(PrimitiveParams.BufferIndex, 1, &MB.Elements[Element].PrimitiveUniformBuffer);
			D3D11DeviceContext->DrawIndexed(MB.Elements[Element].NumTriangles * 3, MB.Elements[Element].FirstIndex, 0);
		}
	}
	*/
}

void SceneRenderer::RenderPrePass()
{
	RenderTargets& SceneContext = RenderTargets::Get();
	for (uint32 ViewIndex = 0; ViewIndex < Views.size(); ++ViewIndex)
	{
		FViewInfo& View = Views[ViewIndex];
		FSceneTexturesUniformParameters SceneTextureParameters;
		SetupSceneTextureUniformParameters(SceneContext, ESceneTextureSetupMode::None, SceneTextureParameters);
		TUniformBufferPtr<FSceneTexturesUniformParameters> PassUniformBuffer = TUniformBufferPtr<FSceneTexturesUniformParameters>::CreateUniformBufferImmediate(SceneTextureParameters);
		FDrawingPolicyRenderState DrawRenderState(View, PassUniformBuffer);

		DrawRenderState.SetBlendState(TStaticBlendState<FALSE,FALSE,0>::GetRHI());

		RenderPrePassView(View, DrawRenderState);

	}
}

FDepthDrawingPolicy::FDepthDrawingPolicy(
	const FVertexFactory* InVertexFactory, 
	const FMaterialRenderProxy* InMaterialRenderProxy,
	const FMaterial& InMaterialResource, 
	/*const FMeshDrawingPolicyOverrideSettings& InOverrideSettings, */ 
	/*ERHIFeatureLevel::Type InFeatureLevel, */ 
	float MobileColorValue)
	: FMeshDrawingPolicy(InVertexFactory, InMaterialRenderProxy, InMaterialResource)
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

void FDepthDrawingPolicy::SetSharedState(const FDrawingPolicyRenderState& DrawRenderState, const FSceneView* View/*, const ContextDataType PolicyContext*/) const
{
	// Set the depth-only shader parameters for the material.
	VertexShader->SetParameters(MaterialRenderProxy, *MaterialResource, *View, DrawRenderState, false/*PolicyContext.bIsInstancedStereo*/, false/*PolicyContext.bIsInstancedStereoEmulated*/);
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
	FMeshDrawingPolicy::SetSharedState(D3D11DeviceContext, DrawRenderState, View/*, PolicyContext*/);
}

FBoundShaderStateInput FDepthDrawingPolicy::GetBoundShaderStateInput() const
{
	return FBoundShaderStateInput(
		FMeshDrawingPolicy::GetVertexDeclaration(),
		VertexShader->GetCode(),
		VertexShader->GetVertexShader(),
		NULL,
		NULL,
		bNeedsPixelShader ? PixelShader->GetPixelShader() : NULL,
		NULL);
}

void FDepthDrawingPolicy::SetMeshRenderState(
	ID3D11DeviceContext* Context, 
	const FSceneView& View, 
	/*const FPrimitiveSceneProxy* PrimitiveSceneProxy, */ 
	const FMeshBatch& Mesh, 
	int32 BatchElementIndex,  
const FDrawingPolicyRenderState& DrawRenderState/*,  */
/*const ElementDataType& ElementData, */ 
/*const ContextDataType PolicyContext */) const
{
	const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];
	VertexShader->SetMesh(VertexFactory, View, /*PrimitiveSceneProxy,*/ BatchElement, DrawRenderState);
// 	if (HullShader && DomainShader)
// 	{
// 		HullShader->SetMesh(VertexFactory, View, /*PrimitiveSceneProxy,*/ BatchElement, DrawRenderState);
// 		DomainShader->SetMesh(VertexFactory, View, /*PrimitiveSceneProxy,*/ BatchElement, DrawRenderState);
// 	}
	if (bNeedsPixelShader)
	{
		PixelShader->SetMesh(VertexFactory, View, /*PrimitiveSceneProxy,*/ BatchElement, DrawRenderState);
	}
}


FPositionOnlyDepthDrawingPolicy::FPositionOnlyDepthDrawingPolicy(
	const FVertexFactory* InVertexFactory,
	const FMaterialRenderProxy* InMaterialRenderProxy, 
	const FMaterial& InMaterialResource/*,*/
	/*const FMeshDrawingPolicyOverrideSettings& InOverrideSettings */
) : FMeshDrawingPolicy(InVertexFactory, InMaterialRenderProxy, InMaterialResource/*, InOverrideSettings, DVSM_None*/)
{
	VertexShader = InMaterialResource.GetShader<TDepthOnlyVS<true>>(InVertexFactory->GetType());
	bUsePositionOnlyVS = true;
	BaseVertexShader = VertexShader;
}

void FPositionOnlyDepthDrawingPolicy::SetSharedState(ID3D11DeviceContext* Context, const FDrawingPolicyRenderState& DrawRenderState, const FSceneView* View/*, const ContextDataType PolicyContext*/) const
{
	VertexShader->SetParameters(MaterialRenderProxy,*MaterialResource,*View,DrawRenderState,false,false);
	VertexFactory->SetPositionStream(Context);
}

FBoundShaderStateInput FPositionOnlyDepthDrawingPolicy::GetBoundShaderStateInput() const
{
	std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>> VertexDeclaration;
	VertexDeclaration = VertexFactory->GetPositionDeclaration();

	assert(MaterialRenderProxy->GetMaterial()->GetBlendMode() == BLEND_Opaque);
	return FBoundShaderStateInput(VertexDeclaration, VertexShader->GetCode(), VertexShader->GetVertexShader(), NULL, NULL, NULL, NULL);
}

void FPositionOnlyDepthDrawingPolicy::SetMeshRenderState(
	ID3D11DeviceContext* Context, 
	const FSceneView& View, 
	/*const FPrimitiveSceneProxy* PrimitiveSceneProxy, */ 
	const FMeshBatch& Mesh, 
	int32 BatchElementIndex,  
	const FDrawingPolicyRenderState& DrawRenderState/*,*/ 
/*const ElementDataType& ElementData, */ 
/*const ContextDataType PolicyContext */) const
{
	VertexShader->SetMesh(VertexFactory, View, Mesh.Elements[BatchElementIndex], DrawRenderState);
}

void FDepthDrawingPolicyFactory::AddStaticMesh(FScene* Scene, FStaticMesh* StaticMesh)
{
	const FMaterialRenderProxy* DefaultProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(false);
	FPositionOnlyDepthDrawingPolicy DrawingPolicy(StaticMesh->VertexFactory,
		DefaultProxy,
		*DefaultProxy->GetMaterial()//,
		//OverrideSettings
	);

	// Add the static mesh to the position-only depth draw list.
	Scene->PositionOnlyDepthDrawList.AddMesh(
		StaticMesh,
		/*FPositionOnlyDepthDrawingPolicy::ElementDataType(),*/
		DrawingPolicy//,
		/*FeatureLevel*/
	);
}

bool FDepthDrawingPolicyFactory::DrawDynamicMesh(ID3D11DeviceContext* Context, const FViewInfo& View, /*ContextType DrawingContext, */ const FMeshBatch& Mesh, /*bool bPreFog, */ /*const FDrawingPolicyRenderState& DrawRenderState, */ /*const FPrimitiveSceneProxy* PrimitiveSceneProxy, */ /*FHitProxyId HitProxyId, */ /*const bool bIsInstancedStereo = false, */ const bool bIsInstancedStereoEmulated /*= false */)
{
	return false;
}

bool FDepthDrawingPolicyFactory::DrawStaticMesh(ID3D11DeviceContext* Context, const FViewInfo& View, /*ContextType DrawingContext, */ const FStaticMesh& StaticMesh, const uint64& BatchElementMask, /*bool bPreFog, */ /*const FDrawingPolicyRenderState& DrawRenderState, */ /*const FPrimitiveSceneProxy* PrimitiveSceneProxy, */ /*FHitProxyId HitProxyId, */ /*const bool bIsInstancedStereo = false, */ const bool bIsInstancedStereoEmulated /*= false */)
{
	return false;
}
