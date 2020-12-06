#pragma once


namespace EComponentMobility
{
	enum Type
	{
		/**
		* Static objects cannot be moved or changed in game.
		* - Allows baked lighting
		* - Fastest rendering
		*/
		Static,

		/**
		* A stationary light will only have its shadowing and bounced lighting from static geometry baked by Lightmass, all other lighting will be dynamic.
		* - It can change color and intensity in game.
		* - Can't move
		* - Allows partial baked lighting
		* - Dynamic shadows
		*/
		Stationary,

		/**
		* Movable objects can be moved and changed in game.
		* - Totally dynamic
		* - Can cast dynamic shadows
		* - Slowest rendering
		*/
		Movable
	};
}

enum EIndirectLightingCacheQuality
{
	/** The indirect lighting cache will be disabled for this object, so no GI from stationary lights on movable objects. */
	ILCQ_Off,
	/** A single indirect lighting sample computed at the bounds origin will be interpolated which fades over time to newer results. */
	ILCQ_Point,
	/** The object will get a 5x5x5 stable volume of interpolated indirect lighting, which allows gradients of lighting intensity across the receiving object. */
	ILCQ_Volume
};

enum EBlendMode
{
	BLEND_Opaque,
	BLEND_Masked,
	BLEND_Translucent,
	BLEND_Additive,
	BLEND_Modulate,
	BLEND_AlphaComposite,
	BLEND_MAX,
};

enum EOcclusionCombineMode
{
	/** Take the minimum occlusion value.  This is effective for avoiding over-occlusion from multiple methods, but can result in indoors looking too flat. */
	OCM_Minimum,
	/**
	* Multiply together occlusion values from Distance Field Ambient Occlusion and Screen Space Ambient Occlusion.
	* This gives a good sense of depth everywhere, but can cause over-occlusion.
	* SSAO should be tweaked to be less strong compared to Minimum.
	*/
	OCM_Multiply,
	OCM_MAX,
};

enum class ELightUnits : uint8
{
	Unitless,
	Candelas,
	Lumens,
};
