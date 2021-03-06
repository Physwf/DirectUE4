#pragma once

#include <dxgi.h>
#include <d3d11.h>
#include <wrl/client.h>

#include "UnrealMath.h"
#include "ShaderParameters.h"

#include <map>
#include <set>
#include <string>
#include <vector>
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>

using namespace Microsoft::WRL;

extern IDXGIFactory*	DXGIFactory;
extern IDXGIAdapter*	DXGIAdapter;
extern IDXGIOutput*	DXGIOutput;
extern IDXGISwapChain* DXGISwapChain;

extern ID3D11Device*			D3D11Device;
extern ID3D11DeviceContext*		D3D11DeviceContext;

extern class FD3D11Texture2D* BackBuffer;

extern ID3D11ShaderResourceView* GBlackTextureSRV;
extern ID3D11SamplerState* GBlackTextureSamplerState;
extern ID3D11ShaderResourceView* GWhiteTextureSRV;
extern ID3D11SamplerState* GWhiteTextureSamplerState;
extern ID3D11ShaderResourceView* GBlackVolumeTextureSRV;
extern ID3D11SamplerState* GBlackVolumeTextureSamplerState;
extern ID3D11ShaderResourceView* GWhiteTextureCubeSRV;
extern ID3D11SamplerState* GWhiteTextureCubeSamplerState;
extern std::shared_ptr<class FD3D11Texture2D> GBlackTextureDepthCube;

extern ComPtr<ID3D11Buffer> GNullColorVertexBuffer;
extern ComPtr<ID3D11ShaderResourceView> GNullColorVertexBufferSRV;

extern LONG WindowWidth;
extern LONG WindowHeight;

bool InitRHI();

struct ParameterAllocation
{
	UINT BufferIndex;
	UINT BaseIndex;
	UINT Size;
};

template <typename T>
inline bool IsValidRef(const ComPtr<T>& InReference)
{
	return InReference.Get() != nullptr;
}

ID3D11Buffer* CreateVertexBuffer(bool bDynamic,unsigned int Size, void* Data = NULL);
ID3D11Buffer* CreateIndexBuffer(void* Data, unsigned int Size);
ID3D11Buffer* CreateConstantBuffer(bool bDynamic, unsigned int Size,const void* Data = NULL);
ID3DBlob* CompileVertexShader(const wchar_t* File, const char* EntryPoint, const D3D_SHADER_MACRO* OtherMacros = NULL, int OtherMacrosCount = 0);
bool CompileShader(const std::string& FileContent, const char* EntryPoint, const char* Target, const D3D_SHADER_MACRO* OtherMacros, ID3DBlob** OutBytecode);
void GetShaderParameterAllocations(ID3DBlob* Code, FShaderParameterMap& OutShaderParameterMap);
ID3DBlob* CompilePixelShader(const wchar_t* File, const char* EntryPoint,const D3D_SHADER_MACRO* OtherMacros = NULL, int OtherMacrosCount = 0);
void GetShaderParameterAllocations(ID3DBlob* Code,std::map<std::string, ParameterAllocation>& OutParams);
ID3D11VertexShader* CreateVertexShader(ID3DBlob* VSBytecode);
ID3D11PixelShader* CreatePixelShader(ID3DBlob* PSBytecode);
ID3D11InputLayout* CreateInputLayout(D3D11_INPUT_ELEMENT_DESC* InputDesc, unsigned int Count, ID3DBlob* VSBytecode);
ID3D11Texture2D* CreateTexture2D(unsigned int W, unsigned int H, DXGI_FORMAT Format, bool bRenderTarget, bool bShaderResource, bool bDepthStencil, UINT MipMapCount = 1);
ID3D11Texture2D* CreateTexture2D(unsigned int W, unsigned int H, DXGI_FORMAT Format, UINT MipMapCount, void* InitData);
ID3D11Texture2D* CreateTextureCube(unsigned int Size, DXGI_FORMAT Format, uint32 MipMapCount, const uint8* InitData);
ID3D11Texture3D* CreateTexture3D(unsigned int X, unsigned int Y, unsigned int Z, DXGI_FORMAT Format, UINT MipMapCount, void* InitData);
ID3D11RenderTargetView* CreateRenderTargetView2D(ID3D11Texture2D* Resource, DXGI_FORMAT Format, UINT MipSlice);
ID3D11DepthStencilView* CreateDepthStencilView2D(ID3D11Texture2D* Resource, DXGI_FORMAT Format, UINT MipSlice);
ID3D11ShaderResourceView* CreateShaderResourceView2D(ID3D11Texture2D* Resource, DXGI_FORMAT Format, UINT MipLevels, UINT MostDetailedMip);
ID3D11ShaderResourceView* CreateShaderResourceViewCube(ID3D11Texture2D* Resource, DXGI_FORMAT Format, UINT MipLevels, UINT MostDetailedMip);
ID3D11ShaderResourceView* CreateShaderResourceView3D(ID3D11Texture3D* Resource, DXGI_FORMAT Format, UINT MipLevels, UINT MostDetailedMip);

template<typename InitializerType, typename RHIRefType, typename RHIParamRefType>
class TStaticStateRHI
{
public:
	static RHIParamRefType GetRHI()
	{
		static StaticStateResource* StaticResource;

		if (!StaticResource)
		{
			StaticResource = new StaticStateResource();
		}
		return StaticResource->StateRHI.Get();
	}
private:
	class StaticStateResource
	{
	public:
		RHIRefType StateRHI;

		StaticStateResource()
		{
			StateRHI = InitializerType::CreateRHI();
		}
		void InitRHI()
		{
			StateRHI = InitializerType::CreateRHI();
		}
		void ReleaseRHI()
		{
			StateRHI->Release();
		}
	};
};
/*
// Determine whether we should use one of the comparison modes
const bool bComparisonEnabled = Initializer.SamplerComparisonFunction != SCF_Never;
switch(Initializer.Filter)
{
case SF_AnisotropicLinear:
case SF_AnisotropicPoint:
if (SamplerDesc.MaxAnisotropy == 1)
{
SamplerDesc.Filter = bComparisonEnabled ? D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
}
else
{
// D3D11 doesn't allow using point filtering for mip filter when using anisotropic filtering
SamplerDesc.Filter = bComparisonEnabled ? D3D11_FILTER_COMPARISON_ANISOTROPIC : D3D11_FILTER_ANISOTROPIC;
}

break;
case SF_Trilinear:
SamplerDesc.Filter = bComparisonEnabled ? D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
break;
case SF_Bilinear:
SamplerDesc.Filter = bComparisonEnabled ? D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT : D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
break;
case SF_Point:
SamplerDesc.Filter = bComparisonEnabled ? D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT : D3D11_FILTER_MIN_MAG_MIP_POINT;
break;
}
*/
template<D3D11_FILTER Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
	D3D11_TEXTURE_ADDRESS_MODE AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
	D3D11_TEXTURE_ADDRESS_MODE AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
	D3D11_TEXTURE_ADDRESS_MODE AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
	int MipBias = 0,
	int MaxAnisotroy = 1,
	unsigned int BorderColor = 0,
	D3D11_COMPARISON_FUNC SamplerComparisonFunction = D3D11_COMPARISON_NEVER
>
class TStaticSamplerState : public TStaticStateRHI<TStaticSamplerState<Filter, AddressU, AddressV, AddressW, MipBias, MaxAnisotroy, BorderColor, SamplerComparisonFunction>, ComPtr<ID3D11SamplerState>, ID3D11SamplerState*>
{
public:
	static ID3D11SamplerState* CreateRHI()
	{
		D3D11_SAMPLER_DESC Desc;
		Desc.Filter = Filter;
		Desc.AddressU = AddressU;
		Desc.AddressV = AddressV;
		Desc.AddressW = AddressW;
		Desc.MipLODBias = MipBias;
		Desc.MaxAnisotropy = MaxAnisotroy;
		FLinearColor LC = FColor(BorderColor);
		Desc.BorderColor[0] = LC.R;
		Desc.BorderColor[1] = LC.G;
		Desc.BorderColor[2] = LC.B;
		Desc.BorderColor[3] = LC.A;
		Desc.ComparisonFunc = SamplerComparisonFunction;
		Desc.MinLOD = 0;
		Desc.MaxLOD = FLT_MAX;
		ID3D11SamplerState* Result;
		if (S_OK == D3D11Device->CreateSamplerState(&Desc, &Result))
		{
			return Result;
		}
		return NULL;
	}
};

template<D3D11_FILL_MODE FillMode = D3D11_FILL_SOLID,
	D3D11_CULL_MODE CullMode = D3D11_CULL_NONE,
	bool bEnableLineAA = FALSE,
	bool bEnableMSAA = TRUE
>
class TStaticRasterizerState : public TStaticStateRHI<TStaticRasterizerState<FillMode, CullMode, bEnableLineAA, bEnableMSAA>, ComPtr<ID3D11RasterizerState>, ID3D11RasterizerState*>
{
public:
	static ID3D11RasterizerState* CreateRHI()
	{
		D3D11_RASTERIZER_DESC Desc;
		ZeroMemory(&Desc, sizeof(D3D11_RASTERIZER_DESC));
		Desc.FillMode = FillMode;
		Desc.CullMode = CullMode;
		Desc.DepthBias = 0;
		Desc.DepthBiasClamp = 0;
		Desc.DepthClipEnable = TRUE;;
		Desc.SlopeScaledDepthBias = 0;
		Desc.AntialiasedLineEnable = bEnableLineAA;
		Desc.MultisampleEnable = bEnableMSAA;
		Desc.FrontCounterClockwise = TRUE;
		Desc.ScissorEnable = TRUE;
		ID3D11RasterizerState* Result;
		if (S_OK == D3D11Device->CreateRasterizerState(&Desc, &Result))
		{
			return Result;
		}
		return NULL;
	}
};

template<
	bool bEnableDepthWrite = true,
	D3D11_COMPARISON_FUNC DepthTest = D3D11_COMPARISON_LESS_EQUAL,
	bool bEnableFrontFaceStencil = false,
	D3D11_COMPARISON_FUNC FrontFaceStencilTest = D3D11_COMPARISON_ALWAYS,
	D3D11_STENCIL_OP FrontFaceStencilFailStencilOp = D3D11_STENCIL_OP_KEEP,
	D3D11_STENCIL_OP FrontFaceDepthFailStencilOp = D3D11_STENCIL_OP_KEEP,
	D3D11_STENCIL_OP FrontFacePassStencilOp = D3D11_STENCIL_OP_KEEP,
	bool bEnableBackFaceStencil = false,
	D3D11_COMPARISON_FUNC BackFaceStencilTest = D3D11_COMPARISON_ALWAYS,
	D3D11_STENCIL_OP BackFaceStencilFailStencilOp = D3D11_STENCIL_OP_KEEP,
	D3D11_STENCIL_OP BackFaceDepthFailStencilOp = D3D11_STENCIL_OP_KEEP,
	D3D11_STENCIL_OP BackFacePassStencilOp = D3D11_STENCIL_OP_KEEP,
	unsigned char StencilReadMask = 0xFF,
	unsigned char StencilWriteMask = 0xFF
