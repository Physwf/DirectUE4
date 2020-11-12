#pragma once

#include "UnrealMath.h"
#include "D3D11RHI.h"

#include <vector>

struct FStaticMeshVertexBuffers
{
	/** The buffer containing vertex data. */
	std::vector<float> StaticMeshVertexBuffer;
	/** The buffer containing the position vertex data. */
	std::vector<Vector> PositionVertexBuffer;
	/** The buffer containing the vertex color data. */
	std::vector<FColor> ColorVertexBuffer;

	ID3D11Buffer* StaticMeshVertexBufferRHI = NULL;
	ID3D11Buffer* PositionVertexBufferRHI = NULL;
	ID3D11Buffer* ColorVertexBufferRHI = NULL;
};