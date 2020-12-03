#pragma once

#include "ShaderCore.h"
#include "SecureHash.h"
#include "ShaderPermutation.h"

#include <vector>
#include <map>
#include <unordered_map>
#include <list>
#include <type_traits>
#include <assert.h>

class FGlobalShaderType;
class FMaterialShaderType;
class FNiagaraShaderType;
class FMeshMaterialShaderType;
class FShaderPipelineType;
class FShaderType;
class FVertexFactoryParameterRef;
class FVertexFactoryType;

extern FSHAHash GGlobalShaderMapHash;

template<typename MetaShaderType>
struct TShaderTypePermutation
{
	MetaShaderType* const Type;
	const int32 PermutationId;

	TShaderTypePermutation(MetaShaderType* InType, int32 InPermutationId)
		: Type(InType)
		, PermutationId(InPermutationId)
	{
	}

	inline bool operator==(const TShaderTypePermutation& Other)const
	{
		return Type == Other.Type && PermutationId == Other.PermutationId;
	}

	inline bool operator!=(const TShaderTypePermutation& Other)const
	{
		return !(*this == Other);
	}
};

using FShaderPermutation = TShaderTypePermutation<FShaderType>;

namespace std
{
	template<typename MetaShaderType> struct hash<TShaderTypePermutation<MetaShaderType>>
	{
		std::size_t operator()(TShaderTypePermutation<MetaShaderType> const& Ref) const noexcept
		{
			std::size_t h1 = std::hash<MetaShaderType*>{}(Ref.Type);
			std::size_t h2 = std::hash<int32>{}(Ref.PermutationId);
			return h1 ^ (h2 << 1); // or use boost::hash_combine
		}
	};
}
class FShaderResourceId
{
public:

	FShaderResourceId() {}

	FShaderResourceId(const uint32 InFrequency, const FSHAHash& InOutputHash, const char* InSpecificShaderTypeName, int32 InSpecificPermutationId) :
		Frequency(InFrequency),
		OutputHash(InOutputHash),
		SpecificShaderTypeName(InSpecificShaderTypeName),
		SpecificPermutationId(InSpecificPermutationId)
	{
		assert(!(SpecificShaderTypeName == nullptr && InSpecificPermutationId != 0));
	}

// 	friend inline uint32 GetTypeHash(const FShaderResourceId& Id)
// 	{
// 		return FCrc::MemCrc_DEPRECATED((const void*)&Id.OutputHash, sizeof(Id.OutputHash));
// 	}

	friend bool operator==(const FShaderResourceId& X, const FShaderResourceId& Y)
	{
		return X.Frequency == Y.Frequency
			&& X.OutputHash == Y.OutputHash
			&& X.SpecificPermutationId == Y.SpecificPermutationId
			&& ((X.SpecificShaderTypeName == NULL && Y.SpecificShaderTypeName == NULL)
				|| (strcmp(X.SpecificShaderTypeName, Y.SpecificShaderTypeName) == 0));
	}

	friend bool operator!=(const FShaderResourceId& X, const FShaderResourceId& Y)
	{
		return !(X == Y);
	}

	/** Target platform and frequency. */
	uint32 Frequency;

	/** Hash of the compiled shader output, which is used to create the FShaderResource. */
	FSHAHash OutputHash;

	/** NULL if type doesn't matter, otherwise the name of the type that this was created specifically for, which is used with geometry shader stream out. */
	const char* SpecificShaderTypeName;

	/** Stores the memory for SpecificShaderTypeName if this is a standalone Id, otherwise is empty and SpecificShaderTypeName points to an FShaderType name. */
	std::string SpecificShaderTypeStorage;

	/** Specific permutation identifier of the shader when SpecificShaderTypeName is non null, ignored otherwise. */
	int32 SpecificPermutationId;
};
namespace std
{
	template<> struct hash<FShaderResourceId>
	{
		std::size_t operator()(FShaderResourceId const& ID) const noexcept
		{
			return crc32c((uint8*)&ID.OutputHash,sizeof(ID.OutputHash));
		}
	};
}
class FShaderResource 
{
	friend class FShader;
public:

	/** Constructor used for deserialization. */
	FShaderResource();

	/** Constructor used when creating a new shader resource from compiled output. */
	FShaderResource(const FShaderCompilerOutput& Output, FShaderType* InSpecificType, int32 InSpecificPermutationId);

	~FShaderResource();

	void Register();

	/** @return the shader's vertex shader */
	inline ID3D11VertexShader* const GetVertexShader()
	{
		assert(Frequency == SF_Vertex);
		return VertexShader.Get();
	}
	/** @return the shader's pixel shader */
	inline ID3D11PixelShader* const GetPixelShader()
	{
		assert(Frequency == SF_Pixel);
		return PixelShader.Get();
	}
	/** @return the shader's hull shader */
	inline ID3D11HullShader* const GetHullShader()
	{
		assert(Frequency == SF_Hull);
		return HullShader.Get();
	}
	/** @return the shader's domain shader */
	inline ID3D11DomainShader* const GetDomainShader()
	{
		assert(Frequency == SF_Domain);
		return DomainShader.Get();
	}
	/** @return the shader's geometry shader */
	inline ID3D11GeometryShader* const GetGeometryShader()
	{
		assert(Frequency == SF_Geometry);
		return GeometryShader.Get();
	}
	/** @return the shader's compute shader */
	inline ID3D11ComputeShader* const GetComputeShader()
	{
		assert(Frequency == SF_Compute);
		return ComputeShader.Get();
	}

	FShaderResourceId GetId() const;

	uint32 GetSizeBytes() const
	{
		return Code->GetBufferSize() + sizeof(FShaderResource);
	}

	// FRenderResource interface.
	void InitRHI();
	void ReleaseRHI();

	/** Finds a matching shader resource in memory if possible. */
	static FShaderResource* FindShaderResourceById(const FShaderResourceId& Id);
	/**
	* Finds a matching shader resource in memory or creates a new one with the given compiler output.
	* SpecificType can be NULL
	*/
	static FShaderResource* FindOrCreateShaderResource(const FShaderCompilerOutput& Output, class FShaderType* SpecificType, int32 SpecificPermutationId);

