#pragma once

#include "D3D11RHI.h"

#include <vector>

class FMultiSizeIndexContainer
{
public:
	void InitResources();
	void ReleaseResources();

	void RebuildIndexBuffer(uint8 InDataTypeSize, const std::vector<uint32>& NewArray);

	ID3D11Buffer* GetIndexBuffer()
	{
		assert(IndexBufferRHI.Get() != NULL);
		return IndexBufferRHI.Get();
	}
	const ID3D11Buffer* GetIndexBuffer() const
	{
		assert(IndexBufferRHI.Get() != NULL);
		return IndexBufferRHI.Get();
	}
private:
	std::vector<uint32> IndexBuffer;
	ComPtr<ID3D11Buffer> IndexBufferRHI;
};