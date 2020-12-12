#include "SkinWeightVertexBuffer.h"

void FSkinWeightVertexBuffer::Init(const std::vector<FSoftSkinVertex>& InVertices)
{
	uint32 HalfStride = bExtraBoneInfluences ? MAX_TOTAL_INFLUENCES : MAX_INFLUENCES_PER_STREAM;
	uint32 Stride = 2 * HalfStride;
	WeightData.resize(Stride * InVertices.size());

	for (uint32 VertIdx = 0; VertIdx < InVertices.size(); VertIdx++)
	{
		uint8* VertexStart = WeightData.data() + Stride * VertIdx;
		memcpy(VertexStart, InVertices[VertIdx].InfluenceBones, HalfStride);
		memcpy(VertexStart + HalfStride, InVertices[VertIdx].InfluenceWeights, HalfStride);
	}
}

void FSkinWeightVertexBuffer::InitResources()
{
	WeightVertexBufferRHI = CreateVertexBuffer(false, WeightData.size(), WeightData.data());
}

void FSkinWeightVertexBuffer::ReleaseResources()
{
	WeightVertexBufferRHI.Reset();
}