	/** Return a list of all shader Ids currently known */
	static void GetAllShaderResourceId(std::vector<FShaderResourceId>& Ids);

private:
	/** Reference to the RHI shader.  Only one of these is ever valid, and it is the one corresponding to Target.Frequency. */
	ComPtr<ID3D11VertexShader> VertexShader;
	ComPtr<ID3D11PixelShader> PixelShader;
	ComPtr<ID3D11HullShader> HullShader;
	ComPtr<ID3D11DomainShader> DomainShader;
	ComPtr<ID3D11GeometryShader> GeometryShader;
	ComPtr<ID3D11ComputeShader> ComputeShader;

	/** Target platform and frequency. */
	uint32 Frequency;

	/** Compiled bytecode. */
	ComPtr<ID3DBlob> Code;

	/** Original bytecode size, before compression */
	uint32 UncompressedCodeSize = 0;
	/**
	* Hash of the compiled bytecode and the generated parameter map.
	* This is used to find existing shader resources in memory or the DDC.
	*/
	FSHAHash OutputHash;

	/** If not NULL, the shader type this resource must be used with. */
	class FShaderType* SpecificType;

	/** Specific permutation identifier of the shader when SpecificType is non null, ignored otherwise. */
	int32 SpecificPermutationId;

	/** The number of instructions the shader takes to execute. */
	uint32 NumInstructions;

	/** Number of texture samplers the shader uses. */
	uint32 NumTextureSamplers;

	/** A 'canary' used to detect when a stale shader resource is being rendered with. */
	uint32 Canary;

	/** Whether the shader code is stored in a shader library. */
	bool bCodeInSharedLocation;

	/** Tracks loaded shader resources by id. */
	static std::unordered_map<FShaderResourceId, FShaderResource*> ShaderResourceIdMap;
};
class FShaderId
{
public:

	/**
	* Hash of the material shader map Id, since this shader depends on the generated material code from that shader map.
	* A hash is used instead of the full shader map Id to shorten the key length, even though this will result in a hash being hashed when we make a DDC key.
	*/
	FSHAHash MaterialShaderMapHash;
	/**
	* Vertex factory type that the shader was created for,
	* This is needed in the Id since a single shader type will be compiled for multiple vertex factories within a material shader map.
	* Will be NULL for global shaders.
	*/
	FVertexFactoryType* VertexFactoryType;

	/** Used to detect changes to the vertex factory source files. */
	FSHAHash VFSourceHash;

	/** Shader type */
	FShaderType* ShaderType;

	/** Unique permutation identifier within the ShaderType. */
	int32 PermutationId;

	/** Used to detect changes to the shader source files. */
	FSHAHash SourceHash;

	/** Shader platform and frequency. */
	uint32 Frequency;

	/** Create a minimally initialized Id.  Members will have to be assigned individually. */
	FShaderId()
		: PermutationId(0)
	{}
	/** Creates an Id for the given material, vertex factory, shader type and target. */
	FShaderId(const FSHAHash& InMaterialShaderMapHash, FVertexFactoryType* InVertexFactoryType, FShaderType* InShaderType, int32 PermutationId, uint32 InFrequency);

// 	friend inline uint32 GetTypeHash(const FShaderId& Id)
// 	{
// 		return FCrc::MemCrc_DEPRECATED((const void*)&Id.MaterialShaderMapHash, sizeof(Id.MaterialShaderMapHash));
// 	}

	friend bool operator==(const FShaderId& X, const FShaderId& Y)
	{
		return X.MaterialShaderMapHash == Y.MaterialShaderMapHash
			&& X.VertexFactoryType == Y.VertexFactoryType
			&& X.VFSourceHash == Y.VFSourceHash
			&& X.ShaderType == Y.ShaderType
			&& X.PermutationId == Y.PermutationId
			&& X.SourceHash == Y.SourceHash
			&& X.Frequency == Y.Frequency;
	}

	friend bool operator!=(const FShaderId& X, const FShaderId& Y)
	{
		return !(X == Y);
	}
};
namespace std
{
	template<> struct hash<FShaderId>
	{
		std::size_t operator()(FShaderId const& ID) const noexcept
		{
			return crc32c((uint8*)&ID.MaterialShaderMapHash, sizeof(ID.MaterialShaderMapHash));
		}
	};
}
/** A compiled shader and its parameter bindings. */
class FShader
{
	friend class FShaderType;
public:

	struct CompiledShaderInitializerType
	{
		FShaderType* Type;
		uint32 Frequency;
		ComPtr<ID3DBlob> Code;
		const FShaderParameterMap& ParameterMap;
		const FSHAHash& OutputHash;
		FShaderResource* Resource;
		FSHAHash MaterialShaderMapHash;
		FVertexFactoryType* VertexFactoryType;
		int32 PermutationId;

		CompiledShaderInitializerType(
			FShaderType* InType,
			int32 InPermutationId,
			const FShaderCompilerOutput& CompilerOutput,
			FShaderResource* InResource,
			const FSHAHash& InMaterialShaderMapHash,
			FVertexFactoryType* InVertexFactoryType
		) :
			Type(InType),
			Frequency(CompilerOutput.Frequency),
			Code(CompilerOutput.ShaderCode),
			ParameterMap(CompilerOutput.ParameterMap),
			OutputHash(CompilerOutput.OutputHash),
			Resource(InResource),
			MaterialShaderMapHash(InMaterialShaderMapHash),
			VertexFactoryType(InVertexFactoryType),
			PermutationId(InPermutationId)
		{}
	};

	/**
	* Used to construct a shader for deserialization.
	* This still needs to initialize members to safe values since FShaderType::GenerateSerializationHistory uses this constructor.
	*/
	FShader();

	/**
	* Construct a shader from shader compiler output.
	*/
	FShader(const CompiledShaderInitializerType& Initializer);

	virtual ~FShader();

	/*
	/ ** Can be overridden by FShader subclasses to modify their compile environment just before compilation occurs. * /
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment){}
	*/

	/** Registers this shader for lookup by ID. */
	void Register();
	/** Removes this shader from the ID lookup map. */
	void Deregister();

