#pragma once

#include "SceneManagement.h"

class FLightMap2D;

/**
* The abstract base class of 1D and 2D light-maps.
*/
class FLightMap 
{
public:
	enum
	{
		LMT_None = 0,
		LMT_1D = 1,
		LMT_2D = 2,
	};

	/** The GUIDs of lights which this light-map stores. */
	std::vector<uint32> LightGuids;

	/** Default constructor. */
	FLightMap();

	/** Destructor. */
	virtual ~FLightMap() {  }

	/**
	* Checks if a light is stored in this light-map.
	* @param	LightGuid - The GUID of the light to check for.
	* @return	True if the light is stored in the light-map.
	*/
	bool ContainsLight(const uint32& LightGuid) const
	{
		return std::find(LightGuids.begin(),LightGuids.end()) != LightGuids.end();
	}

	// FLightMap interface.
	virtual FLightMapInteraction GetInteraction() const = 0;

	// Runtime type casting.
	virtual FLightMap2D* GetLightMap2D() { return NULL; }
	virtual const FLightMap2D* GetLightMap2D() const { return NULL; }

	/**
	* @return true if high quality lightmaps are allowed
	*/
	inline bool AllowsHighQualityLightmaps() const
	{
		return bAllowHighQualityLightMaps;
	}

protected:

	/** Indicates whether the lightmap is being used for directional or simple lighting. */
	bool bAllowHighQualityLightMaps;

	/**
	* Called when the light-map is no longer referenced.  Should release the lightmap's resources.
	*/
	virtual void Cleanup();
};

/**
* A 2D array of incident lighting data.
*/
class FLightMap2D : public FLightMap
{
public:

	FLightMap2D();
	FLightMap2D(bool InAllowHighQualityLightMaps);

	// FLightMap2D interface.
	/**
	* Returns the texture containing the RGB coefficients for a specific basis.
	* @param	BasisIndex - The basis index.
	* @return	The RGB coefficient texture.
	*/
	ID3D11ShaderResourceView* const GetTexture(uint32 BasisIndex) const;
	ID3D11ShaderResourceView* GetTexture(uint32 BasisIndex);
	/**
	* Returns SkyOcclusionTexture.
	* @return	The SkyOcclusionTexture.
	*/
	ID3D11ShaderResourceView* GetSkyOcclusionTexture() const;

	ID3D11ShaderResourceView* GetAOMaterialMaskTexture() const;

	/**
	* Returns whether the specified basis has a valid lightmap texture or not.
	* @param	BasisIndex - The basis index.
	* @return	true if the specified basis has a valid lightmap texture, otherwise false
	*/
	bool IsValid(uint32 BasisIndex) const;

	const Vector2& GetCoordinateScale() const { return CoordinateScale; }
	const Vector2& GetCoordinateBias() const { return CoordinateBias; }

	// FLightMap interface.
	virtual FLightMapInteraction GetInteraction() const;

	// Runtime type casting.
	virtual const FLightMap2D* GetLightMap2D() const { return this; }
	virtual FLightMap2D* GetLightMap2D() { return this; }

	static std::shared_ptr<FLightMap2D> AllocateLightMap();

protected:

	friend struct FLightMapPendingTexture;

	FLightMap2D(const std::vector<uint32>& InLightGuids);

	/** The textures containing the light-map data. */
	ID3D11ShaderResourceView* Textures[2];

	ID3D11ShaderResourceView* SkyOcclusionTexture;

	ID3D11ShaderResourceView* AOMaterialMaskTexture;

	/** A scale to apply to the coefficients. */
	Vector4 ScaleVectors[NUM_STORED_LIGHTMAP_COEF];

	/** Bias value to apply to the coefficients. */
	Vector4 AddVectors[NUM_STORED_LIGHTMAP_COEF];

	/** The scale which is applied to the light-map coordinates before sampling the light-map textures. */
	Vector2 CoordinateScale;

	/** The bias which is applied to the light-map coordinates before sampling the light-map textures. */
	Vector2 CoordinateBias;
};
