#pragma once

#include "UnrealMath.h"
#include "D3D11RHI.h"
#include "ShaderCore.h"

#include <vector>
#include <list>
#include <assert.h>

struct FVertexStreamComponent
{
	/** The vertex buffer to stream data from.  If null, no data can be read from this stream. */
	const ID3D11Buffer* VertexBuffer = nullptr;
	/** The offset to the start of the vertex buffer fetch. */
	uint32 StreamOffset = 0;
	/** The offset of the data, relative to the beginning of each element in the vertex buffer. */
	uint8 Offset = 0;
	/** The stride of the data. */
	uint8 Stride = 0;
	/** The type of the data read from this stream. */
	DXGI_FORMAT Type;

	uint8 VertexStreamUsage;//0,1,2,4
	/**
	* Initializes the data stream to null.
	*/
	FVertexStreamComponent()
	{}

	/**
	* Minimal initialization constructor.
	*/
	FVertexStreamComponent(const ID3D11Buffer* InVertexBuffer, uint32 InOffset, uint32 InStride, DXGI_FORMAT InType, uint8 Usage = 0) :
		VertexBuffer(InVertexBuffer),
		StreamOffset(0),
		Offset(InOffset),
		Stride(InStride),
		Type(InType),
		VertexStreamUsage(Usage)
	{
		assert(InStride <= 0xFF);
		assert(InOffset <= 0xFF);
	}

	FVertexStreamComponent(const ID3D11Buffer* InVertexBuffer, uint32 InStreamOffset, uint32 InOffset, uint32 InStride, DXGI_FORMAT InType, uint8 Usage = 0) :
		VertexBuffer(InVertexBuffer),
		StreamOffset(InStreamOffset),
		Offset(InOffset),
		Stride(InStride),
		Type(InType),
		VertexStreamUsage(Usage)
	{
		assert(InStride <= 0xFF);
		assert(InOffset <= 0xFF);
	}
};

struct FStaticMeshDataType
{
	/** The stream to read the vertex position from. */
	FVertexStreamComponent PositionComponent;

	/** The streams to read the tangent basis from. */
	FVertexStreamComponent TangentBasisComponents[2];

	/** The streams to read the texture coordinates from. */
	FVertexStreamComponent TextureCoordinates;

	/** The stream to read the shadow map texture coordinates from. */
	FVertexStreamComponent LightMapCoordinateComponent;

	/** The stream to read the vertex color from. */
	FVertexStreamComponent ColorComponent;

	ComPtr<ID3D11ShaderResourceView> PositionComponentSRV = nullptr;

	ComPtr<ID3D11ShaderResourceView> TangentsSRV = nullptr;

	/** A SRV to manually bind and load TextureCoordinates in the Vertexshader. */
	ComPtr<ID3D11ShaderResourceView> TextureCoordinatesSRV = nullptr;

	/** A SRV to manually bind and load Colors in the Vertexshader. */
	ComPtr<ID3D11ShaderResourceView> ColorComponentsSRV = nullptr;

	int LightMapCoordinateIndex = -1;
	int NumTexCoords = -1;
	uint32 ColorIndexMask = ~0u;
};

class FShader;
class FMaterial;

class FVertexFactoryShaderParameters
{
public:
	virtual ~FVertexFactoryShaderParameters() {}
	virtual void Bind(const class FShaderParameterMap& ParameterMap) = 0;
	virtual void SetMesh(FShader* VertexShader, const class FVertexFactory* VertexFactory, const class SceneView& View, const struct FMeshBatchElement& BatchElement, uint32 DataFlags) const = 0;
	virtual uint32 GetSize() const { return sizeof(*this); }
};

class FVertexFactoryType
{
public:
	typedef FVertexFactoryShaderParameters* (*ConstructParametersType)(EShaderFrequency ShaderFrequency);
	typedef bool(*ShouldCacheType)(const class FMaterial*, const class FShaderType*);
	typedef void(*ModifyCompilationEnvironmentType)(const class FMaterial*, FShaderCompilerEnvironment&);
	typedef bool(*SupportsTessellationShadersType)();

