#pragma once

#include "UnrealMath.h"
#include "BoneIndices.h"
#include "BoneContainer.h"
#include "AnimEncoding.h"

struct FBoneTransform
{
	/** @todo anim: should be Skeleton bone index in the future, but right now it's CompactBoneIndex **/
	FCompactPoseBoneIndex BoneIndex;

	/** Transform to apply **/
	FTransform Transform;

	FBoneTransform()
		: BoneIndex(INDEX_NONE)
	{}

	FBoneTransform(FCompactPoseBoneIndex InBoneIndex, const FTransform& InTransform)
		: BoneIndex(InBoneIndex)
		, Transform(InTransform)
	{}
};

// Comparison Operator for Sorting.
struct FCompareBoneTransformIndex
{
	inline bool operator()(const FBoneTransform& A, const FBoneTransform& B) const
	{
		return A.BoneIndex < B.BoneIndex;
	}
};
template<class BoneIndexType>
struct FBasePose
{
	inline void InitBones(int32 NumBones) { Bones.clear(); Bones.resize(NumBones); }

	inline uint32 GetNumBones() const { return Bones.size(); }

	inline bool IsValidIndex(const BoneIndexType& BoneIndex) const
	{
		return Bones.IsValidIndex(BoneIndex.GetInt());
	}

	inline FTransform& operator[](const BoneIndexType& BoneIndex)
	{
		return Bones[BoneIndex.GetInt()];
	}

	inline const FTransform& operator[] (const BoneIndexType& BoneIndex) const
	{
		return Bones[BoneIndex.GetInt()];
	}

	//Bone Index Iteration
	template<typename PoseType, typename IterType>
	struct FRangedForSupport
	{
		const PoseType& Pose;

		FRangedForSupport(const PoseType& InPose) : Pose(InPose) {};

		IterType begin() { return Pose.MakeBeginIter(); }
		IterType end() { return Pose.MakeEndIter(); }
	};

	template<typename PoseType, typename IterType>
	struct FRangedForReverseSupport
	{
		const PoseType& Pose;

		FRangedForReverseSupport(const PoseType& InPose) : Pose(InPose) {};

		IterType begin() { return Pose.MakeBeginIterReverse(); }
		IterType end() { return Pose.MakeEndIterReverse(); }
	};

	const std::vector<FTransform>& GetBones() const { return Bones; }
protected:
	std::vector<FTransform> Bones;
};

struct FCompactPoseBoneIndexIterator
{
	int32 Index;

	FCompactPoseBoneIndexIterator(int32 InIndex) : Index(InIndex) {}

	FCompactPoseBoneIndexIterator& operator++() { ++Index; return (*this); }
	bool operator==(FCompactPoseBoneIndexIterator& Rhs) { return Index == Rhs.Index; }
	bool operator!=(FCompactPoseBoneIndexIterator& Rhs) { return Index != Rhs.Index; }
	FCompactPoseBoneIndex operator*() const { return FCompactPoseBoneIndex(Index); }
};

struct FCompactPoseBoneIndexReverseIterator
{
	int32 Index;

	FCompactPoseBoneIndexReverseIterator(int32 InIndex) : Index(InIndex) {}

	FCompactPoseBoneIndexReverseIterator& operator++() { --Index; return (*this); }
	bool operator==(FCompactPoseBoneIndexReverseIterator& Rhs) { return Index == Rhs.Index; }
	bool operator!=(FCompactPoseBoneIndexReverseIterator& Rhs) { return Index != Rhs.Index; }
	FCompactPoseBoneIndex operator*() const { return FCompactPoseBoneIndex(Index); }
};

struct FBaseCompactPose : FBasePose<FCompactPoseBoneIndex>
{
public:

	FBaseCompactPose()
		: BoneContainer(nullptr)
	{}

	typedef FCompactPoseBoneIndex BoneIndexType;
	//--------------------------------------------------------------------------
	//Bone Index Iteration
	typedef typename FBasePose<FCompactPoseBoneIndex>::template FRangedForSupport<FBaseCompactPose, FCompactPoseBoneIndexIterator> RangedForBoneIndexFwd;
	typedef typename FBasePose<FCompactPoseBoneIndex>::template FRangedForReverseSupport<FBaseCompactPose, FCompactPoseBoneIndexReverseIterator> RangedForBoneIndexBwd;

	inline RangedForBoneIndexFwd ForEachBoneIndex() const
	{
		return RangedForBoneIndexFwd(*this);
	}

	inline RangedForBoneIndexBwd ForEachBoneIndexReverse() const
	{
		return RangedForBoneIndexBwd(*this);
	}

	inline FCompactPoseBoneIndexIterator MakeBeginIter() const { return FCompactPoseBoneIndexIterator(0); }

	inline FCompactPoseBoneIndexIterator MakeEndIter() const { return FCompactPoseBoneIndexIterator(this->GetNumBones()); }

