#pragma once

#include "PrimitiveComponent.h"

class AActor;
class UMaterial;
class FSkinWeightVertexBuffer;
class FSkeletalMeshRenderData;

class UMeshComponent : public UPrimitiveComponent
{
public:
	UMeshComponent(AActor* InOwner);
	virtual ~UMeshComponent();

};

class UStaticMeshComponent : public UMeshComponent
{
public:
	UStaticMeshComponent(AActor* InOwner) : UMeshComponent(InOwner) {}
	virtual ~UStaticMeshComponent() { }

	bool SetStaticMesh(class UStaticMesh* NewMesh)
	{
		StaticMesh = NewMesh;
		return true;
	}
	UStaticMesh* GetStaticMesh() const
	{
		return StaticMesh;
	}
	virtual UMaterial* GetMaterial(int32 MaterialIndex) const override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy();

	const class FMeshMapBuildData* GetMeshMapBuildData() const;
private:
	class UStaticMesh* StaticMesh;
};

struct FSkelMeshComponentLODInfo
{
	/** Material corresponds to section. To show/hide each section, use this. */
	std::vector<bool> HiddenMaterials;
	/** Vertex buffer used to override vertex colors */
	//FColorVertexBuffer* OverrideVertexColors;
	/** Vertex buffer used to override skin weights */
	FSkinWeightVertexBuffer* OverrideSkinWeights;
	FSkelMeshComponentLODInfo();
	~FSkelMeshComponentLODInfo();

	void ReleaseOverrideVertexColorsAndBlock();
	void BeginReleaseOverrideVertexColors();
private:
	void CleanUpOverrideVertexColors();

public:
	void ReleaseOverrideSkinWeightsAndBlock();
	void BeginReleaseOverrideSkinWeights();
private:
	void CleanUpOverrideSkinWeights();
};

class USkinnedMeshComponent : public UMeshComponent
{
public:
	USkinnedMeshComponent(AActor* InOwner) : UMeshComponent(InOwner) {}

	class USkeletalMesh* SkeletalMesh;

	class FSkeletalMeshObject*	MeshObject;

	FSkeletalMeshRenderData* GetSkeletalMeshRenderData() const;

	virtual void SetSkeletalMesh(class USkeletalMesh* NewMesh, bool bReinitPose = true);
protected:
	virtual void CreateRenderState_Concurrent() override;
	virtual void SendRenderDynamicData_Concurrent() override;
	virtual void DestroyRenderState_Concurrent() override;
public:

	std::vector<struct FSkelMeshComponentLODInfo> LODInfo;

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

	virtual class UMaterial* GetMaterial(int32 ElementIndex) const override;

	void InitLODInfos();
};

class USkeletalMeshComponent : public USkinnedMeshComponent
{
public:
	USkeletalMeshComponent(AActor* InOwner) : USkinnedMeshComponent(InOwner) {}

};