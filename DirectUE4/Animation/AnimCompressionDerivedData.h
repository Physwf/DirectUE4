#pragma once

#include "UnrealMath.h"

#include <memory>
#include <vector>

class UAnimSequence;
struct FAnimCompressContext;

class FDerivedDataAnimationCompression
{
private:
	// Anim sequence we are providing DDC data for
	UAnimSequence * OriginalAnimSequence;

	// Possible duplicate animation for doing actual build work on
	UAnimSequence* DuplicateSequence;

	// FAnimCompressContext to use during compression if we don't pull from the DDC
	std::shared_ptr<FAnimCompressContext> CompressContext;

	// Whether to do compression work on original animation or duplicate it first.
	bool bDoCompressionInPlace;

public:
	FDerivedDataAnimationCompression(UAnimSequence* InAnimSequence, std::shared_ptr<FAnimCompressContext> InCompressContext, bool bInDoCompressionInPlace);
	virtual ~FDerivedDataAnimationCompression();

	virtual bool Build(std::vector<uint8>& OutData);
};