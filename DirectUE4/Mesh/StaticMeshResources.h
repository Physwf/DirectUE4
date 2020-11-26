#pragma once

#include "VertexFactory.h"
#include "PrimitiveSceneProxy.h"

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

struct StaticMeshSection
{
	int MaterialIndex;

	uint32 FirstIndex;
	uint32 NumTriangles;
	uint32 MinVertexIndex;
	uint32 MaxVertexIndex;
};

struct FStaticMeshLODResources
{
	FStaticMeshVertexBuffers VertexBuffers;
	
	std::vector<uint32> Indices;
	ComPtr<ID3D11Buffer> IndexBuffer = NULL;

	//std::vector<LocalVertex> Vertices;
	//std::vector<PositionOnlyLocalVertex> PositionOnlyVertices;

	std::vector<StaticMeshSection> Sections;

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
protected:
	FStaticMeshRenderData* RenderData;
private:
};