	virtual const FVertexFactoryParameterRef* GetVertexFactoryParameterRef() const { return NULL; }
	inline ID3D11VertexShader* const GetVertexShader() const
	{
		return Resource->GetVertexShader();
	}
	inline ID3D11PixelShader* const GetPixelShader() const
	{
		return Resource->GetPixelShader();
	}
	inline ID3D11HullShader* const GetHullShader() const
	{
		return Resource->GetHullShader();
	}
	inline ID3D11DomainShader* const GetDomainShader() const
	{
		return Resource->GetDomainShader();
	}
	inline ID3D11GeometryShader* const GetGeometryShader() const
	{
		return Resource->GetGeometryShader();
	}
	inline ID3D11ComputeShader* const GetComputeShader() const
	{
		return Resource->GetComputeShader();
	}
	// Accessors.
	inline FShaderType* GetType() const { return Type; }
	inline int32 GetPermutationId() const { return PermutationId; }
	inline uint32 GetNumInstructions() const { return Resource->NumInstructions; }
	inline void SetNumInstructions(uint32 Num) { Resource->NumInstructions = Num; }

	inline uint32 GetNumTextureSamplers() const { return Resource->NumTextureSamplers; }
	inline const ComPtr<ID3DBlob>& GetCode() const { return Resource->Code; }
	FShaderId GetId() const;
	inline FVertexFactoryType* GetVertexFactoryType() const { return VFType; }
	inline FShaderResourceId GetResourceId() const
	{
		return Resource->GetId();
	}
	void SetResource(FShaderResource* InResource);

	/** Called from the main thread to register and set the serialized resource */
	void RegisterSerializedResource();

	static void GetStreamOutElements(FStreamOutElementList& ElementList, std::vector<uint32>& StreamStrides, int32& RasterizedStream) {}

// 	void BeginInitializeResources()
// 	{
// 		BeginInitResource(Resource);
// 	}
	template<typename UniformBufferStructType>
	inline const TShaderUniformBufferParameter<UniformBufferStructType>& GetUniformBufferParameter() const
	{
		const std::string SearchName = UniformBufferStructType::GetConstantBufferName();

		if (UniformBufferParameters.find(SearchName) != UniformBufferParameters.end())
		{
			std::shared_ptr<FShaderUniformBufferParameter> Parameter = UniformBufferParameters.at(SearchName);
			return (const TShaderUniformBufferParameter<UniformBufferStructType>&)(*Parameter.get());
		}
		else
		{
			// This can happen if the uniform buffer was not bound
			// There's no good way to distinguish not being bound due to temporary debugging / compiler optimizations or an actual code bug,
			// Hence failing silently instead of an error message
			static TShaderUniformBufferParameter<UniformBufferStructType> UnboundParameter;
			UnboundParameter.SetInitialized();
			return UnboundParameter;
		}
	}

	/** Checks that the shader is valid by asserting the canary value is set as expected. */
	inline void CheckShaderIsValid() const {}
	/** Checks that the shader is valid and returns itself. */
	inline FShader* GetShaderChecked()
	{
		CheckShaderIsValid();
		return this;
	}
	/** Discards the serialized resource, used when the engine is using NullRHI */
	void DiscardSerializedResource()
	{
		delete SerializedResource;
		SerializedResource = nullptr;
	}
protected:
	/** Indexed the same as UniformBufferParameters.  Packed densely for coherent traversal. */
	//TArray<FUniformBufferStruct*> UniformBufferParameterStructs;
	std::map<std::string,std::shared_ptr<FShaderUniformBufferParameter>> UniformBufferParameters;

private:
	FSHAHash OutputHash;

	/** Pointer to the shader resource that has been serialized from disk, to be registered on the main thread later. */
	FShaderResource* SerializedResource;

	/** Reference to the shader resource, which stores the compiled bytecode and the RHI shader resource. */
	std::shared_ptr<FShaderResource> Resource;

	FSHAHash MaterialShaderMapHash;
	/** Vertex factory type this shader was created for, stored so that an FShaderId can be constructed from this shader. */
	FVertexFactoryType* VFType;
	FSHAHash VFSourceHash;
	/** Shader Type metadata for this shader. */
	FShaderType* Type;
	/** Unique permutation identifier of the shader in the shader type. */
	int32 PermutationId;

	FSHAHash SourceHash;

	uint32 Frequency;
	/** Transient value used to track when this shader's automatically bound uniform buffer parameters were set last. */
	mutable uint32 SetParametersId;

	/** A 'canary' used to detect when a stale shader is being rendered with. */
	uint32 Canary;

public:
	/** Canary is set to this if the FShader is a valid pointer but uninitialized. */
	static const uint32 ShaderMagic_Uninitialized = 0xbd9922df;
	/** Canary is set to this if the FShader is a valid pointer but in the process of being cleaned up. */
	static const uint32 ShaderMagic_CleaningUp = 0xdc67f93b;
	/** Canary is set to this if the FShader is a valid pointer and initialized. */
	static const uint32 ShaderMagic_Initialized = 0x335b43ab;
};

/**
* An object which is used to serialize/deserialize, compile, and cache a particular shader class.
*
* A shader type can manage multiple instance of FShader across mutiple dimensions such as EShaderPlatform, or permutation id.
* The number of permutation of a shader type is simply given by GetPermutationCount().
*/
class FShaderType
{
public:
	enum class EShaderTypeForDynamicCast : uint32
	{
		Global,
		Material,
		MeshMaterial,
		Niagara
	};

	typedef class FShader* (*ConstructSerializedType)();
	typedef void(*GetStreamOutElementsType)(FStreamOutElementList& ElementList, std::vector<uint32>& StreamStrides, int32& RasterizedStream);

	/** @return The global shader factory list. */
	static std::list<FShaderType*>& GetTypeList();

	static FShaderType* GetShaderTypeByName(const char* Name);
	static std::vector<FShaderType*> GetShaderTypesByFilename(const char* Filename);

	/** @return The global shader name to type map */
	static std::map<std::string, FShaderType*>& GetNameToTypeMap();

	/** Gets a list of FShaderTypes whose source file no longer matches what that type was compiled with */
	static void GetOutdatedTypes(std::vector<FShaderType*>& OutdatedShaderTypes, std::vector<const FVertexFactoryType*>& OutdatedFactoryTypes);

	/** Returns true if the source file no longer matches what that type was compiled with */
	bool GetOutdatedCurrentType(std::vector<FShaderType*>& OutdatedShaderTypes, std::vector<const FVertexFactoryType*>& OutdatedFactoryTypes) const;

