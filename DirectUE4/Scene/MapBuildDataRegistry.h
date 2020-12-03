#pragma once

#include "UnrealMath.h"

#include <map>

class FMeshMapBuildData
{
public:
	std::shared_ptr<class FLightMap> LightMap;
	std::shared_ptr<class FShadowMap> ShadowMap;
	//TArray<FGuid> IrrelevantLights;
	//TArray<FPerInstanceLightmapData> PerInstanceLightmapData;

	FMeshMapBuildData() {};
	/** Destructor. */
	~FMeshMapBuildData() {};

};

class UMapBuildDataRegistry
{
public:
	FMeshMapBuildData& AllocateMeshBuildData(const uint32& MeshId, bool bMarkDirty);
	const FMeshMapBuildData* GetMeshBuildData(uint32 MeshId) const;
	FMeshMapBuildData* GetMeshBuildData(uint32 MeshId);
private:
	std::map<uint32, FMeshMapBuildData> MeshBuildData;
};