>
class TStaticDepthStencilState : public TStaticStateRHI<
	TStaticDepthStencilState<
	bEnableDepthWrite,
	DepthTest,
	bEnableFrontFaceStencil,
	FrontFaceStencilTest,
	FrontFaceStencilFailStencilOp,
	FrontFaceDepthFailStencilOp,
	FrontFacePassStencilOp,
	bEnableBackFaceStencil,
	BackFaceStencilTest,
	BackFaceStencilFailStencilOp,
	BackFaceDepthFailStencilOp,
	BackFacePassStencilOp,
	StencilReadMask,
	StencilWriteMask
	>,
	ComPtr<ID3D11DepthStencilState>,
	ID3D11DepthStencilState*>
{
public:
	static ID3D11DepthStencilState* CreateRHI()
	{
		D3D11_DEPTH_STENCIL_DESC Desc;
		Desc.DepthEnable = bEnableDepthWrite || DepthTest != D3D11_COMPARISON_ALWAYS;
		Desc.DepthWriteMask = bEnableDepthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
		Desc.DepthFunc = DepthTest;
		Desc.StencilEnable = bEnableFrontFaceStencil || bEnableBackFaceStencil;
		Desc.FrontFace.StencilFunc = FrontFaceStencilTest;
		Desc.FrontFace.StencilFailOp = FrontFaceStencilFailStencilOp;
		Desc.FrontFace.StencilDepthFailOp = FrontFaceDepthFailStencilOp;
		Desc.FrontFace.StencilPassOp = FrontFacePassStencilOp;
		Desc.BackFace.StencilFunc = BackFaceStencilTest;
		Desc.BackFace.StencilFailOp = BackFaceStencilFailStencilOp;
		Desc.BackFace.StencilDepthFailOp = BackFaceDepthFailStencilOp;
		Desc.BackFace.StencilPassOp = BackFacePassStencilOp;

		ID3D11DepthStencilState* Result;
		if (S_OK == D3D11Device->CreateDepthStencilState(&Desc, &Result))
		{
			return Result;
		}
		return NULL;
	}
};

template<
	BOOL AlphaToCoverageEnable = FALSE,
	BOOL IndependentBlendEnable = TRUE,
	UINT8 RT0ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	D3D11_BLEND_OP RT0ColorBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT0ColorSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT0ColorDestBlend = D3D11_BLEND_ZERO,
	D3D11_BLEND_OP RT0AlphaBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT0AlphaSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT0AlphaDestBlend = D3D11_BLEND_ZERO,
	UINT8 RT1ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	D3D11_BLEND_OP RT1ColorBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT1ColorSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT1ColorDestBlend = D3D11_BLEND_ZERO,
	D3D11_BLEND_OP RT1AlphaBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT1AlphaSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT1AlphaDestBlend = D3D11_BLEND_ZERO,
	UINT8 RT2ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	D3D11_BLEND_OP RT2ColorBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT2ColorSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT2ColorDestBlend = D3D11_BLEND_ZERO,
	D3D11_BLEND_OP RT2AlphaBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT2AlphaSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT2AlphaDestBlend = D3D11_BLEND_ZERO,
	UINT8 RT3ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	D3D11_BLEND_OP RT3ColorBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT3ColorSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT3ColorDestBlend = D3D11_BLEND_ZERO,
	D3D11_BLEND_OP RT3AlphaBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT3AlphaSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT3AlphaDestBlend = D3D11_BLEND_ZERO,
	UINT8 RT4ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	D3D11_BLEND_OP RT4ColorBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT4ColorSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT4ColorDestBlend = D3D11_BLEND_ZERO,
	D3D11_BLEND_OP RT4AlphaBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT4AlphaSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT4AlphaDestBlend = D3D11_BLEND_ZERO,
	UINT8 RT5ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	D3D11_BLEND_OP RT5ColorBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT5ColorSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT5ColorDestBlend = D3D11_BLEND_ZERO,
	D3D11_BLEND_OP RT5AlphaBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT5AlphaSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT5AlphaDestBlend = D3D11_BLEND_ZERO,
	UINT8 RT6ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	D3D11_BLEND_OP RT6ColorBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT6ColorSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT6ColorDestBlend = D3D11_BLEND_ZERO,
	D3D11_BLEND_OP RT6AlphaBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT6AlphaSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT6AlphaDestBlend = D3D11_BLEND_ZERO,
	UINT8 RT7ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	D3D11_BLEND_OP RT7ColorBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT7ColorSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT7ColorDestBlend = D3D11_BLEND_ZERO,
	D3D11_BLEND_OP RT7AlphaBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT7AlphaSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT7AlphaDestBlend = D3D11_BLEND_ZERO
>
class TStaticBlendState : public TStaticStateRHI<
	TStaticBlendState<
	AlphaToCoverageEnable, IndependentBlendEnable,
	RT0ColorWriteMask, RT0ColorBlendOp, RT0ColorSrcBlend, RT0ColorDestBlend, RT0AlphaBlendOp, RT0AlphaSrcBlend, RT0AlphaDestBlend,
	RT1ColorWriteMask, RT1ColorBlendOp, RT1ColorSrcBlend, RT1ColorDestBlend, RT1AlphaBlendOp, RT1AlphaSrcBlend, RT1AlphaDestBlend,
	RT2ColorWriteMask, RT2ColorBlendOp, RT2ColorSrcBlend, RT2ColorDestBlend, RT2AlphaBlendOp, RT2AlphaSrcBlend, RT2AlphaDestBlend,
	RT3ColorWriteMask, RT3ColorBlendOp, RT3ColorSrcBlend, RT3ColorDestBlend, RT3AlphaBlendOp, RT3AlphaSrcBlend, RT3AlphaDestBlend,
	RT4ColorWriteMask, RT4ColorBlendOp, RT4ColorSrcBlend, RT4ColorDestBlend, RT4AlphaBlendOp, RT4AlphaSrcBlend, RT4AlphaDestBlend,
	RT5ColorWriteMask, RT5ColorBlendOp, RT5ColorSrcBlend, RT5ColorDestBlend, RT5AlphaBlendOp, RT5AlphaSrcBlend, RT5AlphaDestBlend,
	RT6ColorWriteMask, RT6ColorBlendOp, RT6ColorSrcBlend, RT6ColorDestBlend, RT6AlphaBlendOp, RT6AlphaSrcBlend, RT6AlphaDestBlend,
	RT7ColorWriteMask, RT7ColorBlendOp, RT7ColorSrcBlend, RT7ColorDestBlend, RT7AlphaBlendOp, RT7AlphaSrcBlend, RT7AlphaDestBlend
	>,
	ComPtr<ID3D11BlendState>,
	ID3D11BlendState*
>
{
public:
	static ID3D11BlendState* CreateRHI()
	{
		D3D11_BLEND_DESC Desc;
		Desc.AlphaToCoverageEnable = AlphaToCoverageEnable;
		Desc.IndependentBlendEnable = IndependentBlendEnable;
		Desc.RenderTarget[0].BlendEnable =
			RT0ColorBlendOp != D3D11_BLEND_OP_ADD || RT0ColorDestBlend != D3D11_BLEND_ZERO || RT0ColorSrcBlend != D3D11_BLEND_ONE ||
			RT0AlphaBlendOp != D3D11_BLEND_OP_ADD || RT0AlphaDestBlend != D3D11_BLEND_ZERO || RT0AlphaSrcBlend != D3D11_BLEND_ONE;
		Desc.RenderTarget[0].RenderTargetWriteMask = RT0ColorWriteMask;
		Desc.RenderTarget[0].BlendOp = RT0ColorBlendOp;
		Desc.RenderTarget[0].SrcBlend = RT0ColorSrcBlend;
		Desc.RenderTarget[0].DestBlend = RT0ColorDestBlend;
		Desc.RenderTarget[0].BlendOpAlpha = RT0AlphaBlendOp;
		Desc.RenderTarget[0].SrcBlendAlpha = RT0AlphaSrcBlend;
		Desc.RenderTarget[0].DestBlendAlpha = RT0AlphaDestBlend;

		Desc.RenderTarget[1].BlendEnable =
			RT1ColorBlendOp != D3D11_BLEND_OP_ADD || RT1ColorDestBlend != D3D11_BLEND_ZERO || RT1ColorSrcBlend != D3D11_BLEND_ONE ||
			RT1AlphaBlendOp != D3D11_BLEND_OP_ADD || RT1AlphaDestBlend != D3D11_BLEND_ZERO || RT1AlphaSrcBlend != D3D11_BLEND_ONE;
		Desc.RenderTarget[1].RenderTargetWriteMask = RT1ColorWriteMask;
		Desc.RenderTarget[1].BlendOp = RT1ColorBlendOp;
		Desc.RenderTarget[1].SrcBlend = RT1ColorSrcBlend;
		Desc.RenderTarget[1].DestBlend = RT1ColorDestBlend;
		Desc.RenderTarget[1].BlendOpAlpha = RT1AlphaBlendOp;
		Desc.RenderTarget[1].SrcBlendAlpha = RT1AlphaSrcBlend;
		Desc.RenderTarget[1].DestBlendAlpha = RT1AlphaDestBlend;

		Desc.RenderTarget[2].BlendEnable =
			RT2ColorBlendOp != D3D11_BLEND_OP_ADD || RT2ColorDestBlend != D3D11_BLEND_ZERO || RT2ColorSrcBlend != D3D11_BLEND_ONE ||
			RT2AlphaBlendOp != D3D11_BLEND_OP_ADD || RT2AlphaDestBlend != D3D11_BLEND_ZERO || RT2AlphaSrcBlend != D3D11_BLEND_ONE;
		Desc.RenderTarget[2].RenderTargetWriteMask = RT2ColorWriteMask;
		Desc.RenderTarget[2].BlendOp = RT2ColorBlendOp;
		Desc.RenderTarget[2].SrcBlend = RT2ColorSrcBlend;
		Desc.RenderTarget[2].DestBlend = RT2ColorDestBlend;
		Desc.RenderTarget[2].BlendOpAlpha = RT2AlphaBlendOp;
		Desc.RenderTarget[2].SrcBlendAlpha = RT2AlphaSrcBlend;
		Desc.RenderTarget[2].DestBlendAlpha = RT2AlphaDestBlend;

		Desc.RenderTarget[3].BlendEnable =
			RT3ColorBlendOp != D3D11_BLEND_OP_ADD || RT3ColorDestBlend != D3D11_BLEND_ZERO || RT3ColorSrcBlend != D3D11_BLEND_ONE ||
			RT3AlphaBlendOp != D3D11_BLEND_OP_ADD || RT3AlphaDestBlend != D3D11_BLEND_ZERO || RT3AlphaSrcBlend != D3D11_BLEND_ONE;
		Desc.RenderTarget[3].RenderTargetWriteMask = RT3ColorWriteMask;
		Desc.RenderTarget[3].BlendOp = RT3ColorBlendOp;
		Desc.RenderTarget[3].SrcBlend = RT3ColorSrcBlend;
		Desc.RenderTarget[3].DestBlend = RT3ColorDestBlend;
		Desc.RenderTarget[3].BlendOpAlpha = RT3AlphaBlendOp;
		Desc.RenderTarget[3].SrcBlendAlpha = RT3AlphaSrcBlend;
		Desc.RenderTarget[3].DestBlendAlpha = RT3AlphaDestBlend;

		Desc.RenderTarget[4].BlendEnable =
			RT4ColorBlendOp != D3D11_BLEND_OP_ADD || RT4ColorDestBlend != D3D11_BLEND_ZERO || RT4ColorSrcBlend != D3D11_BLEND_ONE ||
			RT4AlphaBlendOp != D3D11_BLEND_OP_ADD || RT4AlphaDestBlend != D3D11_BLEND_ZERO || RT4AlphaSrcBlend != D3D11_BLEND_ONE;
		Desc.RenderTarget[4].RenderTargetWriteMask = RT4ColorWriteMask;
		Desc.RenderTarget[4].BlendOp = RT4ColorBlendOp;
		Desc.RenderTarget[4].SrcBlend = RT4ColorSrcBlend;
		Desc.RenderTarget[4].DestBlend = RT4ColorDestBlend;
		Desc.RenderTarget[4].BlendOpAlpha = RT4AlphaBlendOp;
		Desc.RenderTarget[4].SrcBlendAlpha = RT4AlphaSrcBlend;
		Desc.RenderTarget[4].DestBlendAlpha = RT4AlphaDestBlend;

		Desc.RenderTarget[5].BlendEnable =
			RT5ColorBlendOp != D3D11_BLEND_OP_ADD || RT5ColorDestBlend != D3D11_BLEND_ZERO || RT5ColorSrcBlend != D3D11_BLEND_ONE ||
			RT5AlphaBlendOp != D3D11_BLEND_OP_ADD || RT5AlphaDestBlend != D3D11_BLEND_ZERO || RT5AlphaSrcBlend != D3D11_BLEND_ONE;
		Desc.RenderTarget[5].RenderTargetWriteMask = RT5ColorWriteMask;
		Desc.RenderTarget[5].BlendOp = RT5ColorBlendOp;
		Desc.RenderTarget[5].SrcBlend = RT5ColorSrcBlend;
		Desc.RenderTarget[5].DestBlend = RT5ColorDestBlend;
		Desc.RenderTarget[5].BlendOpAlpha = RT5AlphaBlendOp;
		Desc.RenderTarget[5].SrcBlendAlpha = RT5AlphaSrcBlend;
		Desc.RenderTarget[5].DestBlendAlpha = RT5AlphaDestBlend;

		Desc.RenderTarget[6].BlendEnable =
			RT6ColorBlendOp != D3D11_BLEND_OP_ADD || RT6ColorDestBlend != D3D11_BLEND_ZERO || RT6ColorSrcBlend != D3D11_BLEND_ONE ||
			RT6AlphaBlendOp != D3D11_BLEND_OP_ADD || RT6AlphaDestBlend != D3D11_BLEND_ZERO || RT6AlphaSrcBlend != D3D11_BLEND_ONE;
		Desc.RenderTarget[6].RenderTargetWriteMask = RT6ColorWriteMask;
		Desc.RenderTarget[6].BlendOp = RT6ColorBlendOp;
		Desc.RenderTarget[6].SrcBlend = RT6ColorSrcBlend;
		Desc.RenderTarget[6].DestBlend = RT6ColorDestBlend;
		Desc.RenderTarget[6].BlendOpAlpha = RT6AlphaBlendOp;
		Desc.RenderTarget[6].SrcBlendAlpha = RT6AlphaSrcBlend;
		Desc.RenderTarget[6].DestBlendAlpha = RT6AlphaDestBlend;

		Desc.RenderTarget[7].BlendEnable =
			RT7ColorBlendOp != D3D11_BLEND_OP_ADD || RT7ColorDestBlend != D3D11_BLEND_ZERO || RT7ColorSrcBlend != D3D11_BLEND_ONE ||
			RT7AlphaBlendOp != D3D11_BLEND_OP_ADD || RT7AlphaDestBlend != D3D11_BLEND_ZERO || RT7AlphaSrcBlend != D3D11_BLEND_ONE;
		Desc.RenderTarget[7].RenderTargetWriteMask = RT7ColorWriteMask;
		Desc.RenderTarget[7].BlendOp = RT7ColorBlendOp;
		Desc.RenderTarget[7].SrcBlend = RT7ColorSrcBlend;
		Desc.RenderTarget[7].DestBlend = RT7ColorDestBlend;
		Desc.RenderTarget[7].BlendOpAlpha = RT7AlphaBlendOp;
		Desc.RenderTarget[7].SrcBlendAlpha = RT7AlphaSrcBlend;
		Desc.RenderTarget[7].DestBlendAlpha = RT7AlphaDestBlend;

		ID3D11BlendState* Result;
		if (S_OK == D3D11Device->CreateBlendState(&Desc, &Result))
		{
			return Result;
		}
		return NULL;
	}
};

