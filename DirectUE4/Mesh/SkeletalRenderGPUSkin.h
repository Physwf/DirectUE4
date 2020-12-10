#pragma once

#include "SkeletalRenderPublic.h"
#include "GPUSkinVertexFactory.h"
#include "SkeletalMeshRenderData.h"
#include "MeshComponent.h"

class FSceneView;
struct FStaticMeshVertexBuffers;
class FSkinWeightVertexBuffer;

class FSkeletalMeshObjectGPUSkin : public FSkeletalMeshObject
{
public:
	FSkeletalMeshObjectGPUSkin(USkinnedMeshComponent* InMeshComponent, FSkeletalMeshRenderData* InSkelMeshRenderData);
	virtual ~FSkeletalMeshObjectGPUSkin();

	//~ Begin FSkeletalMeshObject Interface
	virtual void InitResources(USkinnedMeshComponent* InMeshComponent) override;
	virtual void ReleaseResources() override;

	virtual const FVertexFactory* GetSkinVertexFactory(const FSceneView* View, int32 LODIndex, int32 ChunkIdx) const override;
	virtual void CacheVertices(int32 LODIndex, bool bForce) const override {}
	virtual bool IsCPUSkinned() const override { return false; }
	virtual int32 GetLOD() const override;

	struct FVertexFactoryBuffers
	{
		FStaticMeshVertexBuffers* StaticVertexBuffers = nullptr;
		FSkinWeightVertexBuffer* SkinWeightVertexBuffer = nullptr;
		//FColorVertexBuffer*	ColorVertexBuffer = nullptr;
		//FMorphVertexBuffer* MorphVertexBuffer = nullptr;
		//FSkeletalMeshVertexClothBuffer*	APEXClothVertexBuffer = nullptr;
		uint32 NumVertices = 0;
	};
private:
	class FVertexFactoryData
	{
	public:
		FVertexFactoryData() {}
// 		FVertexFactoryData(const FVertexFactoryData& rhs) 
// 		{
// 			VertexFactories.reserve(rhs.VertexFactories.size());
// 			for (size_t i = 0; i < rhs.VertexFactories.size(); ++i)
// 			{
// 				VertexFactories.push_back(std::move(rhs.VertexFactories[i]));
// 			}
// 		}

		/** one vertex factory for each chunk */
		std::vector<std::unique_ptr<FGPUBaseSkinVertexFactory>> VertexFactories;


		/** one passthrough vertex factory for each chunk */
		//TArray<TUniquePtr<FGPUSkinPassthroughVertexFactory>> PassthroughVertexFactories;

		/** Vertex factory defining both the base mesh as well as the morph delta vertex decals */
		//TArray<TUniquePtr<FGPUBaseSkinVertexFactory>> MorphVertexFactories;

		/** Vertex factory defining both the base mesh as well as the APEX cloth vertex data */
		//TArray<TUniquePtr<FGPUBaseSkinAPEXClothVertexFactory>> ClothVertexFactories;

		/**
		* Init default vertex factory resources for this LOD
		*
		* @param VertexBuffers - available vertex buffers to reference in vertex factory streams
		* @param Sections - relevant section information (either original or from swapped influence)
		*/
		void InitVertexFactories(const FVertexFactoryBuffers& VertexBuffers, const std::vector<FSkeletalMeshRenderSection>& Sections);
		/**
		* Release default vertex factory resources for this LOD
		*/
		void ReleaseVertexFactories();
		/**
		* Init morph vertex factory resources for this LOD
		*
		* @param VertexBuffers - available vertex buffers to reference in vertex factory streams
		* @param Sections - relevant section information (either original or from swapped influence)
		*/
		//void InitMorphVertexFactories(const FVertexFactoryBuffers& VertexBuffers, const std::vector<FSkeletalMeshRenderSection>& Sections, bool bInUsePerBoneMotionBlur);
		/**
		* Release morph vertex factory resources for this LOD
		*/
		//void ReleaseMorphVertexFactories();
		/**
		* Init APEX cloth vertex factory resources for this LOD
		*
		* @param VertexBuffers - available vertex buffers to reference in vertex factory streams
		* @param Sections - relevant section information (either original or from swapped influence)
		*/
		//void InitAPEXClothVertexFactories(const FVertexFactoryBuffers& VertexBuffers, const std::vector<FSkeletalMeshRenderSection>& Sections);
		/**
		* Release morph vertex factory resources for this LOD
		*/
		//void ReleaseAPEXClothVertexFactories();