	static std::list<FVertexFactoryType*>& GetTypeList();
	static FVertexFactoryType* GetVFByName(const std::string& VFName);

	static void Initialize(const std::map<std::string,std::vector<const char*>>& ShaderFileToUniformBufferVariables);

	virtual FVertexFactoryType* GetType() const { return NULL; }

	FVertexFactoryType(
		const char* InName,
		const char* InShaderFilename,
		bool bInUsedWithMaterials,
		bool bInSupportsStaticLighting,
		bool bInSupportsDynamicLighting,
		bool bInSupportsPrecisePrevWorldPos,
		bool bInSupportsPositionOnly,
		ConstructParametersType InConstructParameters,
		ShouldCacheType InShouldCache,
		ModifyCompilationEnvironmentType InModifyCompilationEnvironment,
		SupportsTessellationShadersType InSupportsTessellationShaders
	);

	virtual ~FVertexFactoryType();

	// Accessors.
	const char* GetName() const { return Name; }
	std::string GetFName() const { return TypeName; }
	const char* GetShaderFilename() const { return ShaderFilename; }
	FVertexFactoryShaderParameters* CreateShaderParameters(EShaderFrequency ShaderFrequency) const { return (*ConstructParameters)(ShaderFrequency); }
	bool IsUsedWithMaterials() const { return bUsedWithMaterials; }
	bool SupportsStaticLighting() const { return bSupportsStaticLighting; }
	bool SupportsDynamicLighting() const { return bSupportsDynamicLighting; }
	bool SupportsPrecisePrevWorldPos() const { return bSupportsPrecisePrevWorldPos; }
	bool SupportsPositionOnly() const { return bSupportsPositionOnly; }
	/** Returns an int32 specific to this vertex factory type. */
	inline int32 GetId() const { return HashIndex; }
	static int32 GetNumVertexFactoryTypes() { return NextHashIndex; }

	bool ShouldCache(const class FMaterial* Material, const class FShaderType* ShaderType)
	{
		return (*ShouldCacheRef)(Material, ShaderType);
	}

	void ModifyCompilationEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		char buffer[512];
		sprintf_s(buffer, "#include \"%s\"", GetShaderFilename());
		std::string VertexFactoryIncludeString = buffer;
		OutEnvironment.IncludeVirtualPathToContentsMap["/Engine/Generated/VertexFactory.hlsl"] = VertexFactoryIncludeString;

		OutEnvironment.SetDefine(("HAS_PRIMITIVE_UNIFORM_BUFFER"), 1);

		//(*ModifyCompilationEnvironmentRef)(Platform, Material, OutEnvironment);
	}
private:
	static uint32 NextHashIndex;

	uint32 HashIndex;
	const char*	Name;
	const char*	ShaderFilename;
	std::string	TypeName;
	uint32 bUsedWithMaterials : 1;
	uint32 bSupportsStaticLighting : 1;
	uint32 bSupportsDynamicLighting : 1;
	uint32 bSupportsPrecisePrevWorldPos : 1;
	uint32 bSupportsPositionOnly : 1;
	ConstructParametersType ConstructParameters;
	ShouldCacheType ShouldCacheRef;
	ModifyCompilationEnvironmentType ModifyCompilationEnvironmentRef;
	SupportsTessellationShadersType SupportsTessellationShadersRef;

	std::list<FVertexFactoryType*>::iterator GlobalListLink;
};
class FVertexFactory
{
public:

	void SetStreams(ID3D11DeviceContext* Context) const;
	void SetPositionStream(ID3D11DeviceContext* Context) const;

	void InitDeclaration(std::vector<D3D11_INPUT_ELEMENT_DESC>& Elements);
	void InitPositionDeclaration(std::vector<D3D11_INPUT_ELEMENT_DESC>& Elements);