template<
	UINT8 RT0ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	UINT8 RT1ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	UINT8 RT2ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	UINT8 RT3ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	UINT8 RT4ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	UINT8 RT5ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	UINT8 RT6ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	UINT8 RT7ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL
>
class TStaticBlendStateWriteMask : public TStaticBlendState<FALSE,FALSE,
	RT0ColorWriteMask, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO,
	RT1ColorWriteMask, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO,
	RT2ColorWriteMask, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO,
	RT3ColorWriteMask, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO,
	RT4ColorWriteMask, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO,
	RT5ColorWriteMask, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO,
	RT6ColorWriteMask, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO,
	RT7ColorWriteMask, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO
>
{
public:
	static ID3D11BlendState* CrateRHI()
	{
		return TStaticBlendState<
			RT0ColorWriteMask, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO,
			RT1ColorWriteMask, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO,
			RT2ColorWriteMask, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO,
			RT3ColorWriteMask, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO,
			RT4ColorWriteMask, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO,
			RT5ColorWriteMask, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO,
			RT6ColorWriteMask, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO,
			RT7ColorWriteMask, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO
		>::CreateRHI();
	}
};

class FExclusiveDepthStencil
{
public:
	enum Type
	{
		// don't use those directly, use the combined versions below
		// 4 bits are used for depth and 4 for stencil to make the hex value readable and non overlapping
		DepthNop = 0x00,
		DepthRead = 0x01,
		DepthWrite = 0x02,
		DepthMask = 0x0f,
		StencilNop = 0x00,
		StencilRead = 0x10,
		StencilWrite = 0x20,
		StencilMask = 0xf0,

		// use those:
		DepthNop_StencilNop = DepthNop + StencilNop,
		DepthRead_StencilNop = DepthRead + StencilNop,
		DepthWrite_StencilNop = DepthWrite + StencilNop,
		DepthNop_StencilRead = DepthNop + StencilRead,
		DepthRead_StencilRead = DepthRead + StencilRead,
		DepthWrite_StencilRead = DepthWrite + StencilRead,
		DepthNop_StencilWrite = DepthNop + StencilWrite,
		DepthRead_StencilWrite = DepthRead + StencilWrite,
		DepthWrite_StencilWrite = DepthWrite + StencilWrite,
	};

private:
	Type Value;

public:
	// constructor
	FExclusiveDepthStencil(Type InValue = DepthNop_StencilNop)
		: Value(InValue)
	{
	}

	inline bool IsUsingDepthStencil() const
	{
		return Value != DepthNop_StencilNop;
	}
	inline bool IsUsingDepth() const
	{
		return (ExtractDepth() != DepthNop);
	}
	inline bool IsUsingStencil() const
	{
		return (ExtractStencil() != StencilNop);
	}
	inline bool IsDepthWrite() const
	{
		return ExtractDepth() == DepthWrite;
	}
	inline bool IsStencilWrite() const
	{
		return ExtractStencil() == StencilWrite;
	}

	inline bool IsAnyWrite() const
	{
		return IsDepthWrite() || IsStencilWrite();
	}

	inline void SetDepthWrite()
	{
		Value = (Type)(ExtractStencil() | DepthWrite);
	}
	inline void SetStencilWrite()
	{
		Value = (Type)(ExtractDepth() | StencilWrite);
	}
	inline void SetDepthStencilWrite(bool bDepth, bool bStencil)
	{
		Value = DepthNop_StencilNop;

		if (bDepth)
		{
			SetDepthWrite();
		}
		if (bStencil)
		{
			SetStencilWrite();
		}
	}
	bool operator==(const FExclusiveDepthStencil& rhs) const
	{
		return Value == rhs.Value;
	}

	bool operator != (const FExclusiveDepthStencil& RHS) const
	{
		return Value != RHS.Value;
	}

	inline bool IsValid(FExclusiveDepthStencil& Current) const
	{
		Type Depth = ExtractDepth();

		if (Depth != DepthNop && Depth != Current.ExtractDepth())
		{
			return false;
		}

		Type Stencil = ExtractStencil();

		if (Stencil != StencilNop && Stencil != Current.ExtractStencil())
		{
			return false;
		}

		return true;
	}

	uint32 GetIndex() const
	{
		// Note: The array to index has views created in that specific order.

		// we don't care about the Nop versions so less views are needed
		// we combine Nop and Write
		switch (Value)
		{
		case DepthWrite_StencilNop:
		case DepthNop_StencilWrite:
		case DepthWrite_StencilWrite:
		case DepthNop_StencilNop:
			return 0; // old DSAT_Writable

		case DepthRead_StencilNop:
		case DepthRead_StencilWrite:
			return 1; // old DSAT_ReadOnlyDepth

		case DepthNop_StencilRead:
		case DepthWrite_StencilRead:
			return 2; // old DSAT_ReadOnlyStencil

		case DepthRead_StencilRead:
			return 3; // old DSAT_ReadOnlyDepthAndStencil
		}
		// should never happen
		assert(0);
		return -1;
	}
	static const uint32 MaxIndex = 4;

private:
	inline Type ExtractDepth() const
	{
		return (Type)(Value & DepthMask);
	}
	inline Type ExtractStencil() const
	{
		return (Type)(Value & StencilMask);
	}
};

enum class EClearBinding
{
	ENoneBound, //no clear color associated with this target.  Target will not do hardware clears on most platforms
	EColorBound, //target has a clear color bound.  Clears will use the bound color, and do hardware clears.
	EDepthStencilBound, //target has a depthstencil value bound.  Clears will use the bound values and do hardware clears.
};
enum class ERHIZBuffer
{
	// Before changing this, make sure all math & shader assumptions are correct! Also wrap your C++ assumptions with
	//		static_assert(ERHIZBuffer::IsInvertedZBuffer(), ...);
	// Shader-wise, make sure to update Definitions.usf, HAS_INVERTED_Z_BUFFER
	FarPlane = 0,
	NearPlane = 1,

	// 'bool' for knowing if the API is using Inverted Z buffer
	IsInverted = (int32)((int32)ERHIZBuffer::FarPlane < (int32)ERHIZBuffer::NearPlane),
};
struct FClearValueBinding
{
	struct DSVAlue
	{
		float Depth;
		uint32 Stencil;
	};

	FClearValueBinding()
		: ColorBinding(EClearBinding::EColorBound)
	{
		Value.Color[0] = 0.0f;
		Value.Color[1] = 0.0f;
		Value.Color[2] = 0.0f;
		Value.Color[3] = 0.0f;
	}

	FClearValueBinding(EClearBinding NoBinding)
		: ColorBinding(NoBinding)
	{
		assert(ColorBinding == EClearBinding::ENoneBound);
	}

	explicit FClearValueBinding(const FLinearColor& InClearColor)
		: ColorBinding(EClearBinding::EColorBound)
	{
		Value.Color[0] = InClearColor.R;
		Value.Color[1] = InClearColor.G;
		Value.Color[2] = InClearColor.B;
		Value.Color[3] = InClearColor.A;
	}

	explicit FClearValueBinding(float DepthClearValue, uint32 StencilClearValue = 0)
		: ColorBinding(EClearBinding::EDepthStencilBound)
	{
		Value.DSValue.Depth = DepthClearValue;
		Value.DSValue.Stencil = StencilClearValue;
	}

	FLinearColor GetClearColor() const
	{
		assert(ColorBinding == EClearBinding::EColorBound);
		return FLinearColor(Value.Color[0], Value.Color[1], Value.Color[2], Value.Color[3]);
	}

	void GetDepthStencil(float& OutDepth, uint32& OutStencil) const
	{
		assert(ColorBinding == EClearBinding::EDepthStencilBound);
		OutDepth = Value.DSValue.Depth;
		OutStencil = Value.DSValue.Stencil;
	}