	inline FCompactPoseBoneIndexReverseIterator MakeBeginIterReverse() const { return FCompactPoseBoneIndexReverseIterator(this->GetNumBones() - 1); }

	inline FCompactPoseBoneIndexReverseIterator MakeEndIterReverse() const { return FCompactPoseBoneIndexReverseIterator(-1); }
	//--------------------------------------------------------------------------

	const FBoneContainer& GetBoneContainer() const
	{
		assert(BoneContainer && BoneContainer->IsValid());
		return *BoneContainer;
	}

	void SetBoneContainer(const FBoneContainer* InBoneContainer)
	{
		assert(InBoneContainer && InBoneContainer->IsValid());
		BoneContainer = InBoneContainer;
		this->InitBones(BoneContainer->GetBoneIndicesArray().size());
	}

	void CopyAndAssignBoneContainer(FBoneContainer& NewBoneContainer)
	{
		NewBoneContainer = *BoneContainer;
		BoneContainer = &NewBoneContainer;
	}

	void InitFrom(const FBaseCompactPose& SrcPose)
	{
		SetBoneContainer(SrcPose.BoneContainer);
		this->Bones = SrcPose.Bones;
	}

	// Copy bone transform from SrcPose to this
// 	void CopyBonesFrom(const FBaseCompactPose& SrcPose)
// 	{
// 		this->Bones = SrcPose.GetBones();
// 		BoneContainer = &SrcPose.GetBoneContainer();
// 	}

	void CopyBonesFrom(const FBaseCompactPose& SrcPose)
	{
		if (this != &SrcPose)
		{
			this->Bones = SrcPose.GetBones();
			BoneContainer = &SrcPose.GetBoneContainer();
		}
	}

	void CopyBonesFrom(const std::vector<FTransform>& SrcPoseBones)
	{
		// only allow if the size is same
		// if size doesn't match, we can't guarantee the bonecontainer would work
		// so we can't accept
		if (this->Bones.size() == SrcPoseBones.size())
		{
			this->Bones = SrcPoseBones;
		}
	}

	void CopyBonesTo(std::vector<FTransform>& DestPoseBones)
	{
		// this won't work if you're copying to FBaseCompactPose without BoneContainer data
		// you'll like to make CopyBonesTo(FBaseCompactPose<OtherAllocator>& DestPose) to fix this properly
		// if you need bone container
		DestPoseBones = this->Bones;
	}

	void Empty()
	{
		BoneContainer = nullptr;
		this->Bones.clear();
	}

	// Sets this pose to its ref pose
	void ResetToRefPose()
	{
		ResetToRefPose(GetBoneContainer());
	}

	// Sets this pose to the supplied BoneContainers ref pose
	void ResetToRefPose(const FBoneContainer& RequiredBones)
	{
		
	}

	// Sets every bone transform to Identity
	void ResetToAdditiveIdentity()
	{
		for (FTransform& Bone : this->Bones)
		{
			Bone.SetIdentity();
			Bone.SetScale3D(FVector::ZeroVector);
		}
	}

	// returns true if all bone rotations are normalized
	bool IsNormalized() const
	{
		for (const FTransform& Bone : this->Bones)
		{
			if (!Bone.IsRotationNormalized())
			{
				return false;
			}
		}

		return true;
	}

	// Returns true if any bone rotation contains NaN or Inf
	bool ContainsNaN() const
	{
		for (const FTransform& Bone : this->Bones)
		{
			if (Bone.ContainsNaN())
			{
				return true;
			}
		}

		return false;
	}

	// Normalizes all rotations in this pose
	void NormalizeRotations()
	{
		for (FTransform& Bone : this->Bones)
		{
			Bone.NormalizeRotation();
		}
	}

	bool IsValid() const
	{
		return (BoneContainer && BoneContainer->IsValid());
	}

	// Returns the bone index for the parent bone
	BoneIndexType GetParentBoneIndex(const BoneIndexType& BoneIndex) const
	{
		return GetBoneContainer().GetParentBoneIndex(BoneIndex);
	}

	// Returns the ref pose for the supplied bone
	const FTransform& GetRefPose(const BoneIndexType& BoneIndex) const
	{
		return GetBoneContainer().GetRefPoseTransform(BoneIndex);
	}
	void PopulateFromAnimation(
		const UAnimSequence& Seq,
		const BoneTrackArray& RotationTracks,
		const BoneTrackArray& TranslationTracks,
		const BoneTrackArray& ScaleTracks,
		float Time)
	{
		// @todo fixme 
		FTransformArray LocalBones;
		LocalBones = this->Bones;

		AnimationFormat_GetAnimationPose(
			LocalBones, //@TODO:@ANIMATION: Nasty hack
			RotationTracks,
			TranslationTracks,
			ScaleTracks,
			Seq,
			Time);
		this->Bones = LocalBones;
	}
protected:
	//Reference to our BoneContainer
	const FBoneContainer* BoneContainer;
};

struct FCompactPose : public FBaseCompactPose
{
};

struct FCompactHeapPose : public FBaseCompactPose
{

};