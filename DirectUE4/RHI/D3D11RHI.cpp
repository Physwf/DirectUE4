#include "DirectXTex.h"
#include "D3D11RHI.h"
#include "log.h"
#include <D3Dcompiler.h>
#include <d3d11shader.h>
#include <memory>
#include <string>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <iterator>

// #define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h"// #include "WICTextureLoader.h"
using namespace DirectX;

IDXGIFactory*	DXGIFactory = NULL;
IDXGIAdapter*	DXGIAdapter = NULL;
IDXGIOutput*	DXGIOutput = NULL;
IDXGISwapChain* DXGISwapChain = NULL;

ID3D11Device*			D3D11Device = NULL;
ID3D11DeviceContext*	D3D11DeviceContext = NULL;

ID3D11Texture2D* BackBuffer = NULL; 
D3D11_VIEWPORT GViewport;

LONG WindowWidth = 1920;
LONG WindowHeight = 1080;

bool InitRHI()
{
	//create DXGIFactory
	HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&DXGIFactory);
	if (FAILED(hr))
	{
		X_LOG("CreateDXGIFactory failed!");
		return FALSE;
	}
	//EnumAapter
	for (UINT AdpaterIndex = 0; SUCCEEDED(DXGIFactory->EnumAdapters(AdpaterIndex, &DXGIAdapter)); ++AdpaterIndex)
	{
		if (DXGIAdapter)
		{
			DXGI_ADAPTER_DESC DESC;
			if (SUCCEEDED(DXGIAdapter->GetDesc(&DESC)))
			{
				X_LOG("Adapter %d: Description:%ls  VendorId:%x, DeviceId:0x%x, SubSysId:0x%x Revision:%u DedicatedVideoMemory:%uM DedicatedSystemMemory:%uM SharedSystemMemory:%uM \n",
					AdpaterIndex,
					DESC.Description,
					DESC.VendorId,
					DESC.DeviceId,
					DESC.SubSysId,
					DESC.Revision,
					DESC.DedicatedVideoMemory / (1024 * 1024),
					DESC.DedicatedSystemMemory / (1024 * 1024),
					DESC.SharedSystemMemory / (1024 * 1024)
				/*DESC.AdapterLuid*/);
			}

			for (UINT OutputIndex = 0; SUCCEEDED(DXGIAdapter->EnumOutputs(OutputIndex, &DXGIOutput)); ++OutputIndex)
			{
				if (DXGIOutput)
				{
					DXGI_OUTPUT_DESC OutDesc;
					if (SUCCEEDED(DXGIOutput->GetDesc(&OutDesc)))
					{
						X_LOG("\tOutput:%u, DeviceName:%ls, Width:%u Height:%u \n", OutputIndex, OutDesc.DeviceName, OutDesc.DesktopCoordinates.right - OutDesc.DesktopCoordinates.left, OutDesc.DesktopCoordinates.bottom - OutDesc.DesktopCoordinates.top);
					}
				}
			}
			break;
		}
	}

	//Create D3D11Device
	if (DXGIAdapter)
	{
		D3D_FEATURE_LEVEL FeatureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1,
		};

		D3D_FEATURE_LEVEL OutFeatureLevel;
		hr = D3D11CreateDevice(
			DXGIAdapter,
			D3D_DRIVER_TYPE_UNKNOWN,
			NULL,
			D3D11_CREATE_DEVICE_DEBUG,
			FeatureLevels,
			6,
			D3D11_SDK_VERSION,
			&D3D11Device,
			&OutFeatureLevel,
			&D3D11DeviceContext
		);

		if (FAILED(hr))
		{
			X_LOG("D3D11CreateDevice failed!");
			return false;
		}

		UINT NumQualityLevels = 0;
		D3D11Device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM,4,&NumQualityLevels);

		extern HWND g_hWind;

		DXGI_SWAP_CHAIN_DESC SwapChainDesc;
		ZeroMemory(&SwapChainDesc, sizeof SwapChainDesc);
		SwapChainDesc.BufferCount = 1;
		SwapChainDesc.BufferDesc.Width = WindowWidth;
		SwapChainDesc.BufferDesc.Height = WindowHeight;
		SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
		SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.OutputWindow = g_hWind;
		SwapChainDesc.SampleDesc.Count = 1;
		SwapChainDesc.SampleDesc.Quality = 0;// NumQualityLevels - 1;
		SwapChainDesc.Windowed = TRUE;
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


		hr = DXGIFactory->CreateSwapChain(D3D11Device, &SwapChainDesc, &DXGISwapChain);
		if (FAILED(hr))
		{
			X_LOG("CreateSwapChain failed!");
			return false;
		}
	}

	hr = DXGISwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&BackBuffer);
	if (FAILED(hr))
	{
		X_LOG("GetBuffer failed!");
		return false;
	}

	GViewport.Width = (FLOAT)WindowWidth;
	GViewport.Height = (FLOAT)WindowHeight;
	GViewport.MinDepth = 0.0f;
	GViewport.MaxDepth = 1.0f;
	GViewport.TopLeftX = 0;
	GViewport.TopLeftY = 0;



	return true;
}

