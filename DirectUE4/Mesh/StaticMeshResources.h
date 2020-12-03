#pragma once

#include "VertexFactory.h"
#include "PrimitiveSceneProxy.h"
#include "SceneManagement.h"

struct FStaticMeshVertexBuffers
{
	/** The buffer containing vertex data. */
	std::vector<Vector4> TangentsVertexBuffer;
	std::vector<Vector2> TexCoordVertexBuffer;
	/** The buffer containing the position vertex data. */
	std::vector<FVector> PositionVertexBuffer;
	/** The buffer containing the vertex color data. */
	std::vector<FColor> ColorVertexBuffer;

	
	ComPtr<ID3D11Buffer> TangentsVertexBufferRHI = NULL;
	ComPtr<ID3D11Buffer> TexCoordVertexBufferRHI = NULL;
	ComPtr<ID3D11ShaderResourceView> TangentsVertexBufferSRV = NULL;
	ComPtr<ID3D11ShaderResourceView> TexCoordVertexBufferSRV = NULL;

	ComPtr<ID3D11Buffer> PositionVertexBufferRHI = NULL;
	ComPtr<ID3D11Buffer> ColorVertexBufferRHI = NULL;

};

class UStaticMesh;

struct FStaticMeshSection
{
	int MaterialIndex;

	uint32 FirstIndex;
	uint32 NumTriangles;
	uint32 MinVertexIndex;
	uint32 MaxVertexIndex;

	bool bCastShadow;
};

struct FStaticMeshLODResources
{
	FStaticMeshVertexBuffers VertexBuffers;
	
	std::vector<uint32> Indices;
	ComPtr<ID3D11Buffer> IndexBuffer = NULL;

	//std::vector<LocalVertex> Vertices;
	//std::vector<PositionOnlyLocalVertex> PositionOnlyVertices;

	std::vector<FStaticMeshSection> Sections;

	void InitResources();
	void ReleaseResources();

};

struct FStaticMeshVertexFactories
{
	FLocalVertexFactory VertexFactory;

	void InitResources(const FStaticMeshLODResources& LodResources, const UStaticMesh* Parent);
	void ReleaseResources();
};

class FStaticMeshRenderData
{
public:
	std::vector<FStaticMeshLODResources*> LODResources;
	std::vector<FStaticMeshVertexFactories*> LODVertexFactories;

	void AllocateLODResources(int32 NumLODs);

	void Cache(UStaticMesh* Owner/*, const FStaticMeshLODSettings& LODSettings*/);

	void InitResources(const UStaticMesh* Owner);
	void ReleaseResources();
};

class UStaticMeshComponent;

class FStaticMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
	FStaticMeshSceneProxy(UStaticMeshComponent* Component, bool bForceLODsShareStaticLighting);

	virtual ~FStaticMeshSceneProxy();

	/** Gets the number of mesh batches required to represent the proxy, aside from section needs. */
	virtual int32 GetNumMeshBatches() const
	{
		return 1;
	}

	/** Sets up a shadow FMeshBatch for a specific LOD. */
	virtual bool GetShadowMeshElement(int32 LODIndex, int32 BatchIndex, uint8 InDepthPriorityGroup, FMeshBatch& OutMeshBatch, bool bDitheredLODTransition) const;

	/** Sets up a FMeshBatch for a specific LOD and element. */
	virtual bool GetMeshElement(
		int32 LODIndex,
		int32 BatchIndex,
		int32 ElementIndex,
		uint8 InDepthPriorityGroup,
		bool bUseSelectedMaterial,
		bool bUseHoveredMaterial,
		bool bAllowPreCulledIndices,
		FMeshBatch& OutMeshBatch) const;

	virtual void DrawStaticElements(FPrimitiveSceneInfo* PrimitiveSceneInfo) override;

	//virtual void GetLightRelevance(const FLightSceneProxy* LightSceneProxy, bool& bDynamic, bool& bRelevant, bool& bLightMapped, bool& bShadowMapped) const override;

	virtual void GetLCIs(std::vector<FLightCacheInterface*>& LCIs) override;
protected:
	FStaticMeshRenderData* RenderData;

	/** Information used by the proxy about a single LOD of the mesh. */
	class FLODInfo : public FLightCacheInterface
	{
	public:

		/** Information about an element of a LOD. */
		struct FSectionInfo
		{
			/** Default constructor. */
			FSectionInfo()
				: Material(NULL)
				, FirstPreCulledIndex(0)
				, NumPreCulledTriangles(-1)
			{}
			/** The material with which to render this section. */
			UMaterial* Material;
			int32 FirstPreCulledIndex;
			int32 NumPreCulledTriangles;
		};

		/** Per-section information. */
		std::vector<FSectionInfo> Sections;

		/** Vertex color data for this LOD (or NULL when not overridden), FStaticMeshComponentLODInfo handle the release of the memory */
		//FColorVertexBuffer* OverrideColorVertexBuffer;

		TUniformBufferPtr<FLocalVertexFactoryUniformShaderParameters> OverrideColorVFUniformBuffer;

		//const FRawStaticIndexBuffer* PreCulledIndexBuffer;

		/** Initialization constructor. */
		FLODInfo(const UStaticMeshComponent* InComponent, const std::vector<FStaticMeshVertexFactories*>& InLODVertexFactories, int32 InLODIndex, bool bLODsShareStaticLighting);

		bool UsesMeshModifyingMaterials() const { return bUsesMeshModifyingMaterials; }

		// FLightCacheInterface.
		virtual FLightInteraction GetInteraction(const FLightSceneProxy* LightSceneProxy) const override;

	private:
		std::vector<uint32> IrrelevantLights;

		/** True if any elements in this LOD use mesh-modifying materials **/
		bool bUsesMeshModifyingMaterials;
	};

	//FStaticMeshOccluderData* OccluderData;

	std::vector<FLODInfo*> LODs;
private:
};

