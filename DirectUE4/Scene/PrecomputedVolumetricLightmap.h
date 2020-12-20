#pragma once

#include "D3D11RHI.h"

class FScene;

class FVolumetricLightmapDataLayer 
{
public:

	FVolumetricLightmapDataLayer() :
		DataSize(0),
		Format(PF_Unknown)
	{}

	const void* GetResourceBulkData() const
	{
		return Data.data();
	}

	uint32 GetResourceBulkDataSize() const
	{
		return Data.size() * sizeof(uint8);
	}

	void Discard()
	{
		//temporarily remove until the semantics around lighting scenarios are fixed.
		//if (!GIsEditor)
		{
			Data.clear();
		}
	}

	void Resize(int32 NewSize)
	{
		Data.clear();
		Data.reserve(NewSize);
		Data.resize(NewSize);
		DataSize = NewSize;
	}

	void CreateTexture(FIntVector Dimensions);

	std::vector<uint8> Data;
	// Stored redundantly for stats after Data has been discarded
	int32 DataSize;
	EPixelFormat Format;
	std::shared_ptr<FD3D11Texture> Texture;
};

class FVolumetricLightmapBrickData
{
public:
	FVolumetricLightmapDataLayer AmbientVector;
	FVolumetricLightmapDataLayer SHCoefficients[6];
	FVolumetricLightmapDataLayer SkyBentNormal;
	FVolumetricLightmapDataLayer DirectionalLightShadowing;
	// Mobile LQ layers:
	FVolumetricLightmapDataLayer LQLightColor;
	FVolumetricLightmapDataLayer LQLightDirection;

	int32 GetMinimumVoxelSize() const;

	size_t GetAllocatedBytes() const
	{
		size_t NumBytes = AmbientVector.DataSize + SkyBentNormal.DataSize + DirectionalLightShadowing.DataSize;
		NumBytes += LQLightColor.Data.size() + LQLightDirection.Data.size();

		for (int32 i = 0; i < 6/*ARRAY_COUNT(SHCoefficients)*/; i++)
		{
			NumBytes += SHCoefficients[i].DataSize;
		}

		return NumBytes;
	}

	// discard the layers used for low quality lightmap (LQ includes direct lighting from stationary lights).
	void DiscardLowQualityLayers()
	{
		LQLightColor.Discard();
		LQLightDirection.Discard();
	}
};

/**
* Data for a Volumetric Lightmap, built during import from Lightmass.
* Its lifetime is managed by UMapBuildDataRegistry.
*/
class FPrecomputedVolumetricLightmapData 
{
public:

	FPrecomputedVolumetricLightmapData();
	virtual ~FPrecomputedVolumetricLightmapData();

	void InitializeOnImport(const FBox& NewBounds, int32 InBrickSize);
	void FinalizeImport();

	void InitRHI();
	void ReleaseRHI();

	size_t GetAllocatedBytes() const;

	const FBox& GetBounds() const
	{
		return Bounds;
	}

	FBox Bounds;

	FIntVector IndirectionTextureDimensions;
	FVolumetricLightmapDataLayer IndirectionTexture;

	int32 BrickSize;
	FIntVector BrickDataDimensions;
	FVolumetricLightmapBrickData BrickData;

private:

	friend class FPrecomputedVolumetricLightmap;
};

/**
* Represents the Volumetric Lightmap for a specific ULevel.
*/
class FPrecomputedVolumetricLightmap
{
public:

	FPrecomputedVolumetricLightmap();
	~FPrecomputedVolumetricLightmap();

	void AddToScene(class FScene* Scene, class UMapBuildDataRegistry* Registry/*, FGuid LevelBuildDataId*/);

	void RemoveFromScene(FScene* Scene);

	void SetData(FPrecomputedVolumetricLightmapData* NewData, FScene* Scene);

	bool IsAddedToScene() const
	{
		return bAddedToScene;
	}

	void ApplyWorldOffset(const FVector& InOffset);

	// Owned by rendering thread
	// ULevel's MapBuildData GC-visible property guarantees that the FPrecomputedVolumetricLightmapData will not be deleted during the lifetime of FPrecomputedVolumetricLightmap.
	FPrecomputedVolumetricLightmapData* Data;

private:

	bool bAddedToScene;

	/** Offset from world origin. Non-zero only when world origin was rebased */
	FVector WorldOriginOffset;
};