#ifndef __UniformBuffer_LocalVF_Definition__
#define __UniformBuffer_LocalVF_Definition__
cbuffer LocalVF
{
	int3 LocalVF_VertexFetch_Parameters;
}
Buffer<float2> LocalVF_VertexFetch_TexCoordBuffer;
Buffer<float4> LocalVF_VertexFetch_PackedTangentsBuffer;
Buffer<float4> LocalVF_VertexFetch_ColorComponentsBuffer;
static const struct
{
	int3 VertexFetch_Parameters;
	Buffer<float2> VertexFetch_TexCoordBuffer;
	Buffer<float4> VertexFetch_PackedTangentsBuffer;
	Buffer<float4> VertexFetch_ColorComponentsBuffer;
} LocalVF = { LocalVF_VertexFetch_Parameters, LocalVF_VertexFetch_TexCoordBuffer, LocalVF_VertexFetch_PackedTangentsBuffer, LocalVF_VertexFetch_ColorComponentsBuffer  };
#endif
