#pragma once

#include "UnrealMath.h"

#define DEFAULT_SAMPLERATE			30.f
#define MINIMUM_ANIMATION_LENGTH	(1/DEFAULT_SAMPLERATE)

namespace SmartName
{
	// ID type, should be used to access SmartNames as fundamental type may change.
	typedef uint16 UID_Type;
	// Max UID used for overflow checking
	static const UID_Type MaxUID = MAX_uint16;
}

enum EAdditiveAnimationType
{
	/** No additive. */
	AAT_None,
	/* Create Additive from LocalSpace Base. */
	AAT_LocalSpaceBase,
	/* Create Additive from MeshSpace Rotation Only, Translation still will be LocalSpace. */
	AAT_RotationOffsetMeshSpace,
	AAT_MAX,
};

enum EAdditiveBasePoseType
{
	/** Will be deprecated. */
	ABPT_None,
	/** Use the Skeleton's ref pose as base. */
	ABPT_RefPose,
	/** Use a whole animation as a base pose. BasePoseSeq must be set. */
	ABPT_AnimScaled,
	/** Use one frame of an animation as a base pose. BasePoseSeq and RefFrameIndex must be set (RefFrameIndex will be clamped). */
	ABPT_AnimFrame,
	ABPT_MAX,
};

enum class EAnimInterpolationType : uint8
{
	/** Linear interpolation when looking up values between keys. */
	Linear,

	/** Step interpolation when looking up values between keys. */
	Step,
};