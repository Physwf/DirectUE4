#include "MeshMaterialShader.h"
#include "MeshBach.h"
#include "DrawingPolicy.h"

template< typename ShaderRHIParamRef >
void FMeshMaterialShader::SetMesh(
	const ShaderRHIParamRef ShaderRHI, 
	const FVertexFactory* VertexFactory, 
	const FSceneView& View, 
	/*const FPrimitiveSceneProxy* Proxy, */ 
	const FMeshBatchElement& BatchElement, 
	const FDrawingPolicyRenderState& DrawRenderState, 
	uint32 DataFlags /*= 0 */)
{

}