	bool operator==(const FClearValueBinding& Other) const
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
	static const FClearValueBinding None;
	static const FClearValueBinding Black;
	static const FClearValueBinding White;
	static const FClearValueBinding Transparent;
	static const FClearValueBinding DepthOne;
	static const FClearValueBinding DepthZero;
	static const FClearValueBinding DepthNear;
	static const FClearValueBinding DepthFar;
	static const FClearValueBinding Green;
	static const FClearValueBinding DefaultNormal8Bit;
};
#undef PF_MAX
enum EPixelFormat
{
	PF_Unknown = 0,
	PF_A32B32G32R32F = 1,
	PF_B8G8R8A8 = 2,
	PF_G8 = 3,
	PF_G16 = 4,
	PF_DXT1 = 5,
	PF_DXT3 = 6,
	PF_DXT5 = 7,
	PF_UYVY = 8,
	PF_FloatRGB = 9,
	PF_FloatRGBA = 10,
	PF_DepthStencil = 11,
	PF_ShadowDepth = 12,
	PF_R32_FLOAT = 13,
	PF_G16R16 = 14,
	PF_G16R16F = 15,
	PF_G16R16F_FILTER = 16,
	PF_G32R32F = 17,
	PF_A2B10G10R10 = 18,
	PF_A16B16G16R16 = 19,
	PF_D24 = 20,
	PF_R16F = 21,
	PF_R16F_FILTER = 22,
	PF_BC5 = 23,
	PF_V8U8 = 24,
	PF_A1 = 25,
	PF_FloatR11G11B10 = 26,
	PF_A8 = 27,
	PF_R32_UINT = 28,
	PF_R32_SINT = 29,
	PF_PVRTC2 = 30,
	PF_PVRTC4 = 31,
	PF_R16_UINT = 32,
	PF_R16_SINT = 33,
	PF_R16G16B16A16_UINT = 34,
	PF_R16G16B16A16_SINT = 35,
	PF_R5G6B5_UNORM = 36,
	PF_R8G8B8A8 = 37,
	PF_A8R8G8B8 = 38,	// Only used for legacy loading; do NOT use!
	PF_BC4 = 39,
	PF_R8G8 = 40,
	PF_ATC_RGB = 41,
	PF_ATC_RGBA_E = 42,
	PF_ATC_RGBA_I = 43,
	PF_X24_G8 = 44,	// Used for creating SRVs to alias a DepthStencil buffer to read Stencil. Don't use for creating textures.
	PF_ETC1 = 45,
	PF_ETC2_RGB = 46,
	PF_ETC2_RGBA = 47,
	PF_R32G32B32A32_UINT = 48,
	PF_R16G16_UINT = 49,
	PF_ASTC_4x4 = 50,	// 8.00 bpp
	PF_ASTC_6x6 = 51,	// 3.56 bpp
	PF_ASTC_8x8 = 52,	// 2.00 bpp
	PF_ASTC_10x10 = 53,	// 1.28 bpp
	PF_ASTC_12x12 = 54,	// 0.89 bpp
	PF_BC6H = 55,
	PF_BC7 = 56,
	PF_R8_UINT = 57,
	PF_L8 = 58,
	PF_XGXR8 = 59,
	PF_R8G8B8A8_UINT = 60,
	PF_R8G8B8A8_SNORM = 61,
	PF_R16G16B16A16_UNORM = 62,
	PF_R16G16B16A16_SNORM = 63,
	PF_MAX = 64,

};

/** Information about a pixel format. */
struct FPixelFormatInfo
{
	const wchar_t*	Name;
	int32			BlockSizeX,
					BlockSizeY,
					BlockSizeZ,
					BlockBytes,
					NumComponents;
	/** Platform specific token, e.g. D3DFORMAT with D3DDrv										*/
	uint32			PlatformFormat;
	/** Whether the texture format is supported on the current platform/ rendering combination	*/
	bool			Supported;
	EPixelFormat	UnrealFormat;
};

class FD3D11Texture
{
public:
	FD3D11Texture(
		uint32 InSizeX,
		uint32 InSizeY,
		uint32 InSizeZ,
		uint32 InNumMips,
		uint32 InNumSamples,
		EPixelFormat InFormat,
		uint32 InFlags,
		const FClearValueBinding& InClearValue,
		ComPtr<ID3D11ShaderResourceView> InShaderResourceView,
		int32 InRTVArraySize,
		bool bInCreatedRTVsPerSlice,
		const std::vector<ComPtr<ID3D11RenderTargetView>>& InRenderTargetViews,
		ComPtr<ID3D11DepthStencilView>* InDepthStencilViews
	) 
		: SizeX(InSizeX)
		, SizeY(InSizeY)
		, SizeZ(InSizeZ)
		, ClearValue(InClearValue)
		, NumMips(InNumMips)
		, NumSamples(InNumSamples)
		, Format(InFormat)
		, Flags(InFlags)
		, ShaderResourceView(InShaderResourceView)
		, RenderTargetViews(InRenderTargetViews)
		, bCreatedRTVsPerSlice(bInCreatedRTVsPerSlice)
		, RTVArraySize(InRTVArraySize)
		, NumDepthStencilViews(0)
	{
		if (InDepthStencilViews != nullptr)
		{
			for (uint32 Index = 0; Index < FExclusiveDepthStencil::MaxIndex; Index++)
			{
				DepthStencilViews[Index] = InDepthStencilViews[Index];
				// New Monolithic Graphics drivers have optional "fast calls" replacing various D3d functions
				// You can't use fast version of XXSetShaderResources (called XXSetFastShaderResource) on dynamic or d/s targets
				if (DepthStencilViews[Index] != NULL)
					NumDepthStencilViews++;
			}
		}
	}
	virtual ID3D11Resource* GetResource() const { return NULL; }

	virtual class FD3D11Texture2D* GetTexture2D() { return NULL; }
	virtual class FD3D11Texture2D* GetTexture2DArray() { return NULL; }
	virtual class FD3D11Texture3D* GetTexture3D() { return NULL; }
	virtual class FD3D11Texture2D* GetTextureCube() { return NULL; }

	uint32 GetSizeX() const { return SizeX; }
	uint32 GetSizeY() const { return SizeY; }
	uint32 GetSizeZ() const { return SizeZ; }
	inline FIntPoint GetSizeXY() const { return FIntPoint(SizeX, SizeY); }
	inline FIntVector GetSizeXYZ() const { return FIntVector(SizeX, SizeY, SizeZ); };
	uint32 GetNumMips() const { return NumMips; }
	EPixelFormat GetFormat() const { return Format; }
	uint32 GetFlags() const { return Flags; }
	uint32 GetNumSamples() const { return NumSamples; }
	bool IsMultisampled() const { return NumSamples > 1; }

	ID3D11ShaderResourceView* GetShaderResourceView() const { return ShaderResourceView.Get(); }
	ID3D11RenderTargetView* GetRenderTargetView(int32 MipIndex, int32 ArraySliceIndex) const
	{
		int32 ArrayIndex = MipIndex;

		if (bCreatedRTVsPerSlice)
		{
			assert(ArraySliceIndex >= 0);
			ArrayIndex = MipIndex * RTVArraySize + ArraySliceIndex;
		}
		else
		{
			// Catch attempts to use a specific slice without having created the texture to support it
			assert(ArraySliceIndex == -1 || ArraySliceIndex == 0);
		}

		if ((uint32)ArrayIndex < (uint32)RenderTargetViews.size())
		{
			return RenderTargetViews[ArrayIndex].Get();
		}
		return 0;
	}
	ID3D11DepthStencilView* GetDepthStencilView(FExclusiveDepthStencil AccessType) const
	{
		return DepthStencilViews[AccessType.GetIndex()].Get();
	}
	// New Monolithic Graphics drivers have optional "fast calls" replacing various D3d functions
	// You can't use fast version of XXSetShaderResources (called XXSetFastShaderResource) on dynamic or d/s targets
	bool HasDepthStencilView()
	{
		return (NumDepthStencilViews > 0);
	}
	bool HasClearValue() const
	{
		return ClearValue.ColorBinding != EClearBinding::ENoneBound;
	}
	FLinearColor GetClearColor() const
	{
		return ClearValue.GetClearColor();
	}
	void GetDepthStencilClearValue(float& OutDepth, uint32& OutStencil) const
	{
		return ClearValue.GetDepthStencil(OutDepth, OutStencil);
	}
	float GetDepthClearValue() const
	{
		float Depth;
		uint32 Stencil;
		ClearValue.GetDepthStencil(Depth, Stencil);
		return Depth;
	}
	uint32 GetStencilClearValue() const
	{
		float Depth;
		uint32 Stencil;
		ClearValue.GetDepthStencil(Depth, Stencil);
		return Stencil;
	}
	const FClearValueBinding GetClearBinding() const
	{
		return ClearValue;
	}

private:
	uint32 SizeX;
	uint32 SizeY;
	uint32 SizeZ;
	FClearValueBinding ClearValue;
	uint32 NumMips;
	uint32 NumSamples;
	EPixelFormat Format;
	uint32 Flags;

	ComPtr<ID3D11ShaderResourceView> ShaderResourceView;
	std::vector<ComPtr<ID3D11RenderTargetView>> RenderTargetViews;
	bool bCreatedRTVsPerSlice;
	int32 RTVArraySize;
	ComPtr<ID3D11DepthStencilView> DepthStencilViews[FExclusiveDepthStencil::MaxIndex];
	uint32	NumDepthStencilViews;
};

class FD3D11Texture2D : public FD3D11Texture
{
public:
	FD3D11Texture2D(
		uint32 InSizeX, 
		uint32 InSizeY,
		uint32 InSizeZ,
		uint32 InNumMips, 
		uint32 InNumSamples, 
		EPixelFormat InFormat,
		bool bInCubemap,
		uint32 InFlags, 
		const FClearValueBinding& InClearValue,
		ComPtr<ID3D11Texture2D> InResource,
		ComPtr<ID3D11ShaderResourceView> InShaderResourceView,
		int32 InRTVArraySize,
		bool bInCreatedRTVsPerSlice,
		const std::vector<ComPtr<ID3D11RenderTargetView>>& InRenderTargetViews,
		ComPtr<ID3D11DepthStencilView>* InDepthStencilViews
		) 
	: FD3D11Texture(InSizeX, InSizeY, InSizeZ, InNumMips, InNumSamples, InFormat, InFlags, InClearValue, InShaderResourceView, InRTVArraySize, bInCreatedRTVsPerSlice, InRenderTargetViews, InDepthStencilViews)
	, bCubemap(bInCubemap)
	, Resource(InResource)
	{}

	virtual class FD3D11Texture2D* GetTexture2D() override { return this; }
	virtual class FD3D11Texture2D* GetTexture2DArray() override { return GetSizeZ() > 0 ?  this :  nullptr; }
	virtual class FD3D11Texture2D* GetTextureCube() override { return IsCube() ? this : nullptr; }

	bool IsCube() const { return bCubemap; }
	virtual ID3D11Texture2D* GetResource() const override { return Resource.Get(); }
	void Release() {}
private:
	ComPtr<ID3D11Texture2D> Resource;
	/** Number of Depth Stencil Views - used for fast call tracking. */
	const uint32 bCubemap : 1;
};
class FD3D11Texture3D : public FD3D11Texture
{
public:
	FD3D11Texture3D(
		ID3D11Texture3D* InResource,
		ID3D11ShaderResourceView* InShaderResourceView, 
		const std::vector<ComPtr<ID3D11RenderTargetView>>& InRenderTargetViews, 
		uint32 InSizeX, 
		uint32 InSizeY, 
		uint32 InSizeZ, 
		uint32 InNumMips, 
		EPixelFormat InFormat, 
		uint32 InFlags, 
		const FClearValueBinding& InClearValue
	) 
	: FD3D11Texture(InSizeX, InSizeY, InSizeZ, InNumMips, 1, InFormat, InFlags, InClearValue, InShaderResourceView, 1, false, InRenderTargetViews, NULL)
	, Resource(InResource)
	{

	}
	virtual class FD3D11Texture3D* GetTexture3D() override { return this; }

	virtual ID3D11Texture3D* GetResource() const override { return Resource.Get(); }
	//FRHIResourceInfo ResourceInfo;

	void SetName(const std::string& InName)
	{
		TextureName = InName;
	}

	std::string GetName() const
	{
		return TextureName;
	}
private:
	ComPtr<ID3D11Texture3D> Resource;
	std::string TextureName;
};
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
	TexCreate_FastVRAMPartialAlloc = 1 << 13,
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
	TexCreate_Virtual = 1 << 22,
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

