#pragma once

#include "UnrealMath.h"
#include "SkeletalMeshModel.h"
#include "D3D11RHI.h"

#include <vector>

class FSkinWeightVertexBuffer
{
public:
	void Init(const std::vector<SoftSkinVertex>& InVertices);

	void SetHasExtraBoneInfluences(bool bInHasExtraBoneInfluences)
	{
		bExtraBoneInfluences = bInHasExtraBoneInfluences;
	}

	void InitResources();
	void ReleaseResources();

	ComPtr<ID3D11Buffer> WeightVertexBufferRHI = NULL;
private:
	std::vector<uint8> WeightData;

	bool bExtraBoneInfluences;
};