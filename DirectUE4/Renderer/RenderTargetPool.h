#pragma once

#include "UnrealMath.h"
#include "D3D11RHI.h"
#include <assert.h>
#include <vector>

struct PooledRenderTargetDesc
{
	/** Default constructor, use one of the factory functions below to make a valid description */
	PooledRenderTargetDesc()
		: ClearValue(FClearValueBinding())
		, Extent(0, 0)
		, Depth(0)
		, ArraySize(1)
		, bIsArray(false)
		, bIsCubemap(false)
		, NumMips(0)
		, NumSamples(1)
		, Format(PF_Unknown)
		, Flags(TexCreate_None)
		, TargetableFlags(TexCreate_None)
		, bForceSeparateTargetAndShaderResource(false)
		, AutoWritable(true)
		, bCreateRenderTargetWriteMask(false)
	{
		//check(!IsValid());
	}

	/**
	* Factory function to create 2D texture description
	* @param InFlags bit mask combined from elements of ETextureCreateFlags e.g. TexCreate_UAV
	*/
	static PooledRenderTargetDesc Create2DDesc(
		FIntPoint InExtent,
		EPixelFormat InFormat,
		const FClearValueBinding& InClearValue,
		uint32 InFlags,
		uint32 InTargetableFlags,
		bool bInForceSeparateTargetAndShaderResource,
		uint16 InNumMips = 1,
		bool InAutowritable = true,
		bool InCreateRTWriteMask = false)
	{
		assert(InExtent.X);
		assert(InExtent.Y);

		PooledRenderTargetDesc NewDesc;
		NewDesc.ClearValue = InClearValue;
		NewDesc.Extent = InExtent;
		NewDesc.Depth = 0;
		NewDesc.ArraySize = 1;
		NewDesc.bIsArray = false;
		NewDesc.bIsCubemap = false;
		NewDesc.NumMips = InNumMips;
		NewDesc.NumSamples = 1;
		NewDesc.Format = InFormat;
		NewDesc.Flags = InFlags;
		NewDesc.TargetableFlags = InTargetableFlags;
		NewDesc.bForceSeparateTargetAndShaderResource = bInForceSeparateTargetAndShaderResource;
		NewDesc.AutoWritable = InAutowritable;
		NewDesc.bCreateRenderTargetWriteMask = InCreateRTWriteMask;
		assert(NewDesc.Is2DTexture());
		return NewDesc;
	}

	/**
	* Factory function to create cube map texture description
	* @param InFlags bit mask combined from elements of ETextureCreateFlags e.g. TexCreate_UAV
	*/
	static PooledRenderTargetDesc CreateCubemapDesc(
		uint32 InExtent,
		EPixelFormat InFormat,
		const FClearValueBinding& InClearValue,
		uint32 InFlags,
		uint32 InTargetableFlags,
		bool bInForceSeparateTargetAndShaderResource,
		uint32 InArraySize = 1,
		uint16 InNumMips = 1,
		bool InAutowritable = true)
	{
		assert(InExtent);

		PooledRenderTargetDesc NewDesc;
		NewDesc.ClearValue = InClearValue;
		NewDesc.Extent = FIntPoint(InExtent, InExtent);
		NewDesc.Depth = 0;
		NewDesc.ArraySize = InArraySize;
		// Note: this doesn't allow an array of size 1
		NewDesc.bIsArray = InArraySize > 1;
		NewDesc.bIsCubemap = true;
		NewDesc.NumMips = InNumMips;
		NewDesc.NumSamples = 1;
		NewDesc.Format = InFormat;
		NewDesc.Flags = InFlags;
		NewDesc.TargetableFlags = InTargetableFlags;
		NewDesc.bForceSeparateTargetAndShaderResource = bInForceSeparateTargetAndShaderResource;
		//NewDesc.DebugName = TEXT("UnknownTextureCube");
		NewDesc.AutoWritable = InAutowritable;
		assert(NewDesc.IsCubemap());

		return NewDesc;
	}

	/** Comparison operator to test if a render target can be reused */
	bool Compare(const PooledRenderTargetDesc& rhs, bool bExact) const
	{
		auto LhsFlags = Flags;
		auto RhsFlags = rhs.Flags;

		return Extent == rhs.Extent
			&& Depth == rhs.Depth
			&& bIsArray == rhs.bIsArray
			&& bIsCubemap == rhs.bIsCubemap
			&& ArraySize == rhs.ArraySize
			&& NumMips == rhs.NumMips
			&& NumSamples == rhs.NumSamples
			&& Format == rhs.Format
			&& LhsFlags == RhsFlags
			&& TargetableFlags == rhs.TargetableFlags
			&& bForceSeparateTargetAndShaderResource == rhs.bForceSeparateTargetAndShaderResource
			&& ClearValue == rhs.ClearValue
			&& AutoWritable == rhs.AutoWritable;
	}

	bool IsCubemap() const
	{
		return bIsCubemap;
	}

	bool Is2DTexture() const
	{
		return Extent.X != 0 && Extent.Y != 0 && Depth == 0 && !bIsCubemap;
	}

	bool Is3DTexture() const
	{
		return Extent.X != 0 && Extent.Y != 0 && Depth != 0 && !bIsCubemap;
	}

	// @return true if this texture is a texture array
	bool IsArray() const
	{
		return bIsArray;
	}

