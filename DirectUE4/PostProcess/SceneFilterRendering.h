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

void InitScreenRectangleResources();

std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>> GetFilterInputDelcaration();

struct alignas(16) FDrawRectangleParameters
{
	FDrawRectangleParameters()
	{
		ConstructUniformBufferInfo(*this);
	}
	struct ConstantStruct
	{
		Vector4 PosScaleBias;
		Vector4 UVScaleBias;
		Vector4 InvTargetSizeAndTextureSize;
	} Constants;

	static std::string GetConstantBufferName()
	{
		return "DrawRectangleParameters";
	}
	static std::map<std::string, ID3D11ShaderResourceView*> GetSRVs(const FDrawRectangleParameters& DrawRectangleParameters)
	{
		std::map<std::string, ID3D11ShaderResourceView*> List;
		return List;
	}
	static std::map<std::string, ID3D11SamplerState*> GetSamplers(const FDrawRectangleParameters& DrawRectangleParameters)
	{
		std::map<std::string, ID3D11SamplerState*> List;
		return List;
	}
	static std::map<std::string, ID3D11UnorderedAccessView*> GetUAVs(const FDrawRectangleParameters& DrawRectangleParameters)
	{
		std::map<std::string, ID3D11UnorderedAccessView*> List;
		return List;
	}

};

extern void DrawPostProcessPass(
	float X,
	float Y,
	float SizeX,
	float SizeY,
	float U,
	float V,
	float SizeU,
	float SizeV,
	FIntPoint TargetSize,
	FIntPoint TextureSize,
	class FShader* VertexShader//,
	/*EStereoscopicPass StereoView,*/
	//bool bHasCustomMesh,
	//EDrawRectangleFlags Flags = EDRF_Default
);