	std::vector<D3D11_INPUT_ELEMENT_DESC>& GetDeclaration() { return Declaration; }
	const std::vector<D3D11_INPUT_ELEMENT_DESC>& GetDeclaration() const { return Declaration; }
	const std::vector<D3D11_INPUT_ELEMENT_DESC>& GetPositionDeclaration() const { return PositionDeclaration; }
protected:
	D3D11_INPUT_ELEMENT_DESC AccessStreamComponent(const FVertexStreamComponent& Component, uint8 AttributeIndex);
	D3D11_INPUT_ELEMENT_DESC AccessPositionStreamComponent(const FVertexStreamComponent& Component, uint8 AttributeIndex);

	struct FVertexStream
	{
		const ID3D11Buffer* VertexBuffer = nullptr;
		uint32 Stride = 0;
		uint32 Offset = 0;
		uint8 VertexStreamUsage = 0;

		friend bool operator==(const FVertexStream& A, const FVertexStream& B)
		{
			return A.VertexBuffer == B.VertexBuffer && A.Stride == B.Stride && A.Offset == B.Offset && A.VertexStreamUsage == B.VertexStreamUsage;
		}

		FVertexStream()
		{
		}
	};
	/** The vertex streams used to render the factory. */
	std::vector<FVertexStream> Streams;
private:
	static uint32 NextHashIndex;

	std::vector<FVertexStream> PositionStream;
	std::vector<D3D11_INPUT_ELEMENT_DESC> Declaration;
	std::vector<D3D11_INPUT_ELEMENT_DESC> PositionDeclaration;
};

struct alignas(16) FLocalVertexFactoryUniformShaderParameters
{
	IntVector VertexFetch_Parameters;
};
struct FLocalVertexFactoryUniform
{
	ComPtr<ID3D11Buffer> Resource;
	ComPtr<ID3D11ShaderResourceView> VertexFetch_TexCoordBuffer;
	ComPtr<ID3D11ShaderResourceView> VertexFetch_PackedTangentsBuffer;
};

extern std::shared_ptr<FLocalVertexFactoryUniform> CreateLocalVFUniformBuffer(const class FLocalVertexFactory* VertexFactory);


class FLocalVertexFactory : public FVertexFactory
{
public:
	FLocalVertexFactory(const FStaticMeshDataType* InStaticMeshDataType = nullptr)
	{
		StaticMeshDataType = InStaticMeshDataType ? InStaticMeshDataType : &Data;
	}

	struct FDataType : public FStaticMeshDataType
	{
	};

	void SetData(const FDataType& InData);

	void InitResource();
	void ReleaseResource();

	void InitRHI();
	void ReleaseRHI();

	inline const ComPtr<ID3D11ShaderResourceView> GetPositionsSRV() const
	{
		return StaticMeshDataType->PositionComponentSRV;
	}

	inline const ComPtr<ID3D11ShaderResourceView> GetTangentsSRV() const
	{
		return StaticMeshDataType->TangentsSRV;
	}

	inline const ComPtr<ID3D11ShaderResourceView> GetTextureCoordinatesSRV() const
	{
		return StaticMeshDataType->TextureCoordinatesSRV;
	}

	inline const ComPtr<ID3D11ShaderResourceView> GetColorComponentsSRV() const
	{
		return StaticMeshDataType->ColorComponentsSRV;
	}

	inline const uint32 GetColorIndexMask() const
	{
		return StaticMeshDataType->ColorIndexMask;
	}

	inline const int GetLightMapCoordinateIndex() const
	{
		return StaticMeshDataType->LightMapCoordinateIndex;
	}

	inline const int GetNumTexcoords() const
	{
		return StaticMeshDataType->NumTexCoords;
	}

protected:
	const FDataType& GetData() const { return Data; }
	FDataType Data;
	const FStaticMeshDataType* StaticMeshDataType;

	std::shared_ptr<FLocalVertexFactoryUniform> UniformBuffer;
};

class GPUSkinVertexFactory : public FVertexFactory
{
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

	GPUSkinVertexFactory(uint32 InNumVertices):NumVertices(InNumVertices)
	{}

	void InitRHI();
	void ReleaseRHI();

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

private:
	FVertexStreamComponent TangentStreamComponents[2];
	uint32 NumVertices;
	FDataType Data;
};