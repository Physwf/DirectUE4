#pragma once

#include <vector>
#include <d3d11.h>
#include "Actor.h"
#include "UnrealMath.h"
#include "MeshDescription.h"
#include "FBXImporter.h"
#include <unordered_map>

struct StaticMeshBuildVertex
{
	Vector Position;

	Vector TangentX;
	float TangentYSign;
	Vector TangentZ;

	FColor C;
	Vector2 UVs;
	Vector2 LightMapCoordinate;
};

struct LocalVertex
{
	Vector4 Position;
	Vector  TangentX;
	Vector4 TangentZ;
	Vector4 Color;
	Vector2 TexCoords;
	Vector2	LightMapCoordinate;
};

struct PositionOnlyLocalVertex
{
	Vector4 Position;
};

struct StaticMeshSection
{
	int MaterialIndex;

	uint32 FirstIndex;
	uint32 NumTriangles;
	uint32 MinVertexIndex;
	uint32 MaxVertexIndex;
};

struct MeshLODResources
{
	ID3D11Buffer* VertexBuffer = NULL;
	ID3D11Buffer* PositionOnlyVertexBuffer = NULL;
	ID3D11Buffer* IndexBuffer = NULL;

	std::vector<LocalVertex> Vertices;
	std::vector<PositionOnlyLocalVertex> PositionOnlyVertices;
	std::vector<uint32> Indices;

	std::vector<StaticMeshSection> Sections;

	void InitResource();
	void ReleaseResource();
};


struct alignas(16) PrimitiveUniform
{
	Matrix LocalToWorld;
	Matrix WorldToLocal;
	Vector4 ObjectWorldPositionAndRadius; // needed by some materials
	Vector ObjectBounds;
	float LocalToWorldDeterminantSign;
	Vector ActorWorldPosition;
	float DecalReceiverMask;
	float PerObjectGBufferData;
	float UseSingleSampleShadowFromStationaryLights;
	float UseVolumetricLightmapShadowFromStationaryLights;
	float UseEditorDepthTest;
	Vector4 ObjectOrientation;
	Vector4 NonUniformScale;
	Vector4 InvNonUniformScale;
	Vector LocalObjectBoundsMin;
	float PrePadding_Primitive_252;
	Vector LocalObjectBoundsMax;
	uint32 LightingChannelMask;
	float LpvBiasMultiplier;
};

struct MeshElement
{
	ID3D11Buffer* PrimitiveUniformBuffer = nullptr;
	int MaterialIndex;

	uint32 FirstIndex;
	uint32 NumTriangles;
};

struct MeshBatch
{
	std::vector<MeshElement> Elements;

	ID3D11Buffer* VertexBuffer = NULL;
	ID3D11Buffer* PositionOnlyVertexBuffer = NULL;
	ID3D11Buffer* IndexBuffer = NULL;
};

struct MeshMaterial
{
	ID3D11Texture2D* BaseColor;
	ID3D11Texture2D* NormalMap;
	ID3D11Texture2D* MetalTexture;
	ID3D11Texture2D* RoughnessTexture;
	ID3D11Texture2D* SpecularTexture;
};

class StaticMesh : public Actor
{
public:
	StaticMesh(){}
	StaticMesh(const char* ResourcePath);
	~StaticMesh() {}

	void ImportFromFBX(const char* pFileName);
	void GeneratePlane(float InWidth, float InHeight,int InNumSectionW, int InNumSectionH);
	void GnerateBox(float InSizeX, float InSizeY, float InSizeZ, int InNumSectionX, int InNumSectionY, int InNumSectionZ);


	void InitResource();
	void ReleaseResource();

	virtual void Register() override;
	virtual void UnRegister() override;
	virtual void Tick(float fDeltaTime) override;

	void UpdateUniformBuffer();
	int GetNumberBatches() { return 1; }
	bool GetMeshElement(int BatchIndex, int SectionIndex, MeshBatch& OutMeshBatch);
	void DrawStaticElement(class Scene* InScene);
private:
	void Build();
	void BuildVertexBuffer(const MeshDescription& MD2, std::vector<std::vector<uint32> >& OutPerSectionIndices, std::vector<StaticMeshBuildVertex>& StaticMeshBuildVertices);
	void GetRenderMeshDescription(const MeshDescription& InOriginalMeshDescription, MeshDescription& OutRenderMeshDescription);
private:
	MeshDescription MD;
	MeshLODResources LODResource;
	ID3D11InputLayout* InputLayout = NULL;
	ID3D11Buffer* PrimitiveUniformBuffer = NULL;
	std::vector<MeshMaterial> Materials;
	std::multimap<int32, int32> OverlappingCorners;
	
};

static void RegisterMeshAttributes(MeshDescription& MD);