#pragma once

#include "D3D11RHI.h"
#include "UnrealMath.h"
#include "SceneView.h"
#include "MeshBach.h"
#include "VertexFactory.h"
#include "MaterialShader.h"
#include "Material.h"
#include "EnumClassFlags.h"

enum class EDrawingPolicyOverrideFlags
{
	None = 0,
	TwoSided = 1 << 0,
	DitheredLODTransition = 1 << 1,
	Wireframe = 1 << 2,
	ReverseCullMode = 1 << 3,
};
ENUM_CLASS_FLAGS(EDrawingPolicyOverrideFlags);

struct FDrawingPolicyRenderState
{
	FDrawingPolicyRenderState(const FSceneView& SceneView, FUniformBuffer* InPassUniformBuffer = nullptr) :
		BlendState(nullptr)
		, DepthStencilState(nullptr)
		, DepthStencilAccess(FExclusiveDepthStencil::DepthRead_StencilRead)
		, ViewUniformBuffer(SceneView.ViewUniformBuffer)
		, PassUniformBuffer(InPassUniformBuffer)
		, StencilRef(0)
		, ViewOverrideFlags(EDrawingPolicyOverrideFlags::None)
		, DitheredLODTransitionAlpha(0.0f)
	{
		ViewOverrideFlags |= SceneView.bReverseCulling ? EDrawingPolicyOverrideFlags::ReverseCullMode : EDrawingPolicyOverrideFlags::None;
		ViewOverrideFlags |= SceneView.bRenderSceneTwoSided ? EDrawingPolicyOverrideFlags::TwoSided : EDrawingPolicyOverrideFlags::None;
	}

	FDrawingPolicyRenderState() :
		BlendState(nullptr)
		, DepthStencilState(nullptr)
		, ViewUniformBuffer()
		, PassUniformBuffer(nullptr)
		, StencilRef(0)
		, ViewOverrideFlags(EDrawingPolicyOverrideFlags::None)
		, DitheredLODTransitionAlpha(0.0f)
	{
	}

	inline FDrawingPolicyRenderState(const FDrawingPolicyRenderState& DrawRenderState) :
		BlendState(DrawRenderState.BlendState)
		, DepthStencilState(DrawRenderState.DepthStencilState)
		, DepthStencilAccess(DrawRenderState.DepthStencilAccess)
		, ViewUniformBuffer(DrawRenderState.ViewUniformBuffer)
		, PassUniformBuffer(DrawRenderState.PassUniformBuffer)
		, StencilRef(DrawRenderState.StencilRef)
		, ViewOverrideFlags(DrawRenderState.ViewOverrideFlags)
		, DitheredLODTransitionAlpha(DrawRenderState.DitheredLODTransitionAlpha)
	{
	}

	~FDrawingPolicyRenderState()
	{
	}

public:
	inline void SetBlendState(ID3D11BlendState* InBlendState)
	{
		BlendState = InBlendState;
	}

	inline ID3D11BlendState* const GetBlendState() const
	{
		return BlendState;
	}

	inline void SetDepthStencilState(ID3D11DepthStencilState* InDepthStencilState)
	{
		DepthStencilState = InDepthStencilState;
		StencilRef = 0;
	}

	inline void SetStencilRef(uint32 InStencilRef)
	{
		StencilRef = InStencilRef;
	}

	inline ID3D11DepthStencilState* const GetDepthStencilState() const
	{
		return DepthStencilState;
	}

	inline void SetDepthStencilAccess(FExclusiveDepthStencil::Type InDepthStencilAccess)
	{
		DepthStencilAccess = InDepthStencilAccess;
	}

	inline FExclusiveDepthStencil::Type GetDepthStencilAccess() const
	{
		return DepthStencilAccess;
	}

	inline void SetViewUniformBuffer(const TUniformBufferPtr<FViewUniformShaderParameters>& InViewUniformBuffer)
	{
		ViewUniformBuffer = InViewUniformBuffer;
	}

	inline const TUniformBufferPtr<FViewUniformShaderParameters> GetViewUniformBuffer() const
	{
		return ViewUniformBuffer;
	}

	inline void SetPassUniformBuffer(FUniformBuffer* InPassUniformBuffer)
	{
		PassUniformBuffer = InPassUniformBuffer;
	}