ID3D11Buffer* RHICreateVertexBuffer(UINT Size, D3D11_USAGE InUsage, UINT BindFlags, UINT MiscFlags, const void* Data = NULL);
std::shared_ptr<FD3D11Texture2D> CreateD3D11Texture2D(uint32 SizeX, uint32 SizeY, uint32 SizeZ, bool bTextureArray, bool bCubeTexture, EPixelFormat Format, uint32 NumMips, uint32 NumSamples, uint32 Flags, FClearValueBinding ClearBindingValue = FClearValueBinding::Transparent, const void* BulkData = nullptr, uint32 BulkDataSize = 0);
ComPtr<ID3D11ShaderResourceView> RHICreateShaderResourceView(FD3D11Texture2D* Texture2DRHI, uint16 MipLevel);
ComPtr<ID3D11ShaderResourceView> RHICreateShaderResourceView(FD3D11Texture3D* Texture2DRHI, uint16 MipLevel);
ComPtr<ID3D11ShaderResourceView> RHICreateShaderResourceView(ID3D11Buffer* VertexBuffer, UINT Stride, DXGI_FORMAT Format);
inline std::shared_ptr<FD3D11Texture2D> RHICreateTexture2D(uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, uint32 Flags, FClearValueBinding ClearBindingValue = FClearValueBinding::Transparent, const void* BulkData = nullptr, uint32 BulkDataSize = 0)
{
	return CreateD3D11Texture2D(SizeX, SizeY,1,false,false, (EPixelFormat)Format, NumMips, NumSamples, Flags, ClearBindingValue, BulkData, BulkDataSize);
}
inline std::shared_ptr<FD3D11Texture2D> RHICreateTexture2DArray(uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, uint32 Flags, FClearValueBinding ClearBindingValue = FClearValueBinding::Transparent, const void* BulkData = nullptr, uint32 BulkDataSize = 0)
{
	return CreateD3D11Texture2D(SizeX, SizeY, SizeZ, true, false, (EPixelFormat)Format, NumMips, 1, Flags, ClearBindingValue, BulkData, BulkDataSize);
}
inline std::shared_ptr<FD3D11Texture2D> RHICreateTextureCube(uint32 Size, uint8 Format, uint32 NumMips, uint32 Flags, FClearValueBinding ClearBindingValue = FClearValueBinding::Transparent, const void* BulkData = nullptr, uint32 BulkDataSize = 0)
{
	return CreateD3D11Texture2D(Size, Size, 6, false, true, (EPixelFormat)Format, NumMips, 1, Flags, ClearBindingValue, BulkData, BulkDataSize);
}
inline std::shared_ptr<FD3D11Texture2D> RHICreateTextureCubeArray(uint32 Size, uint32 ArraySize, uint8 Format, uint32 NumMips, uint32 Flags, FClearValueBinding ClearBindingValue = FClearValueBinding::Transparent, const void* BulkData = nullptr, uint32 BulkDataSize = 0)
{
	return CreateD3D11Texture2D(Size, Size, 6 * ArraySize, true, true, (EPixelFormat)Format, NumMips, 1, Flags, ClearBindingValue, BulkData, BulkDataSize);
}
std::shared_ptr<FD3D11Texture3D> CreateD3D11Texture3D(uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, uint32 Flags, FClearValueBinding ClearBindingValue = FClearValueBinding::Transparent, const void* BulkData = nullptr, uint32 BulkDataSize = 0);
inline std::shared_ptr<FD3D11Texture3D> RHICreateTexture3D(uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, uint32 Flags, FClearValueBinding ClearBindingValue = FClearValueBinding::Transparent, const void* BulkData = nullptr, uint32 BulkDataSize = 0)
{
	return CreateD3D11Texture3D(SizeX, SizeY, SizeZ, Format, NumMips, Flags, ClearBindingValue, BulkData, BulkDataSize);
}
enum class ERenderTargetLoadAction : uint8
{
	ENoAction,
	ELoad,
	EClear,

	Num,
	NumBits = 2,
};
enum class ERenderTargetStoreAction : uint8
{
	ENoAction,
	EStore,
	EMultisampleResolve,

	Num,
	NumBits = 2,
};
inline void RHICreateTargetableShaderResource2D(
	uint32 SizeX,
	uint32 SizeY,
	uint8 Format,
	uint32 NumMips,
	uint32 Flags,
	uint32 TargetableTextureFlags,
	bool bForceSeparateTargetAndShaderResource,
	FClearValueBinding ClearValue,
	std::shared_ptr<FD3D11Texture>& OutTargetableTexture,
	std::shared_ptr<FD3D11Texture>& OutShaderResourceTexture,
	uint32 NumSamples = 1
)
{
	// Ensure none of the usage flags are passed in.
	assert(!(Flags & TexCreate_RenderTargetable));
	assert(!(Flags & TexCreate_ResolveTargetable));
	assert(!(Flags & TexCreate_ShaderResource));

	// Ensure that all of the flags provided for the targetable texture are not already passed in Flags.
	assert(!(Flags & TargetableTextureFlags));

	// Ensure that the targetable texture is either render or depth-stencil targetable.
	assert(TargetableTextureFlags & (TexCreate_RenderTargetable | TexCreate_DepthStencilTargetable | TexCreate_UAV));

	if (NumSamples > 1)
	{
		bForceSeparateTargetAndShaderResource = true; // RHISupportsSeparateMSAAAndResolveTextures(GMaxRHIShaderPlatform);
	}

	if (!bForceSeparateTargetAndShaderResource/* && GSupportsRenderDepthTargetableShaderResources*/)
	{
		// Create a single texture that has both TargetableTextureFlags and TexCreate_ShaderResource set.
		OutTargetableTexture = OutShaderResourceTexture = RHICreateTexture2D(SizeX, SizeY, Format, NumMips, NumSamples, Flags | TargetableTextureFlags | TexCreate_ShaderResource, ClearValue);
	}
	else
	{
		uint32 ResolveTargetableTextureFlags = TexCreate_ResolveTargetable;
		if (TargetableTextureFlags & TexCreate_DepthStencilTargetable)
		{
			ResolveTargetableTextureFlags |= TexCreate_DepthStencilResolveTarget;
		}
		// Create a texture that has TargetableTextureFlags set, and a second texture that has TexCreate_ResolveTargetable and TexCreate_ShaderResource set.
		OutTargetableTexture = RHICreateTexture2D(SizeX, SizeY, Format, NumMips, NumSamples, Flags | TargetableTextureFlags, ClearValue);
		OutShaderResourceTexture = RHICreateTexture2D(SizeX, SizeY, Format, NumMips, 1, Flags | ResolveTargetableTextureFlags | TexCreate_ShaderResource, ClearValue);
	}
}
inline void RHICreateTargetableShaderResource2DArray(
	uint32 SizeX,
	uint32 SizeY,
	uint32 SizeZ,
	uint8 Format,
	uint32 NumMips,
	uint32 Flags,
	uint32 TargetableTextureFlags,
	FClearValueBinding ClearValue,
	std::shared_ptr<FD3D11Texture>& OutTargetableTexture,
	std::shared_ptr<FD3D11Texture>& OutShaderResourceTexture,
	uint32 NumSamples = 1
)
{
	// Ensure none of the usage flags are passed in.
	assert(!(Flags & TexCreate_RenderTargetable));
	assert(!(Flags & TexCreate_ResolveTargetable));
	assert(!(Flags & TexCreate_ShaderResource));

	// Ensure that all of the flags provided for the targetable texture are not already passed in Flags.
	assert(!(Flags & TargetableTextureFlags));

	// Ensure that the targetable texture is either render or depth-stencil targetable.
	assert(TargetableTextureFlags & (TexCreate_RenderTargetable | TexCreate_DepthStencilTargetable));

	// Create a single texture that has both TargetableTextureFlags and TexCreate_ShaderResource set.
	OutTargetableTexture = OutShaderResourceTexture = RHICreateTexture2DArray(SizeX, SizeY, SizeZ, Format, NumMips, Flags | TargetableTextureFlags | TexCreate_ShaderResource, ClearValue);
}

inline void RHICreateTargetableShaderResourceCube(
	uint32 LinearSize,
	uint8 Format,
	uint32 NumMips,
	uint32 Flags,
	uint32 TargetableTextureFlags,
	bool bForceSeparateTargetAndShaderResource,
	FClearValueBinding ClearValue,
	std::shared_ptr<FD3D11Texture>& OutTargetableTexture,
	std::shared_ptr<FD3D11Texture>& OutShaderResourceTexture
)
{
	// Ensure none of the usage flags are passed in.
	assert(!(Flags & TexCreate_RenderTargetable));
	assert(!(Flags & TexCreate_ResolveTargetable));
	assert(!(Flags & TexCreate_ShaderResource));

	// Ensure that all of the flags provided for the targetable texture are not already passed in Flags.
	assert(!(Flags & TargetableTextureFlags));

	// Ensure that the targetable texture is either render or depth-stencil targetable.
	assert(TargetableTextureFlags & (TexCreate_RenderTargetable | TexCreate_DepthStencilTargetable));

	// ES2 doesn't support resolve operations.
	bForceSeparateTargetAndShaderResource &= true;// (GMaxRHIFeatureLevel > ERHIFeatureLevel::ES2);

	if (!bForceSeparateTargetAndShaderResource/* && GSupportsRenderDepthTargetableShaderResources*/)
	{
		// Create a single texture that has both TargetableTextureFlags and TexCreate_ShaderResource set.
		OutTargetableTexture = OutShaderResourceTexture = RHICreateTextureCube(LinearSize, Format, NumMips, Flags | TargetableTextureFlags | TexCreate_ShaderResource, ClearValue);
	}
	else
	{
		// Create a texture that has TargetableTextureFlags set, and a second texture that has TexCreate_ResolveTargetable and TexCreate_ShaderResource set.
		OutTargetableTexture = RHICreateTextureCube(LinearSize, Format, NumMips, Flags | TargetableTextureFlags, ClearValue);
		OutShaderResourceTexture = RHICreateTextureCube(LinearSize, Format, NumMips, Flags | TexCreate_ResolveTargetable | TexCreate_ShaderResource, ClearValue);
	}
}
inline void RHICreateTargetableShaderResourceCubeArray(
	uint32 LinearSize,
	uint32 ArraySize,
	uint8 Format,
	uint32 NumMips,
	uint32 Flags,
	uint32 TargetableTextureFlags,
	bool bForceSeparateTargetAndShaderResource,
	FClearValueBinding ClearValue,
	std::shared_ptr<FD3D11Texture>& OutTargetableTexture,
	std::shared_ptr<FD3D11Texture>& OutShaderResourceTexture
)
{
	// Ensure none of the usage flags are passed in.
	assert(!(Flags & TexCreate_RenderTargetable));
	assert(!(Flags & TexCreate_ResolveTargetable));
	assert(!(Flags & TexCreate_ShaderResource));

	// Ensure that all of the flags provided for the targetable texture are not already passed in Flags.
	assert(!(Flags & TargetableTextureFlags));

	// Ensure that the targetable texture is either render or depth-stencil targetable.
	assert(TargetableTextureFlags & (TexCreate_RenderTargetable | TexCreate_DepthStencilTargetable));

	if (!bForceSeparateTargetAndShaderResource/* && GSupportsRenderDepthTargetableShaderResources*/)
	{
		// Create a single texture that has both TargetableTextureFlags and TexCreate_ShaderResource set.
		OutTargetableTexture = OutShaderResourceTexture = RHICreateTextureCubeArray(LinearSize, ArraySize, Format, NumMips, Flags | TargetableTextureFlags | TexCreate_ShaderResource, ClearValue);
	}
	else
	{
		// Create a texture that has TargetableTextureFlags set, and a second texture that has TexCreate_ResolveTargetable and TexCreate_ShaderResource set.
		OutTargetableTexture = RHICreateTextureCubeArray(LinearSize, ArraySize, Format, NumMips, Flags | TargetableTextureFlags, ClearValue);
		OutShaderResourceTexture = RHICreateTextureCubeArray(LinearSize, ArraySize, Format, NumMips, Flags | TexCreate_ResolveTargetable | TexCreate_ShaderResource, ClearValue);
	}
}
extern FPixelFormatInfo GPixelFormats[PF_MAX];
/** RHI representation of a single stream out element. */
struct FStreamOutElement
{
	/** Index of the output stream from the geometry shader. */
	uint32 Stream;

	/** Semantic name of the output element as defined in the geometry shader.  This should not contain the semantic number. */
	const char* SemanticName;

