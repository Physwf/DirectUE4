#include "AnimCurveTypes.h"

void FAnimCurveParam::Initialize(USkeleton* Skeleton)
{
	if (Name != "")
	{
		UID = Skeleton->GetUIDByName(USkeleton::AnimCurveMappingName, Name);
	}
	else
	{
		// invalidate current UID
		UID = SmartName::MaxUID;
	}
}

