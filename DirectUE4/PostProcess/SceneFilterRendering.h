#pragma once

#include "UnrealMath.h"
#include "D3D11RHI.h"

/** The vertex data used to filter a texture. */
struct FilterVertex
{
	Vector4 Position;
	Vector2 UV;
};

extern ID3D11Buffer* GScreenRectangleVertexBuffer; 
extern ID3D11Buffer* GScreenRectangleIndexBuffer;
extern ID3D11InputLayout* GFilterInputLayout;
extern ID3D11VertexShader* GCommonPostProcessVS;

void InitScreenRectangleResources();