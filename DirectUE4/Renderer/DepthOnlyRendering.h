#pragma once

#include "D3D11RHI.h"
#include "UnrealMath.h"
#include "DrawingPolicy.h"

void InitPrePass();

class FDepthDrawingPolicy : public FMeshDrawingPolicy
{
public:
	FDepthDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		//const FMaterialRenderProxy* InMaterialRenderProxy,
		//const FMaterial& InMaterialResource,
		//const FMeshDrawingPolicyOverrideSettings& InOverrideSettings,
		//ERHIFeatureLevel::Type InFeatureLevel,
		float MobileColorValue
	);

	void SetMeshRenderState(
		ID3D11DeviceContext* Context,
		const SceneView& View,
		//const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMeshBatch& Mesh,
		int32 BatchElementIndex,
		const FDrawingPolicyRenderState& DrawRenderState/*,*/
		//const ElementDataType& ElementData,
		//const ContextDataType PolicyContext
	) const;

private:
	ID3D11VertexShader* VertexShader;
	ID3D11PixelShader* PixelShader;
};
/**
* Writes out depth for opaque materials on meshes which support a position-only vertex buffer.
* Using the position-only vertex buffer saves vertex fetch bandwidth during the z prepass.
*/
class FPositionOnlyDepthDrawingPolicy : public FMeshDrawingPolicy
{
public:

	// 	struct ContextDataType : public FMeshDrawingPolicy::ContextDataType
	// 	{
	// 		explicit ContextDataType(const bool InbIsInstancedStereo) : FMeshDrawingPolicy::ContextDataType(InbIsInstancedStereo), bIsInstancedStereoEmulated(false) {};
	// 		ContextDataType(const bool InbIsInstancedStereo, const bool InbIsInstancedStereoEmulated) : FMeshDrawingPolicy::ContextDataType(InbIsInstancedStereo), bIsInstancedStereoEmulated(InbIsInstancedStereoEmulated) {};
	// 		ContextDataType() : bIsInstancedStereoEmulated(false) {};
	// 		bool bIsInstancedStereoEmulated;
	// 	};

	FPositionOnlyDepthDrawingPolicy(
		const FVertexFactory* InVertexFactory//,
											 //const FMaterialRenderProxy* InMaterialRenderProxy,
											 //const FMaterial& InMaterialResource,
											 //const FMeshDrawingPolicyOverrideSettings& InOverrideSettings
	);

	// FMeshDrawingPolicy interface.

	//void ApplyDitheredLODTransitionState(FDrawingPolicyRenderState& DrawRenderState, const FViewInfo& ViewInfo, const FStaticMesh& Mesh, const bool InAllowStencilDither);

	// 	FDrawingPolicyMatchResult Matches(const FPositionOnlyDepthDrawingPolicy& Other, bool bForReals = false) const
	// 	{
	// 		DRAWING_POLICY_MATCH_BEGIN
	// 			DRAWING_POLICY_MATCH(FMeshDrawingPolicy::Matches(Other, bForReals)) &&
	// 			DRAWING_POLICY_MATCH(VertexShader == Other.VertexShader);
	// 		DRAWING_POLICY_MATCH_END
	// 	}

	void SetSharedState(ID3D11DeviceContext* Context, const FDrawingPolicyRenderState& DrawRenderState, const SceneView* View/*,*/ /*const FPositionOnlyDepthDrawingPolicy::ContextDataType PolicyContext*/) const;

	/**
	* Create bound shader state using the vertex decl from the mesh draw policy
	* as well as the shaders needed to draw the mesh
	* @return new bound shader state object
	*/
	//FBoundShaderStateInput GetBoundShaderStateInput(ERHIFeatureLevel::Type InFeatureLevel) const;

	void SetMeshRenderState(
		ID3D11DeviceContext* Context,
		const SceneView& View,
		//const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMeshBatch& Mesh,
		int32 BatchElementIndex,
	const FDrawingPolicyRenderState& DrawRenderState/*,*/
	//const ElementDataType& ElementData,
	//const ContextDataType PolicyContext
	) const;

	//void SetInstancedEyeIndex(ID3D11DeviceContext* Context, const uint32 EyeIndex) const;

	//friend int32 CompareDrawingPolicy(const FPositionOnlyDepthDrawingPolicy& A, const FPositionOnlyDepthDrawingPolicy& B);

private:
	//FShaderPipeline * ShaderPipeline;
	ID3D11VertexShader * VertexShader;
};

class FStaticMesh;
class ViewInfo;
/**
* A drawing policy factory for the depth drawing policy.
*/
class FDepthDrawingPolicyFactory
{
public:

// 	enum { bAllowSimpleElements = false };
// 	struct ContextType
// 	{
// 		EDepthDrawingMode DepthDrawingMode;
// 		float MobileColorValue;
// 
// 		/** Uses of FDepthDrawingPolicyFactory that are not the depth pre-pass will not want the bUseAsOccluder flag to interfere. */
// 		bool bRespectUseAsOccluderFlag;
// 
// 		ContextType(EDepthDrawingMode InDepthDrawingMode, bool bInRespectUseAsOccluderFlag) :
// 			DepthDrawingMode(InDepthDrawingMode),
// 			MobileColorValue(0.0f),
// 			bRespectUseAsOccluderFlag(bInRespectUseAsOccluderFlag)
// 		{}
// 	};

	static void AddStaticMesh(FScene* Scene, FStaticMesh* StaticMesh);
	static bool DrawDynamicMesh(
		ID3D11DeviceContext* Context,
		const ViewInfo& View,
		//ContextType DrawingContext,
		const FMeshBatch& Mesh,
		//bool bPreFog,
		//const FDrawingPolicyRenderState& DrawRenderState,
		//const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		//FHitProxyId HitProxyId,
		//const bool bIsInstancedStereo = false,
		const bool bIsInstancedStereoEmulated = false
	);

	static bool DrawStaticMesh(
		ID3D11DeviceContext* Context,
		const ViewInfo& View,
		//ContextType DrawingContext,
		const FStaticMesh& StaticMesh,
		const uint64& BatchElementMask,
		//bool bPreFog,
		//const FDrawingPolicyRenderState& DrawRenderState,
		//const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		//FHitProxyId HitProxyId,
		//const bool bIsInstancedStereo = false,
		const bool bIsInstancedStereoEmulated = false
	);

private:
	/**
	* Render a dynamic or static mesh using a depth draw policy
	* @return true if the mesh rendered
	*/
	static bool DrawMesh(
		ID3D11DeviceContext* Context,
		const ViewInfo& View,
		//ContextType DrawingContext,
		const FMeshBatch& Mesh,
		const uint64& BatchElementMask,
		const FDrawingPolicyRenderState& DrawRenderState,
		bool bPreFog,
		//const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		//FHitProxyId HitProxyId,
		const bool bIsInstancedStereo = false,
		const bool bIsInstancedStereoEmulated = false
	);
};
