#pragma once

#include <vector>
#include <d3d11.h>
#include "Actor.h"
#include "UnrealMath.h"
#include "MeshDescription.h"
#include "FBXImporter.h"
#include "Mesh.h"
#include "StaticMeshResources.h"
#include <unordered_map>

struct StaticMeshBuildVertex
{
	FVector Position;

	FVector TangentX;
	float TangentYSign;
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

class StaticMesh : public MeshPrimitive
{
public:
	StaticMesh();
	virtual ~StaticMesh() {}

	MeshDescription& GetMeshDescription() { return MD; }

	virtual void InitResources();
	virtual void ReleaseResources();
	int GetNumberBatches() { return 1; }
	bool GetMeshElement(int BatchIndex, int SectionIndex, FMeshBatch& OutMeshBatch);
	virtual void DrawStaticElements() override;
	virtual void GetDynamicMeshElements(const std::vector<const FSceneView*>& Views, const SceneViewFamily& ViewFamily, uint32 VisibilityMap/*, FMeshElementCollector& Collector*/) const {};
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const { return FPrimitiveViewRelevance(); }
	void PostLoad();
	void GetRenderMeshDescription(const MeshDescription& InOriginalMeshDescription, MeshDescription& OutRenderMeshDescription);

	void CacheDerivedData();
private:
	MeshDescription MD;

	std::unique_ptr<class FStaticMeshRenderData> RenderData;

	std::multimap<int32, int32> OverlappingCorners;

	class UMaterial* Material;
	
};

static void RegisterMeshAttributes(MeshDescription& MD);