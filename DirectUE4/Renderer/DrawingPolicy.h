#pragma once

#include "D3D11RHI.h"
#include "UnrealMath.h"
#include "SceneView.h"
#include "MeshBach.h"
#include "VertexFactory.h"

struct FDrawingPolicyRenderState
{
	FDrawingPolicyRenderState(const SceneView& View, std::shared_ptr<FUniformBuffer> InPassUniformBuffer = nullptr) :
		BlendState(nullptr)
		, DepthStencilState(nullptr)
		, DepthStencilAccess(FExclusiveDepthStencil::DepthRead_StencilRead)
		, ViewUniformBuffer(View.ViewUniformBuffer)
		, PassUniformBuffer(InPassUniformBuffer)
		, StencilRef(0)
		//, ViewOverrideFlags(EDrawingPolicyOverrideFlags::None)
		, DitheredLODTransitionAlpha(0.0f)
	{
		//ViewOverrideFlags |= SceneView.bReverseCulling ? EDrawingPolicyOverrideFlags::ReverseCullMode : EDrawingPolicyOverrideFlags::None;
		//ViewOverrideFlags |= SceneView.bRenderSceneTwoSided ? EDrawingPolicyOverrideFlags::TwoSided : EDrawingPolicyOverrideFlags::None;
	}

	FDrawingPolicyRenderState() :
		BlendState(nullptr)
		, DepthStencilState(nullptr)
		//, ViewUniformBuffer(nullptr)
		//, PassUniformBuffer(nullptr)
		, StencilRef(0)
		//, ViewOverrideFlags(EDrawingPolicyOverrideFlags::None)
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
		//, ViewOverrideFlags(DrawRenderState.ViewOverrideFlags)
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

	inline const ID3D11BlendState* GetBlendState() const
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

	inline const ID3D11DepthStencilState* GetDepthStencilState() const
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

	inline void SetPassUniformBuffer(std::shared_ptr<FUniformBuffer> InPassUniformBuffer)
	{
		PassUniformBuffer = InPassUniformBuffer;
	}

	inline std::shared_ptr<FUniformBuffer> GetPassUniformBuffer() const
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


// 	inline EDrawingPolicyOverrideFlags& ModifyViewOverrideFlags()
// 	{
// 		return ViewOverrideFlags;
// 	}
// 
// 	inline EDrawingPolicyOverrideFlags GetViewOverrideFlags() const
// 	{
// 		return ViewOverrideFlags;
// 	}
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
	std::shared_ptr<FUniformBuffer>	PassUniformBuffer;
	uint32							StencilRef;

	//not sure if those should belong here
	//EDrawingPolicyOverrideFlags		ViewOverrideFlags;
	float							DitheredLODTransitionAlpha;
};


class FMeshDrawingPolicy
{
public:
	FMeshDrawingPolicy(
		const FVertexFactory* InVertexFactory//,
		//const FMaterialRenderProxy* InMaterialRenderProxy,
		//const FMaterial& InMaterialResource,
		//const FMeshDrawingPolicyOverrideSettings& InOverrideSettings,
		//EDebugViewShaderMode InDebugViewShaderMode = DVSM_None
	);

	void SetMeshRenderState(
		ID3D11DeviceContext* Context,
		const SceneView& View,
		//const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMeshBatch& Mesh,
		int32 BatchElementIndex//,
		//const FDrawingPolicyRenderState& DrawRenderState,
		//const ElementDataType& ElementData,
		//const ContextDataType PolicyContext
	) const
	{
	}

	void DrawMesh(ID3D11DeviceContext* Context, const SceneView& View, const FMeshBatch& Mesh, int32 BatchElementIndex, const bool bIsInstancedStereo = false) const;

	void SetSharedState(ID3D11DeviceContext* Context, const FDrawingPolicyRenderState& DrawRenderState, const SceneView* View/*, const FMeshDrawingPolicy::ContextDataType PolicyContext*/) const;
protected:
	const ID3D11VertexShader* BaseVertexShader = nullptr;
	const FVertexFactory* VertexFactory;
	//const FMaterialRenderProxy* MaterialRenderProxy;
	//const FMaterial* MaterialResource;

	D3D11_FILL_MODE MeshFillMode;
	D3D11_CULL_MODE MeshCullMode;
	D3D11_PRIMITIVE_TOPOLOGY		MeshPrimitiveType;

	uint32 InstanceFactor = 1;
	uint32 bIsDitheredLODTransitionMaterial : 1;
	uint32 bUsePositionOnlyVS : 1;
	uint32 DebugViewShaderMode : 6; // EDebugViewShaderMode
};