#pragma once

#include "VertexFactory.h"
#include "ResourcePool.h"


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

class FSharedPoolPolicyData
{
public:
	/** Buffers are created with a simple byte size */
	typedef uint32 CreationArguments;
	enum
	{
		NumSafeFrames = 3, /** Number of frames to leaves buffers before reclaiming/reusing */
		NumPoolBucketSizes = 17, /** Number of pool buckets */
		NumToDrainPerFrame = 10, /** Max. number of resources to cull in a single frame */
		CullAfterFramesNum = 30 /** Resources are culled if unused for more frames than this */
	};

	/** Get the pool bucket index from the size
	* @param Size the number of bytes for the resource
	* @returns The bucket index.
	*/
	uint32 GetPoolBucketIndex(uint32 Size);

	/** Get the pool bucket size from the index
	* @param Bucket the bucket index
	* @returns The bucket size.
	*/
	uint32 GetPoolBucketSize(uint32 Bucket);

private:
	/** The bucket sizes */
	static uint32 BucketSizes[NumPoolBucketSizes];
};

struct FVertexBufferAndSRV
{
	void SafeRelease()
	{
		VertexBufferRHI.Reset();
		VertexBufferSRV.Reset();
	}
	
	ComPtr<ID3D11Buffer> VertexBufferRHI;
	ComPtr<ID3D11ShaderResourceView> VertexBufferSRV;
};

inline bool IsValidRef(const FVertexBufferAndSRV& Buffer)
{
	return IsValidRef(Buffer.VertexBufferRHI) && IsValidRef(Buffer.VertexBufferSRV);
}
/** The policy for pooling bone vertex buffers */
class FBoneBufferPoolPolicy : public FSharedPoolPolicyData
{
public:
	enum
	{
		NumSafeFrames = FSharedPoolPolicyData::NumSafeFrames,
		NumPoolBuckets = FSharedPoolPolicyData::NumPoolBucketSizes,
		NumToDrainPerFrame = FSharedPoolPolicyData::NumToDrainPerFrame,
		CullAfterFramesNum = FSharedPoolPolicyData::CullAfterFramesNum
	};
	/** Creates the resource
	* @param Args The buffer size in bytes.
	*/
	FVertexBufferAndSRV CreateResource(FSharedPoolPolicyData::CreationArguments Args);

	/** Gets the arguments used to create resource
	* @param Resource The buffer to get data for.
	* @returns The arguments used to create the buffer.
	*/
	FSharedPoolPolicyData::CreationArguments GetCreationArguments(const FVertexBufferAndSRV& Resource);

	/** Frees the resource
	* @param Resource The buffer to prepare for release from the pool permanently.
	*/
	void FreeResource(FVertexBufferAndSRV Resource);
};
class FBoneBufferPool : public TRenderResourcePool<FVertexBufferAndSRV, FBoneBufferPoolPolicy, FSharedPoolPolicyData::CreationArguments>
{
public:
	/** Destructor */
	virtual ~FBoneBufferPool();
};

class FGPUBaseSkinVertexFactory : public FVertexFactory
{
public:
	struct FShaderDataType
	{
		FShaderDataType()
			: CurrentBuffer(0)
			, PreviousRevisionNumber(0)
			, CurrentRevisionNumber(0)
		{
			MaxGPUSkinBones = GetMaxGPUSkinBones();
		}
		bool UpdateBoneData(const std::vector<FMatrix>& ReferenceToLocalMatrices, const std::vector<FBoneIndexType>& BoneMap, uint32 RevisionNumber, bool bPrevious, bool bUseSkinCache);

		void ReleaseBoneData()
		{
			for (uint32 i = 0; i < 2; ++i)
			{
				if (IsValidRef(BoneBuffer[i]))
				{
					//BoneBufferPool.ReleasePooledResource(BoneBuffer[i]);
				}
				BoneBuffer[i].SafeRelease();
			}
		}

		// @param bPrevious true:previous, false:current
		const FVertexBufferAndSRV& GetBoneBufferForReading(bool bPrevious) const
		{
			const FVertexBufferAndSRV* RetPtr = &GetBoneBufferInternal(bPrevious);

			if (!RetPtr->VertexBufferRHI.Get())
			{
				// this only should happen if we request the old data
				assert(bPrevious);

				// if we don't have any old data we use the current one
				RetPtr = &GetBoneBufferInternal(false);

				// at least the current one needs to be valid when reading
				assert(RetPtr->VertexBufferRHI.Get());
			}

			return *RetPtr;
		}

		// @param bPrevious true:previous, false:current
		// @return IsValid() can fail, then you have to create the buffers first (or if the size changes)
		FVertexBufferAndSRV& GetBoneBufferForWriting(bool bPrevious)
		{
			const FShaderDataType* This = (const FShaderDataType*)this;
			// non const version maps to const version
			return (FVertexBufferAndSRV&)This->GetBoneBufferInternal(bPrevious);
		}