	bool IsValid() const
	{
		if (NumSamples != 1)
		{
			if (NumSamples < 1 || NumSamples > 8)
			{
				// D3D11 limitations
				return false;
			}

			if (!Is2DTexture())
			{
				return false;
			}
		}

		return Extent.X != 0 && NumMips != 0 && NumSamples >= 1 && NumSamples <= 16 && Format != PF_Unknown && ((TargetableFlags & TexCreate_UAV) == 0 || true/*GMaxRHIFeatureLevel == ERHIFeatureLevel::SM5*/);
	}

	FIntVector GetSize() const
	{
		return FIntVector(Extent.X, Extent.Y, Depth);
	}

	void Reset()
	{
		// Usually we don't want to propagate MSAA samples.
		NumSamples = 1;

		bForceSeparateTargetAndShaderResource = false;
		AutoWritable = true;

		// Remove UAV flag for rendertargets that don't need it (some formats are incompatible)
		TargetableFlags |= TexCreate_RenderTargetable;
		TargetableFlags &= (~TexCreate_UAV);
	}

	FClearValueBinding ClearValue;
	/** In pixels, (0,0) if not set, (x,0) for cube maps, todo: make 3d int vector for volume textures */
	FIntPoint Extent;
	/** 0, unless it's texture array or volume texture */
	uint32 Depth;
	/** >1 if a texture array should be used (not supported on DX9) */
	uint32 ArraySize;
	/** true if an array texture. Note that ArraySize still can be 1 */
	bool bIsArray;
	/** true if a cubemap texture */
	bool bIsCubemap;
	/** Number of mips */
	uint16 NumMips;
	/** Number of MSAA samples, default: 1  */
	uint16 NumSamples;
	/** Texture format e.g. PF_B8G8R8A8 */
	EPixelFormat Format;
	/** The flags that must be set on both the shader-resource and the targetable texture. bit mask combined from elements of ETextureCreateFlags e.g. TexCreate_UAV */
	uint32 Flags;
	/** The flags that must be set on the targetable texture. bit mask combined from elements of ETextureCreateFlags e.g. TexCreate_UAV */
	uint32 TargetableFlags;
	/** Whether the shader-resource and targetable texture must be separate textures. */
	bool bForceSeparateTargetAndShaderResource;
	/** automatically set to writable via barrier during */
	bool AutoWritable;
	/** create render target write mask (supported only on specific platforms) */
	bool bCreateRenderTargetWriteMask;
};

struct PooledRenderTarget
{
	PooledRenderTarget(const PooledRenderTargetDesc& InDesc, class RenderTargetPool* InRenderTargetPool)
		: NumRefs(0)
		, Desc(InDesc)
		, bSnapshot(false)
		, Pool(InRenderTargetPool)
	{
	}
	PooledRenderTarget(const PooledRenderTarget& SnaphotSource)
		: NumRefs(1)
		, Desc(SnaphotSource.Desc)
		, bSnapshot(true)
		, Pool(SnaphotSource.Pool)
	{
		//check(IsInRenderingThread());
		//RenderTargetItem = SnaphotSource.RenderTargetItem;
	}

	virtual ~PooledRenderTarget()
	{
		assert(!NumRefs || (bSnapshot && NumRefs == 1));
		//RenderTargetItem.SafeRelease();
	}

	bool IsSnapshot() const
	{
		return bSnapshot;
	}

	uint32 AddRef() const;
	uint32 Release();
	uint32 GetRefCount() const;
	bool IsFree() const;

	bool IsTransient() const
	{
		return !!(Desc.Flags & TexCreate_Transient);
	}
	virtual const PooledRenderTargetDesc& GetDesc() const;
	/** The 2D or cubemap texture that may be used as a render or depth-stencil target. */
	std::shared_ptr<FD3D11Texture2D> TargetableTexture;
	/** The 2D or cubemap shader-resource 2D texture that the targetable textures may be resolved to. */
	std::shared_ptr<FD3D11Texture2D> ShaderResourceTexture;
	/** only created if requested through the flag, same as MipUAVs[0] */
	// TODO: refactor all the code to only use MipUAVs?
	ComPtr<ID3D11UnorderedAccessView> UAV;
	/** only created if requested through the flag  */
	std::vector<ComPtr<ID3D11UnorderedAccessView>> MipUAVs;
	/** only created if requested through the flag  */
	std::vector<ComPtr<ID3D11ShaderResourceView>> MipSRVs;

	//ID3D11ShaderResourceView* RTWriteMaskBufferRHI_SRV;
	//FStructuredBufferRHIRef RTWriteMaskDataBufferRHI;

private:
	mutable int32 NumRefs;
	PooledRenderTargetDesc Desc;
	RenderTargetPool* Pool;

	/** Snapshots are sortof fake pooled render targets, they don't own anything and can outlive the things that created them. These are for threaded rendering. */
	bool bSnapshot;

	friend class RenderTargetPool;
};

class RenderTargetPool
{
public:
	RenderTargetPool();

	bool FindFreeElement(const PooledRenderTargetDesc& Desc,ComPtr<PooledRenderTarget> &Out, const wchar_t* DebugName);

	// @return -1 if not found
	int32 FindIndex(PooledRenderTarget* In) const;
private:
	std::vector< ComPtr<PooledRenderTarget> > PooledRenderTargets;
	std::vector< ComPtr<PooledRenderTarget> > DeferredDeleteArray;
	//std::vector< FTextureRHIParamRef > TransitionTargets;

	/** These are snapshots, have odd life times, live in the scene allocator, and don't contribute to any accounting or other management. */
	std::vector<PooledRenderTarget*> PooledRenderTargetSnapshots;
};

extern RenderTargetPool GRenderTargetPool;