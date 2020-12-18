#include "VolumeRendering.h"

IMPLEMENT_SHADER_TYPE(, FWriteToSliceGS, ("TranslucentLightingShaders.dusf"), ("WriteToSliceMainGS"), SF_Geometry);
IMPLEMENT_SHADER_TYPE(, FWriteToSliceVS, ("TranslucentLightingShaders.dusf"), ("WriteToSliceMainVS"), SF_Vertex);

FVolumeRasterizeVertexBuffer GVolumeRasterizeVertexBuffer;

void RasterizeToVolumeTexture(FVolumeBounds VolumeBounds)
{
	// Setup the viewport to only render to the given bounds
	RHISetViewport(VolumeBounds.MinX, VolumeBounds.MinY, 0, VolumeBounds.MaxX, VolumeBounds.MaxY, 0);
	UINT Stride = sizeof(FScreenVertex);
	UINT Offset = 0;
	D3D11DeviceContext->IASetVertexBuffers(0, 1, &GVolumeRasterizeVertexBuffer.VertexBufferRHI, &Stride, &Offset);
	//RHICmdList.SetStreamSource(0, GVolumeRasterizeVertexBuffer.VertexBufferRHI, 0);
	const int32 NumInstances = VolumeBounds.MaxZ - VolumeBounds.MinZ;
	// Render a quad per slice affected by the given bounds
	//RHICmdList.DrawPrimitive(PT_TriangleStrip, 0, 2, NumInstances);
	D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	D3D11DeviceContext->DrawInstanced(2, NumInstances,0,0);	
}