	/** Initialize FShaderType static members, this must be called before any shader types are created. */
	static void Initialize(const std::map<std::string, std::vector<const char*> >& ShaderFileToUniformBufferVariables);

	/** Uninitializes FShaderType cached data. */
	static void Uninitialize();

	/** Minimal initialization constructor. */
	FShaderType(
		EShaderTypeForDynamicCast InShaderTypeForDynamicCast,
		const char* InName,
		const char* InSourceFilename,
		const char* InFunctionName,
		uint32 InFrequency,
		int32 InTotalPermutationCount,
		ConstructSerializedType InConstructSerializedRef,
		GetStreamOutElementsType InGetStreamOutElementsRef
	);

	virtual ~FShaderType();

	/**
	* Finds a shader of this type by ID.
	* @return NULL if no shader with the specified ID was found.
	*/
	FShader* FindShaderById(const FShaderId& Id);

	/** Constructs a new instance of the shader type for deserialization. */
	FShader* ConstructForDeserialization() const;

	/** Hashes a pointer to a shader type. */
// 	friend uint32 GetTypeHash(FShaderType* Ref)
// 	{
// 		return Ref ? Ref->HashIndex : 0;
// 	}

	template<typename T> friend struct std::hash;
	// Dynamic casts.
	inline FGlobalShaderType* GetGlobalShaderType()
	{
		return (ShaderTypeForDynamicCast == EShaderTypeForDynamicCast::Global) ? reinterpret_cast<FGlobalShaderType*>(this) : nullptr;
	}
	inline const FGlobalShaderType* GetGlobalShaderType() const
	{
		return (ShaderTypeForDynamicCast == EShaderTypeForDynamicCast::Global) ? reinterpret_cast<const FGlobalShaderType*>(this) : nullptr;
	}
	inline FMaterialShaderType* GetMaterialShaderType()
	{
		return (ShaderTypeForDynamicCast == EShaderTypeForDynamicCast::Material) ? reinterpret_cast<FMaterialShaderType*>(this) : nullptr;
	}
	inline const FMaterialShaderType* GetMaterialShaderType() const
	{
		return (ShaderTypeForDynamicCast == EShaderTypeForDynamicCast::Material) ? reinterpret_cast<const FMaterialShaderType*>(this) : nullptr;
	}
	inline FMeshMaterialShaderType* GetMeshMaterialShaderType()
	{
		return (ShaderTypeForDynamicCast == EShaderTypeForDynamicCast::MeshMaterial) ? reinterpret_cast<FMeshMaterialShaderType*>(this) : nullptr;
	}
	inline const FMeshMaterialShaderType* GetMeshMaterialShaderType() const
	{
		return (ShaderTypeForDynamicCast == EShaderTypeForDynamicCast::MeshMaterial) ? reinterpret_cast<const FMeshMaterialShaderType*>(this) : nullptr;
	}
	inline const FNiagaraShaderType* GetNiagaraShaderType() const
	{
		return (ShaderTypeForDynamicCast == EShaderTypeForDynamicCast::Niagara) ? reinterpret_cast<const FNiagaraShaderType*>(this) : nullptr;
	}
	inline FNiagaraShaderType* GetNiagaraShaderType()
	{
		return (ShaderTypeForDynamicCast == EShaderTypeForDynamicCast::Niagara) ? reinterpret_cast<FNiagaraShaderType*>(this) : nullptr;
	}
	inline EShaderFrequency GetFrequency() const
	{
		return (EShaderFrequency)Frequency;
	}
	inline const char* GetName() const
	{
		return Name;
	}
	inline const std::string& GetFName() const
	{
		return TypeName;
	}
	inline const char* GetShaderFilename() const
	{
		return SourceFilename;
	}
	inline const char* GetFunctionName() const
	{
		return FunctionName;
	}
	inline int32 GetNumShaders() const
	{
		return (int32)ShaderIdMap.size();
	}

	inline int32 GetPermutationCount() const
	{
		return TotalPermutationCount;
	}

	void AddToShaderIdMap(FShaderId Id, FShader* Shader)
	{
		ShaderIdMap.insert(std::make_pair(Id, Shader));
	}

	inline void RemoveFromShaderIdMap(FShaderId Id)
	{
		ShaderIdMap.erase(Id);
	}
	void GetStreamOutElements(FStreamOutElementList& ElementList, std::vector<uint32>& StreamStrides, int32& RasterizedStream)
	{
		(*GetStreamOutElementsRef)(ElementList, StreamStrides, RasterizedStream);
	}

	inline const std::map<const char*, FCachedUniformBufferDeclaration>& GetReferencedUniformBufferStructsCache() const
	{
		return ReferencedUniformBufferStructsCache;
	}
	bool LimitShaderResourceToThisType() const
	{
		return GetStreamOutElementsRef != &FShader::GetStreamOutElements;
	}

	void AddReferencedUniformBufferIncludes(FShaderCompilerEnvironment& OutEnvironment, std::string& OutSourceFilePrefix);
private:
	EShaderTypeForDynamicCast ShaderTypeForDynamicCast;
	uint32 HashIndex;
	const char* Name;
	std::string TypeName;
	const char* SourceFilename;
	const char* FunctionName;
	uint32 Frequency;
	int32 TotalPermutationCount;

	ConstructSerializedType ConstructSerializedRef;
	GetStreamOutElementsType GetStreamOutElementsRef;
	/** A map from shader ID to shader.  A shader will be removed from it when deleted, so this doesn't need to use a TRefCountPtr. */
	std::unordered_map<FShaderId, FShader*> ShaderIdMap;

	std::list<FShaderType*>::iterator GlobalListLink;

	/** Tracks whether serialization history for all shader types has been initialized. */
	static bool bInitializedSerializationHistory;

protected:
	/** Tracks what platforms ReferencedUniformBufferStructsCache has had declarations cached for. */
	bool bCachedUniformBufferStructDeclarations;

