#include "MultiSizeIndexContainer.h"

void FMultiSizeIndexContainer::InitResources()
{
	IndexBufferRHI = CreateIndexBuffer(IndexBuffer.data(), IndexBuffer.size() * sizeof(uint32));
}

void FMultiSizeIndexContainer::ReleaseResources()
{
	IndexBufferRHI.Reset();
}

void FMultiSizeIndexContainer::RebuildIndexBuffer(uint8 InDataTypeSize, const std::vector<uint32>& NewArray)
{
	IndexBuffer = NewArray;
}

