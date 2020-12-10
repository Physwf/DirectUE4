#pragma once

#include "PrimitiveSceneProxy.h"
#include "SkeletalMeshRenderData.h"

class USkinnedMeshComponent;
class FSceneView;
class FSceneViewFamily;
class FMeshElementCollector;

class FSkeletalMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
	FSkeletalMeshSceneProxy(const USkinnedMeshComponent* Component, FSkeletalMeshRenderData* InSkelMeshRenderData);

	virtual void GetDynamicMeshElements(const std::vector<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
protected:
	class FSkeletalMeshObject* MeshObject;
	FSkeletalMeshRenderData* SkeletalMeshRenderData;

	/** info for section element in an LOD */
	struct FSectionElementInfo
	{
		FSectionElementInfo(UMaterial* InMaterial, bool bInEnableShadowCasting, int32 InUseMaterialIndex)
			: Material(InMaterial)
			, bEnableShadowCasting(bInEnableShadowCasting)
			, UseMaterialIndex(InUseMaterialIndex)

		{}

		UMaterial* Material;

		/** Whether shadow casting is enabled for this section. */
		bool bEnableShadowCasting;

		/** Index into the materials array of the skel mesh or the component after LOD mapping */
		int32 UseMaterialIndex;

	};

	/** Section elements for a particular LOD */
	struct FLODSectionElements
	{
		std::vector<FSectionElementInfo> SectionElements;
	};

	std::vector<FLODSectionElements> LODSections;

	void GetDynamicElementsSection(const std::vector<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap,
		const FSkeletalMeshLODRenderData& LODData, const int32 LODIndex, const int32 SectionIndex, bool bSectionSelected,
		const FSectionElementInfo& SectionElementInfo, bool bInSelectable, FMeshElementCollector& Collector) const;

	void GetMeshElementsConditionallySelectable(const std::vector<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, bool bInSelectable, uint32 VisibilityMap, FMeshElementCollector& Collector) const;

};