	std::map<const char*, FCachedUniformBufferDeclaration> ReferencedUniformBufferStructsCache;
};
namespace std
{
	template<> struct hash<FShaderType*>
	{
		std::size_t operator()(FShaderType* const& Ref) const noexcept
		{
			return Ref ? Ref->HashIndex : 0;
		}
	};
}
/** A collection of shaders of different types, but the same meta type. */
template<typename ShaderMetaType>
class TShaderMap
{
	/** List of serialzied shaders to be processed and registered on the game thread */
	std::vector<FShader*> SerializedShaders;
private:
	/** Flag that makes sure this shader map isn't used until all shaders have been registerd */
	bool bHasBeenRegistered;
public:

	using FShaderPrimaryKey = TShaderTypePermutation<FShaderType>;

	/** Used to compare two shader types by name. */
	class FCompareShaderPrimaryKey
	{
	public:
		inline bool operator()(const FShaderPrimaryKey& A, const FShaderPrimaryKey& B) const
		{
			int32 AL = strlen(A.Type->GetName());
			int32 BL = strlen(B.Type->GetName());
			if (AL == BL)
			{
				return strncmp(A.Type->GetName(), B.Type->GetName(), AL) > 0 || A.PermutationId > B.PermutationId;
			}
			return AL > BL;
		}
	};

	/** Default constructor. */
	TShaderMap()
		: bHasBeenRegistered(true)
	{}

	/** Destructor ensures pipelines cleared up. */
	virtual ~TShaderMap()
	{
	}

	/** Finds the shader with the given type.  Asserts on failure. */
	template<typename ShaderType>
	ShaderType* GetShader(int32 PermutationId = 0) const
	{
		assert(bHasBeenRegistered);
		auto it = Shaders.find(FShaderPrimaryKey(&ShaderType::StaticType, PermutationId));
		assert(it != Shaders.end());
		const std::shared_ptr<FShader> ShaderRef = it->second;
		return (ShaderType*)((*ShaderRef).GetShaderChecked());
	}

	/** Finds the shader with the given type.  May return NULL. */
	FShader* GetShader(FShaderType* ShaderType, int32 PermutationId = 0) const
	{
		assert(bHasBeenRegistered);
		auto it = Shaders.find(FShaderPrimaryKey(ShaderType, PermutationId));
		return it != Shaders.end() ? it->second->GetShaderChecked() : nullptr;
	}

	/** Finds the shader with the given type. */
	bool HasShader(FShaderType* Type, int32 PermutationId) const
	{
		assert(bHasBeenRegistered);
		return Shaders.find(FShaderPrimaryKey(Type, PermutationId)) != Shaders.end();
	}

	inline const std::map<FShaderPrimaryKey, std::shared_ptr<FShader>>& GetShaders() const
	{
		assert(bHasBeenRegistered);
		return Shaders;
	}

	void AddShader(FShaderType* Type, int32 PermutationId, FShader* Shader)
	{
		assert(Type);
		Shaders.insert(std::make_pair(FShaderPrimaryKey(Type, PermutationId), Shader));
	}

	/**
	* Removes the shader of the given type from the shader map
	* @param Type Shader type to remove the entry for
	*/
	void RemoveShaderTypePermutaion(FShaderType* Type, int32 PermutationId)
	{
		Shaders.erase(FShaderPrimaryKey(Type, PermutationId));
	}

	/** Builds a list of the shaders in a shader map. */
	void GetShaderList(std::map<FShaderId, FShader*>& OutShaders) const
	{
		assert(bHasBeenRegistered);
		for (typename TMap<FShaderPrimaryKey, std::shared_ptr<FShader>>::TConstIterator ShaderIt(Shaders); ShaderIt; ++ShaderIt)
		{
			if (ShaderIt.Value())
			{
				OutShaders.Add(ShaderIt.Value()->GetId(), ShaderIt.Value());
			}
		}
	}

	/** Builds a list of the shaders in a shader map. Key is FShaderType::TypeName */
	void GetShaderList(std::map<std::string, FShader*>& OutShaders) const
	{
		assert(bHasBeenRegistered);
		for (auto ShaderIt = Shaders.begin(); ShaderIt != Shaders.end(); ++ShaderIt)
		{
			if (ShaderIt->second)
			{
				OutShaders.Add(ShaderIt.second->GetType()->GetFName(), ShaderIt->second);
			}
		}
	}
	/** Registered all shaders that have been serialized (maybe) on another thread */
	virtual void RegisterSerializedShaders()
	{
		bHasBeenRegistered = true;
		for (FShader* Shader : SerializedShaders)
		{
			Shader->RegisterSerializedResource();

			FShaderType* Type = Shader->GetType();
			FShader* ExistingShader = Type->FindShaderById(Shader->GetId());

			if (ExistingShader != nullptr)
			{
				delete Shader;
				Shader = ExistingShader;
			}
			else
			{
				// Register the shader now that it is valid, so that it can be reused
				Shader->Register();
			}
			AddShader(Shader->GetType(), Shader->GetPermutationId(), Shader);
		}
		SerializedShaders.clear();
	}

	/** Discards serialized shaders when they are not going to be used for anything (NullRHI) */
	virtual void DiscardSerializedShaders()
	{
		for (FShader* Shader : SerializedShaders)
		{
			if (Shader)
			{
				Shader->DiscardSerializedResource();
			}
			delete Shader;
		}
		SerializedShaders.clear();
	}

	/** @return true if the map is empty */
	inline bool IsEmpty() const
	{
		assert(bHasBeenRegistered);
		return Shaders.size() == 0;
	}

	/** @return The number of shaders in the map. */
	inline uint32 GetNumShaders() const
	{
		assert(bHasBeenRegistered);
		return Shaders.size();
	}

protected:
	std::unordered_map<FShaderPrimaryKey, std::shared_ptr<FShader> > Shaders;
};

struct FGlobalShaderPermutationParameters
{
	/** Unique permutation identifier of the global shader type. */
	const int32 PermutationId;

	FGlobalShaderPermutationParameters(int32 InPermutationId)
		: PermutationId(InPermutationId)
	{ }
};
/**
* A shader meta type for the simplest shaders; shaders which are not material or vertex factory linked.
* There should only a single instance of each simple shader type.
*/
class FGlobalShaderType : public FShaderType
{
	friend class FGlobalShaderTypeCompiler;
public:

