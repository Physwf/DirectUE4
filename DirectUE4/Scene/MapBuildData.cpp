#include "MapBuildDataRegistry.h"


FMeshMapBuildData& UMapBuildDataRegistry::AllocateMeshBuildData(const uint32& MeshId, bool bMarkDirty)
{
	MeshBuildData.insert(std::make_pair(MeshId, FMeshMapBuildData()));
	return MeshBuildData[MeshId];
}

const FMeshMapBuildData* UMapBuildDataRegistry::GetMeshBuildData(uint32 MeshId) const
{
	auto It = MeshBuildData.find(MeshId);
	if (It != MeshBuildData.end())
	{
		return &It->second;
	}
	return NULL;
}

FMeshMapBuildData* UMapBuildDataRegistry::GetMeshBuildData(uint32 MeshId)
{
	auto It = MeshBuildData.find(MeshId);
	if (It != MeshBuildData.end())
	{
		return &It->second;
	}
	return NULL;
}
