#pragma once

#include "DrawingPolicy.h"

#include <vector>


class FViewInfo;

template<typename DrawingPolicyType>
class TStaticMeshDrawList
{
	struct FElement
	{
		//ElementPolicyDataType PolicyData;
		FStaticMesh* Mesh;
		//FBoxSphereBounds Bounds;
		//bool bBackground;
		//TRefCountPtr<FElementHandle> Handle;

		/** Default constructor. */
		FElement() :
			Mesh(NULL)
		{}

		/** Minimal initialization constructor. */
		FElement(
			FStaticMesh* InMesh,
			//const ElementPolicyDataType& InPolicyData,
			TStaticMeshDrawList* StaticMeshDrawList,
			//FSetElementId SetId,
			int32 ElementIndex
		) :
			//PolicyData(InPolicyData),
			Mesh(InMesh)//,
			//Handle(new FElementHandle(StaticMeshDrawList, SetId, ElementIndex))
		{
			// Cache bounds so we can use them for sorting quickly, without having to dereference the proxy
			//Bounds = Mesh->PrimitiveSceneInfo->Proxy->GetBounds();
			//bBackground = Mesh->PrimitiveSceneInfo->Proxy->TreatAsBackgroundForOcclusion();
		}

		/** Destructor. */
		~FElement()
		{
			if (Mesh)
			{
				//Mesh->UnlinkDrawList(Handle);
			}
		}
	};

	/** A set of draw list elements with the same drawing policy. */
	struct FDrawingPolicyLink
	{
		/** The elements array and the compact elements array are always synchronized */
		//TArray<FElementCompact>		 CompactElements;
		std::vector<FElement>			 Elements;
		DrawingPolicyType		 		 DrawingPolicy;
		//FBoundShaderStateInput		 BoundShaderStateInput;
		//ERHIFeatureLevel::Type		 FeatureLevel;

		/** Used when sorting policy links */
		//FSphere						 CachedBoundingSphere;

		/** The id of this link in the draw list's set of drawing policy links. */
		//FSetElementId SetId;

		TStaticMeshDrawList* DrawList;

		uint32 VisibleCount;

		/** Initialization constructor. */
		FDrawingPolicyLink(TStaticMeshDrawList* InDrawList, const DrawingPolicyType& InDrawingPolicy/*, ERHIFeatureLevel::Type InFeatureLevel*/) :
			DrawingPolicy(InDrawingPolicy),
			//FeatureLevel(InFeatureLevel),
			DrawList(InDrawList),
			VisibleCount(0)
		{
			//check(IsInRenderingThread());
			//BoundShaderStateInput = DrawingPolicy.GetBoundShaderStateInput(FeatureLevel);
		}

// 		SIZE_T GetSizeBytes() const
// 		{
// 			return sizeof(*this) + CompactElements.GetAllocatedSize() + Elements.GetAllocatedSize();
// 		}
	};
	int32 DrawElement(ID3D11DeviceContext* Context, const FViewInfo& View, /*const typename DrawingPolicyType::ContextDataType PolicyContext,*/ FDrawingPolicyRenderState& DrawRenderState, const FElement& Element, /*uint64 BatchElementMask, */FDrawingPolicyLink* DrawingPolicyLink/*, bool &bDrawnShared*/);
public:
	void AddMesh(
		FStaticMesh* Mesh,
		//const ElementPolicyDataType& PolicyData,
		const DrawingPolicyType& InDrawingPolicy//,
	);

	bool DrawVisible(
		ID3D11DeviceContext* Context, 
		const FViewInfo& View, 
		/*const typename DrawingPolicyType::ContextDataType PolicyContext, */
		const FDrawingPolicyRenderState& DrawRenderState//,
		/*const TBitArray<SceneRenderingBitArrayAllocator>& StaticMeshVisibilityMap,*/ 
		/*const TArray<uint64, SceneRenderingAllocator>& BatchVisibilityArray*/);

private:
	std::vector<FDrawingPolicyLink> DrawingPolicySet;
};

template<typename DrawingPolicyType>
void TStaticMeshDrawList<DrawingPolicyType>::AddMesh(FStaticMesh* Mesh, /*const ElementPolicyDataType& PolicyData, */ const DrawingPolicyType& InDrawingPolicy/*, */)
{
	DrawingPolicySet.push_back(FDrawingPolicyLink(this, InDrawingPolicy/*, InFeatureLevel*/));
	FDrawingPolicyLink* DrawingPolicyLink = &DrawingPolicySet.back();
	const int32 ElementIndex = (int32)DrawingPolicyLink->Elements.size();
	DrawingPolicyLink->Elements.push_back(FElement(Mesh, /*PolicyData,*/ this,/* DrawingPolicyLink->SetId,*/ ElementIndex));
	//FElement* Element = new(DrawingPolicyLink->Elements) FElement(Mesh, PolicyData, this, DrawingPolicyLink->SetId, ElementIndex);
}

template<typename DrawingPolicyType>
bool TStaticMeshDrawList<DrawingPolicyType>::DrawVisible(
	ID3D11DeviceContext* Context, 
	const FViewInfo& View, 
	/*const typename DrawingPolicyType::ContextDataType PolicyContext, */ 
	const FDrawingPolicyRenderState& DrawRenderState//, 
	/*const TBitArray<SceneRenderingBitArrayAllocator>& StaticMeshVisibilityMap,*/ 
	/*const TArray<uint64, SceneRenderingAllocator>& BatchVisibilityArray*/)
{
	FDrawingPolicyRenderState DrawRenderStateLocal(DrawRenderState);

	for (uint32 Index = 0; Index < DrawingPolicySet.size(); Index++)
	{
		FDrawingPolicyLink* DrawingPolicyLink = &DrawingPolicySet[Index];
		bool bDrawnShared = false;
		const uint32 NumElements = DrawingPolicyLink->Elements.size();
		uint32 Count = 0;
		for (uint32 ElementIndex = 0; ElementIndex < NumElements; ElementIndex++)
		{
			const FElement& Element = DrawingPolicyLink->Elements[ElementIndex];
			// Avoid the cache miss looking up batch visibility if there is only one element.
			//uint64 BatchElementMask = Element.Mesh->bRequiresPerElementVisibility ? (*BatchVisibilityArray)[Element.Mesh->BatchVisibilityId] : ((1ull << SubCount) - 1);
			Count += DrawElement(Context, View, /*PolicyContext, */DrawRenderStateLocal, Element, /*BatchElementMask,*/ DrawingPolicyLink/*, bDrawnShared*/);
		}
	}
	return true;
}


template<typename DrawingPolicyType>
int32 TStaticMeshDrawList<DrawingPolicyType>::DrawElement(ID3D11DeviceContext* Context, const FViewInfo& View, /*const typename DrawingPolicyType::ContextDataType PolicyContext,*/ FDrawingPolicyRenderState& DrawRenderState, const FElement& Element, /*uint64 BatchElementMask, */FDrawingPolicyLink* DrawingPolicyLink/*, bool &bDrawnShared*/)
{
	int32 BatchElementIndex = 0;

	DrawingPolicyLink->DrawingPolicy.SetMeshRenderState(
		Context,
		View,
		//Proxy,
		*Element.Mesh,
		BatchElementIndex,
		DrawRenderState//,
		//Element.PolicyData,
		//PolicyContext
	);

	DrawingPolicyLink->DrawingPolicy.DrawMesh(Context, View, *Element.Mesh, BatchElementIndex, false);

	return 0;
}