		// @param bPrevious true:previous, false:current
		// @return returns revision number 
		uint32 GetRevisionNumber(bool bPrevious)
		{
			return (bPrevious) ? PreviousRevisionNumber : CurrentRevisionNumber;
		}
	private:
		// double buffered bone positions+orientations to support normal rendering and velocity (new-old position) rendering
		FVertexBufferAndSRV BoneBuffer[2];
		// 0 / 1 to index into BoneBuffer
		uint32 CurrentBuffer;
		// RevisionNumber Tracker
		uint32 PreviousRevisionNumber;
		uint32 CurrentRevisionNumber;

		static uint32 MaxGPUSkinBones;

		// @param RevisionNumber - updated last revision number
		// This flips revision number to previous if this is new
		// otherwise, it keeps current version
		void SetCurrentRevisionNumber(uint32 RevisionNumber)
		{
			if (CurrentRevisionNumber != RevisionNumber)
			{
				PreviousRevisionNumber = CurrentRevisionNumber;
				CurrentRevisionNumber = RevisionNumber;
				CurrentBuffer = 1 - CurrentBuffer;
			}
		}
		// to support GetBoneBufferForWriting() and GetBoneBufferForReading()
		// @param bPrevious true:previous, false:current
		// @return might not pass the IsValid() 
		const FVertexBufferAndSRV& GetBoneBufferInternal(bool bPrevious) const
		{
			if ((CurrentRevisionNumber - PreviousRevisionNumber) > 1)
			{
				bPrevious = false;
			}

			uint32 BufferIndex = CurrentBuffer ^ (uint32)bPrevious;

			const FVertexBufferAndSRV& Ret = BoneBuffer[BufferIndex];
			return Ret;
		}
	};


	FGPUBaseSkinVertexFactory(uint32 InNumVertices)
		: FVertexFactory()
		, NumVertices(InNumVertices)
	{
	}
	virtual ~FGPUBaseSkinVertexFactory() {}

	FShaderDataType& GetShaderData()
	{
		return ShaderData;
	}
	const FShaderDataType& GetShaderData() const
	{
		return ShaderData;
	}

	virtual bool UsesExtraBoneInfluences() const { return false; }

	static bool SupportsTessellationShaders() { return true; }

	uint32 GetNumVertices() const
	{
		return NumVertices;
	}

	static int32 GetMaxGPUSkinBones();

	static const uint32 GHardwareMaxGPUSkinBones = 256;

	virtual const ComPtr<ID3D11ShaderResourceView> GetPositionsSRV() const = 0;
	virtual const ComPtr<ID3D11ShaderResourceView> GetTangentsSRV() const = 0;
	virtual const ComPtr<ID3D11ShaderResourceView> GetTextureCoordinatesSRV() const = 0;
	virtual const ComPtr<ID3D11ShaderResourceView> GetColorComponentsSRV() const = 0;
	virtual uint32 GetNumTexCoords() const = 0;
	virtual const uint32 GetColorIndexMask() const = 0;

	inline const FVertexStreamComponent& GetTangentStreamComponent(int Index)
	{
		assert(TangentStreamComponents[Index].VertexBuffer != nullptr);
		return TangentStreamComponents[Index];
	}
protected:
	FShaderDataType ShaderData;

	static FBoneBufferPool BoneBufferPool;

	FVertexStreamComponent TangentStreamComponents[2];
private:
	uint32 NumVertices;
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

	TGPUSkinVertexFactory(uint32 InNumVertices) 
		 : FGPUBaseSkinVertexFactory(InNumVertices)
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

	const uint32 GetColorIndexMask() const override
	{
		return Data.ColorIndexMask;
	}

	const ComPtr<ID3D11ShaderResourceView> GetPositionsSRV() const override
	{
		return Data.PositionComponentSRV;
	}

	const ComPtr<ID3D11ShaderResourceView> GetTangentsSRV() const override
	{
		return Data.TangentsSRV;
	}

	const ComPtr<ID3D11ShaderResourceView> GetTextureCoordinatesSRV() const override
	{
		return Data.TextureCoordinatesSRV;
	}

	uint32 GetNumTexCoords() const override
	{
		return Data.NumTexCoords;
	}

	const ComPtr<ID3D11ShaderResourceView> GetColorComponentsSRV() const override
	{
		return Data.ColorComponentsSRV;
	}
protected:
	void AddVertexElements(FDataType& InData, std::vector<D3D11_INPUT_ELEMENT_DESC>& OutElements);

	inline const FDataType& GetData() const { return Data; }
private:
	FDataType Data;
};

