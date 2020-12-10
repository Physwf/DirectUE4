#pragma once

#include "VertexFactory.h"

struct alignas(16) FSkinMatrix3x4
{
	float M[3][4];
	inline void SetMatrix(const FMatrix& Mat)
	{
		const float* Src = &(Mat.M[0][0]);
		float* Dest = &(M[0][0]);

		Dest[0] = Src[0];   // [0][0]
		Dest[1] = Src[1];   // [0][1]
		Dest[2] = Src[2];   // [0][2]
		Dest[3] = Src[3];   // [0][3]

		Dest[4] = Src[4];   // [1][0]
		Dest[5] = Src[5];   // [1][1]
		Dest[6] = Src[6];   // [1][2]
		Dest[7] = Src[7];   // [1][3]

		Dest[8] = Src[8];   // [2][0]
		Dest[9] = Src[9];   // [2][1]
		Dest[10] = Src[10]; // [2][2]
		Dest[11] = Src[11]; // [2][3]
	}

	inline void SetMatrixTranspose(const FMatrix& Mat)
	{

		const float* Src = &(Mat.M[0][0]);
		float* Dest = &(M[0][0]);

		Dest[0] = Src[0];   // [0][0]
		Dest[1] = Src[4];   // [1][0]
		Dest[2] = Src[8];   // [2][0]
		Dest[3] = Src[12];  // [3][0]

		Dest[4] = Src[1];   // [0][1]
		Dest[5] = Src[5];   // [1][1]
		Dest[6] = Src[9];   // [2][1]
		Dest[7] = Src[13];  // [3][1]

		Dest[8] = Src[2];   // [0][2]
		Dest[9] = Src[6];   // [1][2]
		Dest[10] = Src[10]; // [2][2]
		Dest[11] = Src[14]; // [3][2]
	}
};

enum
{
	MAX_GPU_BONE_MATRICES_UNIFORMBUFFER = 75,
};

struct alignas(16) FBoneMatricesUniformShaderParameters
{
	FSkinMatrix3x4 BoneMatrices[MAX_GPU_BONE_MATRICES_UNIFORMBUFFER];
};


class FGPUBaseSkinVertexFactory : public FVertexFactory
{
public:
	static bool SupportsTessellationShaders() { return true; }
};

template<bool bExtraBoneInfluencesT>
class TGPUSkinVertexFactory : public FGPUBaseSkinVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(TGPUSkinVertexFactory<bExtraBoneInfluencesT>);
public:
	struct FDataType : public FStaticMeshDataType
	{
		/** The stream to read the bone indices from */
		FVertexStreamComponent BoneIndices;

		/** The stream to read the extra bone indices from */
		FVertexStreamComponent ExtraBoneIndices;

		/** The stream to read the bone weights from */
		FVertexStreamComponent BoneWeights;

		/** The stream to read the extra bone weights from */
		FVertexStreamComponent ExtraBoneWeights;
	};

	TGPUSkinVertexFactory(uint32 InNumVertices) :NumVertices(InNumVertices)
	{}

	static void ModifyCompilationEnvironment(const class FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment);
	static bool ShouldCompilePermutation(const class FMaterial* Material, const FShaderType* ShaderType);


	void InitRHI();
	void ReleaseRHI();

	static FVertexFactoryShaderParameters* ConstructShaderParameters(EShaderFrequency ShaderFrequency);

	void SetData(const FDataType& InData)
	{
		Data = InData;
		TangentStreamComponents[0] = InData.TangentBasisComponents[0];
		TangentStreamComponents[1] = InData.TangentBasisComponents[1];
		ReleaseRHI();
		InitRHI();
	}


	const ComPtr<ID3D11ShaderResourceView> GetPositionsSRV() const
	{
		return Data.PositionComponentSRV;
	}

	const ComPtr<ID3D11ShaderResourceView> GetTangentsSRV() const
	{
		return Data.TangentsSRV;
	}

	const ComPtr<ID3D11ShaderResourceView> GetTextureCoordinatesSRV() const
	{
		return Data.TextureCoordinatesSRV;
	}

	uint32 GetNumTexCoords() const
	{
		return Data.NumTexCoords;
	}

	const ComPtr<ID3D11ShaderResourceView> GetColorComponentsSRV() const
	{
		return Data.ColorComponentsSRV;
	}
protected:
	void AddVertexElements(FDataType& InData, std::vector<D3D11_INPUT_ELEMENT_DESC>& OutElements);

	inline const FDataType& GetData() const { return Data; }
private:
	FVertexStreamComponent TangentStreamComponents[2];
	uint32 NumVertices;
	FDataType Data;
};