	typedef FShader::CompiledShaderInitializerType CompiledShaderInitializerType;
	typedef FShader* (*ConstructCompiledType)(const CompiledShaderInitializerType&);
	typedef bool(*ShouldCompilePermutationType)(const FGlobalShaderPermutationParameters&);
	typedef bool(*ValidateCompiledResultType)(const FShaderParameterMap&, std::vector<std::string>&);
	typedef void(*ModifyCompilationEnvironmentType)(const FGlobalShaderPermutationParameters&, FShaderCompilerEnvironment&);

	FGlobalShaderType(
		const char* InName,
		const char* InSourceFilename,
		const char* InFunctionName,
		uint32 InFrequency,
		int32 InTotalPermutationCount,
		ConstructSerializedType InConstructSerializedRef,
		ConstructCompiledType InConstructCompiledRef,
		ModifyCompilationEnvironmentType InModifyCompilationEnvironmentRef,
		ShouldCompilePermutationType InShouldCompilePermutationRef,
		ValidateCompiledResultType InValidateCompiledResultRef,
		GetStreamOutElementsType InGetStreamOutElementsRef
	) :
		FShaderType(EShaderTypeForDynamicCast::Global, InName, InSourceFilename, InFunctionName, InFrequency, InTotalPermutationCount, InConstructSerializedRef, InGetStreamOutElementsRef),
		ConstructCompiledRef(InConstructCompiledRef),
		ShouldCompilePermutationRef(InShouldCompilePermutationRef),
		ValidateCompiledResultRef(InValidateCompiledResultRef),
		ModifyCompilationEnvironmentRef(InModifyCompilationEnvironmentRef)
	{
// 		checkf(FPaths::GetExtension(InSourceFilename) == TEXT("usf"),
// 			TEXT("Incorrect virtual shader path extension for global shader '%s': Only .usf files should be "
// 				"compiled."),
// 			InSourceFilename);
	}

	/**
	* Checks if the shader type should be cached for a particular platform.
	* @param Platform - The platform to check.
	* @return True if this shader type should be cached.
	*/
	bool ShouldCompilePermutation(int32 PermutationId) const
	{
		return (*ShouldCompilePermutationRef)(FGlobalShaderPermutationParameters(PermutationId));
	}

	/**
	* Sets up the environment used to compile an instance of this shader type.
	* @param Platform - Platform to compile for.
	* @param Environment - The shader compile environment that the function modifies.
	*/
	void SetupCompileEnvironment(int32 PermutationId, FShaderCompilerEnvironment& Environment)
	{
		// Allow the shader type to modify its compile environment.
		(*ModifyCompilationEnvironmentRef)(FGlobalShaderPermutationParameters(PermutationId), Environment);
	}

	/**
	* Checks if the shader type should pass compilation for a particular set of parameters.
	* @param Platform - Platform to validate.
	* @param ParameterMap - Shader parameters to validate.
	* @param OutError - List for appending validation errors.
	*/
	bool ValidateCompiledResult(const FShaderParameterMap& ParameterMap, std::vector<std::string>& OutError) const
	{
		return (*ValidateCompiledResultRef)(ParameterMap, OutError);
	}

private:
	ConstructCompiledType ConstructCompiledRef;
	ShouldCompilePermutationType ShouldCompilePermutationRef;
	ValidateCompiledResultType ValidateCompiledResultRef;
	ModifyCompilationEnvironmentType ModifyCompilationEnvironmentRef;
};

extern TShaderMap<FGlobalShaderType>* GGlobalShaderMap;
/**
* A shader meta type for material-linked shaders.
*/
class FUniformExpressionSet;
class FMaterial;

class FMaterialShaderType : public FShaderType
{
public:
	struct CompiledShaderInitializerType : FGlobalShaderType::CompiledShaderInitializerType
	{
		const FUniformExpressionSet& UniformExpressionSet;
		const std::string DebugDescription;

		CompiledShaderInitializerType(
			FShaderType* InType,
			const FShaderCompilerOutput& CompilerOutput,
			FShaderResource* InResource,
			const FUniformExpressionSet& InUniformExpressionSet,
			const FSHAHash& InMaterialShaderMapHash,
			FVertexFactoryType* InVertexFactoryType,
			const std::string& InDebugDescription
		)
			: FGlobalShaderType::CompiledShaderInitializerType(InType,/** PermutationId = */ 0, CompilerOutput, InResource, InMaterialShaderMapHash,  InVertexFactoryType)
			, UniformExpressionSet(InUniformExpressionSet)
			, DebugDescription(InDebugDescription)
		{}
	};

	typedef FShader* (*ConstructCompiledType)(const CompiledShaderInitializerType&);
	typedef bool(*ShouldCompilePermutationType)(const FMaterial*);
	typedef bool(*ValidateCompiledResultType)(const std::vector<FMaterial*>&, const FShaderParameterMap&, std::vector<std::string>&);
	typedef void(*ModifyCompilationEnvironmentType)(const FMaterial*, FShaderCompilerEnvironment&);

	FMaterialShaderType(
		const char* InName,
		const char* InSourceFilename,
		const char* InFunctionName,
		uint32 InFrequency,
		int32 InTotalPermutationCount,
		ConstructSerializedType InConstructSerializedRef,
		ConstructCompiledType InConstructCompiledRef,
		ModifyCompilationEnvironmentType InModifyCompilationEnvironmentRef,
		ShouldCompilePermutationType InShouldCompilePermutationRef,
		ValidateCompiledResultType InValidateCompiledResultRef,
		GetStreamOutElementsType InGetStreamOutElementsRef
	) :
		FShaderType(EShaderTypeForDynamicCast::Material, InName, InSourceFilename, InFunctionName, InFrequency, InTotalPermutationCount, InConstructSerializedRef, InGetStreamOutElementsRef),
		ConstructCompiledRef(InConstructCompiledRef),
		ShouldCompilePermutationRef(InShouldCompilePermutationRef),
		ValidateCompiledResultRef(InValidateCompiledResultRef),
		ModifyCompilationEnvironmentRef(InModifyCompilationEnvironmentRef)
	{
// 		checkf(FPaths::GetExtension(InSourceFilename) == TEXT("usf"),
// 			TEXT("Incorrect virtual shader path extension for material shader '%s': Only .usf files should be compiled."),
// 			InSourceFilename);
// 		assert(InTotalPermutationCount == 1);
	}

