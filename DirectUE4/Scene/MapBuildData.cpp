#include "MapBuildDataRegistry.h"
#include <fstream>

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

static void LoadDataFromFile(const char* filename,std::vector<uint8>& OutData)
{
	std::ifstream includeFile(filename, std::ios::in | std::ios::binary | std::ios::ate);
	if (includeFile.is_open()) {
		OutData.clear();
		unsigned int fileSize = (unsigned int)includeFile.tellg();
		OutData.resize(fileSize);
		includeFile.seekg(0, std::ios::beg);
		includeFile.read((char*)OutData.data(), fileSize);
		includeFile.close();
	}
	else
	{
		assert(false);
	}
}

FPrecomputedVolumetricLightmapData& UMapBuildDataRegistry::AllocateLevelPrecomputedVolumetricLightmapBuildData(/*const FGuid& LevelId*/)
{
	LevelPrecomputedVolumetricLightmapBuildData[0] = FPrecomputedVolumetricLightmapData(); 
	FPrecomputedVolumetricLightmapData* Data = &LevelPrecomputedVolumetricLightmapBuildData[0];
	Data->IndirectionTextureDimensions = FIntVector(16);
	Data->IndirectionTexture.Format = PF_R8G8B8A8_UINT;
	LoadDataFromFile("./dx11demo/IndirectionTexture", Data->IndirectionTexture.Data);
	Data->BrickSize = 4;
	Data->BrickDataDimensions = FIntVector(210,5,5);
	Data->BrickData.AmbientVector.Format = PF_FloatR11G11B10;
	LoadDataFromFile("./dx11demo/BrickData.AmbientVector", Data->BrickData.AmbientVector.Data);
	for (int i = 0; i < 6; ++i)
	{
		std::string SHCoefficients("./dx11demo/BrickData.SHCoefficients");
		Data->BrickData.SHCoefficients[i].Format = PF_B8G8R8A8;
		SHCoefficients += std::to_string(i);
		LoadDataFromFile(SHCoefficients.c_str(), Data->BrickData.SHCoefficients[i].Data);
	}
	Data->BrickData.SkyBentNormal.Format = PF_B8G8R8A8;
	LoadDataFromFile("./dx11demo/BrickData.SkyBentNormal", Data->BrickData.SkyBentNormal.Data);
	Data->BrickData.DirectionalLightShadowing.Format = PF_G8;
	LoadDataFromFile("./dx11demo/BrickData.DirectionalLightShadowing", Data->BrickData.DirectionalLightShadowing.Data);

	return *Data;
}

void UMapBuildDataRegistry::AddLevelPrecomputedVolumetricLightmapBuildData(/*const FGuid& LevelId, */FPrecomputedVolumetricLightmapData* InData)
{

}

const FPrecomputedVolumetricLightmapData* UMapBuildDataRegistry::GetLevelPrecomputedVolumetricLightmapBuildData(/*FGuid LevelId*/) const
{
	return &LevelPrecomputedVolumetricLightmapBuildData.at(0);
}

FPrecomputedVolumetricLightmapData* UMapBuildDataRegistry::GetLevelPrecomputedVolumetricLightmapBuildData(/*FGuid LevelId*/)
{
	return &LevelPrecomputedVolumetricLightmapBuildData[0];
}


