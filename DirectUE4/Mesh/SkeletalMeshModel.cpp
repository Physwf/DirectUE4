#include "SkeletalMeshModel.h"

void SkeletalMeshSection::CalcMaxBoneInfluences()
{
	// if we only have rigid verts then there is only one bone
	MaxBoneInfluences = 1;
	// iterate over all the soft vertices for this chunk and find max # of bones used
	for (uint32 VertIdx = 0; VertIdx < SoftVertices.size(); VertIdx++)
	{
		SoftSkinVertex& SoftVert = SoftVertices[VertIdx];

		// calc # of bones used by this soft skinned vertex
		int32 BonesUsed = 0;
		for (int32 InfluenceIdx = 0; InfluenceIdx < MAX_TOTAL_INFLUENCES; InfluenceIdx++)
		{
			if (SoftVert.InfluenceWeights[InfluenceIdx] > 0)
			{
				BonesUsed++;
			}
		}
		// reorder bones so that there aren't any unused influence entries within the [0,BonesUsed] range
		for (int32 InfluenceIdx = 0; InfluenceIdx < BonesUsed; InfluenceIdx++)
		{
			if (SoftVert.InfluenceWeights[InfluenceIdx] == 0)
			{
				for (int32 ExchangeIdx = InfluenceIdx + 1; ExchangeIdx < MAX_TOTAL_INFLUENCES; ExchangeIdx++)
				{
					if (SoftVert.InfluenceWeights[ExchangeIdx] != 0)
					{
						Exchange(SoftVert.InfluenceWeights[InfluenceIdx], SoftVert.InfluenceWeights[ExchangeIdx]);
						Exchange(SoftVert.InfluenceBones[InfluenceIdx], SoftVert.InfluenceBones[ExchangeIdx]);
						break;
					}
				}
			}
		}

		// maintain max bones used
		MaxBoneInfluences = Math::Max(MaxBoneInfluences, BonesUsed);
	}
}

