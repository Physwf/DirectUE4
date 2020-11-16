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
		MaxBoneInfluences = FMath::Max(MaxBoneInfluences, BonesUsed);
	}
}

void SkeletalMeshLODModel::GetVertices(std::vector<SoftSkinVertex>& Vertices) const
{
	Vertices.clear();
	Vertices.resize(NumVertices);

	// Initialize the vertex data
	// All chunks are combined into one (rigid first, soft next)
	SoftSkinVertex* DestVertex = (SoftSkinVertex*)Vertices.data();
	for (uint32 SectionIndex = 0; SectionIndex < Sections.size(); SectionIndex++)
	{
		const SkeletalMeshSection& Section = Sections[SectionIndex];
		memcpy(DestVertex, Section.SoftVertices.data(), Section.SoftVertices.size() * sizeof(SoftSkinVertex));
		DestVertex += Section.SoftVertices.size();
	}
}

bool SkeletalMeshLODModel::DoSectionsNeedExtraBoneInfluences() const
{
	for (uint32 SectionIdx = 0; SectionIdx < Sections.size(); ++SectionIdx)
	{
		if (Sections[SectionIdx].HasExtraBoneInfluences())
		{
			return true;
		}
	}

	return false;
}
