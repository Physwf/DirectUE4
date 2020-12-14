#include "AnimSequence.h"

void UAnimSequence::CleanAnimSequenceForImport()
{
	RawAnimationData.clear();
	//RawDataGuid.Invalidate();
	AnimationTrackNames.clear();
	TrackToSkeletonMapTable.clear();
	//CompressedTrackOffsets.Empty(0);
	//CompressedByteStream.Empty(0);
	//CompressedScaleOffsets.Empty(0);
	SourceRawAnimationData.clear();
}

