#include "DrawingPolicy.h"

FMeshDrawingPolicy::FMeshDrawingPolicy(
	const FVertexFactory* InVertexFactory/*, */ 
/*const FMaterialRenderProxy* InMaterialRenderProxy, */
/*const FMaterial& InMaterialResource, */ 
/*const FMeshDrawingPolicyOverrideSettings& InOverrideSettings, */
/*EDebugViewShaderMode InDebugViewShaderMode = DVSM_None */)
	: VertexFactory(InVertexFactory)
{
	MeshFillMode = D3D11_FILL_SOLID;
	MeshCullMode = D3D11_CULL_BACK;
	MeshPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

void FMeshDrawingPolicy::DrawMesh(ID3D11DeviceContext* Context, const FSceneView& View, const FMeshBatch& Mesh, int32 BatchElementIndex, const bool bIsInstancedStereo /*= false*/) const
{
	const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];

	Context->IASetIndexBuffer((ID3D11Buffer*)BatchElement.IndexBuffer,DXGI_FORMAT_R32_UINT,0);
	Context->IASetPrimitiveTopology(Mesh.Type);
	Context->DrawIndexed(BatchElement.NumPrimitives, BatchElement.FirstIndex, BatchElement.BaseVertexIndex);
}

void FMeshDrawingPolicy::SetSharedState(ID3D11DeviceContext* Context, const FDrawingPolicyRenderState& DrawRenderState, const FSceneView* View/*, const ContextDataType PolicyContext*/) const
{
	VertexFactory->SetStreams(Context);
}

