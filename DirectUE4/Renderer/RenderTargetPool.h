#pragma once

#include "UnrealMath.h"
#include "D3D11RHI.h"
#include <assert.h>
#include <vector>

/** Flags used for texture creation */
enum ETextureCreateFlags
{
	TexCreate_None = 0,

	// Texture can be used as a render target
	TexCreate_RenderTargetable = 1 << 0,
	// Texture can be used as a resolve target
	TexCreate_ResolveTargetable = 1 << 1,
	// Texture can be used as a depth-stencil target.
	TexCreate_DepthStencilTargetable = 1 << 2,
	// Texture can be used as a shader resource.
	TexCreate_ShaderResource = 1 << 3,

	// Texture is encoded in sRGB gamma space
	TexCreate_SRGB = 1 << 4,
	// Texture will be created without a packed miptail
	TexCreate_NoMipTail = 1 << 5,
	// Texture will be created with an un-tiled format
	TexCreate_NoTiling = 1 << 6,
	// Texture that may be updated every frame
	TexCreate_Dynamic = 1 << 8,
	// Allow silent texture creation failure
	TexCreate_AllowFailure = 1 << 9,
	// Disable automatic defragmentation if the initial texture memory allocation fails.
	TexCreate_DisableAutoDefrag = 1 << 10,
	// Create the texture with automatic -1..1 biasing
	TexCreate_BiasNormalMap = 1 << 11,
	// Create the texture with the flag that allows mip generation later, only applicable to D3D11
	TexCreate_GenerateMipCapable = 1 << 12,

	// The texture can be partially allocated in fastvram
	//TexCreate_FastVRAMPartialAlloc = 1 << 13,
	// UnorderedAccessView (DX11 only)
	// Warning: Causes additional synchronization between draw calls when using a render target allocated with this flag, use sparingly
	// See: GCNPerformanceTweets.pdf Tip 37
	TexCreate_UAV = 1 << 16,
	// Render target texture that will be displayed on screen (back buffer)
	TexCreate_Presentable = 1 << 17,
	// Texture data is accessible by the CPU
	TexCreate_CPUReadback = 1 << 18,
	// Texture was processed offline (via a texture conversion process for the current platform)
	TexCreate_OfflineProcessed = 1 << 19,
	// Texture needs to go in fast VRAM if available (HINT only)
	TexCreate_FastVRAM = 1 << 20,
	// by default the texture is not showing up in the list - this is to reduce clutter, using the FULL option this can be ignored
	TexCreate_HideInVisualizeTexture = 1 << 21,
	// Texture should be created in virtual memory, with no physical memory allocation made
	// You must make further calls to RHIVirtualTextureSetFirstMipInMemory to allocate physical memory
	// and RHIVirtualTextureSetFirstMipVisible to map the first mip visible to the GPU
	//TexCreate_Virtual = 1 << 22,
	// Creates a RenderTargetView for each array slice of the texture
	// Warning: if this was specified when the resource was created, you can't use SV_RenderTargetArrayIndex to route to other slices!
	TexCreate_TargetArraySlicesIndependently = 1 << 23,
	// Texture that may be shared with DX9 or other devices
	TexCreate_Shared = 1 << 24,
	// RenderTarget will not use full-texture fast clear functionality.
	TexCreate_NoFastClear = 1 << 25,
	// Texture is a depth stencil resolve target
	TexCreate_DepthStencilResolveTarget = 1 << 26,
	// Flag used to indicted this texture is a streamable 2D texture, and should be counted towards the texture streaming pool budget.
	TexCreate_Streamable = 1 << 27,
	// Render target will not FinalizeFastClear; Caches and meta data will be flushed, but clearing will be skipped (avoids potentially trashing metadata)
	TexCreate_NoFastClearFinalize = 1 << 28,
	// Hint to the driver that this resource is managed properly by the engine for Alternate-Frame-Rendering in mGPU usage.
	TexCreate_AFRManual = 1 << 29,
	// Workaround for 128^3 volume textures getting bloated 4x due to tiling mode on PS4
	TexCreate_ReduceMemoryWithTilingMode = 1 << 30,
	/** Texture should be allocated from transient memory. */
	TexCreate_Transient = 1 << 31
};

enum class EClearBinding
{
	ENoneBound, //no clear color associated with this target.  Target will not do hardware clears on most platforms
	EColorBound, //target has a clear color bound.  Clears will use the bound color, and do hardware clears.
	EDepthStencilBound, //target has a depthstencil value bound.  Clears will use the bound values and do hardware clears.
};

struct ClearValueBinding
{
	struct DSVAlue
	{
		float Depth;
		uint32 Stencil;
	};

	ClearValueBinding()
		: ColorBinding(EClearBinding::EColorBound)
	{
		Value.Color[0] = 0.0f;
		Value.Color[1] = 0.0f;
		Value.Color[2] = 0.0f;
		Value.Color[3] = 0.0f;
	}

	ClearValueBinding(EClearBinding NoBinding)
		: ColorBinding(NoBinding)
	{
		assert(ColorBinding == EClearBinding::ENoneBound);
	}

