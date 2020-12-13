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

	bool SetStaticMesh(class UStaticMesh* NewMesh);
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
enum EBoneVisibilityStatus
{
	/** Bone is hidden because it's parent is hidden. */
	BVS_HiddenByParent,
	/** Bone is visible. */
	BVS_Visible,
	/** Bone is hidden directly. */
	BVS_ExplicitlyHidden,
	BVS_MAX,
};
class USkinnedMeshComponent : public UMeshComponent
{
public:
	USkinnedMeshComponent(AActor* InOwner) : UMeshComponent(InOwner) 
	{
		bDoubleBufferedComponentSpaceTransforms = true;
		CurrentEditableComponentTransforms = 0;
		CurrentReadComponentTransforms = 1;
		bNeedToFlipSpaceBaseBuffers = false;

		CurrentBoneTransformRevisionNumber = 0;
	}

	class USkeletalMesh* SkeletalMesh;

	class FSkeletalMeshObject*	MeshObject = NULL;
	
	int32 PredictedLODLevel;

	std::weak_ptr<class USkinnedMeshComponent> MasterPoseComponent;

	std::vector<uint8> BoneVisibilityStates;

	uint32 GetBoneTransformRevisionNumber() const
	{
		return (!MasterPoseComponent.expired() ? MasterPoseComponent.lock()->CurrentBoneTransformRevisionNumber : CurrentBoneTransformRevisionNumber);
	}

	FSkeletalMeshRenderData* GetSkeletalMeshRenderData() const;

	virtual void SetSkeletalMesh(class USkeletalMesh* NewMesh, bool bReinitPose = true);

	void SetMasterPoseComponent(USkinnedMeshComponent* NewMasterBoneComponent, bool bForceUpdate = false);
protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void CreateRenderState_Concurrent() override;
	virtual void SendRenderDynamicData_Concurrent() override;
	virtual void DestroyRenderState_Concurrent() override;

	virtual bool UpdateLODStatus();

	std::vector<int32> MasterBoneMap;

	int32 CurrentReadComponentTransforms;
	int32 CurrentEditableComponentTransforms;
	uint32 CurrentBoneTransformRevisionNumber;

	std::vector<uint8> PreviousBoneVisibilityStates;
	std::vector<FTransform> PreviousComponentSpaceTransformsArray;

	bool bHasValidBoneTransform;

	uint8 bDoubleBufferedComponentSpaceTransforms : 1;
	uint8 bNeedToFlipSpaceBaseBuffers : 1;

private:
	std::vector<FTransform> ComponentSpaceTransformsArray[2];
public:
	const std::vector<int32>& GetMasterBoneMap() const { return MasterBoneMap; }

	void UpdateMasterBoneMap();

	const std::vector<uint8>& GetPreviousBoneVisibilityStates() const { return PreviousBoneVisibilityStates; }
	const std::vector<FTransform>& GetPreviousComponentTransformsArray() const { return  PreviousComponentSpaceTransformsArray; }

	const std::vector<FTransform>& GetComponentSpaceTransforms() const
	{
		return ComponentSpaceTransformsArray[CurrentReadComponentTransforms];
	}
	std::vector<FTransform>& GetEditableComponentSpaceTransforms()
	{
		return ComponentSpaceTransformsArray[CurrentEditableComponentTransforms];
	}
	const std::vector<FTransform>& GetEditableComponentSpaceTransforms() const
	{
		return ComponentSpaceTransformsArray[CurrentEditableComponentTransforms];
	}
	uint32 GetNumComponentSpaceTransforms() const
	{
		return GetComponentSpaceTransforms().size();
	}

	std::vector<struct FSkelMeshComponentLODInfo> LODInfo;

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

	virtual class UMaterial* GetMaterial(int32 ElementIndex) const override;

	void InitLODInfos();


protected:
	void FlipEditableSpaceBases();

	virtual bool AllocateTransformData();
	virtual void DeallocateTransformData();
};

class USkeletalMeshComponent : public USkinnedMeshComponent
{
public:
	USkeletalMeshComponent(AActor* InOwner) : USkinnedMeshComponent(InOwner) {}

	virtual void SetSkeletalMesh(class USkeletalMesh* NewMesh, bool bReinitPose = true) override;

	virtual void InitAnim(bool bForceReinit);

	void RecalcRequiredBones(int32 LODIndex);

	void ComputeRequiredBones(std::vector<FBoneIndexType>& OutRequiredBones, std::vector<FBoneIndexType>& OutFillComponentSpaceTransformsRequiredBones, int32 LODIndex, bool bIgnorePhysicsAsset) const;
	
	std::vector<FBoneIndexType> RequiredBones;

	std::vector<FTransform> BoneSpaceTransforms;

	std::vector<FBoneIndexType> FillComponentSpaceTransformsRequiredBones;
protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
private:
	void FillComponentSpaceTransforms(const USkeletalMesh* InSkeletalMesh, const std::vector<FTransform>& InBoneSpaceTransforms, std::vector<FTransform>& OutComponentSpaceTransforms) const;
};