void D3D11Present()
{
	DXGISwapChain->Present(0, 0);
}

ID3D11Buffer* CreateVertexBuffer(bool bDynamic, unsigned int Size, void* Data /*= NULL*/)
{
	D3D11_BUFFER_DESC VertexDesc;
	ZeroMemory(&VertexDesc, sizeof(VertexDesc));
	VertexDesc.ByteWidth = Size;
	VertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA InitData;
	if (bDynamic)
	{
		VertexDesc.Usage = D3D11_USAGE_DYNAMIC;
		VertexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		VertexDesc.MiscFlags = 0;
	}
	else
	{
		VertexDesc.Usage = D3D11_USAGE_DEFAULT;
		VertexDesc.CPUAccessFlags = 0;
		VertexDesc.MiscFlags = 0;

		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = Data;
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;
	}

	ID3D11Buffer* Result;
	if (S_OK == D3D11Device->CreateBuffer(&VertexDesc, Data != NULL ? &InitData : NULL, &Result))
	{
		return Result;
	}
	X_LOG("CreateVertexBuffer failed!");
	return NULL;
}

ID3D11Buffer* CreateIndexBuffer(void* Data, unsigned int Size)
{
	D3D11_BUFFER_DESC IndexDesc;
	ZeroMemory(&IndexDesc, sizeof(IndexDesc));
	IndexDesc.Usage = D3D11_USAGE_DEFAULT;
	IndexDesc.ByteWidth = Size;
	IndexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	IndexDesc.CPUAccessFlags = 0;
	IndexDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA IndexData;
	ZeroMemory(&IndexData, sizeof(IndexData));
	IndexData.pSysMem = Data;
	IndexData.SysMemPitch = 0;
	IndexData.SysMemSlicePitch = 0;

	ID3D11Buffer* Result;
	if (S_OK == D3D11Device->CreateBuffer(&IndexDesc, &IndexData, &Result))
	{
		return Result;
	}
	X_LOG("CreateIndexBuffer failed!");
	return NULL;
}

ID3D11Buffer* CreateConstantBuffer(bool bDynamic, unsigned int Size, void* Data /*= NULL*/)
{
	D3D11_BUFFER_DESC Desc;
	ZeroMemory(&Desc, sizeof(D3D11_BUFFER_DESC));
	Desc.ByteWidth = Size;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	D3D11_SUBRESOURCE_DATA InitData;
	if (bDynamic)
	{
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		Desc.Usage = D3D11_USAGE_DEFAULT;

		InitData.pSysMem = Data;
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;
	}

	ID3D11Buffer* Result;
	if (S_OK == D3D11Device->CreateBuffer(&Desc, Data != NULL ?  &InitData : NULL , &Result))
	{
		return Result;
	}
	X_LOG("CreateConstantBuffer failed!");
	return NULL;
}