	/**
	* Enqueues a compilation for a new shader of this type.
	* @param Material - The material to link the shader with.
	*/
	class FShaderCompileJob* BeginCompileShader(
		uint32 ShaderMapId,
		const FMaterial* Material,
		FShaderCompilerEnvironment* MaterialEnvironment,
		std::vector<FShaderCompileJob*>& NewJobs
	);
	/**
	* Either creates a new instance of this type or returns an equivalent existing shader.
	* @param Material - The material to link the shader with.
	* @param CurrentJob - Compile job that was enqueued by BeginCompileShader.
	*/
	FShader* FinishCompileShader(
		const FUniformExpressionSet& UniformExpressionSet,
		const FSHAHash& MaterialShaderMapHash,
		const FShaderCompileJob& CurrentJob,
		const std::string& InDebugDescription
	);
	/**
	* Checks if the shader type should be cached for a particular platform and material.
	* @param Platform - The platform to check.
	* @param Material - The material to check.
	* @return True if this shader type should be cached.
	*/
	bool ShouldCache(const FMaterial* Material) const
	{
		return (*ShouldCompilePermutationRef)(Material);
	}
	/**
	* Checks if the shader type should pass compilation for a particular set of parameters.
	* @param Platform - Platform to validate.
	* @param Materials - Materials to validate.
	* @param ParameterMap - Shader parameters to validate.
	* @param OutError - List for appending validation errors.
	*/
	bool ValidateCompiledResult(const std::vector<FMaterial*>& Materials, const FShaderParameterMap& ParameterMap, std::vector<std::string>& OutError) const
	{
		return (*ValidateCompiledResultRef)(Materials, ParameterMap, OutError);
	}
protected:
	/**
	* Sets up the environment used to compile an instance of this shader type.
	* @param Platform - Platform to compile for.
	* @param Environment - The shader compile environment that the function modifies.
	*/
	void SetupCompileEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& Environment)
	{
		// Allow the shader type to modify its compile environment.
		(*ModifyCompilationEnvironmentRef)(Material, Environment);
	}

private:
	ConstructCompiledType ConstructCompiledRef;
	ShouldCompilePermutationType ShouldCompilePermutationRef;
	ValidateCompiledResultType ValidateCompiledResultRef;
	ModifyCompilationEnvironmentType ModifyCompilationEnvironmentRef;
};

/**
* A shader meta type for material-linked shaders which use a vertex factory.
*/
class FMeshMaterialShaderType : public FShaderType
{
public:
	struct CompiledShaderInitializerType : FMaterialShaderType::CompiledShaderInitializerType
	{
		FVertexFactoryType* VertexFactoryType;
		CompiledShaderInitializerType(
			FShaderType* InType,
			const FShaderCompilerOutput& CompilerOutput,
			FShaderResource* InResource,
			const FUniformExpressionSet& InUniformExpressionSet,
			const FSHAHash& InMaterialShaderMapHash,
			const std::string& InDebugDescription,
			FVertexFactoryType* InVertexFactoryType
		) :
			FMaterialShaderType::CompiledShaderInitializerType(InType, CompilerOutput, InResource, InUniformExpressionSet, InMaterialShaderMapHash, InVertexFactoryType, InDebugDescription),
			VertexFactoryType(InVertexFactoryType)
		{}
	};
	typedef FShader* (*ConstructCompiledType)(const CompiledShaderInitializerType&);
	typedef bool(*ShouldCompilePermutationType)(const FMaterial*, const FVertexFactoryType* VertexFactoryType);
	typedef bool(*ValidateCompiledResultType)(const std::vector<FMaterial*>&, const FVertexFactoryType*, const FShaderParameterMap&, std::vector<std::string>&);
	typedef void(*ModifyCompilationEnvironmentType)(const FMaterial*, FShaderCompilerEnvironment&);

	FMeshMaterialShaderType(
		const char* InName,
		const char* InSourceFilename,
		const char* InFunctionName,
		uint32 InFrequency,
		int32 InTotalPermutationCount,
		ConstructSerializedType InConstructSerializedRef,
		ConstructCompiledType InConstructCompiledRef,
		ModifyCompilationEnvironmentType InModifyCompilationEnvironmentRef,
		ShouldCompilePermutationType InShouldCompilePermutationRef,
		ValidateCompiledResultType InValidateCompiledResultRef,
		GetStreamOutElementsType InGetStreamOutElementsRef
	) :
		FShaderType(EShaderTypeForDynamicCast::MeshMaterial, InName, InSourceFilename, InFunctionName, InFrequency, InTotalPermutationCount, InConstructSerializedRef, InGetStreamOutElementsRef),
		ConstructCompiledRef(InConstructCompiledRef),
		ShouldCompilePermutationRef(InShouldCompilePermutationRef),
		ValidateCompiledResultRef(InValidateCompiledResultRef),
		ModifyCompilationEnvironmentRef(InModifyCompilationEnvironmentRef)
	{
// 		checkf(FPaths::GetExtension(InSourceFilename) == TEXT("usf"),
// 			TEXT("Incorrect virtual shader path extension for mesh material shader '%s': Only .usf files should be compiled."),
// 			InSourceFilename);
// 		assert(InTotalPermutationCount == 1);
	}

	/**
	* Enqueues a compilation for a new shader of this type.
	* @param Platform - The platform to compile for.
	* @param Material - The material to link the shader with.
	* @param VertexFactoryType - The vertex factory to compile with.
	*/
	class FShaderCompileJob* BeginCompileShader(
		uint32 ShaderMapId,
		const FMaterial* Material,
		FShaderCompilerEnvironment* MaterialEnvironment,
		FVertexFactoryType* VertexFactoryType,
		const FShaderPipelineType* ShaderPipeline,
		std::vector<FShaderCompileJob*>& NewJobs
	);

	/**
	* Either creates a new instance of this type or returns an equivalent existing shader.
	* @param Material - The material to link the shader with.
	* @param CurrentJob - Compile job that was enqueued by BeginCompileShader.
	*/
	FShader* FinishCompileShader(
		const FUniformExpressionSet& UniformExpressionSet,
		const FSHAHash& MaterialShaderMapHash,
		const FShaderCompileJob& CurrentJob,
		const std::string& InDebugDescription
	);