		/**
		* Clear factory arrays
		*/
		void ClearFactories()
		{
			VertexFactories.clear();
			//MorphVertexFactories.Empty();
			//ClothVertexFactories.Empty();
		}

	private:
		//FVertexFactoryData(const FVertexFactoryData&);
		//FVertexFactoryData& operator=(const FVertexFactoryData&);
	};

	/** vertex data for rendering a single LOD */
	struct FSkeletalMeshObjectLOD
	{
		FSkeletalMeshObjectLOD(FSkeletalMeshRenderData* InSkelMeshRenderData, int32 InLOD)
			: SkelMeshRenderData(InSkelMeshRenderData)
			, LODIndex(InLOD)
			//, MorphVertexBuffer(InSkelMeshRenderData, LODIndex)
			, MeshObjectWeightBuffer(nullptr)
			//, MeshObjectColorBuffer(nullptr)
		{
		}

		/**
		* Init rendering resources for this LOD
		* @param MeshLODInfo - information about the state of the bone influence swapping
		* @param CompLODInfo - information about this LOD from the skeletal component
		*/
		void InitResources(const FSkelMeshObjectLODInfo& MeshLODInfo, FSkelMeshComponentLODInfo* CompLODInfo);

		/**
		* Release rendering resources for this LOD
		*/
		void ReleaseResources();

		/**
		* Init rendering resources for the morph stream of this LOD
		* @param MeshLODInfo - information about the state of the bone influence swapping
		* @param Chunks - relevant chunk information (either original or from swapped influence)
		*/
		//void InitMorphResources(const FSkelMeshObjectLODInfo& MeshLODInfo, bool bInUsePerBoneMotionBlur);

		/**
		* Release rendering resources for the morph stream of this LOD
		*/
		//void ReleaseMorphResources();


		FSkeletalMeshRenderData* SkelMeshRenderData;
		// index into FSkeletalMeshRenderData::LODRenderData[]
		int32 LODIndex;

		/** Vertex buffer that stores the morph target vertex deltas. Updated on the CPU */
		//FMorphVertexBuffer MorphVertexBuffer;

		/** Default GPU skinning vertex factories and matrices */
		FVertexFactoryData GPUSkinVertexFactories;

		/** Skin weight buffer to use, could be from asset or component override */
		FSkinWeightVertexBuffer* MeshObjectWeightBuffer;

		/** Color buffer to user, could be from asset or component override */
		//FColorVertexBuffer* MeshObjectColorBuffer;

		/**
		* Update the contents of the morphtarget vertex buffer by accumulating all
		* delta positions and delta normals from the set of active morph targets
		* @param ActiveMorphTargets - Morph to accumulate. assumed to be weighted and have valid targets
		* @param MorphTargetWeights - All Morph weights
		*/
		//void UpdateMorphVertexBufferCPU(const TArray<FActiveMorphTarget>& ActiveMorphTargets, const TArray<float>& MorphTargetWeights);
		//void UpdateMorphVertexBufferGPU(FRHICommandListImmediate& RHICmdList, const TArray<float>& MorphTargetWeights, const FMorphTargetVertexInfoBuffers& MorphTargetVertexInfoBuffers, const TArray<int32>& SectionIdsUseByActiveMorphTargets);

		/**
		* Determine the current vertex buffers valid for this LOD
		*
		* @param OutVertexBuffers output vertex buffers
		*/
		void GetVertexBuffers(FVertexFactoryBuffers& OutVertexBuffers, FSkeletalMeshLODRenderData& LODData);

		// Temporary arrays used on UpdateMorphVertexBuffer(); these grow to the max and are not thread safe.
		//static TArray<float> MorphAccumulatedWeightArray;
	};


	std::vector<struct FSkeletalMeshObjectLOD> LODs;
};