#pragma once

#include <vector>
#include <d3d11.h>
#include "Actor.h"
#include "UnrealMath.h"
#include "MeshDescription.h"
#include "FBXImporter.h"
#include "PrimitiveComponent.h"
#include "StaticMeshResources.h"
#include <unordered_map>

struct StaticMeshBuildVertex
{
	FVector Position;

	FVector TangentX;
	FVector TangentY;
	FVector TangentZ;

	FColor C;
	Vector2 UVs;
	Vector2 LightMapCoordinate;
};



struct MeshMaterial
{
	ID3D11Texture2D* BaseColor;
	ID3D11Texture2D* NormalMap;
	ID3D11Texture2D* MetalTexture;
	ID3D11Texture2D* RoughnessTexture;
	ID3D11Texture2D* SpecularTexture;
};

struct FMeshBatch;

class UStaticMesh
{
public:
	UStaticMesh(class AActor* InOwner);
	virtual ~UStaticMesh() {}

	MeshDescription& GetMeshDescription() { return MD; }

	virtual void InitResources();
	virtual void ReleaseResources();

	void PostLoad();
	void GetRenderMeshDescription(const MeshDescription& InOriginalMeshDescription, MeshDescription& OutRenderMeshDescription);

	UMaterial* GetMaterial(int32 MaterialIndex) const;

	void CacheDerivedData();

	const std::multimap<int32, int32>& GetOverlappingCorners() const { return OverlappingCorners; }
private:
	MeshDescription MD;

	std::unique_ptr<class FStaticMeshRenderData> RenderData;

	std::multimap<int32, int32> OverlappingCorners;

	class UMaterial* Material;
	
	friend class FStaticMeshSceneProxy;
};

static void RegisterMeshAttributes(MeshDescription& MD);