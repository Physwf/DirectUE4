#pragma once

#include "SceneManagement.h"

class FShadowMap2D;
/**
* The abstract base class of 1D and 2D shadow-maps.
*/
class FShadowMap 
{
public:
	enum
	{
		SMT_None = 0,
		SMT_2D = 2,
	};

	/** The GUIDs of lights which this shadow-map stores. */
	std::vector<uint32> LightGuids;

	/** Default constructor. */
	FShadowMap() 
	{
	}

	FShadowMap(std::vector<uint32> InLightGuids)
		: LightGuids(std::move(InLightGuids))
	{
	}

	/** Destructor. */
	virtual ~FShadowMap() { }

	/**
	* Checks if a light is stored in this shadow-map.
	* @param	LightGuid - The GUID of the light to check for.
	* @return	True if the light is stored in the light-map.
	*/
	bool ContainsLight(const uint32& LightGuid) const
	{
		return std::find(LightGuids.begin(), LightGuids.end(), LightGuid) != LightGuids.end();
	}

	// FShadowMap interface.
	virtual FShadowMapInteraction GetInteraction() const = 0;

	// Runtime type casting.
	virtual FShadowMap2D* GetShadowMap2D() { return NULL; }
	virtual const FShadowMap2D* GetShadowMap2D() const { return NULL; }

protected:
	/**
	* Called when the light-map is no longer referenced.  Should release the lightmap's resources.
	*/
	virtual void Cleanup();
};

class FShadowMap2D : public FShadowMap
{
public:

	static std::shared_ptr<FShadowMap2D> AllocateShadowMap();

	FShadowMap2D();

	//FShadowMap2D(const TMap<ULightComponent*, FShadowMapData2D*>& ShadowMapData);
	FShadowMap2D(std::vector<uint32> InLightGuids);

	// Accessors.
	ID3D11ShaderResourceView* GetTexture() const { assert(IsValid()); return Texture; }
	const Vector2& GetCoordinateScale() const { assert(IsValid()); return CoordinateScale; }
	const Vector2& GetCoordinateBias() const { assert(IsValid()); return CoordinateBias; }
	bool IsValid() const { return Texture != NULL; }
	bool IsShadowFactorTexture() const { return false; }

	// FShadowMap interface.
	virtual FShadowMapInteraction GetInteraction() const;

	// Runtime type casting.
	virtual FShadowMap2D* GetShadowMap2D() { return this; }
	virtual const FShadowMap2D* GetShadowMap2D() const { return this; }



protected:

	/** The texture which contains the shadow-map data. */
	ID3D11ShaderResourceView* Texture;

	/** The scale which is applied to the shadow-map coordinates before sampling the shadow-map textures. */
	Vector2 CoordinateScale;

	/** The bias which is applied to the shadow-map coordinates before sampling the shadow-map textures. */
	Vector2 CoordinateBias;

	/** Tracks which of the 4 channels has valid texture data. */
	bool bChannelValid[4];

	/** Stores the inverse of the penumbra size, normalized.  Stores 1 to interpret the shadowmap as a shadow factor directly, instead of as a distance field. */
	Vector4 InvUniformPenumbraSize;

};