	/** Semantic index of the output element as defined in the geometry shader.  For example "TEXCOORD5" in the shader would give a SemanticIndex of 5. */
	uint32 SemanticIndex;

	/** Start component index of the shader output element to stream out. */
	uint8 StartComponent;

	/** Number of components of the shader output element to stream out. */
	uint8 ComponentCount;

	/** Stream output target slot, corresponding to the streams set by RHISetStreamOutTargets. */
	uint8 OutputSlot;

	FStreamOutElement() {}
	FStreamOutElement(uint32 InStream, const char* InSemanticName, uint32 InSemanticIndex, uint8 InComponentCount, uint8 InOutputSlot) :
		Stream(InStream),
		SemanticName(InSemanticName),
		SemanticIndex(InSemanticIndex),
		StartComponent(0),
		ComponentCount(InComponentCount),
		OutputSlot(InOutputSlot)
	{}
};

typedef std::vector<FStreamOutElement> FStreamOutElementList;

enum EShaderFrequency
{
	SF_Vertex = 0,
	SF_Hull = 1,
	SF_Domain = 2,
	SF_Pixel = 3,
	SF_Geometry = 4,
	SF_Compute = 5,

	SF_NumFrequencies = 6,

	SF_NumBits = 3,
};
/** The base type of a value in a uniform buffer. */
enum EUniformBufferBaseType
{
	UBMT_INVALID,
	UBMT_BOOL,
	UBMT_INT32,
	UBMT_UINT32,
	UBMT_FLOAT32,
	UBMT_STRUCT,
	UBMT_SRV,
	UBMT_UAV,
	UBMT_SAMPLER,
	UBMT_TEXTURE,

	EUniformBufferBaseType_Num,
	EUniformBufferBaseType_NumBits = 4,
};

struct UniformBufferInfo
{
	std::string ConstantBufferName;
	std::vector<std::string> SRVNames;
	std::vector<std::string> SamplerNames;
	std::vector<std::string> UAVNames;
};
struct UniformBufferInfoCompare
{
	bool operator()(const UniformBufferInfo& lhs, const UniformBufferInfo& rhs)
	{
		return lhs.ConstantBufferName < rhs.ConstantBufferName;
	}
};
typedef std::set<UniformBufferInfo, UniformBufferInfoCompare> UniformBufferInfoSet;
inline UniformBufferInfoSet& GetUniformBufferInfoList()
{
	static UniformBufferInfoSet GUniformInfoList;
	return GUniformInfoList;
}

template<typename UniformParameters>
inline void ConstructUniformBufferInfo(const UniformParameters& Param)
{
	UniformBufferInfo UBInfo;
	UBInfo.ConstantBufferName = UniformParameters::GetConstantBufferName();
	for (auto it : UniformParameters::GetSRVs(Param))
	{
		UBInfo.SRVNames.push_back(it.first);
	}
	for (auto it : UniformParameters::GetSamplers(Param))
	{
		UBInfo.SamplerNames.push_back(it.first);
	}
	for (auto it : UniformParameters::GetUAVs(Param))
	{
		UBInfo.UAVNames.push_back(it.first);
	}
	GetUniformBufferInfoList().insert(std::move(UBInfo)) ;
}

struct FUniformBuffer
{
	~FUniformBuffer()
	{
		if (ConstantBuffer)
		{
			ConstantBuffer->Release();
		}
	}
	std::string ConstantBufferName;
	ID3D11Buffer* ConstantBuffer = NULL;
	std::map<std::string, ID3D11ShaderResourceView*> SRVs;
	std::map<std::string, ID3D11SamplerState*> Samplers;
	std::map<std::string, ID3D11UnorderedAccessView*> UAVs;
};

std::shared_ptr<FUniformBuffer> RHICreateUniformBuffer(
	UINT Size,
	const void* Contents, 
	std::map<std::string, ID3D11ShaderResourceView*>& SRVs,
	std::map<std::string, ID3D11SamplerState*>& Samplers,
	std::map<std::string, ID3D11UnorderedAccessView*>& UAVs
);

template<typename TBufferStruct>
class TUniformBuffer;

template<typename TBufferStruct>
class TUniformBufferPtr : public std::shared_ptr<FUniformBuffer>
{
public:
	TUniformBufferPtr()
	{}

	/** Initializes the reference to point to a buffer. */
	TUniformBufferPtr(const TUniformBuffer<TBufferStruct>& InBuffer)
		: std::shared_ptr<FUniformBuffer>(InBuffer.GetUniformBufferRHI())
	{}

	static TUniformBufferPtr<TBufferStruct> CreateUniformBufferImmediate(const TBufferStruct& Value)
	{
		TUniformBufferPtr<TBufferStruct> Result(std::make_shared<FUniformBuffer>());
		Result->ConstantBuffer = CreateConstantBuffer(false,sizeof(TBufferStruct::ConstantStruct), &Value.Constants);
		Result->ConstantBufferName = TBufferStruct::GetConstantBufferName();
		Result->SRVs = TBufferStruct::GetSRVs(Value);
		Result->Samplers = TBufferStruct::GetSamplers(Value);
		Result->UAVs = TBufferStruct::GetUAVs(Value);
		return Result;
	}
private:
	TUniformBufferPtr(std::shared_ptr<FUniformBuffer> InRHIRef)
		: std::shared_ptr<FUniformBuffer>(InRHIRef)
	{}
};
#define UNIFORM_BUFFER_STRUCT_ALIGNMENT 16
template<typename TBufferStruct>
class TUniformBuffer
{
public:
	TUniformBuffer()
		: Contents(nullptr)
	{
	}

	~TUniformBuffer()
	{
		if (Contents)
		{
			_aligned_free(Contents);
		}
	}

	/** Sets the contents of the uniform buffer. */
	void SetContents(const TBufferStruct& NewContents)
	{
		SetContentsNoUpdate(NewContents);
		ReleaseDynamicRHI();
		InitDynamicRHI();
	}
	/** Sets the contents of the uniform buffer to all zeros. */
	void SetContentsToZero()
	{
		if (!Contents)
		{
			Contents = (uint8*)_aligned_malloc(sizeof(TBufferStruct), UNIFORM_BUFFER_STRUCT_ALIGNMENT);
		}
		memset(Contents,0, sizeof(TBufferStruct));
		ReleaseDynamicRHI();
		InitDynamicRHI();
	}
	// FRenderResource interface.
	void InitDynamicRHI()
	{
		UniformBufferRHI.reset();
		if (Contents)
		{
			UniformBufferRHI = RHICreateUniformBuffer(sizeof(TBufferStruct), Contents, TBufferStruct::GetSRVs(*(TBufferStruct*)Contents), TBufferStruct::GetSamplers(*(TBufferStruct*)Contents), TBufferStruct::GetUAVs(*(TBufferStruct*)Contents));
		}
	}
	void ReleaseDynamicRHI()
	{
		UniformBufferRHI.reset();
	}
	// Accessors.
	std::shared_ptr<FUniformBuffer> GetUniformBufferRHI() const
	{
		assert(UniformBufferRHI); // you are trying to use a UB that was never filled with anything
		return UniformBufferRHI;
	}
protected:
	/** Sets the contents of the uniform buffer. Used within calls to InitDynamicRHI */
	void SetContentsNoUpdate(const TBufferStruct& NewContents)
	{
		if (!Contents)
		{
			Contents = (uint8*)_aligned_malloc(sizeof(TBufferStruct), UNIFORM_BUFFER_STRUCT_ALIGNMENT);
		}
		memcpy(Contents, &NewContents, sizeof(TBufferStruct));
	}
private:
	std::shared_ptr<FUniformBuffer> UniformBufferRHI;
	uint8* Contents;
};

void SetRenderTarget(FD3D11Texture* NewRenderTarget, FD3D11Texture* NewDepthStencilTarget, int32 MipIndex=0, int32 ArraySliceIndex=0);
void SetRenderTarget(FD3D11Texture* NewRenderTarget, FD3D11Texture* NewDepthStencilTarget, bool bClearColor = false, bool bClearDepth = false, bool bClearStencil = false, int32 MipIndex = 0, int32 ArraySliceIndex = 0);
void SetRenderTargets(uint32 NumRTV, FD3D11Texture** NewRenderTarget, FD3D11Texture* NewDepthStencilTarget, bool bClearColor = false, bool bClearDepth = false, bool bClearStencil = false);
void SetRenderTargetAndClear(FD3D11Texture* NewRenderTarget, FD3D11Texture* NewDepthStencilTarget);

void ClearRenderState();
enum ECubeFace
{
	CubeFace_PosX = 0,
	CubeFace_NegX,
	CubeFace_PosY,
	CubeFace_NegY,
	CubeFace_PosZ,
	CubeFace_NegZ,
	CubeFace_MAX
};

class FD3D11ConstantBuffer
{
public:
	FD3D11ConstantBuffer(uint32 InSize = 0, uint32 SubBuffers = 1);
	virtual ~FD3D11ConstantBuffer();

	void	InitDynamicRHI();
	void	ReleaseDynamicRHI();

	void UpdateConstant(const uint8* Data, uint16 Offset, uint16 InSize)
	{
		// Check that the data we are shadowing fits in the allocated shadow data
		assert((uint32)Offset + (uint32)InSize <= MaxSize);
		memcpy(ShadowData + Offset, Data, InSize);
		CurrentUpdateSize = FMath::Max((uint32)(Offset + InSize), CurrentUpdateSize);
	}
	bool CommitConstantsToDevice(bool bDiscardSharedConstants);

	ID3D11Buffer* GetConstantBuffer() const
	{
		return ConstantBufferRHI.Get();
	}
protected:
	ComPtr<ID3D11Buffer> ConstantBufferRHI;
	uint32	MaxSize;
	uint8*	ShadowData;
	uint32	CurrentUpdateSize;
	uint32	TotalUpdateSize;
};


void InitConstantBuffers();
void CommitNonComputeShaderConstants();

void RHISetShaderParameter(ID3D11VertexShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue);
void RHISetShaderParameter(ID3D11HullShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue);
void RHISetShaderParameter(ID3D11DomainShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue);
void RHISetShaderParameter(ID3D11PixelShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue);
void RHISetShaderParameter(ID3D11GeometryShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue);

template<typename ShaderRHIParamRef, class ParameterType>
void SetShaderValue(
	ShaderRHIParamRef Shader,
	const FShaderParameter& Parameter,
	const ParameterType& Value,
	uint32 ElementIndex = 0
)
{
	//static_assert(!TIsPointer<ParameterType>::Value, "Passing by value is not valid.");

	const uint32 AlignedTypeSize = Align(sizeof(ParameterType), 16);
	const int32 NumBytesToSet = FMath::Min<int32>(sizeof(ParameterType), Parameter.GetNumBytes() - ElementIndex * AlignedTypeSize);

	// This will trigger if the parameter was not serialized
	assert(Parameter.IsInitialized());

	if (NumBytesToSet > 0)
	{
		RHISetShaderParameter(
			Shader,
			Parameter.GetBufferIndex(),
			Parameter.GetBaseIndex() + ElementIndex * AlignedTypeSize,
			(uint32)NumBytesToSet,
			&Value
		);
	}
}
template<typename ShaderRHIParamRef>
void SetShaderValue(
	ShaderRHIParamRef Shader,
	const FShaderParameter& Parameter,
	const bool& Value,
	uint32 ElementIndex = 0
)
{
	const uint32 BoolValue = Value;
	SetShaderValue(Shader, Parameter, BoolValue, ElementIndex);
}
extern ComPtr<ID3D11Buffer> BoundUniformBuffers[6][14];

