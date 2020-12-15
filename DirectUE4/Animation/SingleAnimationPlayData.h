#pragma once

#include "UnrealMath.h"

class UAnimSingleNodeInstance;

struct FSingleAnimationPlayData
{
	// @todo in the future, we should make this one UObject
	// and have detail customization to display different things
	// The default sequence to play on this skeletal mesh
	class UAnimationAsset* AnimToPlay;
	/** Default setting for looping for SequenceToPlay. This is not current state of looping. */
	uint32 bSavedLooping : 1;
	/** Default setting for playing for SequenceToPlay. This is not current state of playing. */
	uint32 bSavedPlaying : 1;
	/** Default setting for position of SequenceToPlay to play. */
	float SavedPosition;
	/** Default setting for play rate of SequenceToPlay to play. */
	float SavedPlayRate;

	FSingleAnimationPlayData()
	{
		AnimToPlay = nullptr;
		SavedPlayRate = 1.0f;
		bSavedLooping = true;
		bSavedPlaying = true;
		SavedPosition = 0.f;
	}

	/** Called when initialized. */
	void Initialize(class UAnimSingleNodeInstance* Instance);

	/** Populates this play data with the current state of the supplied instance. */
	void PopulateFrom(UAnimSingleNodeInstance* Instance);

	void ValidatePosition();
};