class ShaderIncludeHandler : public ID3DInclude
{
public:
	HRESULT __stdcall Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
	{
		std::string IncludeFile;
		switch (IncludeType)
		{
		case D3D_INCLUDE_LOCAL:
			IncludeFile = std::string("./Shaders/") + pFileName;
			break;
		case D3D_INCLUDE_SYSTEM:
			IncludeFile = std::string("./Shaders/") + pFileName;
			break;
		}
		X_LOG("filename:%s\n",IncludeFile.c_str());
// 		FILE* file;
// 		fopen_s(&file,IncludeFile.c_str(), "r");
// 		if (file)
// 		{
// 			fseek(file, 0, SEEK_END);
// 			size_t filesize = ftell(file);
// 			X_LOG("filesize:%d\n", filesize);
// 			fseek(file, 0, SEEK_SET);
// 			filedata = std::make_unique<char[]>(filesize);
// 			fread(filedata.get(), 1, filesize, file);
// 			fclose(file);
// 			X_LOG("content:%s\n", filedata.get());
//  			*ppData = filedata.get();
//  			*pBytes = filesize;
// 			return S_OK;
// 		}
		std::ifstream includeFile(IncludeFile.c_str(), std::ios::in | std::ios::binary | std::ios::ate);


		if (includeFile.is_open()) {
			unsigned int fileSize = (unsigned int)includeFile.tellg();
			char* buf = new char[fileSize];
			includeFile.seekg(0, std::ios::beg);
			includeFile.read(buf, fileSize);
			includeFile.close();
			*ppData = buf;
			*pBytes = fileSize;
			return S_OK;
		}
		else {
			return E_FAIL;
		}
// 		CString D(pFileName);
// 		D3DReadFileToBlob(D.GetBuffer(), &blob);
// 		D.ReleaseBuffer();
// 
// 		(*ppData) = blob->GetBufferPointer();
// 		(*pBytes) = blob->GetBufferSize();
		return E_FAIL;
	}
	HRESULT __stdcall Close(LPCVOID pData)
	{
		//blob->Release();
		char* buf = (char*)pData;
		delete[] buf;
		return S_OK;
	}
private:
	std::unique_ptr<char[]> filedata;
	//ID3DBlob* blob;
};
ID3DBlob* CompileVertexShader(const wchar_t* File, const char* EntryPoint, const D3D_SHADER_MACRO* OtherMacros/* = NULL*/, int OtherMacrosCount/* = 0*/)
{
	ID3DBlob* Bytecode;
	ID3DBlob* OutErrorMsg;
	LPCSTR VSTarget = D3D11Device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0 ? "vs_5_0" : "vs_4_0";
	UINT VSFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;//*| D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY*/;
	ShaderIncludeHandler IncludeHandler;
	D3D_SHADER_MACRO Macros[] =
	{
		"PIXELSHADER",										"0",
		"COMPILER_HLSL",									"1",
		"MATERIAL_TANGENTSPACENORMAL",						"1",
		"LIGHTMAP_UV_ACCESS",								"0",
		"USE_INSTANCING",									"0",
		"TEX_COORD_SCALE_ANALYSIS",							"0",
		"TEX_COORD_SCALE_ANALYSIS",							"0",
		"INTERPOLATE_VERTEX_COLOR",							"0",//bUsesVertexColor
		"ALLOW_STATIC_LIGHTING",							"1",
		"LIGHT_SOURCE_SHAPE",								"0",//directional 
		"USE_LIGHTING_CHANNELS",							"0",
		"METAL_PROFILE",									"1",
		"GPUSKIN_PASS_THROUGH",								"0",
		"USE_PARTICLE_SUBUVS",								"0",
		"MAX_NUM_LIGHTMAP_COEF",							"2",
		"HQ_TEXTURE_LIGHTMAP",								"1",
		"LQ_TEXTURE_LIGHTMAP",								"0",
		"USES_AO_MATERIAL_MASK",							"0",
		"PRECOMPUTED_IRRADIANCE_VOLUME_LIGHTING",			"0",
		"CACHED_VOLUME_INDIRECT_LIGHTING",					"0",
		"CACHED_POINT_INDIRECT_LIGHTING",					"0",
		"STATICLIGHTING_TEXTUREMASK",						"0",
		"STATICLIGHTING_SIGNEDDISTANCEFIELD",				"0",
		"ALLOW_STATIC_LIGHTING",							"1",
		"SELECTIVE_BASEPASS_OUTPUTS",						"1",
		"NEEDS_BASEPASS_VERTEX_FOGGING",					"0",
		"NEEDS_BASEPASS_PIXEL_FOGGING",						"0",
		"ENABLE_SKY_LIGHT",									"1",
		"MATERIAL_SHADINGMODEL_TWOSIDED_FOLIAGE",			"0",
		"ATMOSPHERIC_NO_LIGHT_SHAFT",						"1",
		NULL,NULL,
	};
	std::vector<D3D_SHADER_MACRO> CombinedMacros(std::begin(Macros), std::end(Macros));
	if (OtherMacrosCount > 0)
		CombinedMacros.insert(CombinedMacros.begin(), OtherMacros, OtherMacros + OtherMacrosCount);
	if (S_OK == D3DCompileFromFile(File, CombinedMacros.data(), &IncludeHandler, EntryPoint, VSTarget, VSFlags, 0, &Bytecode, &OutErrorMsg))
	{
		return Bytecode; 
	}
	X_LOG("D3DCompileFromFile failed! %s", (const char*)OutErrorMsg->GetBufferPointer());
	return NULL; 
}

