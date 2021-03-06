#include "DrawingPolicy.h"

FMeshDrawingPolicy::FMeshDrawingPolicy(
	const FVertexFactory* InVertexFactory,  
const FMaterialRenderProxy* InMaterialRenderProxy, 
const FMaterial& InMaterialResource, 
const FMeshDrawingPolicyOverrideSettings& InOverrideSettings
/*EDebugViewShaderMode InDebugViewShaderMode = DVSM_None */)
	: VertexFactory(InVertexFactory),
	MaterialRenderProxy(InMaterialRenderProxy),
	MaterialResource(&InMaterialResource)//,
{
	MeshFillMode = D3D11_FILL_SOLID;
	MeshCullMode = D3D11_CULL_BACK;
	MeshPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	bool bMaterialResourceIsTwoSided = InMaterialResource.IsTwoSided();

	const bool bIsWireframeMaterial = InMaterialResource.IsWireframe() || !!(InOverrideSettings.MeshOverrideFlags & EDrawingPolicyOverrideFlags::Wireframe);
	MeshFillMode = bIsWireframeMaterial ? D3D11_FILL_SOLID : D3D11_FILL_WIREFRAME;

	const bool bInTwoSidedOverride = !!(InOverrideSettings.MeshOverrideFlags & EDrawingPolicyOverrideFlags::TwoSided);
	const bool bInReverseCullModeOverride = !!(InOverrideSettings.MeshOverrideFlags & EDrawingPolicyOverrideFlags::ReverseCullMode);
	const bool bIsTwoSided = (bMaterialResourceIsTwoSided || bInTwoSidedOverride);
	const bool bMeshRenderTwoSided = bIsTwoSided || bInTwoSidedOverride;
	MeshCullMode = (bMeshRenderTwoSided) ? D3D11_CULL_NONE : (bInReverseCullModeOverride ? D3D11_CULL_BACK : D3D11_CULL_FRONT);

	bUsePositionOnlyVS = false;
}

void FMeshDrawingPolicy::DrawMesh(ID3D11DeviceContext* Context, const FSceneView& View, const FMeshBatch& Mesh, int32 BatchElementIndex, const bool bIsInstancedStereo /*= false*/) const
{
	const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];
	const uint32 InstanceCount = ((bIsInstancedStereo && !BatchElement.bIsInstancedMesh) ? 2 : BatchElement.NumInstances);
	SetInstanceParameters(View, BatchElement.BaseVertexIndex, 0, InstanceCount);

	CommitNonComputeShaderConstants();
	Context->IASetIndexBuffer((ID3D11Buffer*)BatchElement.IndexBuffer,DXGI_FORMAT_R32_UINT,0);
	Context->DrawIndexed(BatchElement.NumPrimitives*3, BatchElement.FirstIndex, BatchElement.BaseVertexIndex);
	ClearRenderState();
}

void FMeshDrawingPolicy::SetSharedState(ID3D11DeviceContext* Context, const FDrawingPolicyRenderState& DrawRenderState, const FSceneView* View, const ContextDataType PolicyContext) const
{
	VertexFactory->SetStreams(Context);
}

const std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>>& FMeshDrawingPolicy::GetVertexDeclaration() const
{
	const std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>>& VertexDeclaration = VertexFactory->GetDeclaration();
	return VertexDeclaration;
}

void FMeshDrawingPolicy::SetInstanceParameters(const FSceneView& View, uint32 InVertexOffset, uint32 InInstanceOffset, uint32 InInstanceCount) const
{
	BaseVertexShader->SetInstanceParameters(InVertexOffset, InInstanceOffset, InInstanceCount);
}