	inline FUniformBuffer* GetPassUniformBuffer() const
	{
		return PassUniformBuffer;
	}

	inline uint32 GetStencilRef() const
	{
		return StencilRef;
	}

	inline void SetDitheredLODTransitionAlpha(float InDitheredLODTransitionAlpha)
	{
		DitheredLODTransitionAlpha = InDitheredLODTransitionAlpha;
	}

	inline float GetDitheredLODTransitionAlpha() const
	{
		return DitheredLODTransitionAlpha;
	}


	inline EDrawingPolicyOverrideFlags& ModifyViewOverrideFlags()
	{
		return ViewOverrideFlags;
	}

	inline EDrawingPolicyOverrideFlags GetViewOverrideFlags() const
	{
		return ViewOverrideFlags;
	}
// 
// 	inline void ApplyToPSO(FGraphicsPipelineStateInitializer& GraphicsPSOInit) const
// 	{
// 		GraphicsPSOInit.BlendState = BlendState;
// 		GraphicsPSOInit.DepthStencilState = DepthStencilState;
// 	}

private:
	ID3D11BlendState*				BlendState;
	ID3D11DepthStencilState*		DepthStencilState;
	FExclusiveDepthStencil::Type	DepthStencilAccess;

	TUniformBufferPtr<FViewUniformShaderParameters>	ViewUniformBuffer;
	FUniformBuffer*					PassUniformBuffer;
	uint32							StencilRef;

	//not sure if those should belong here
	EDrawingPolicyOverrideFlags		ViewOverrideFlags;
	float							DitheredLODTransitionAlpha;
};
struct FMeshDrawingPolicyOverrideSettings
{
	EDrawingPolicyOverrideFlags	MeshOverrideFlags = EDrawingPolicyOverrideFlags::None;
	D3D11_PRIMITIVE_TOPOLOGY	MeshPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
};
inline FMeshDrawingPolicyOverrideSettings ComputeMeshOverrideSettings(const FMeshBatch& Mesh)
{
	FMeshDrawingPolicyOverrideSettings OverrideSettings;
	OverrideSettings.MeshPrimitiveType = (D3D_PRIMITIVE_TOPOLOGY)Mesh.Type;

	OverrideSettings.MeshOverrideFlags |= Mesh.bDisableBackfaceCulling ? EDrawingPolicyOverrideFlags::TwoSided : EDrawingPolicyOverrideFlags::None;
	OverrideSettings.MeshOverrideFlags |= Mesh.bDitheredLODTransition ? EDrawingPolicyOverrideFlags::DitheredLODTransition : EDrawingPolicyOverrideFlags::None;
	OverrideSettings.MeshOverrideFlags |= Mesh.bWireframe ? EDrawingPolicyOverrideFlags::Wireframe : EDrawingPolicyOverrideFlags::None;
	OverrideSettings.MeshOverrideFlags |= Mesh.ReverseCulling ? EDrawingPolicyOverrideFlags::ReverseCullMode : EDrawingPolicyOverrideFlags::None;
	return OverrideSettings;
}
/**
* Creates and sets the base PSO so that resources can be set. Generally best to call during SetSharedState.
*/
template<class DrawingPolicyType>
void CommitGraphicsPipelineState(const DrawingPolicyType& DrawingPolicy, const FDrawingPolicyRenderState& DrawRenderState, const FBoundShaderStateInput& BoundShaderStateInput)
{
	//FGraphicsPipelineStateInitializer GraphicsPSOInit;

	//GraphicsPSOInit.PrimitiveType = DrawingPolicy.GetPrimitiveType();
	//GraphicsPSOInit.BoundShaderState = BoundShaderStateInput;
	//GraphicsPSOInit.RasterizerState = DrawingPolicy.ComputeRasterizerState(DrawRenderState.GetViewOverrideFlags());

	D3D11DeviceContext->IASetPrimitiveTopology(DrawingPolicy.GetPrimitiveType());
	D3D11DeviceContext->VSSetShader(BoundShaderStateInput.VertexShaderRHI, 0, 0);
	D3D11DeviceContext->HSSetShader(BoundShaderStateInput.HullShaderRHI, 0, 0);
	D3D11DeviceContext->DSSetShader(BoundShaderStateInput.DomainShaderRHI, 0, 0);
	D3D11DeviceContext->GSSetShader(BoundShaderStateInput.GeometryShaderRHI, 0, 0);
	D3D11DeviceContext->PSSetShader(BoundShaderStateInput.PixelShaderRHI, 0, 0);
	D3D11DeviceContext->RSSetState(TStaticRasterizerState<>::GetRHI());

	//check(DrawRenderState.GetDepthStencilState());
	//check(DrawRenderState.GetBlendState());
	//DrawRenderState.ApplyToPSO(GraphicsPSOInit);

	ID3D11InputLayout* InputLayout = GetInputLayout(BoundShaderStateInput.VertexDeclarationRHI.get(), BoundShaderStateInput.VSCode);
	D3D11DeviceContext->IASetInputLayout(InputLayout);
	D3D11DeviceContext->OMSetBlendState((ID3D11BlendState*)DrawRenderState.GetBlendState(),NULL,0xffffffff);
	D3D11DeviceContext->OMSetDepthStencilState((ID3D11DepthStencilState*)DrawRenderState.GetDepthStencilState(),0);

	//RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

	//CreateGraphicsPipelineState
	//SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
	//RHICmdList.SetStencilRef(DrawRenderState.GetStencilRef());
}