ID3DBlob* CompilePixelShader(const wchar_t* File, const char* EntryPoint, const D3D_SHADER_MACRO* OtherMacros/* = NULL*/, int OtherMacrosCount/* = 0*/)
{
	ID3DBlob* Bytecode;
	ID3DBlob* OutErrorMsg;
	LPCSTR VSTarget = D3D11Device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0 ? "ps_5_0" : "ps_4_0";
	UINT VSFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;/*| D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY*/;
	ShaderIncludeHandler IncludeHandler;
	
		D3D_SHADER_MACRO Macros[] =
	{
		"PIXELSHADER",										"1",
		"COMPILER_HLSL",									"1",
		"MATERIAL_TANGENTSPACENORMAL",						"1",
		"LIGHTMAP_UV_ACCESS",								"0",
		"USE_INSTANCING",									"0",
		"TEX_COORD_SCALE_ANALYSIS",							"0",
		"TEX_COORD_SCALE_ANALYSIS",							"0",
		"INTERPOLATE_VERTEX_COLOR",							"0",//bUsesVertexColor
		"ALLOW_STATIC_LIGHTING",							"1",
		"LIGHT_SOURCE_SHAPE",								"0",
		"USE_LIGHTING_CHANNELS",							"0",
		"METAL_PROFILE",									"1",
		"GPUSKIN_PASS_THROUGH",								"0",
		"USE_PARTICLE_SUBUVS",								"0",
		"MAX_NUM_LIGHTMAP_COEF",							"2",
		"HQ_TEXTURE_LIGHTMAP",								"1",
		"LQ_TEXTURE_LIGHTMAP",								"0",
		"USES_AO_MATERIAL_MASK",							"0",
		"PRECOMPUTED_IRRADIANCE_VOLUME_LIGHTING",			"0",
		"CACHED_VOLUME_INDIRECT_LIGHTING",					"0",
		"CACHED_POINT_INDIRECT_LIGHTING",					"0",
		"STATICLIGHTING_TEXTUREMASK",						"0",
		"STATICLIGHTING_SIGNEDDISTANCEFIELD",				"0",
		"ALLOW_STATIC_LIGHTING",							"1",
		"SELECTIVE_BASEPASS_OUTPUTS",						"1",
		"NEEDS_BASEPASS_VERTEX_FOGGING",					"0",
		"NEEDS_BASEPASS_PIXEL_FOGGING",						"0",
		"ENABLE_SKY_LIGHT",									"1",
		"MATERIAL_SHADINGMODEL_TWOSIDED_FOLIAGE",			"0",
		"ATMOSPHERIC_NO_LIGHT_SHAFT",						"1",
		NULL,NULL,
	};
	std::vector<D3D_SHADER_MACRO> CombinedMacros(std::begin(Macros), std::end(Macros));
	if(OtherMacrosCount > 0)
		CombinedMacros.insert(CombinedMacros.begin(), OtherMacros, OtherMacros + OtherMacrosCount);
	if (S_OK == D3DCompileFromFile(File, CombinedMacros.data(), &IncludeHandler, EntryPoint, VSTarget, VSFlags, 0, &Bytecode, &OutErrorMsg))
	{
		return Bytecode;   
	}
	X_LOG("D3DCompileFromFile failed! %s", (const char*)OutErrorMsg->GetBufferPointer());
	return NULL;
}

