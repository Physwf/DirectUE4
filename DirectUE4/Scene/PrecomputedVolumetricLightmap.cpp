#include "PrecomputedVolumetricLightmap.h"
#include "Scene.h"
#include "MapBuildDataRegistry.h"

void FVolumetricLightmapDataLayer::CreateTexture(FIntVector Dimensions)
{
	Texture = RHICreateTexture3D(
		Dimensions.X,
		Dimensions.Y,
		Dimensions.Z,
		Format,
		1,
		TexCreate_ShaderResource | TexCreate_DisableAutoDefrag,
		FClearValueBinding::Transparent,
		GetResourceBulkData(),GetResourceBulkDataSize());
}

int32 FVolumetricLightmapBrickData::GetMinimumVoxelSize() const
{
	assert(AmbientVector.Format != PF_Unknown);
	int32 VoxelSize = GPixelFormats[AmbientVector.Format].BlockBytes;

	for (int32 i = 0; i < 6/* ARRAY_COUNT(SHCoefficients)*/; i++)
	{
		VoxelSize += GPixelFormats[SHCoefficients[i].Format].BlockBytes;
	}

	// excluding SkyBentNormal because it is conditional

	VoxelSize += GPixelFormats[DirectionalLightShadowing.Format].BlockBytes;

	return VoxelSize;
}

FPrecomputedVolumetricLightmapData::FPrecomputedVolumetricLightmapData()
{

}

FPrecomputedVolumetricLightmapData::~FPrecomputedVolumetricLightmapData()
{

}

void FPrecomputedVolumetricLightmapData::InitializeOnImport(const FBox& NewBounds, int32 InBrickSize)
{
	Bounds = NewBounds;
	BrickSize = InBrickSize;
}

void FPrecomputedVolumetricLightmapData::FinalizeImport()
{

}

void FPrecomputedVolumetricLightmapData::InitRHI()
{
	IndirectionTexture.CreateTexture(IndirectionTextureDimensions);
	BrickData.AmbientVector.CreateTexture(BrickDataDimensions);

	for (int32 i = 0; i < 6/*ARRAY_COUNT(BrickData.SHCoefficients)*/; i++)
	{
		BrickData.SHCoefficients[i].CreateTexture(BrickDataDimensions);
	}

	if (BrickData.SkyBentNormal.Data.size() > 0)
	{
		BrickData.SkyBentNormal.CreateTexture(BrickDataDimensions);
	}

	BrickData.DirectionalLightShadowing.CreateTexture(BrickDataDimensions);
}

void FPrecomputedVolumetricLightmapData::ReleaseRHI()
{

}

size_t FPrecomputedVolumetricLightmapData::GetAllocatedBytes() const
{
	return IndirectionTexture.DataSize + BrickData.GetAllocatedBytes();
}


FPrecomputedVolumetricLightmap::FPrecomputedVolumetricLightmap() :
Data(NULL),
bAddedToScene(false),
WorldOriginOffset(0)
{
}

FPrecomputedVolumetricLightmap::~FPrecomputedVolumetricLightmap() 
{

}

void FPrecomputedVolumetricLightmap::AddToScene(class FScene* Scene, class UMapBuildDataRegistry* Registry/*, FGuid LevelBuildDataId*/)
{
	FPrecomputedVolumetricLightmapData* NewData = NULL;

	if (Registry)
	{
		NewData = Registry->GetLevelPrecomputedVolumetricLightmapBuildData(/*LevelBuildDataId*/);
	}

	if (NewData && Scene)
	{
		bAddedToScene = true;

		FPrecomputedVolumetricLightmap* Volume = this;

// 		ENQUEUE_RENDER_COMMAND(SetVolumeDataCommand)
// 			([Volume, NewData, Scene](FRHICommandListImmediate& RHICmdList)
// 				{
					Volume->SetData(NewData, Scene);
				//});
		Scene->AddPrecomputedVolumetricLightmap(this);
	}
}

void FPrecomputedVolumetricLightmap::RemoveFromScene(FScene* Scene)
{
	if (bAddedToScene)
	{
		bAddedToScene = false;

		if (Scene)
		{
			Scene->RemovePrecomputedVolumetricLightmap(this);
		}
	}

	WorldOriginOffset = FVector::ZeroVector;
}

void FPrecomputedVolumetricLightmap::SetData(FPrecomputedVolumetricLightmapData* NewData, FScene* Scene)
{
	Data = NewData;

	if (Data /*&& RHISupportsVolumeTextures(Scene->GetFeatureLevel())*/)
	{
		Data->InitRHI();
	}
}

void FPrecomputedVolumetricLightmap::ApplyWorldOffset(const FVector& InOffset)
{
	WorldOriginOffset += InOffset;
}

