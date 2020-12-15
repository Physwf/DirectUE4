#pragma once

#include "AnimTypes.h"
#include "Skeleton.h"
#include "BoneContainer.h"

#include <vector>
#include <string>

struct FAnimCurveParam
{
	std::string Name;
	// name UID for fast access
	SmartName::UID_Type UID;
	FAnimCurveParam()
		: UID(SmartName::MaxUID)
	{}
	// initialize
	void Initialize(USkeleton* Skeleton);
	// this doesn't check CurveType flag
	// because it's possible you don't care about your curve types
	bool IsValid() const
	{
		return UID != SmartName::MaxUID;
	}

	bool IsValidToEvaluate() const
	{
		return IsValid();
	}
};

struct FAnimCurveBase
{
	//FSmartName	Name;
private:
	int32		CurveTypeFlags;
public:
	FAnimCurveBase() : CurveTypeFlags(0) {}
};

struct FFloatCurve : public FAnimCurveBase
{

};

struct FCurveElement
{
	/** Curve Value */
	float					Value;
	/** Whether this value is set or not */
	bool					bValid;

	FCurveElement(float InValue)
		: Value(InValue)
		, bValid(true)
	{}

	FCurveElement()
		: Value(0.f)
		, bValid(false)
	{}

	bool IsValid() const
	{
		return bValid;
	}

	void SetValue(float InValue)
	{
		Value = InValue;
		bValid = true;
	}
};

struct FBaseBlendedCurve
{
	std::vector<FCurveElement> Elements;
	std::vector<uint16> const * UIDToArrayIndexLUT;
	uint16 NumValidCurveCount;

	FBaseBlendedCurve()
		: UIDToArrayIndexLUT(nullptr)
		, NumValidCurveCount(0)
		, bInitialized(false)
	{
	}
	void InitFrom(const FBoneContainer& RequiredBones)
	{
		InitFrom(&RequiredBones.GetUIDToArrayLookupTable());
	}
	void InitFrom(std::vector<uint16> const * InUIDToArrayIndexLUT)
	{
		assert(InUIDToArrayIndexLUT != nullptr);
		UIDToArrayIndexLUT = InUIDToArrayIndexLUT;
		NumValidCurveCount = GetValidElementCount(UIDToArrayIndexLUT);
		Elements.clear();
		Elements.resize(NumValidCurveCount);
		// no name, means no curve
		bInitialized = true;
	}
	void InitFrom(const FBaseBlendedCurve& InCurveToInitFrom)
	{
		// make sure this doesn't happen
		if (/*ensure*/(&InCurveToInitFrom != this))
		{
			assert(InCurveToInitFrom.UIDToArrayIndexLUT != nullptr);
			UIDToArrayIndexLUT = InCurveToInitFrom.UIDToArrayIndexLUT;
			NumValidCurveCount = GetValidElementCount(UIDToArrayIndexLUT);
			Elements.clear();
			Elements.resize(NumValidCurveCount);
			bInitialized = true;
		}
	}
	void CopyFrom(const FBaseBlendedCurve& CurveToCopyFrom)
	{

	}
	static int32 GetValidElementCount(std::vector<uint16> const* InUIDToArrayIndexLUT)
	{
		int32 Count = 0;
		if (InUIDToArrayIndexLUT)
		{
			for (uint32 Index = 0; Index < InUIDToArrayIndexLUT->size(); ++Index)
			{
				if ((*InUIDToArrayIndexLUT)[Index] != MAX_uint16)
				{
					++Count;
				}
			}
		}

		return Count;
	}
	bool bInitialized;
};

struct FBlendedCurve : public FBaseBlendedCurve
{
};

struct FBlendedHeapCurve : public FBaseBlendedCurve
{
};