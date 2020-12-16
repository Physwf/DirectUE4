#include "AnimCompressionDerivedData.h"
#include "AnimationUtils.h"
#include "AnimSequence.h"

FDerivedDataAnimationCompression::FDerivedDataAnimationCompression(UAnimSequence* InAnimSequence, std::shared_ptr<FAnimCompressContext> InCompressContext, bool bInDoCompressionInPlace)
	: OriginalAnimSequence(InAnimSequence)
	, DuplicateSequence(nullptr)
	, CompressContext(InCompressContext)
	, bDoCompressionInPlace(bInDoCompressionInPlace)
{

}

FDerivedDataAnimationCompression::~FDerivedDataAnimationCompression()
{

}

bool FDerivedDataAnimationCompression::Build(std::vector<uint8>& OutData)
{
	assert(OriginalAnimSequence != NULL);

	UAnimSequence* AnimToOperateOn;

	if (!bDoCompressionInPlace)
	{
// 		DuplicateSequence = DuplicateObject<UAnimSequence>(OriginalAnimSequence, GetTransientPackage());
// 		DuplicateSequence->AddToRoot();
// 		AnimToOperateOn = DuplicateSequence;
		AnimToOperateOn = OriginalAnimSequence;
	}
	else
	{
		AnimToOperateOn = OriginalAnimSequence;
	}

	bool bCompressionSuccessful = false;
	{
		AnimToOperateOn->UpdateCompressedTrackMapFromRaw();
// 		AnimToOperateOn->CompressedCurveData = AnimToOperateOn->RawCurveData; //Curves don't actually get compressed, but could have additives baked in
// 
// 		const float MaxCurveError = AnimToOperateOn->CompressionScheme->MaxCurveError;
// 		for (FFloatCurve& Curve : AnimToOperateOn->CompressedCurveData.FloatCurves)
// 		{
// 			Curve.FloatCurve.RemoveRedundantKeys(MaxCurveError);
// 		}


		AnimToOperateOn->CompressedByteStream.clear();
		AnimToOperateOn->CompressedTrackOffsets.clear();
		FAnimationUtils::CompressAnimSequence(AnimToOperateOn, *CompressContext.get());

		bCompressionSuccessful = AnimToOperateOn->IsCompressedDataValid();

		assert(bCompressionSuccessful);

		//AnimToOperateOn->CompressedRawDataSize = AnimToOperateOn->GetApproxRawSize();
	}

	return bCompressionSuccessful;
}