	/**
	* Checks if the shader type should be cached for a particular platform, material, and vertex factory type.
	* @param Platform - The platform to check.
	* @param Material - The material to check.
	* @param VertexFactoryType - The vertex factory type to check.
	* @return True if this shader type should be cached.
	*/
	bool ShouldCache(const FMaterial* Material, const FVertexFactoryType* VertexFactoryType) const
	{
		return (*ShouldCompilePermutationRef)(Material, VertexFactoryType);
	}

	/**
	* Checks if the shader type should pass compilation for a particular set of parameters.
	* @param Platform - Platform to validate.
	* @param Materials - Materials to validate.
	* @param VertexFactoryType - Vertex factory to validate.
	* @param ParameterMap - Shader parameters to validate.
	* @param OutError - List for appending validation errors.
	*/
	bool ValidateCompiledResult(const std::vector<FMaterial*>& Materials, const FVertexFactoryType* VertexFactoryType, const FShaderParameterMap& ParameterMap, std::vector<std::string>& OutError) const
	{
		return (*ValidateCompiledResultRef)(Materials, VertexFactoryType, ParameterMap, OutError);
	}

protected:

	/**
	* Sets up the environment used to compile an instance of this shader type.
	* @param Platform - Platform to compile for.
	* @param Environment - The shader compile environment that the function modifies.
	*/
	void SetupCompileEnvironment(const FMaterial* Material, FShaderCompilerEnvironment& Environment)
	{
		// Allow the shader type to modify its compile environment.
		(*ModifyCompilationEnvironmentRef)(Material, Environment);
	}

private:
	ConstructCompiledType ConstructCompiledRef;
	ShouldCompilePermutationType ShouldCompilePermutationRef;
	ValidateCompiledResultType ValidateCompiledResultRef;
	ModifyCompilationEnvironmentType ModifyCompilationEnvironmentRef;
};

/**
* A macro to declare a new shader type.  This should be called in the class body of the new shader type.
* @param ShaderClass - The name of the class representing an instance of the shader type.
* @param ShaderMetaTypeShortcut - The shortcut for the shader meta type: simple, material, meshmaterial, etc.  The shader meta type
*	controls
*/
#define DECLARE_EXPORTED_SHADER_TYPE(ShaderClass,ShaderMetaTypeShortcut,RequiredAPI, ...) \
	public: \
	using FPermutationDomain = FShaderPermutationNone; \
	using ShaderMetaType = F##ShaderMetaTypeShortcut##ShaderType; \
	\
	static RequiredAPI ShaderMetaType StaticType; \
	\
	static FShader* ConstructSerializedInstance() { return new ShaderClass(); } \
	static FShader* ConstructCompiledInstance(const ShaderMetaType::CompiledShaderInitializerType& Initializer) \
	{ return new ShaderClass(Initializer); } 

#define DECLARE_SHADER_TYPE(ShaderClass,ShaderMetaTypeShortcut,...) \
	DECLARE_EXPORTED_SHADER_TYPE(ShaderClass,ShaderMetaTypeShortcut,, ##__VA_ARGS__)

/** A macro to implement a shader type. */
#define IMPLEMENT_SHADER_TYPE(TemplatePrefix,ShaderClass,SourceFilename,FunctionName,Frequency) \
	TemplatePrefix \
	ShaderClass::ShaderMetaType ShaderClass::StaticType( \
		#ShaderClass, \
		SourceFilename, \
		FunctionName, \
		Frequency, \
		1, \
		ShaderClass::ConstructSerializedInstance, \
		ShaderClass::ConstructCompiledInstance, \
		ShaderClass::ModifyCompilationEnvironment, \
		ShaderClass::ShouldCompilePermutation, \
		ShaderClass::ValidateCompiledResult, \
		ShaderClass::GetStreamOutElements \
		);

#define IMPLEMENT_SHADER_TYPE2(ShaderClass,Frequency) \
	template<> \
	ShaderClass::ShaderMetaType ShaderClass::StaticType( \
	#ShaderClass, \
	ShaderClass::GetSourceFilename(), \
	ShaderClass::GetFunctionName(), \
	Frequency, \
	1, \
	ShaderClass::ConstructSerializedInstance, \
	ShaderClass::ConstructCompiledInstance, \
	ShaderClass::ModifyCompilationEnvironment, \
	ShaderClass::ShouldCompilePermutation, \
	ShaderClass::ValidateCompiledResult, \
	ShaderClass::GetStreamOutElements \
	);


/** todo: this should replace IMPLEMENT_SHADER_TYPE */
#define IMPLEMENT_SHADER_TYPE3(ShaderClass,Frequency) \
	ShaderClass::ShaderMetaType ShaderClass::StaticType( \
	#ShaderClass, \
	ShaderClass::GetSourceFilename(), \
	ShaderClass::GetFunctionName(), \
	Frequency, \
	1, \
	ShaderClass::ConstructSerializedInstance, \
	ShaderClass::ConstructCompiledInstance, \
	ShaderClass::ModifyCompilationEnvironment, \
	ShaderClass::ShouldCompilePermutation, \
	ShaderClass::ValidateCompiledResult, \
	ShaderClass::GetStreamOutElements \
	);

/** A reference which is initialized with the requested shader type from a shader map. */
template<typename ShaderType>
class TShaderMapRef
{
public:
	TShaderMapRef(const TShaderMap<typename ShaderType::ShaderMetaType>* ShaderIndex)
		: Shader(ShaderIndex->template GetShader<ShaderType>(/* PermutationId = */ 0)) // gcc3 needs the template quantifier so it knows the < is not a less-than
	{
		static_assert(
			std::is_same<typename ShaderType::FPermutationDomain, FShaderPermutationNone>::value,
			"Missing permutation vector argument for shader that have a permutation domain.");
	}

	TShaderMapRef(
		const TShaderMap<typename ShaderType::ShaderMetaType>* ShaderIndex,
		const typename ShaderType::FPermutationDomain& PermutationVector)
		: Shader(ShaderIndex->template GetShader<ShaderType>(PermutationVector.ToDimensionValueId())) // gcc3 needs the template quantifier so it knows the < is not a less-than
	{ }

	inline ShaderType* operator->() const
	{
		return Shader;
	}

	inline ShaderType* operator*() const
	{
		return Shader;
	}

private:
	ShaderType * Shader;
};