inline void SetShaderUniformBuffer(ID3D11VertexShader*, uint32 BaseIndex, ID3D11Buffer* ConstantBuffer)
{
	BoundUniformBuffers[0][BaseIndex] = ConstantBuffer;
	D3D11DeviceContext->VSSetConstantBuffers(BaseIndex, 1,&ConstantBuffer);
}
inline void SetShaderUniformBuffer(ID3D11HullShader*, uint32 BaseIndex, ID3D11Buffer* ConstantBuffer)
{
	BoundUniformBuffers[1][BaseIndex] = ConstantBuffer;
	D3D11DeviceContext->HSSetConstantBuffers(BaseIndex, 1, &ConstantBuffer);
}
inline void SetShaderUniformBuffer(ID3D11DomainShader*, uint32 BaseIndex, ID3D11Buffer* ConstantBuffer)
{
	BoundUniformBuffers[2][BaseIndex] = ConstantBuffer;
	D3D11DeviceContext->DSSetConstantBuffers(BaseIndex, 1, &ConstantBuffer);
}
inline void SetShaderUniformBuffer(ID3D11PixelShader*, uint32 BaseIndex, ID3D11Buffer* ConstantBuffer)
{
	BoundUniformBuffers[3][BaseIndex] = ConstantBuffer;
	D3D11DeviceContext->PSSetConstantBuffers(BaseIndex, 1, &ConstantBuffer);
}
inline void SetShaderUniformBuffer(ID3D11GeometryShader*, uint32 BaseIndex, ID3D11Buffer* ConstantBuffer)
{
	BoundUniformBuffers[4][BaseIndex] = ConstantBuffer;
	D3D11DeviceContext->GSSetConstantBuffers(BaseIndex, 1, &ConstantBuffer);
}
inline void SetShaderUniformBuffer(ID3D11ComputeShader*, uint32 BaseIndex, ID3D11Buffer* ConstantBuffer)
{
	BoundUniformBuffers[5][BaseIndex] = ConstantBuffer;
	D3D11DeviceContext->CSSetConstantBuffers(BaseIndex, 1, &ConstantBuffer);
}
template <EShaderFrequency ShaderFrequency>
void InternalSetShaderResourceView(ID3D11ShaderResourceView* SRV, int32 ResourceIndex);
//srv
inline void SetShaderSRV(ID3D11VertexShader*, uint32 BaseIndex, ID3D11ShaderResourceView* SRV)
{
	InternalSetShaderResourceView<SF_Vertex>(SRV, BaseIndex);
}
inline void SetShaderSRV(ID3D11PixelShader*, uint32 BaseIndex, ID3D11ShaderResourceView* SRV)
{
	InternalSetShaderResourceView<SF_Pixel>(SRV, BaseIndex);
}
inline void SetShaderSRV(ID3D11HullShader*, uint32 BaseIndex, ID3D11ShaderResourceView* SRV)
{
	InternalSetShaderResourceView<SF_Hull>(SRV, BaseIndex);
}
inline void SetShaderSRV(ID3D11DomainShader*, uint32 BaseIndex, ID3D11ShaderResourceView* SRV)
{
	InternalSetShaderResourceView<SF_Domain>(SRV, BaseIndex);
}
inline void SetShaderSRV(ID3D11GeometryShader*, uint32 BaseIndex, ID3D11ShaderResourceView* SRV)
{
	InternalSetShaderResourceView<SF_Geometry>(SRV, BaseIndex);
}
inline void SetShaderSRV(ID3D11ComputeShader*, uint32 BaseIndex, ID3D11ShaderResourceView* SRV)
{
	InternalSetShaderResourceView<SF_Compute>(SRV, BaseIndex);
}
//sampler
inline void SetShaderSampler(ID3D11VertexShader*, uint32 BaseIndex, ID3D11SamplerState* Sampler)
{
	D3D11DeviceContext->VSSetSamplers(BaseIndex, 1, &Sampler);
}
inline void SetShaderSampler(ID3D11PixelShader*, uint32 BaseIndex, ID3D11SamplerState* Sampler)
{
	D3D11DeviceContext->PSSetSamplers(BaseIndex, 1, &Sampler);
}
inline void SetShaderSampler(ID3D11HullShader*, uint32 BaseIndex, ID3D11SamplerState* Sampler)
{
	D3D11DeviceContext->HSSetSamplers(BaseIndex, 1, &Sampler);
}
inline void SetShaderSampler(ID3D11DomainShader*, uint32 BaseIndex, ID3D11SamplerState* Sampler)
{
	D3D11DeviceContext->DSSetSamplers(BaseIndex, 1, &Sampler);
}
inline void SetShaderSampler(ID3D11GeometryShader*, uint32 BaseIndex, ID3D11SamplerState* Sampler)
{
	D3D11DeviceContext->GSSetSamplers(BaseIndex, 1, &Sampler);
}
inline void SetShaderSampler(ID3D11ComputeShader*, uint32 BaseIndex, ID3D11SamplerState* Sampler)
{
	D3D11DeviceContext->CSSetSamplers(BaseIndex, 1, &Sampler);
}
//UAV
inline void SetShaderUAV(ID3D11VertexShader*, uint32 BaseIndex, ID3D11UnorderedAccessView* UAV)
{
	assert(false);
}
inline void SetShaderUAV(ID3D11PixelShader*, uint32 BaseIndex, ID3D11UnorderedAccessView* UAV)
{
	assert(false);
}
inline void SetShaderUAV(ID3D11HullShader*, uint32 BaseIndex, ID3D11UnorderedAccessView* UAV)
{
	assert(false);
}
inline void SetShaderUAV(ID3D11DomainShader*, uint32 BaseIndex, ID3D11UnorderedAccessView* UAV)
{
	assert(false);
}
inline void SetShaderUAV(ID3D11GeometryShader*, uint32 BaseIndex, ID3D11UnorderedAccessView* UAV)
{
	assert(false);
}
inline void SetShaderUAV(ID3D11ComputeShader*, uint32 BaseIndex, ID3D11UnorderedAccessView* UAV)
{
	//D3D11DeviceContext->CSSetUnorderedAccessViews(BaseIndex, 1, &UAV);
}
/** Sets the value of a shader uniform buffer parameter to a uniform buffer containing the struct. */
template<typename TShaderRHIRef>
inline void SetUniformBufferParameter(
	TShaderRHIRef Shader,
	const FShaderUniformBufferParameter& Parameter,
	FUniformBuffer* UniformBufferRHI
)
{
	// This will trigger if the parameter was not serialized
	assert(Parameter.IsInitialized());
	// If it is bound, we must set it so something valid
	assert(!Parameter.IsBound() || UniformBufferRHI);
	if (Parameter.IsBound())
	{
		SetShaderUniformBuffer(Shader, Parameter.GetBaseIndex(), UniformBufferRHI->ConstantBuffer);
	}
	for (auto& Pair : Parameter.GetSRVs())
	{
		SetShaderSRV(Shader, Pair.second, UniformBufferRHI->SRVs[Pair.first]);
	}
	for (auto& Pair : Parameter.GetSamplers())
	{
		SetShaderSampler(Shader, Pair.second, UniformBufferRHI->Samplers[Pair.first]);
	}
	for (auto& Pair : Parameter.GetUAVs())
	{
		SetShaderUAV(Shader, Pair.second, UniformBufferRHI->UAVs[Pair.first]);
	}
}

/** Sets the value of a shader uniform buffer parameter to a uniform buffer containing the struct. */
template<typename TShaderRHIRef, typename TBufferStruct>
inline void SetUniformBufferParameter(
	TShaderRHIRef Shader,
	const TShaderUniformBufferParameter<TBufferStruct>& Parameter,
	const TUniformBufferPtr<TBufferStruct>& UniformBufferRef
)
{
	// This will trigger if the parameter was not serialized
	assert(Parameter.IsInitialized());
	// If it is bound, we must set it so something valid
	assert(!Parameter.IsBound() || UniformBufferRef);
	if (Parameter.IsBound())
	{
		SetShaderUniformBuffer(Shader, Parameter.GetBaseIndex(), UniformBufferRef->ConstantBuffer);
	}
	for (auto& Pair : Parameter.GetSRVs())
	{
		SetShaderSRV(Shader, Pair.second, UniformBufferRef->SRVs[Pair.first]);
	}
	for (auto& Pair : Parameter.GetSamplers())
	{
		SetShaderSampler(Shader, Pair.second, UniformBufferRef->Samplers[Pair.first]);
	}
	for (auto& Pair : Parameter.GetUAVs())
	{
		SetShaderUAV(Shader, Pair.second, UniformBufferRef->UAVs[Pair.first]);
	}
}

/** Sets the value of a shader uniform buffer parameter to a uniform buffer containing the struct. */
template<typename TShaderRHIRef, typename TBufferStruct>
inline void SetUniformBufferParameter(
	TShaderRHIRef Shader,
	const TShaderUniformBufferParameter<TBufferStruct>& Parameter,
	const TUniformBuffer<TBufferStruct>& UniformBuffer
)
{
	// This will trigger if the parameter was not serialized
	assert(Parameter.IsInitialized());
	// If it is bound, we must set it so something valid
	assert(!Parameter.IsBound() || UniformBuffer.GetUniformBufferRHI());
	std::shared_ptr<FUniformBuffer> UniformBufferRHI = UniformBuffer.GetUniformBufferRHI();
	if (Parameter.IsBound())
	{
		SetShaderUniformBuffer(Shader, Parameter.GetBaseIndex(), UniformBuffer.GetUniformBufferRHI()->ConstantBuffer);
	}
	for (auto& Pair : Parameter.GetSRVs())
	{
		SetShaderSRV(Shader, Pair.second, UniformBuffer.GetUniformBufferRHI()->SRVs[Pair.first]);
	}
	for (auto& Pair : Parameter.GetSamplers())
	{
		SetShaderSampler(Shader, Pair.second, UniformBuffer.GetUniformBufferRHI()->Samplers[Pair.first]);
	}
	for (auto& Pair : Parameter.GetUAVs())
	{
		SetShaderUAV(Shader, Pair.second, UniformBuffer.GetUniformBufferRHI()->UAVs[Pair.first]);
	}
}
/** Sets the value of a shader uniform buffer parameter to a value of the struct. */
template<typename TShaderRHIRef, typename TBufferStruct>
inline void SetUniformBufferParameterImmediate(
	TShaderRHIRef Shader,
	const TShaderUniformBufferParameter<TBufferStruct>& Parameter,
	const TBufferStruct& UniformBufferValue
)
{
	// This will trigger if the parameter was not serialized
	assert(Parameter.IsInitialized());
	if (Parameter.IsBound())
	{
		SetShaderUniformBuffer(
			Shader,
			Parameter.GetBaseIndex(),
			CreateConstantBuffer(false, sizeof(TBufferStruct::Constants), &UniformBufferValue)
		);
	}
}
/**
* Sets the value of a shader resource view parameter
* Template'd on shader type (e.g. pixel shader or compute shader).
*/
template<typename ShaderTypeRHIParamRef>
inline void SetSRVParameter(
	ShaderTypeRHIParamRef Shader,
	const FShaderResourceParameter& Parameter,
	ID3D11ShaderResourceView* NewShaderResourceViewRHI
)
{
	if (Parameter.IsBound())
	{
		SetShaderSRV( Shader, Parameter.GetBaseIndex(), NewShaderResourceViewRHI );
	}
}
/**
* Sets the value of a shader sampler parameter. Template'd on shader type.
*/
template<typename ShaderTypeRHIParamRef>
inline void SetSamplerParameter(
	ShaderTypeRHIParamRef Shader,
	const FShaderResourceParameter& Parameter,
	ID3D11SamplerState* SamplerStateRHI
)
{
	if (Parameter.IsBound())
	{
		SetShaderSampler(
			Shader,
			Parameter.GetBaseIndex(),
			SamplerStateRHI
		);
	}
}

template<typename ShaderRHIParamRef, class ParameterType>
void SetShaderValueArray(
	ShaderRHIParamRef Shader,
	const FShaderParameter& Parameter,
	const ParameterType* Values,
	uint32 NumElements,
	uint32 BaseElementIndex = 0
)
{
	const uint32 AlignedTypeSize = Align(sizeof(ParameterType), 16);
	const int32 NumBytesToSet = FMath::Min<int32>(NumElements * AlignedTypeSize, Parameter.GetNumBytes() - BaseElementIndex * AlignedTypeSize);

	// This will trigger if the parameter was not serialized
	assert(Parameter.IsInitialized());

	if (NumBytesToSet > 0)
	{
		RHISetShaderParameter(
			Shader,
			Parameter.GetBufferIndex(),
			Parameter.GetBaseIndex() + BaseElementIndex * AlignedTypeSize,
			(uint32)NumBytesToSet,
			Values
		);
	}
}