class FMeshDrawingPolicy
{
public:
	struct ElementDataType {};

	/** Context data required by the drawing policy that is not known when caching policies in static mesh draw lists. */
	struct ContextDataType
	{
		ContextDataType(const bool InbIsInstancedStereo) : bIsInstancedStereo(InbIsInstancedStereo) {};
		ContextDataType() : bIsInstancedStereo(false) {};
		bool bIsInstancedStereo;
	};

	FMeshDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		const FMaterial& InMaterialResource,
		const FMeshDrawingPolicyOverrideSettings& InOverrideSettings
		//EDebugViewShaderMode InDebugViewShaderMode = DVSM_None
	);

	void SetMeshRenderState(
		ID3D11DeviceContext* Context,
		const FSceneView& View,
		//const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMeshBatch& Mesh,
		int32 BatchElementIndex//,
		//const FDrawingPolicyRenderState& DrawRenderState,
		//const ElementDataType& ElementData,
		//const ContextDataType PolicyContext
	) const
	{
	}

	void DrawMesh(ID3D11DeviceContext* Context, const FSceneView& View, const FMeshBatch& Mesh, int32 BatchElementIndex, const bool bIsInstancedStereo = false) const;

	void SetupPipelineState(FDrawingPolicyRenderState& DrawRenderState, const FSceneView& View) const {}

	void SetSharedState(ID3D11DeviceContext* Context, const FDrawingPolicyRenderState& DrawRenderState, const FSceneView* View, const FMeshDrawingPolicy::ContextDataType PolicyContext) const;

	const std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>>& GetVertexDeclaration() const;

	// Accessors.
	bool IsTwoSided() const
	{
		return MeshCullMode == D3D11_CULL_NONE;
	}
	bool IsDitheredLODTransition() const
	{
		return bIsDitheredLODTransitionMaterial;
	}
	bool IsWireframe() const
	{
		return MeshFillMode == D3D11_FILL_WIREFRAME;
	}

	D3D11_PRIMITIVE_TOPOLOGY GetPrimitiveType() const
	{
		return MeshPrimitiveType;
	}

	bool GetUsePositionOnlyVS() const
	{
		return bUsePositionOnlyVS;
	}
protected:
	const FMaterialShader* BaseVertexShader = nullptr;
	const FVertexFactory* VertexFactory;
	const FMaterialRenderProxy* MaterialRenderProxy;
	const FMaterial* MaterialResource;

	D3D11_FILL_MODE MeshFillMode;
	D3D11_CULL_MODE MeshCullMode;
	D3D11_PRIMITIVE_TOPOLOGY		MeshPrimitiveType;

	uint32 InstanceFactor = 1;
	uint32 bIsDitheredLODTransitionMaterial : 1;
	uint32 bUsePositionOnlyVS : 1;
	uint32 DebugViewShaderMode : 6; // EDebugViewShaderMode

private:
	void SetInstanceParameters( const FSceneView& View, uint32 InVertexOffset, uint32 InInstanceOffset, uint32 InInstanceCount) const;
};