	explicit ClearValueBinding(const LinearColor& InClearColor)
		: ColorBinding(EClearBinding::EColorBound)
	{
		Value.Color[0] = InClearColor.R;
		Value.Color[1] = InClearColor.G;
		Value.Color[2] = InClearColor.B;
		Value.Color[3] = InClearColor.A;
	}

	explicit ClearValueBinding(float DepthClearValue, uint32 StencilClearValue = 0)
		: ColorBinding(EClearBinding::EDepthStencilBound)
	{
		Value.DSValue.Depth = DepthClearValue;
		Value.DSValue.Stencil = StencilClearValue;
	}

	LinearColor GetClearColor() const
	{
		assert(ColorBinding == EClearBinding::EColorBound);
		return LinearColor(Value.Color[0], Value.Color[1], Value.Color[2], Value.Color[3]);
	}

	void GetDepthStencil(float& OutDepth, uint32& OutStencil) const
	{
		assert(ColorBinding == EClearBinding::EDepthStencilBound);
		OutDepth = Value.DSValue.Depth;
		OutStencil = Value.DSValue.Stencil;
	}

	bool operator==(const ClearValueBinding& Other) const
	{
		if (ColorBinding == Other.ColorBinding)
		{
			if (ColorBinding == EClearBinding::EColorBound)
			{
				return
					Value.Color[0] == Other.Value.Color[0] &&
					Value.Color[1] == Other.Value.Color[1] &&
					Value.Color[2] == Other.Value.Color[2] &&
					Value.Color[3] == Other.Value.Color[3];

			}
			if (ColorBinding == EClearBinding::EDepthStencilBound)
			{
				return
					Value.DSValue.Depth == Other.Value.DSValue.Depth &&
					Value.DSValue.Stencil == Other.Value.DSValue.Stencil;
			}
			return true;
		}
		return false;
	}

	EClearBinding ColorBinding;

	union ClearValueType
	{
		float Color[4];
		DSVAlue DSValue;
	} Value;

	// common clear values
// 	static RHI_API const FClearValueBinding None;
// 	static RHI_API const FClearValueBinding Black;
// 	static RHI_API const FClearValueBinding White;
// 	static RHI_API const FClearValueBinding Transparent;
// 	static RHI_API const FClearValueBinding DepthOne;
// 	static RHI_API const FClearValueBinding DepthZero;
// 	static RHI_API const FClearValueBinding DepthNear;
// 	static RHI_API const FClearValueBinding DepthFar;
// 	static RHI_API const FClearValueBinding Green;
// 	static RHI_API const FClearValueBinding DefaultNormal8Bit;
};

struct PooledRenderTargetDesc
{
	/** Default constructor, use one of the factory functions below to make a valid description */
	PooledRenderTargetDesc()
		: ClearValue(ClearValueBinding())
		, Extent(0, 0)
		, Depth(0)
		, ArraySize(1)
		, bIsArray(false)
		, bIsCubemap(false)
		, NumMips(0)
		, NumSamples(1)
		, Format(DXGI_FORMAT_UNKNOWN)
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
		IntPoint InExtent,
		DXGI_FORMAT InFormat,
		const ClearValueBinding& InClearValue,
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

		return Extent.X != 0 && NumMips != 0 && NumSamples >= 1 && NumSamples <= 16 && Format != DXGI_FORMAT_UNKNOWN && ((TargetableFlags & TexCreate_UAV) == 0);
	}

	IntVector GetSize() const
	{
		return IntVector(Extent.X, Extent.Y, Depth);
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

	ClearValueBinding ClearValue;
	/** In pixels, (0,0) if not set, (x,0) for cube maps, todo: make 3d int vector for volume textures */
	IntPoint Extent;
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
	DXGI_FORMAT Format;
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
	ID3D11Resource* TargetableTexture;
	/** The 2D or cubemap shader-resource 2D texture that the targetable textures may be resolved to. */
	ID3D11Resource* ShaderResourceTexture;
	/** only created if requested through the flag, same as MipUAVs[0] */
	// TODO: refactor all the code to only use MipUAVs?
	ID3D11UnorderedAccessView* UAV;
	/** only created if requested through the flag  */
	std::vector<ID3D11UnorderedAccessView*> MipUAVs;
	/** only created if requested through the flag  */
	std::vector<ID3D11ShaderResourceView*> MipSRVs;

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

	bool FindFreeElement(const PooledRenderTargetDesc& Desc,ComPtr<PooledRenderTarget> &Out);

	// @return -1 if not found
	int32 FindIndex(PooledRenderTarget* In) const;
private:
	std::vector< ComPtr<PooledRenderTarget> > PooledRenderTargets;
	std::vector< ComPtr<PooledRenderTarget> > DeferredDeleteArray;
	//std::vector< FTextureRHIParamRef > TransitionTargets;

	/** These are snapshots, have odd life times, live in the scene allocator, and don't contribute to any accounting or other management. */
	std::vector<PooledRenderTarget*> PooledRenderTargetSnapshots;
};