/**
* Sets the value of a shader texture parameter. Template'd on shader type.
*/
template<typename ShaderTypeRHIParamRef>
inline void SetTextureParameter(
	ShaderTypeRHIParamRef Shader,
	const FShaderResourceParameter& TextureParameter,
	const FShaderResourceParameter& SamplerParameter,
	ID3D11SamplerState* SamplerStateRHI,
	ID3D11ShaderResourceView* TextureRHI,
	uint32 ElementIndex = 0
)
{
	// This will trigger if the parameter was not serialized
	assert(TextureParameter.IsInitialized());
	assert(SamplerParameter.IsInitialized());
	if (TextureParameter.IsBound())
	{
		if (ElementIndex < TextureParameter.GetNumResources())
		{
			SetShaderSRV(Shader, TextureParameter.GetBaseIndex() + ElementIndex, TextureRHI);
			//RHICmdList.SetShaderTexture(Shader, TextureParameter.GetBaseIndex() + ElementIndex, TextureRHI);
		}
	}
	// @todo ue4 samplerstate Should we maybe pass in two separate values? SamplerElement and TextureElement? Or never allow an array of samplers? Unsure best
	// if there is a matching sampler for this texture array index (ElementIndex), then set it. This will help with this case:
	//			Texture2D LightMapTextures[NUM_LIGHTMAP_COEFFICIENTS];
	//			SamplerState LightMapTexturesSampler;
	// In this case, we only set LightMapTexturesSampler when ElementIndex is 0, we don't set the sampler state for all 4 textures
	// This assumes that the all textures want to use the same sampler state
	if (SamplerParameter.IsBound())
	{
		if (ElementIndex < SamplerParameter.GetNumResources())
		{
			SetShaderSampler(Shader, SamplerParameter.GetBaseIndex() + ElementIndex, SamplerStateRHI);
		}
	}
}

/**
* Sets the value of a shader surface parameter (e.g. to access MSAA samples).
* Template'd on shader type (e.g. pixel shader or compute shader).
*/
template<typename ShaderTypeRHIParamRef>
inline void SetTextureParameter(
	ShaderTypeRHIParamRef Shader,
	const FShaderResourceParameter& Parameter,
	ID3D11ShaderResourceView* NewTextureRHI
)
{
	if (Parameter.IsBound())
	{
		SetShaderSRV(Shader, Parameter.GetBaseIndex(), NewTextureRHI);
	}
}

struct FBoundShaderStateInput
{
	std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>> VertexDeclarationRHI;
	ID3DBlob* VSCode;
	ID3D11VertexShader* VertexShaderRHI;
	ID3D11HullShader* HullShaderRHI;
	ID3D11DomainShader* DomainShaderRHI;
	ID3D11PixelShader* PixelShaderRHI;
	ID3D11GeometryShader* GeometryShaderRHI;

	inline FBoundShaderStateInput()
		: VertexDeclarationRHI(nullptr)
		, VSCode(nullptr)
		, VertexShaderRHI(nullptr)
		, HullShaderRHI(nullptr)
		, DomainShaderRHI(nullptr)
		, PixelShaderRHI(nullptr)
		, GeometryShaderRHI(nullptr)
	{
	}

	inline FBoundShaderStateInput(
		std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>> InVertexDeclarationRHI,
		ID3DBlob* InVSCode,
		ID3D11VertexShader* InVertexShaderRHI,
		ID3D11HullShader* InHullShaderRHI,
		ID3D11DomainShader* InDomainShaderRHI,
		ID3D11PixelShader* InPixelShaderRHI,
		ID3D11GeometryShader* InGeometryShaderRHI
	)
		: VertexDeclarationRHI(InVertexDeclarationRHI)
		, VSCode(InVSCode)
		, VertexShaderRHI(InVertexShaderRHI)
		, HullShaderRHI(InHullShaderRHI)
		, DomainShaderRHI(InDomainShaderRHI)
		, PixelShaderRHI(InPixelShaderRHI)
		, GeometryShaderRHI(InGeometryShaderRHI)
	{
	}
};
std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>>& GetVertexDeclarationFVector4();
std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>>& GetScreenVertexDeclaration();

extern ID3D11InputLayout* GetInputLayout(const std::vector<D3D11_INPUT_ELEMENT_DESC>* InputDecl, ID3DBlob* VSCode);

void DrawRectangle(
	float X,
	float Y,
	float SizeX,
	float SizeY,
	float U,
	float V,
	float SizeU,
	float SizeV,
	FIntPoint TargetSize,
	FIntPoint TextureSize,
	class FShader* VertexShader,
	uint32 InstanceCount = 1
);

extern int32 GMaxShadowDepthBufferSizeX; 
extern int32 GMaxShadowDepthBufferSizeY;

void RHISetViewport(uint32 MinX, uint32 MinY, float MinZ, uint32 MaxX, uint32 MaxY, float MaxZ);
void RHISetScissorRect(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY);

extern void DrawClearQuadMRT(bool bClearColor, int32 NumClearColors, const FLinearColor* ClearColorArray, bool bClearDepth, float Depth, bool bClearStencil, uint32 Stencil);
extern void DrawClearQuadMRT(bool bClearColor, int32 NumClearColors, const FLinearColor* ClearColorArray, bool bClearDepth, float Depth, bool bClearStencil, uint32 Stencil, FIntPoint ViewSize, FIntRect ExcludeRect);

inline void DrawClearQuad(bool bClearColor, const FLinearColor& Color, bool bClearDepth, float Depth, bool bClearStencil, uint32 Stencil)
{
	DrawClearQuadMRT(bClearColor, 1, &Color, bClearDepth, Depth, bClearStencil, Stencil);
}

inline void DrawClearQuad(bool bClearColor, const FLinearColor& Color, bool bClearDepth, float Depth, bool bClearStencil, uint32 Stencil, FIntPoint ViewSize, FIntRect ExcludeRect)
{
	DrawClearQuadMRT(bClearColor, 1, &Color, bClearDepth, Depth, bClearStencil, Stencil, ViewSize, ExcludeRect);
}

inline void DrawClearQuad(const FLinearColor& Color)
{
	DrawClearQuadMRT(true, 1, &Color, false, 0, false, 0);
}

void DrawIndexedPrimitiveUP(
	D3D11_PRIMITIVE_TOPOLOGY PrimitiveType,
	uint32 MinVertexIndex,
	uint32 NumVertices,
	uint32 NumPrimitives,
	const void* IndexData,
	uint32 IndexDataStride,
	const void* VertexData,
	uint32 VertexDataStride);

inline uint32 GetD3D11CubeFace(ECubeFace Face)
{
	switch (Face)
	{
	case CubeFace_PosX:
	default:
		return 0;//D3DCUBEMAP_FACE_POSITIVE_X;
	case CubeFace_NegX:
		return 1;//D3DCUBEMAP_FACE_NEGATIVE_X;
	case CubeFace_PosY:
		return 2;//D3DCUBEMAP_FACE_POSITIVE_Y;
	case CubeFace_NegY:
		return 3;//D3DCUBEMAP_FACE_NEGATIVE_Y;
	case CubeFace_PosZ:
		return 4;//D3DCUBEMAP_FACE_POSITIVE_Z;
	case CubeFace_NegZ:
		return 5;//D3DCUBEMAP_FACE_NEGATIVE_Z;
	};
}
void RHIReadSurfaceFloatData(FD3D11Texture* TextureRHI, FIntRect InRect, std::vector<FFloat16Color>& OutData, ECubeFace CubeFace, int32 ArrayIndex, int32 MipIndex);

struct FResolveRect
{
	int32 X1;
	int32 Y1;
	int32 X2;
	int32 Y2;
	// e.g. for a a full 256 x 256 area starting at (0, 0) it would be 
	// the values would be 0, 0, 256, 256
	inline FResolveRect(int32 InX1 = -1, int32 InY1 = -1, int32 InX2 = -1, int32 InY2 = -1)
		: X1(InX1)
		, Y1(InY1)
		, X2(InX2)
		, Y2(InY2)
	{}

	inline FResolveRect(const FResolveRect& Other)
		: X1(Other.X1)
		, Y1(Other.Y1)
		, X2(Other.X2)
		, Y2(Other.Y2)
	{}

	bool IsValid() const
	{
		return X1 >= 0 && Y1 >= 0 && X2 - X1 > 0 && Y2 - Y1 > 0;
	}
};

struct FResolveParams
{
	/** used to specify face when resolving to a cube map texture */
	ECubeFace CubeFace;
	/** resolve RECT bounded by [X1,Y1]..[X2,Y2]. Or -1 for fullscreen */
	FResolveRect Rect;
	FResolveRect DestRect;
	/** The mip index to resolve in both source and dest. */
	int32 MipIndex;
	/** Array index to resolve in the source. */
	int32 SourceArrayIndex;
	/** Array index to resolve in the dest. */
	int32 DestArrayIndex;

	/** constructor */
	FResolveParams(
		const FResolveRect& InRect = FResolveRect(),
		ECubeFace InCubeFace = CubeFace_PosX,
		int32 InMipIndex = 0,
		int32 InSourceArrayIndex = 0,
		int32 InDestArrayIndex = 0,
		const FResolveRect& InDestRect = FResolveRect())
		: CubeFace(InCubeFace)
		, Rect(InRect)
		, DestRect(InDestRect)
		, MipIndex(InMipIndex)
		, SourceArrayIndex(InSourceArrayIndex)
		, DestArrayIndex(InDestArrayIndex)
	{}

	inline FResolveParams(const FResolveParams& Other)
		: CubeFace(Other.CubeFace)
		, Rect(Other.Rect)
		, DestRect(Other.DestRect)
		, MipIndex(Other.MipIndex)
		, SourceArrayIndex(Other.SourceArrayIndex)
		, DestArrayIndex(Other.DestArrayIndex)
	{}
};


void RHICopyToResolveTarget(FD3D11Texture* SourceTextureRHI, FD3D11Texture* DestTextureRHI, const FResolveParams& ResolveParams);
struct FRHICopyTextureInfo
{
	// Number of texels to copy. Z Must be always be > 0.
	FIntVector Size = { 0, 0, 1 };

	/** The mip index to copy in both source and dest. */
	int32 MipIndex = 0;

	/** Array index or cube face to resolve in the source. For Cubemap arrays this would be ArraySlice * 6 + FaceIndex. */
	int32 SourceArraySlice = 0;
	/** Array index or cube face to resolve in the dest. */
	int32 DestArraySlice = 0;
	// How many slices or faces to copy.
	int32 NumArraySlices = 1;

	// 2D copy
	FRHICopyTextureInfo(int32 InWidth, int32 InHeight)
		: Size(InWidth, InHeight, 1)
	{
	}

	FRHICopyTextureInfo(const FIntVector& InSize)
		: Size(InSize)
	{
	}

	void AdvanceMip()
	{
		++MipIndex;
		Size.X = FMath::Max(Size.X / 2, 1);
		Size.Y = FMath::Max(Size.Y / 2, 1);
	}
};
void RHICopyTexture(FD3D11Texture* SourceTexture, FD3D11Texture* DestTexture, const FRHICopyTextureInfo& CopyInfo);

typedef uint16 FBoneIndexType;
void RHIUpdateBoneBuffer(ID3D11Buffer* InVertexBuffer, uint32 InBufferSize, const std::vector<FMatrix>& InReferenceToLocalMatrices, const std::vector<FBoneIndexType>& InBoneMap);

extern float GProjectionSignY;
extern EPixelFormat GRHIHDRDisplayOutputFormat;
extern ID3D11ShaderResourceView* HighFrequencyNoiseTexture;