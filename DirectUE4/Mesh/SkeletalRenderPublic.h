#pragma once

#include "UnrealMath.h"

#include <vector>

class USkinnedMeshComponent;
class FSkeletalMeshRenderData;
class FVertexFactory;
class FSceneView;
struct FSkeletalMeshLODInfo;

class FSkeletalMeshObject
{
public:
	FSkeletalMeshObject(USkinnedMeshComponent* InMeshComponent, FSkeletalMeshRenderData* InSkelMeshRenderData);
	virtual ~FSkeletalMeshObject();

	virtual void InitResources(USkinnedMeshComponent* InMeshComponent) = 0;

	/**
	* Release rendering resources for each LOD
	*/
	virtual void ReleaseResources() = 0;

	virtual int32 GetLOD() const = 0;

	virtual const FVertexFactory* GetSkinVertexFactory(const FSceneView* View, int32 LODIndex, int32 ChunkIdx) const = 0;

	virtual void CacheVertices(int32 LODIndex, bool bForce) const = 0;

	virtual bool IsCPUSkinned() const = 0;

	FSkeletalMeshRenderData& GetSkeletalMeshRenderData() const { return *SkeletalMeshRenderData; }

	void InitLODInfos(const USkinnedMeshComponent* InMeshComponent);

	struct FSkelMeshObjectLODInfo
	{
		/** Hidden Material Section Flags for rendering - That is Material Index, not Section Index  */
		std::vector<bool>	HiddenMaterials;
	};

	std::vector<FSkelMeshObjectLODInfo> LODInfo;


	int32 MinDesiredLODLevel;
protected:
	FSkeletalMeshRenderData * SkeletalMeshRenderData;

	std::vector<FSkeletalMeshLODInfo> SkeletalMeshLODInfo;
};

