#pragma once

#include "VertexFactory.h"

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

class StaticMesh;

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
	ID3D11Buffer* IndexBuffer = NULL;

	//std::vector<LocalVertex> Vertices;
	//std::vector<PositionOnlyLocalVertex> PositionOnlyVertices;

	std::vector<StaticMeshSection> Sections;

	void InitResources();
	void ReleaseResources();

};

struct FStaticMeshVertexFactories
{
	FLocalVertexFactory VertexFactory;

	void InitResources(const FStaticMeshLODResources& LodResources, const StaticMesh* Parent);
	void ReleaseResources();
};

class FStaticMeshRenderData
{
public:
	std::vector<FStaticMeshLODResources*> LODResources;
	std::vector<FStaticMeshVertexFactories*> LODVertexFactories;

	void AllocateLODResources(int32 NumLODs);

	void Cache(StaticMesh* Owner/*, const FStaticMeshLODSettings& LODSettings*/);

	void InitResources(const StaticMesh* Owner);
	void ReleaseResources();
};