void GetShaderParameterAllocations(ID3DBlob* Code, std::map<std::string, ParameterAllocation>& OutParams)
{
	//see UE4 D3D11ShaderCompiler.cpp CompileAndProcessD3DShader()
	ID3D11ShaderReflection* pReflector = NULL;
	D3DReflect(Code->GetBufferPointer(), Code->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&pReflector);
	D3D11_SHADER_DESC ShaderDesc;
	pReflector->GetDesc(&ShaderDesc);
	for (UINT ResourceIndex = 0; ResourceIndex < ShaderDesc.BoundResources; ResourceIndex++)
	{
		D3D11_SHADER_INPUT_BIND_DESC BindDesc;
		pReflector->GetResourceBindingDesc(ResourceIndex, &BindDesc);
		if (BindDesc.Type == D3D10_SIT_CBUFFER || BindDesc.Type == D3D10_SIT_TBUFFER)
		{
			const UINT CBIndex = BindDesc.BindPoint;
			ID3D11ShaderReflectionConstantBuffer* ConstantBuffer = pReflector->GetConstantBufferByName(BindDesc.Name);
			D3D11_SHADER_BUFFER_DESC CBDesc;
			ConstantBuffer->GetDesc(&CBDesc);
			bool bGlobalCB = (strcmp(CBDesc.Name, "$Globals") == 0);
			if (bGlobalCB)
			{
				for (UINT ContantIndex = 0; ContantIndex < CBDesc.Variables; ContantIndex++)
				{
					ID3D11ShaderReflectionVariable* Variable = ConstantBuffer->GetVariableByIndex(ContantIndex);
					D3D11_SHADER_VARIABLE_DESC VariableDesc;
					Variable->GetDesc(&VariableDesc);
					if (VariableDesc.uFlags & D3D10_SVF_USED)
					{
						OutParams.insert(std::make_pair<std::string,ParameterAllocation>(VariableDesc.Name, { CBIndex ,VariableDesc.StartOffset,VariableDesc.Size }));
					}
				}
			}
			else
			{
				OutParams.insert(std::make_pair<std::string, ParameterAllocation>(CBDesc.Name, { CBIndex,0,0 }));
			}
		}
		else if (BindDesc.Type == D3D10_SIT_TEXTURE || BindDesc.Type == D3D10_SIT_SAMPLER)
		{
			UINT BindCount = BindDesc.BindCount;

			OutParams.insert(std::make_pair<std::string, ParameterAllocation>(BindDesc.Name, { 0,BindDesc.BindPoint,BindCount }));
		}
		else if (BindDesc.Type == D3D11_SIT_UAV_RWTYPED || BindDesc.Type == D3D11_SIT_UAV_RWSTRUCTURED ||
			BindDesc.Type == D3D11_SIT_UAV_RWBYTEADDRESS || BindDesc.Type == D3D11_SIT_UAV_RWSTRUCTURED_WITH_COUNTER ||
			BindDesc.Type == D3D11_SIT_UAV_APPEND_STRUCTURED)
		{
		}
	}

}

ID3D11VertexShader* CreateVertexShader(ID3DBlob* VSBytecode)
{
	ID3D11VertexShader* Result;
	if (S_OK == D3D11Device->CreateVertexShader(VSBytecode->GetBufferPointer(), VSBytecode->GetBufferSize(), NULL, &Result))
	{
		return Result;
	}

	X_LOG("CreateVertexShader failed!");
	return NULL;
}

ID3D11PixelShader* CreatePixelShader(ID3DBlob* PSBytecode)
{
	ID3D11PixelShader* Result;
	if (S_OK == D3D11Device->CreatePixelShader(PSBytecode->GetBufferPointer(), PSBytecode->GetBufferSize(), NULL, &Result))
	{
		return Result;
	}

	X_LOG("CreateVertexShader failed!");
	return NULL;
}

ID3D11InputLayout* CreateInputLayout(D3D11_INPUT_ELEMENT_DESC* InputDesc, unsigned int Count, ID3DBlob* VSBytecode)
{
	ID3D11InputLayout* Result;
	if (S_OK == D3D11Device->CreateInputLayout(InputDesc, Count, VSBytecode->GetBufferPointer(), VSBytecode->GetBufferSize(), &Result))
	{
		return Result;
	}
	X_LOG("CreateInputLayout failed!");
	return NULL;
}

