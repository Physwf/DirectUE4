#pragma once

#include <dxgi.h>
#include <d3d11.h>
#include <wrl/client.h>

#include <map>
#include <string>
#include <vector>
#include <assert.h>

using namespace Microsoft::WRL;

extern IDXGIFactory*	DXGIFactory;
extern IDXGIAdapter*	DXGIAdapter;
extern IDXGIOutput*	DXGIOutput;
extern IDXGISwapChain* DXGISwapChain;

extern ID3D11Device*			D3D11Device;
extern ID3D11DeviceContext*	D3D11DeviceContext;

extern ID3D11Texture2D* BackBuffer;
extern ID3D11RenderTargetView* BackBufferRTV;
extern D3D11_VIEWPORT GViewport;

extern LONG WindowWidth;
extern LONG WindowHeight;

bool InitRHI();
void D3D11Present();

struct ParameterAllocation
{
	UINT BufferIndex;
	UINT BaseIndex;
	UINT Size;
};

ID3D11Buffer* CreateVertexBuffer(bool bDynamic,unsigned int Size, void* Data = NULL);
ID3D11Buffer* CreateIndexBuffer(void* Data, unsigned int Size);
ID3D11Buffer* CreateConstantBuffer(bool bDynamic, unsigned int Size, void* Data = NULL);
ID3DBlob* CompileVertexShader(const wchar_t* File, const char* EntryPoint,const D3D_SHADER_MACRO* OtherMacros = NULL, int OtherMacrosCount = 0);
ID3DBlob* CompilePixelShader(const wchar_t* File, const char* EntryPoint,const D3D_SHADER_MACRO* OtherMacros = NULL, int OtherMacrosCount = 0);
void GetShaderParameterAllocations(ID3DBlob* Code,std::map<std::string, ParameterAllocation>& OutParams);
ID3D11VertexShader* CreateVertexShader(ID3DBlob* VSBytecode);
ID3D11PixelShader* CreatePixelShader(ID3DBlob* PSBytecode);
ID3D11InputLayout* CreateInputLayout(D3D11_INPUT_ELEMENT_DESC* InputDesc, unsigned int Count, ID3DBlob* VSBytecode);
ID3D11Texture2D* CreateTexture2D(unsigned int W, unsigned int H, DXGI_FORMAT Format, bool bRenderTarget, bool bShaderResource, bool bDepthStencil, UINT MipMapCount = 1);
ID3D11Texture2D* CreateTexture2D(unsigned int W, unsigned int H, DXGI_FORMAT Format, UINT MipMapCount, void* InitData);
ID3D11RenderTargetView* CreateRenderTargetView2D(ID3D11Texture2D* Resource, DXGI_FORMAT Format, UINT MipSlice);
ID3D11DepthStencilView* CreateDepthStencilView2D(ID3D11Texture2D* Resource, DXGI_FORMAT Format, UINT MipSlice);
ID3D11ShaderResourceView* CreateShaderResourceView2D(ID3D11Texture2D* Resource, DXGI_FORMAT Format, UINT MipLevels, UINT MostDetailedMip);
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
		LinearColor LC = FColor(BorderColor);
		Desc.BorderColor[0] = LC.R;
		Desc.BorderColor[1] = LC.G;
		Desc.BorderColor[2] = LC.B;
		Desc.BorderColor[3] = LC.A;
		Desc.ComparisonFunc = SamplerComparisonFunction;
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
	bool bEnableLineAA = false,
	bool bEnableMSAA = true
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
		Desc.SlopeScaledDepthBias = 0;
		Desc.AntialiasedLineEnable = bEnableLineAA;
		Desc.MultisampleEnable = bEnableMSAA;
		Desc.FrontCounterClockwise = TRUE;
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
	BOOL IndependentBlendEnable = FALSE,
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
class TStaticBlendStateWriteMask
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