ID3D11Texture2D* CreateTexture2D(unsigned int W, unsigned int H, DXGI_FORMAT Format, bool bRenderTarget, bool bShaderResource, bool bDepthStencil, UINT MipMapCount/* = 1*/)
{
	D3D11_TEXTURE2D_DESC Desc;
	ZeroMemory(&Desc, sizeof(Desc));
	Desc.Width = W;
	Desc.Height = H;
	Desc.Format = Format;
	Desc.ArraySize = 1;
	Desc.MipLevels = MipMapCount;
	Desc.SampleDesc.Count = 1;
	Desc.SampleDesc.Quality = 0;
	if (bRenderTarget)
	{
		Desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	}
	if (bShaderResource)
	{
		Desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
	}
	if (bDepthStencil)
	{
		Desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
	}
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.CPUAccessFlags = 0;
	Desc.MiscFlags = 0;

	ID3D11Texture2D* Result;
	if (S_OK == D3D11Device->CreateTexture2D(&Desc,NULL, &Result))
	{
		return Result;
	}
	X_LOG("CreateTexture2D failed!");
	return NULL;
}

ID3D11Texture2D* CreateTexture2D(unsigned int W, unsigned int H, DXGI_FORMAT Format, UINT MipMapCount, void* InitData)
{
	D3D11_TEXTURE2D_DESC Desc;
	ZeroMemory(&Desc, sizeof(Desc));
	Desc.Width = W;
	Desc.Height = H;
	Desc.Format = Format;
	Desc.ArraySize = 1;
	Desc.MipLevels = MipMapCount;
	Desc.SampleDesc.Count = 1;
	Desc.SampleDesc.Quality = 0;
	Desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.CPUAccessFlags = 0;
	Desc.MiscFlags = 0;

	unsigned int PixelSize = 0;
	switch (Format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		PixelSize = 4;
		break;
	}
	D3D11_SUBRESOURCE_DATA Data;
	Data.pSysMem = InitData;
	Data.SysMemPitch = W * PixelSize;
	Data.SysMemSlicePitch = W * H * PixelSize;

	ID3D11Texture2D* Result;
	if (S_OK == D3D11Device->CreateTexture2D(&Desc, &Data, &Result))
	{
		return Result;
	}
	X_LOG("CreateTexture2D failed!");
	return NULL;
}


bool CreateTexture2DFromTGA(const wchar_t* FileName)
{
	TexMetadata Metadata;
	ScratchImage sImage;
	if (S_OK == LoadFromTGAFile(FileName, TGA_FLAGS_FORCE_SRGB, &Metadata, sImage))
	{

	}
	return NULL;
}

ID3D11RenderTargetView* CreateRenderTargetView2D(ID3D11Texture2D* Resource, DXGI_FORMAT Format, UINT MipSlice)
{
	D3D11_RENDER_TARGET_VIEW_DESC Desc;
	Desc.Format = Format;
	Desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	Desc.Texture2D.MipSlice = MipSlice;

	ID3D11RenderTargetView* Result;
	if (S_OK == D3D11Device->CreateRenderTargetView(Resource, &Desc, &Result))
	{
		return Result;
	}
	X_LOG("CreateRenderTargetView2D failed!");
	return NULL;
}

ID3D11DepthStencilView* CreateDepthStencilView2D(ID3D11Texture2D* Resource, DXGI_FORMAT Format, UINT MipSlice)
{
	D3D11_DEPTH_STENCIL_VIEW_DESC Desc;
	Desc.Format = Format;
	Desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	Desc.Flags = 0;
	Desc.Texture2D.MipSlice = MipSlice;

	ID3D11DepthStencilView* Result;
	if (S_OK == D3D11Device->CreateDepthStencilView(Resource, &Desc, &Result))
	{
		return Result;
	}
	X_LOG("CreateDepthStencilView failed!");
	return NULL;
}

ID3D11ShaderResourceView* CreateShaderResourceView2D(ID3D11Texture2D* Resource, DXGI_FORMAT Format, UINT MipLevels, UINT MostDetailedMip)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC Desc;
	Desc.Format = Format;
	Desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
	Desc.Texture2D.MipLevels = MipLevels;
	Desc.Texture2D.MostDetailedMip = MostDetailedMip;

	ID3D11ShaderResourceView* Result;
	if (S_OK == D3D11Device->CreateShaderResourceView(Resource, &Desc, &Result))
	{
		return Result;
	}
	X_LOG("CreateShaderResourceView2D failed!");
	return NULL;
}
