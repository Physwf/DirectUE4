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
#include "ShaderCore.h"
#include "SceneFilterRendering.h"
#include "Shader.h"
#include "OneColorShader.h"
#include "VolumeRendering.h"
#include "Viewport.h"

// #define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h"
// #include "WICTextureLoader.h"

using namespace DirectX;

IDXGIFactory*	DXGIFactory = NULL;
IDXGIAdapter*	DXGIAdapter = NULL;
IDXGIOutput*	DXGIOutput = NULL;
IDXGISwapChain* DXGISwapChain = NULL;

ID3D11Device*			D3D11Device = NULL;
ID3D11DeviceContext*	D3D11DeviceContext = NULL;

FD3D11Texture2D* BackBuffer;

ID3D11Texture2D* GBlackTexture;
ID3D11ShaderResourceView* GBlackTextureSRV;
ID3D11SamplerState* GBlackTextureSamplerState;
ID3D11Texture2D* GWhiteTexture;
ID3D11ShaderResourceView* GWhiteTextureSRV;
ID3D11SamplerState* GWhiteTextureSamplerState;

ID3D11Texture3D* GBlackVolumeTexture;
ID3D11ShaderResourceView* GBlackVolumeTextureSRV;
ID3D11SamplerState* GBlackVolumeTextureSamplerState;

ID3D11Texture2D* GWhiteTextureCube;
ID3D11ShaderResourceView* GWhiteTextureCubeSRV;
ID3D11SamplerState* GWhiteTextureCubeSamplerState;

std::shared_ptr<FD3D11Texture2D> GBlackTextureDepthCube;

ComPtr<ID3D11Buffer> DynamicVB;
ComPtr<ID3D11Buffer> DynamicIB;

ComPtr<ID3D11Buffer> GNullColorVertexBuffer;
ComPtr<ID3D11ShaderResourceView> GNullColorVertexBufferSRV;

LONG WindowWidth = 1920;
LONG WindowHeight = 1080;



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

ID3D11Buffer* CreateConstantBuffer(bool bDynamic, unsigned int Size, const void* Data /*= NULL*/)
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
			if (strncmp(pFileName, "/Generated/", strlen("/Generated/"))) return S_OK;//skip Generated Include
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

class ShaderVirtualIncludeHandler : public ID3DInclude
{
public:
	ShaderVirtualIncludeHandler(const std::map<std::string, std::string>& InIncludeVirtualPathToContentsMap, const std::map<std::string, std::shared_ptr<std::string>>& InIncludeVirtualPathToExternalContentsMap)
		:IncludeVirtualPathToContentsMap(InIncludeVirtualPathToContentsMap),
		IncludeVirtualPathToExternalContentsMap(InIncludeVirtualPathToExternalContentsMap)
	{}

	HRESULT __stdcall Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
	{
		std::string IncludeFile;
		switch (IncludeType)
		{
		case D3D_INCLUDE_LOCAL:
			if (strncmp(pFileName, "/Generated/", strlen("/Generated/")) == 0)
			{
				for (auto& Pair : IncludeVirtualPathToContentsMap)
				{
					if (Pair.first.find(pFileName) != std::string::npos)
					{
						char* buf = new char[Pair.second.size() + 1];
						strcpy_s(buf, Pair.second.size() + 1, Pair.second.c_str());
						*ppData = buf;
						*pBytes = Pair.second.size();
						return S_OK;
					}
				}
				for (auto& Pair : IncludeVirtualPathToExternalContentsMap)
				{
					//if (strstr(pFileName, Pair.first.c_str()) != NULL)
					if (Pair.first.find(pFileName) != std::string::npos)
					{
						char* buf = new char[Pair.second->size() + 1];
						strcpy_s(buf, Pair.second->size() + 1, Pair.second->c_str());
						*ppData = buf;
						*pBytes = Pair.second->size();
						return S_OK;
					}
				}
				return E_FAIL;
			}
			IncludeFile = std::string("./Shaders/") + pFileName;
			break;
		case D3D_INCLUDE_SYSTEM:
			IncludeFile = std::string("./Shaders/") + pFileName;
			break;
		}
		X_LOG("filename:%s\n", IncludeFile.c_str());
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
		return E_FAIL;
	}
	HRESULT __stdcall Close(LPCVOID pData)
	{
		char* buf = (char*)pData;
		delete[] buf;
		return S_OK;
	}
private:
	std::unique_ptr<char[]> filedata;
	const std::map<std::string, std::string>& IncludeVirtualPathToContentsMap;
	const std::map<std::string, std::shared_ptr<std::string>>& IncludeVirtualPathToExternalContentsMap;
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

bool CompileShader(const std::string& FileContent, const char* EntryPoint, const char* Target, const D3D_SHADER_MACRO* OtherMacros, ID3DBlob** OutBytecode)
{
	//ShaderVirtualIncludeHandler IncludeHandler(InIncludeVirtualPathToContentsMap, InIncludeVirtualPathToExternalContentsMap);
	ID3DBlob* OutErrorMsg;
	//X_LOG("%s", FileContent.c_str());
	HRESULT HR = D3DCompile(
		FileContent.c_str(),
		FileContent.size(),
		NULL,
		OtherMacros,
		NULL,
		EntryPoint,
		Target,
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR/*| D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY*/,
		0,
		OutBytecode,
		&OutErrorMsg
	);
	if(HR != S_OK)
	{
		X_LOG("D3DCompileFromFile failed! %s", (const char*)OutErrorMsg->GetBufferPointer());
		assert(false);
		return false;
	}
	return true;
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

void GetShaderParameterAllocations(ID3DBlob* Code, FShaderParameterMap& OutShaderParameterMap)
{
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
						//OutParams.insert(std::make_pair<std::string, ParameterAllocation>(VariableDesc.Name, { CBIndex ,VariableDesc.StartOffset,VariableDesc.Size }));
						OutShaderParameterMap.AddParameterAllocation(VariableDesc.Name, CBIndex, VariableDesc.StartOffset, VariableDesc.Size);
					}
				}
			}
			else
			{
				//OutParams.insert(std::make_pair<std::string, ParameterAllocation>(CBDesc.Name, { CBIndex,0,0 }));
				OutShaderParameterMap.AddParameterAllocation(CBDesc.Name, CBIndex, 0, 0);
			}
		}
		else if (BindDesc.Type == D3D10_SIT_TEXTURE || BindDesc.Type == D3D10_SIT_SAMPLER)
		{
			UINT BindCount = BindDesc.BindCount;

			//OutParams.insert(std::make_pair<std::string, ParameterAllocation>(BindDesc.Name, { 0,BindDesc.BindPoint,BindCount }));
			OutShaderParameterMap.AddParameterAllocation(BindDesc.Name, 0, BindDesc.BindPoint, BindCount);
		}
		else if (BindDesc.Type == D3D11_SIT_UAV_RWTYPED || BindDesc.Type == D3D11_SIT_UAV_RWSTRUCTURED ||
			BindDesc.Type == D3D11_SIT_UAV_RWBYTEADDRESS || BindDesc.Type == D3D11_SIT_UAV_RWSTRUCTURED_WITH_COUNTER ||
			BindDesc.Type == D3D11_SIT_UAV_APPEND_STRUCTURED)
		{
			OutShaderParameterMap.AddParameterAllocation(
				BindDesc.Name,
				0,
				BindDesc.BindPoint,
				1
			);
		}
		else if (BindDesc.Type == D3D11_SIT_STRUCTURED || BindDesc.Type == D3D11_SIT_BYTEADDRESS)
		{
			OutShaderParameterMap.AddParameterAllocation(
				BindDesc.Name,
				0,
				BindDesc.BindPoint,
				1
			);
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
	case DXGI_FORMAT_R8G8B8A8_UNORM:
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


ID3D11Texture2D* CreateTextureCube(unsigned int Size, DXGI_FORMAT Format, uint32 MipMapCount,const uint8* InitData)
{
	D3D11_TEXTURE2D_DESC Desc;
	ZeroMemory(&Desc, sizeof(Desc));
	Desc.Width = Size;
	Desc.Height = Size;
	Desc.Format = Format;
	Desc.ArraySize = 6;
	Desc.MipLevels = MipMapCount;
	Desc.SampleDesc.Count = 1;
	Desc.SampleDesc.Quality = 0;
	Desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.CPUAccessFlags = 0;
	Desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	unsigned int BlockSizeX = 0;
	unsigned int BlockSizeY = 0;
	unsigned int BlockBytes = 0;
	switch (Format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		BlockSizeX = 1;
		BlockSizeY = 1;
		BlockBytes = 4;
		break;
	}

	std::vector<D3D11_SUBRESOURCE_DATA> SubResourceData;
	SubResourceData.resize(MipMapCount * 6);
	uint32 SliceOffset = 0;
	for (uint32 ArraySliceIndex = 0; ArraySliceIndex < 6; ++ArraySliceIndex)
	{
		uint32 MipOffset = 0;
		for (uint32 MipIndex = 0; MipIndex < MipMapCount; ++MipIndex)
		{
			uint32 DataOffset = SliceOffset + MipOffset;
			uint32 SubResourceIndex = ArraySliceIndex * MipMapCount + MipIndex;

			uint32 NumBlocksX = FMath::Max<uint32>(1, (Size >> MipIndex) / BlockSizeX/*GPixelFormats[Format].BlockSizeX*/);
			uint32 NumBlocksY = FMath::Max<uint32>(1, (Size >> MipIndex) / BlockSizeY/*GPixelFormats[Format].BlockSizeY*/);

			SubResourceData[SubResourceIndex].pSysMem = &InitData[DataOffset];
			SubResourceData[SubResourceIndex].SysMemPitch = NumBlocksX * BlockBytes/*GPixelFormats[Format].BlockBytes*/;
			SubResourceData[SubResourceIndex].SysMemSlicePitch = NumBlocksX * NumBlocksY * SubResourceData[MipIndex].SysMemPitch;

			MipOffset += NumBlocksY * SubResourceData[MipIndex].SysMemPitch;
		}
		SliceOffset += MipOffset;
	}

	ID3D11Texture2D* Result;
	if (S_OK == D3D11Device->CreateTexture2D(&Desc, SubResourceData.data(), &Result))
	{
		return Result;
	}
	X_LOG("CreateTexture2D failed!");
	return NULL;
}

ID3D11Texture3D* CreateTexture3D(unsigned int Width, unsigned int Height, unsigned int Depth, DXGI_FORMAT Format, UINT MipMapCount, void* InitData)
{
	D3D11_TEXTURE3D_DESC Desc;
	ZeroMemory(&Desc, sizeof(Desc));
	Desc.Width = Width;
	Desc.Height = Height;
	Desc.Depth = Depth;
	Desc.MipLevels = MipMapCount;
	Desc.Format = Format;
	Desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
	Desc.Usage = D3D11_USAGE_DEFAULT;

	unsigned int PixelSize = 0;
	switch (Format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		PixelSize = 4;
		break;
	}

	D3D11_SUBRESOURCE_DATA Data;
	Data.pSysMem = InitData;
	Data.SysMemPitch = Width * PixelSize;
	Data.SysMemSlicePitch = Width * Height * PixelSize;

	ID3D11Texture3D* Result;
	if (S_OK == D3D11Device->CreateTexture3D(&Desc, &Data, &Result))
	{
		return Result;
	}
	X_LOG("CreateTexture3D failed!");
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

ID3D11ShaderResourceView* CreateShaderResourceViewCube(ID3D11Texture2D* Resource, DXGI_FORMAT Format, UINT MipLevels, UINT MostDetailedMip)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC Desc;
	Desc.Format = Format;
	Desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBE;
	Desc.TextureCube.MipLevels = MipLevels;
	Desc.TextureCube.MostDetailedMip = MostDetailedMip;

	ID3D11ShaderResourceView* Result;
	if (S_OK == D3D11Device->CreateShaderResourceView(Resource, &Desc, &Result))
	{
		return Result;
	}
	X_LOG("CreateShaderResourceViewCube failed!");
	return NULL;
}

ID3D11ShaderResourceView* CreateShaderResourceView3D(ID3D11Texture3D* Resource, DXGI_FORMAT Format, UINT MipLevels, UINT MostDetailedMip)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC Desc;
	Desc.Format = Format;
	Desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE3D;
	Desc.Texture3D.MipLevels = MipLevels;
	Desc.Texture3D.MostDetailedMip = MostDetailedMip;

	ID3D11ShaderResourceView* Result;
	if (S_OK == D3D11Device->CreateShaderResourceView(Resource, &Desc, &Result))
	{
		return Result;
	}
	X_LOG("CreateShaderResourceView3D failed!");
	return NULL;
}

inline DXGI_FORMAT FindShaderResourceDXGIFormat(DXGI_FORMAT InFormat, bool bSRGB)
{
	if (bSRGB)
	{
		switch (InFormat)
		{
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:    return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:    return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case DXGI_FORMAT_BC1_TYPELESS:         return DXGI_FORMAT_BC1_UNORM_SRGB;
		case DXGI_FORMAT_BC2_TYPELESS:         return DXGI_FORMAT_BC2_UNORM_SRGB;
		case DXGI_FORMAT_BC3_TYPELESS:         return DXGI_FORMAT_BC3_UNORM_SRGB;
		case DXGI_FORMAT_BC7_TYPELESS:         return DXGI_FORMAT_BC7_UNORM_SRGB;
		};
	}
	else
	{
		switch (InFormat)
		{
		case DXGI_FORMAT_B8G8R8A8_TYPELESS: return DXGI_FORMAT_B8G8R8A8_UNORM;
		case DXGI_FORMAT_R8G8B8A8_TYPELESS: return DXGI_FORMAT_R8G8B8A8_UNORM;
		case DXGI_FORMAT_BC1_TYPELESS:      return DXGI_FORMAT_BC1_UNORM;
		case DXGI_FORMAT_BC2_TYPELESS:      return DXGI_FORMAT_BC2_UNORM;
		case DXGI_FORMAT_BC3_TYPELESS:      return DXGI_FORMAT_BC3_UNORM;
		case DXGI_FORMAT_BC7_TYPELESS:      return DXGI_FORMAT_BC7_UNORM;
		};
	}
	switch (InFormat)
	{
	case DXGI_FORMAT_R24G8_TYPELESS: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	case DXGI_FORMAT_R32_TYPELESS: return DXGI_FORMAT_R32_FLOAT;
	case DXGI_FORMAT_R16_TYPELESS: return DXGI_FORMAT_R16_UNORM;
#if DEPTH_32_BIT_CONVERSION
		// Changing Depth Buffers to 32 bit on Dingo as D24S8 is actually implemented as a 32 bit buffer in the hardware
	case DXGI_FORMAT_R32G8X24_TYPELESS: return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
#endif
	}
	return InFormat;
}
/** Find the appropriate depth-stencil targetable DXGI format for the given format. */
inline DXGI_FORMAT FindDepthStencilDXGIFormat(DXGI_FORMAT InFormat)
{
	switch (InFormat)
	{
	case DXGI_FORMAT_R24G8_TYPELESS:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
#if DEPTH_32_BIT_CONVERSION
		// Changing Depth Buffers to 32 bit on Dingo as D24S8 is actually implemented as a 32 bit buffer in the hardware
	case DXGI_FORMAT_R32G8X24_TYPELESS:
		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
#endif
	case DXGI_FORMAT_R32_TYPELESS:
		return DXGI_FORMAT_D32_FLOAT;
	case DXGI_FORMAT_R16_TYPELESS:
		return DXGI_FORMAT_D16_UNORM;
	};
	return InFormat;
}
inline bool HasStencilBits(DXGI_FORMAT InFormat)
{
	switch (InFormat)
	{
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		return true;
#if  DEPTH_32_BIT_CONVERSION
		// Changing Depth Buffers to 32 bit on Dingo as D24S8 is actually implemented as a 32 bit buffer in the hardware
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		return true;
#endif
	};
	return false;
}
DXGI_FORMAT GetPlatformTextureResourceFormat(DXGI_FORMAT InFormat, uint32 InFlags)
{
	// DX 11 Shared textures must be B8G8R8A8_UNORM
	if (InFlags & TexCreate_Shared)
	{
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	}
	return InFormat;
}

#define DX_MAX_MSAA_COUNT 8

uint32 GetMaxMSAAQuality(uint32 SampleCount)
{
	if (SampleCount <= DX_MAX_MSAA_COUNT)
	{
		// 0 has better quality (a more even distribution)
		// higher quality levels might be useful for non box filtered AA or when using weighted samples 
		return 0;
		//		return AvailableMSAAQualities[SampleCount];
	}
	// not supported
	return 0xffffffff;
}
FPixelFormatInfo	GPixelFormats[PF_MAX] =
{
	// Name						BlockSizeX	BlockSizeY	BlockSizeZ	BlockBytes	NumComponents	PlatformFormat	Supported		UnrealFormat

	{ TEXT("unknown"),			0,			0,			0,			0,			0,				0,				0,				PF_Unknown },
	{ TEXT("A32B32G32R32F"),	1,			1,			1,			16,			4,				0,				1,				PF_A32B32G32R32F },
	{ TEXT("B8G8R8A8"),			1,			1,			1,			4,			4,				0,				1,				PF_B8G8R8A8 },
	{ TEXT("G8"),				1,			1,			1,			1,			1,				0,				1,				PF_G8 },
	{ TEXT("G16"),				1,			1,			1,			2,			1,				0,				1,				PF_G16 },
	{ TEXT("DXT1"),				4,			4,			1,			8,			3,				0,				1,				PF_DXT1 },
	{ TEXT("DXT3"),				4,			4,			1,			16,			4,				0,				1,				PF_DXT3 },
	{ TEXT("DXT5"),				4,			4,			1,			16,			4,				0,				1,				PF_DXT5 },
	{ TEXT("UYVY"),				2,			1,			1,			4,			4,				0,				0,				PF_UYVY },
	{ TEXT("FloatRGB"),			1,			1,			1,			4,			3,				0,				1,				PF_FloatRGB },
	{ TEXT("FloatRGBA"),		1,			1,			1,			8,			4,				0,				1,				PF_FloatRGBA },
	{ TEXT("DepthStencil"),		1,			1,			1,			4,			1,				0,				0,				PF_DepthStencil },
	{ TEXT("ShadowDepth"),		1,			1,			1,			4,			1,				0,				0,				PF_ShadowDepth },
	{ TEXT("R32_FLOAT"),		1,			1,			1,			4,			1,				0,				1,				PF_R32_FLOAT },
	{ TEXT("G16R16"),			1,			1,			1,			4,			2,				0,				1,				PF_G16R16 },
	{ TEXT("G16R16F"),			1,			1,			1,			4,			2,				0,				1,				PF_G16R16F },
	{ TEXT("G16R16F_FILTER"),	1,			1,			1,			4,			2,				0,				1,				PF_G16R16F_FILTER },
	{ TEXT("G32R32F"),			1,			1,			1,			8,			2,				0,				1,				PF_G32R32F },
	{ TEXT("A2B10G10R10"),      1,          1,          1,          4,          4,              0,              1,				PF_A2B10G10R10 },
	{ TEXT("A16B16G16R16"),		1,			1,			1,			8,			4,				0,				1,				PF_A16B16G16R16 },
	{ TEXT("D24"),				1,			1,			1,			4,			1,				0,				1,				PF_D24 },
	{ TEXT("PF_R16F"),			1,			1,			1,			2,			1,				0,				1,				PF_R16F },
	{ TEXT("PF_R16F_FILTER"),	1,			1,			1,			2,			1,				0,				1,				PF_R16F_FILTER },
	{ TEXT("BC5"),				4,			4,			1,			16,			2,				0,				1,				PF_BC5 },
	{ TEXT("V8U8"),				1,			1,			1,			2,			2,				0,				1,				PF_V8U8 },
	{ TEXT("A1"),				1,			1,			1,			1,			1,				0,				0,				PF_A1 },
	{ TEXT("FloatR11G11B10"),	1,			1,			1,			4,			3,				0,				0,				PF_FloatR11G11B10 },
	{ TEXT("A8"),				1,			1,			1,			1,			1,				0,				1,				PF_A8 },
	{ TEXT("R32_UINT"),			1,			1,			1,			4,			1,				0,				1,				PF_R32_UINT },
	{ TEXT("R32_SINT"),			1,			1,			1,			4,			1,				0,				1,				PF_R32_SINT },

	// IOS Support
	{ TEXT("PVRTC2"),			8,			4,			1,			8,			4,				0,				0,				PF_PVRTC2 },
	{ TEXT("PVRTC4"),			4,			4,			1,			8,			4,				0,				0,				PF_PVRTC4 },

	{ TEXT("R16_UINT"),			1,			1,			1,			2,			1,				0,				1,				PF_R16_UINT },
	{ TEXT("R16_SINT"),			1,			1,			1,			2,			1,				0,				1,				PF_R16_SINT },
	{ TEXT("R16G16B16A16_UINT"),1,			1,			1,			8,			4,				0,				1,				PF_R16G16B16A16_UINT },
	{ TEXT("R16G16B16A16_SINT"),1,			1,			1,			8,			4,				0,				1,				PF_R16G16B16A16_SINT },
	{ TEXT("R5G6B5_UNORM"),     1,          1,          1,          2,          3,              0,              1,              PF_R5G6B5_UNORM },
	{ TEXT("R8G8B8A8"),			1,			1,			1,			4,			4,				0,				1,				PF_R8G8B8A8 },
	{ TEXT("A8R8G8B8"),			1,			1,			1,			4,			4,				0,				1,				PF_A8R8G8B8 },
	{ TEXT("BC4"),				4,			4,			1,			8,			1,				0,				1,				PF_BC4 },
	{ TEXT("R8G8"),				1,			1,			1,			2,			2,				0,				1,				PF_R8G8 },

	{ TEXT("ATC_RGB"),			4,			4,			1,			8,			3,				0,				0,				PF_ATC_RGB },
	{ TEXT("ATC_RGBA_E"),		4,			4,			1,			16,			4,				0,				0,				PF_ATC_RGBA_E },
	{ TEXT("ATC_RGBA_I"),		4,			4,			1,			16,			4,				0,				0,				PF_ATC_RGBA_I },
	{ TEXT("X24_G8"),			1,			1,			1,			1,			1,				0,				0,				PF_X24_G8 },
	{ TEXT("ETC1"),				4,			4,			1,			8,			3,				0,				0,				PF_ETC1 },
	{ TEXT("ETC2_RGB"),			4,			4,			1,			8,			3,				0,				0,				PF_ETC2_RGB },
	{ TEXT("ETC2_RGBA"),		4,			4,			1,			16,			4,				0,				0,				PF_ETC2_RGBA },
	{ TEXT("PF_R32G32B32A32_UINT"),1,		1,			1,			16,			4,				0,				1,				PF_R32G32B32A32_UINT },
	{ TEXT("PF_R16G16_UINT"),	1,			1,			1,			4,			4,				0,				1,				PF_R16G16_UINT },

	// ASTC support
	{ TEXT("ASTC_4x4"),			4,			4,			1,			16,			4,				0,				0,				PF_ASTC_4x4 },
	{ TEXT("ASTC_6x6"),			6,			6,			1,			16,			4,				0,				0,				PF_ASTC_6x6 },
	{ TEXT("ASTC_8x8"),			8,			8,			1,			16,			4,				0,				0,				PF_ASTC_8x8 },
	{ TEXT("ASTC_10x10"),		10,			10,			1,			16,			4,				0,				0,				PF_ASTC_10x10 },
	{ TEXT("ASTC_12x12"),		12,			12,			1,			16,			4,				0,				0,				PF_ASTC_12x12 },

	{ TEXT("BC6H"),				4,			4,			1,			16,			3,				0,				1,				PF_BC6H },
	{ TEXT("BC7"),				4,			4,			1,			16,			4,				0,				1,				PF_BC7 },
	{ TEXT("R8_UINT"),			1,			1,			1,			1,			1,				0,				1,				PF_R8_UINT },
	{ TEXT("L8"),				1,			1,			1,			1,			1,				0,				0,				PF_L8 },
	{ TEXT("XGXR8"),			1,			1,			1,			4,			4,				0,				1,				PF_XGXR8 },
	{ TEXT("R8G8B8A8_UINT"),	1,			1,			1,			4,			4,				0,				1,				PF_R8G8B8A8_UINT },
	{ TEXT("R8G8B8A8_SNORM"),	1,			1,			1,			4,			4,				0,				1,				PF_R8G8B8A8_SNORM },

	{ TEXT("R16G16B16A16_UINT"),1,			1,			1,			8,			4,				0,				1,				PF_R16G16B16A16_UNORM },
	{ TEXT("R16G16B16A16_SINT"),1,			1,			1,			8,			4,				0,				1,				PF_R16G16B16A16_SNORM },
};

std::shared_ptr<FUniformBuffer> RHICreateUniformBuffer(
	UINT Size, 
	const void* Contents, 
	std::map<std::string, ID3D11ShaderResourceView*>& SRVs, 
	std::map<std::string, ID3D11SamplerState*>& Samplers,
	std::map<std::string, ID3D11UnorderedAccessView*>& UAVs)
{
	std::shared_ptr<FUniformBuffer> Result = std::make_shared<FUniformBuffer>();
	Result->ConstantBuffer = CreateConstantBuffer(false, Size, Contents);
	Result->SRVs = SRVs;
	Result->Samplers = Samplers;
	Result->UAVs = UAVs;
	return Result;
}
ID3D11ShaderResourceView* CurrentShaderResourceViews[SF_NumFrequencies][D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
int32 MaxBoundShaderResourcesIndex[SF_NumFrequencies];

template <EShaderFrequency ShaderFrequency>
void InternalSetShaderResourceView(ID3D11ShaderResourceView* SRV, int32 ResourceIndex)
{
	assert(ResourceIndex < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

	int32& MaxResourceIndex = MaxBoundShaderResourcesIndex[ShaderFrequency];

	if (SRV)
	{
		// We are binding a new SRV.
		// Update the max resource index to the highest bound resource index.
		MaxResourceIndex = FMath::Max(MaxResourceIndex, ResourceIndex);
	}
	else 
	{
		// If this was the highest bound resource...
		if (MaxResourceIndex == ResourceIndex)
		{
			// Adjust the max resource index downwards until we
			// hit the next non-null slot, or we've run out of slots.
			do
			{
				MaxResourceIndex--;
			} while (MaxResourceIndex >= 0 && CurrentShaderResourceViews[ShaderFrequency][MaxResourceIndex] == nullptr);
		}
	}

	if ((CurrentShaderResourceViews[ShaderFrequency][ResourceIndex] != SRV) /*|| GD3D11SkipStateCaching*/)
	{
		if (SRV)
		{
			SRV->AddRef();
		}
		if (CurrentShaderResourceViews[ShaderFrequency][ResourceIndex])
		{
			CurrentShaderResourceViews[ShaderFrequency][ResourceIndex]->Release();
		}
		CurrentShaderResourceViews[ShaderFrequency][ResourceIndex] = SRV;
		switch (ShaderFrequency)
		{
		case SF_Vertex:		D3D11DeviceContext->VSSetShaderResources(ResourceIndex, 1, &SRV); break;
		case SF_Hull:		D3D11DeviceContext->HSSetShaderResources(ResourceIndex, 1, &SRV); break;
		case SF_Domain:		D3D11DeviceContext->DSSetShaderResources(ResourceIndex, 1, &SRV); break;
		case SF_Geometry:	D3D11DeviceContext->GSSetShaderResources(ResourceIndex, 1, &SRV); break;
		case SF_Pixel:		D3D11DeviceContext->PSSetShaderResources(ResourceIndex, 1, &SRV); break;
		case SF_Compute:	D3D11DeviceContext->CSSetShaderResources(ResourceIndex, 1, &SRV); break;
		}
	}
}

template <EShaderFrequency ShaderFrequency>
void ClearShaderResourceViews(ID3D11ShaderResourceView* Resource)
{
	int32 MaxIndex = MaxBoundShaderResourcesIndex[ShaderFrequency];
	for (int32 ResourceIndex = MaxIndex; ResourceIndex >= 0; --ResourceIndex)
	{
		if (CurrentShaderResourceViews[ShaderFrequency][ResourceIndex] == Resource)
		{
			// Unset the SRV from the device context
			InternalSetShaderResourceView<ShaderFrequency>(nullptr, ResourceIndex);
		}
	}
}

void ConditionalClearShaderResource(ID3D11ShaderResourceView* Resource)
{
	assert(Resource);
	ClearShaderResourceViews<SF_Vertex>(Resource);
	ClearShaderResourceViews<SF_Hull>(Resource);
	ClearShaderResourceViews<SF_Domain>(Resource);
	ClearShaderResourceViews<SF_Pixel>(Resource);
	ClearShaderResourceViews<SF_Geometry>(Resource);
	ClearShaderResourceViews<SF_Compute>(Resource);
}

void SetRenderTarget(FD3D11Texture* NewRenderTarget, FD3D11Texture* NewDepthStencilTarget, int32 MipIndex/* = 0*/, int32 ArraySliceIndex /*= 0*/)
{
	ID3D11RenderTargetView* RTV = NewRenderTarget->GetRenderTargetView(MipIndex, ArraySliceIndex);
	ID3D11DepthStencilView* DSV = NewDepthStencilTarget ? NewDepthStencilTarget->GetDepthStencilView(FExclusiveDepthStencil::DepthWrite_StencilWrite) : NULL;
	if (RTV) ConditionalClearShaderResource(NewRenderTarget->GetShaderResourceView());
	if (DSV) ConditionalClearShaderResource(NewDepthStencilTarget->GetShaderResourceView());
	D3D11DeviceContext->OMSetRenderTargets(1, &RTV, DSV);
}

void SetRenderTarget(FD3D11Texture* NewRenderTarget, FD3D11Texture* NewDepthStencilTarget, bool bClearColor/*=false*/, bool bClearDepth /*= false*/, bool bClearStencil /*= false*/, int32 MipIndex/* = 0*/, int32 ArraySliceIndex /*= 0*/)
{
	ID3D11RenderTargetView* RTV = NewRenderTarget ? NewRenderTarget->GetRenderTargetView(MipIndex, ArraySliceIndex) : NULL;
	FExclusiveDepthStencil AccessType = FExclusiveDepthStencil::DepthWrite_StencilWrite;
	if (bClearDepth && !bClearStencil) AccessType = FExclusiveDepthStencil::DepthWrite_StencilRead;
	if (!bClearDepth && bClearStencil) AccessType = FExclusiveDepthStencil::DepthRead_StencilWrite;
	ID3D11DepthStencilView* DSV = NewDepthStencilTarget ? NewDepthStencilTarget->GetDepthStencilView(AccessType) : NULL;

	if (RTV) ConditionalClearShaderResource(NewRenderTarget->GetShaderResourceView());
	if (DSV) ConditionalClearShaderResource(NewDepthStencilTarget->GetShaderResourceView());

	D3D11DeviceContext->OMSetRenderTargets(1, &RTV, DSV);
	if (bClearColor)
	{
		assert(RTV);
		FLinearColor ClearColor = NewRenderTarget->GetClearColor();
		D3D11DeviceContext->ClearRenderTargetView(RTV, (float*)&ClearColor);
	}
	if (bClearDepth || bClearStencil)
	{
		assert(DSV);
		UINT ClearFlags = 0;
		if (bClearDepth) ClearFlags |= D3D11_CLEAR_DEPTH;
		if (bClearStencil) ClearFlags |= D3D11_CLEAR_STENCIL;
		float OutDepth; 
		uint32 OutStencil;
		NewDepthStencilTarget->GetDepthStencilClearValue(OutDepth, OutStencil);
		D3D11DeviceContext->ClearDepthStencilView(DSV, ClearFlags, OutDepth, OutStencil);
	}
}


void SetRenderTargets(uint32 NumRTV, FD3D11Texture** NewRenderTarget, FD3D11Texture* NewDepthStencilTarget, bool bClearColor /*= false*/, bool bClearDepth /*= false*/, bool bClearStencil /*= false*/)
{
	std::vector<ID3D11RenderTargetView*> RTVs;

	for (uint32 i = 0; i < NumRTV; ++i)
	{
		ID3D11RenderTargetView* RTV = NewRenderTarget[i] ? NewRenderTarget[i]->GetRenderTargetView(0, 0) : NULL;
		RTVs.push_back(RTV);
		if (RTV) ConditionalClearShaderResource(NewRenderTarget[i]->GetShaderResourceView());
		if (bClearColor)
		{
			assert(RTV);
			ConditionalClearShaderResource(NewRenderTarget[i]->GetShaderResourceView());
			FLinearColor ClearColor = NewRenderTarget[i]->GetClearColor();
			D3D11DeviceContext->ClearRenderTargetView(RTV, (float*)&ClearColor);
		}
	}

	FExclusiveDepthStencil AccessType = FExclusiveDepthStencil::DepthWrite_StencilWrite;
	if (bClearDepth && !bClearStencil) AccessType = FExclusiveDepthStencil::DepthWrite_StencilRead;
	if (!bClearDepth && bClearStencil) AccessType = FExclusiveDepthStencil::DepthRead_StencilWrite;
	ID3D11DepthStencilView* DSV = NewDepthStencilTarget ? NewDepthStencilTarget->GetDepthStencilView(AccessType) : NULL;
	if (DSV) ConditionalClearShaderResource(NewDepthStencilTarget->GetShaderResourceView());
	D3D11DeviceContext->OMSetRenderTargets(NumRTV, RTVs.data(), DSV);
	if (bClearDepth || bClearStencil)
	{
		assert(DSV);
		UINT ClearFlags = 0;
		if (bClearDepth) ClearFlags |= D3D11_CLEAR_DEPTH;
		if (bClearStencil) ClearFlags |= D3D11_CLEAR_STENCIL;
		float OutDepth;
		uint32 OutStencil;
		NewDepthStencilTarget->GetDepthStencilClearValue(OutDepth, OutStencil);
		D3D11DeviceContext->ClearDepthStencilView(DSV, ClearFlags, OutDepth, OutStencil);
	}
}

void SetRenderTargetAndClear(FD3D11Texture* NewRenderTarget, FD3D11Texture* NewDepthStencilTarget)
{

}

void ClearRenderState()
{
	D3D11DeviceContext->VSSetShader(NULL, 0, 0);
	D3D11DeviceContext->HSSetShader(NULL, 0, 0);
	D3D11DeviceContext->DSSetShader(NULL, 0, 0);
	D3D11DeviceContext->GSSetShader(NULL, 0, 0);
	D3D11DeviceContext->PSSetShader(NULL, 0, 0);
}

ID3D11Buffer* RHICreateVertexBuffer(UINT Size, D3D11_USAGE InUsage, UINT BindFlags, UINT MiscFlags, const void* Data /*= NULL*/)
{
	D3D11_BUFFER_DESC Desc;
	ZeroMemory(&Desc, sizeof(D3D11_BUFFER_DESC));
	Desc.ByteWidth = Size;
	Desc.Usage = InUsage;
	Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER | BindFlags;
	Desc.MiscFlags = MiscFlags;
	Desc.CPUAccessFlags = (InUsage & D3D11_USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;
	//Desc.MiscFlags = 0;
	//Desc.StructureByteStride = 0;

	// If a resource array was provided for the resource, create the resource pre-populated
	D3D11_SUBRESOURCE_DATA InitData;
	D3D11_SUBRESOURCE_DATA* pInitData = NULL;
	if (Data)
	{
		InitData.pSysMem = Data;
		InitData.SysMemPitch = Size;
		InitData.SysMemSlicePitch = 0;
		pInitData = &InitData;
	}

	ID3D11Buffer* VertexBufferResource;
	assert(S_OK == D3D11Device->CreateBuffer(&Desc, pInitData, &VertexBufferResource));
	return VertexBufferResource;
}

std::shared_ptr<FD3D11Texture2D> CreateD3D11Texture2D(uint32 SizeX, uint32 SizeY, uint32 SizeZ, bool bTextureArray, bool bCubeTexture, EPixelFormat Format, uint32 NumMips, uint32 NumSamples, uint32 Flags, FClearValueBinding ClearBindingValue/* = FClearValueBinding::Transparent*/, const void* BulkData /*= nullptr*/, uint32 BulkDataSize /*= 0*/)
{
	const bool bSRGB = (Flags & TexCreate_SRGB) != 0;

	const DXGI_FORMAT PlatformResourceFormat = GetPlatformTextureResourceFormat((DXGI_FORMAT)GPixelFormats[Format].PlatformFormat, Flags);
	const DXGI_FORMAT PlatformShaderResourceFormat = FindShaderResourceDXGIFormat(PlatformResourceFormat, bSRGB);
	const DXGI_FORMAT PlatformRenderTargetFormat = FindShaderResourceDXGIFormat(PlatformResourceFormat, bSRGB);

	// Determine the MSAA settings to use for the texture.
	D3D11_DSV_DIMENSION DepthStencilViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	D3D11_RTV_DIMENSION RenderTargetViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	D3D11_SRV_DIMENSION ShaderResourceViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	uint32 CPUAccessFlags = 0;
	D3D11_USAGE TextureUsage = D3D11_USAGE_DEFAULT;
	uint32 BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bool bCreateShaderResource = true;

	uint32 ActualMSAACount = NumSamples;

	uint32 ActualMSAAQuality = GetMaxMSAAQuality(ActualMSAACount);

	// 0xffffffff means not supported
	if (ActualMSAAQuality == 0xffffffff || (Flags & TexCreate_Shared) != 0)
	{
		// no MSAA
		ActualMSAACount = 1;
		ActualMSAAQuality = 0;
	}

	if (ActualMSAACount > 1)
	{
		DepthStencilViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
		RenderTargetViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
		ShaderResourceViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
	}

	if (Flags & TexCreate_CPUReadback)
	{
		assert(!(Flags & TexCreate_RenderTargetable));
		assert(!(Flags & TexCreate_DepthStencilTargetable));
		assert(!(Flags & TexCreate_ShaderResource));

		CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		TextureUsage = D3D11_USAGE_STAGING;
		BindFlags = 0;
		bCreateShaderResource = false;
	}

	// Describe the texture.
	D3D11_TEXTURE2D_DESC TextureDesc;
	ZeroMemory(&TextureDesc, sizeof(D3D11_TEXTURE2D_DESC));
	TextureDesc.Width = SizeX;
	TextureDesc.Height = SizeY;
	TextureDesc.MipLevels = NumMips;
	TextureDesc.ArraySize = SizeZ;
	TextureDesc.Format = PlatformResourceFormat;
	TextureDesc.SampleDesc.Count = ActualMSAACount;
	TextureDesc.SampleDesc.Quality = ActualMSAAQuality;
	TextureDesc.Usage = TextureUsage;
	TextureDesc.BindFlags = BindFlags;
	TextureDesc.CPUAccessFlags = CPUAccessFlags;
	TextureDesc.MiscFlags = bCubeTexture ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;

	if (Flags & TexCreate_Shared)
	{
		TextureDesc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED;
	}

	if (Flags & TexCreate_GenerateMipCapable)
	{
		// Set the flag that allows us to call GenerateMips on this texture later
		TextureDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}

	// Set up the texture bind flags.
	bool bCreateRTV = false;
	bool bCreateDSV = false;
	bool bCreatedRTVPerSlice = false;

	if (Flags & TexCreate_RenderTargetable)
	{
		assert(!(Flags & TexCreate_DepthStencilTargetable));
		assert(!(Flags & TexCreate_ResolveTargetable));
		TextureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		bCreateRTV = true;
	}
	else if (Flags & TexCreate_DepthStencilTargetable)
	{
		assert(!(Flags & TexCreate_RenderTargetable));
		assert(!(Flags & TexCreate_ResolveTargetable));
		TextureDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
		bCreateDSV = true;
	}
	else if (Flags & TexCreate_ResolveTargetable)
	{
		assert(!(Flags & TexCreate_RenderTargetable));
		assert(!(Flags & TexCreate_DepthStencilTargetable));
		if (Format == PF_DepthStencil || Format == PF_ShadowDepth || Format == PF_D24)
		{
			TextureDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
			bCreateDSV = true;
		}
		else
		{
			TextureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			bCreateRTV = true;
		}
	}

	if (Flags & TexCreate_UAV)
	{
		TextureDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	}

	ComPtr<ID3D11Texture2D> TextureResource;
	ComPtr<ID3D11ShaderResourceView> ShaderResourceView;
	std::vector<ComPtr<ID3D11RenderTargetView>> RenderTargetViews;
	ComPtr<ID3D11DepthStencilView> DepthStencilViews[FExclusiveDepthStencil::MaxIndex];

	std::vector<D3D11_SUBRESOURCE_DATA> SubResourceData;

	if (BulkData)
	{
		uint8* Data = (uint8*)BulkData;

		// each mip of each array slice counts as a subresource
		SubResourceData.resize(NumMips * SizeZ);

		uint32 SliceOffset = 0;
		for (uint32 ArraySliceIndex = 0; ArraySliceIndex < SizeZ; ++ArraySliceIndex)
		{
			uint32 MipOffset = 0;
			for (uint32 MipIndex = 0; MipIndex < NumMips; ++MipIndex)
			{
				uint32 DataOffset = SliceOffset + MipOffset;
				uint32 SubResourceIndex = ArraySliceIndex * NumMips + MipIndex;

				uint32 NumBlocksX = FMath::Max<uint32>(1, (SizeX >> MipIndex) / GPixelFormats[Format].BlockSizeX);
				uint32 NumBlocksY = FMath::Max<uint32>(1, (SizeY >> MipIndex) / GPixelFormats[Format].BlockSizeY);

				SubResourceData[SubResourceIndex].pSysMem = &Data[DataOffset];
				SubResourceData[SubResourceIndex].SysMemPitch = NumBlocksX * GPixelFormats[Format].BlockBytes;
				SubResourceData[SubResourceIndex].SysMemSlicePitch = NumBlocksX * NumBlocksY * SubResourceData[MipIndex].SysMemPitch;

				MipOffset += NumBlocksY * SubResourceData[MipIndex].SysMemPitch;
			}
			SliceOffset += MipOffset;
		}
	}

	assert(S_OK == D3D11Device->CreateTexture2D(&TextureDesc, BulkData != NULL ? SubResourceData.data() : NULL, TextureResource.GetAddressOf()));

	if (bCreateRTV)
	{
		// Create a render target view for each mip
		for (uint32 MipIndex = 0; MipIndex < NumMips; MipIndex++)
		{
			if ((Flags & TexCreate_TargetArraySlicesIndependently) && (bTextureArray || bCubeTexture))
			{
				bCreatedRTVPerSlice = true;

				for (uint32 SliceIndex = 0; SliceIndex < TextureDesc.ArraySize; SliceIndex++)
				{
					D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
					memset(&RTVDesc, 0, sizeof(RTVDesc));
					RTVDesc.Format = PlatformRenderTargetFormat;
					RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
					RTVDesc.Texture2DArray.FirstArraySlice = SliceIndex;
					RTVDesc.Texture2DArray.ArraySize = 1;
					RTVDesc.Texture2DArray.MipSlice = MipIndex;

					ComPtr<ID3D11RenderTargetView> RenderTargetView;
					assert(S_OK == D3D11Device->CreateRenderTargetView(TextureResource.Get(), &RTVDesc, RenderTargetView.GetAddressOf()));
					RenderTargetViews.push_back(RenderTargetView);
				}
			}
			else
			{
				D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
				memset(&RTVDesc, 0, sizeof(RTVDesc));
				RTVDesc.Format = PlatformRenderTargetFormat;
				if (bTextureArray || bCubeTexture)
				{
					RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
					RTVDesc.Texture2DArray.FirstArraySlice = 0;
					RTVDesc.Texture2DArray.ArraySize = TextureDesc.ArraySize;
					RTVDesc.Texture2DArray.MipSlice = MipIndex;
				}
				else
				{
					RTVDesc.ViewDimension = RenderTargetViewDimension;
					RTVDesc.Texture2D.MipSlice = MipIndex;
				}

				ComPtr<ID3D11RenderTargetView> RenderTargetView;
				assert(S_OK == D3D11Device->CreateRenderTargetView(TextureResource.Get(), &RTVDesc, RenderTargetView.GetAddressOf()));
				RenderTargetViews.push_back(RenderTargetView);
			}
		}
	}

	if (bCreateDSV)
	{
		// Create a depth-stencil-view for the texture.
		D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc;
		memset(&DSVDesc,0, sizeof(DSVDesc));
		DSVDesc.Format = FindDepthStencilDXGIFormat(PlatformResourceFormat);
		if (bTextureArray || bCubeTexture)
		{
			DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			DSVDesc.Texture2DArray.FirstArraySlice = 0;
			DSVDesc.Texture2DArray.ArraySize = TextureDesc.ArraySize;
			DSVDesc.Texture2DArray.MipSlice = 0;
		}
		else
		{
			DSVDesc.ViewDimension = DepthStencilViewDimension;
			DSVDesc.Texture2D.MipSlice = 0;
		}

		for (uint32 AccessType = 0; AccessType < FExclusiveDepthStencil::MaxIndex; ++AccessType)
		{
			// Create a read-only access views for the texture.
			// Read-only DSVs are not supported in Feature Level 10 so 
			// a dummy DSV is created in order reduce logic complexity at a higher-level.
			if (D3D11Device->GetFeatureLevel() == D3D_FEATURE_LEVEL_11_0 || D3D11Device->GetFeatureLevel() == D3D_FEATURE_LEVEL_11_1)
			{
				DSVDesc.Flags = (AccessType & FExclusiveDepthStencil::DepthRead_StencilWrite) ? D3D11_DSV_READ_ONLY_DEPTH : 0;
				if (HasStencilBits(DSVDesc.Format))
				{
					DSVDesc.Flags |= (AccessType & FExclusiveDepthStencil::DepthWrite_StencilRead) ? D3D11_DSV_READ_ONLY_STENCIL : 0;
				}
			}
			assert(S_OK == D3D11Device->CreateDepthStencilView(TextureResource.Get(), &DSVDesc, DepthStencilViews[AccessType].GetAddressOf()));
		}
	}

	if (bCreateShaderResource)
	{
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
			SRVDesc.Format = PlatformShaderResourceFormat;

			if (bCubeTexture && bTextureArray)
			{
				SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
				SRVDesc.TextureCubeArray.MostDetailedMip = 0;
				SRVDesc.TextureCubeArray.MipLevels = NumMips;
				SRVDesc.TextureCubeArray.First2DArrayFace = 0;
				SRVDesc.TextureCubeArray.NumCubes = SizeZ / 6;
			}
			else if (bCubeTexture)
			{
				SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				SRVDesc.TextureCube.MostDetailedMip = 0;
				SRVDesc.TextureCube.MipLevels = NumMips;
			}
			else if (bTextureArray)
			{
				SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				SRVDesc.Texture2DArray.MostDetailedMip = 0;
				SRVDesc.Texture2DArray.MipLevels = NumMips;
				SRVDesc.Texture2DArray.FirstArraySlice = 0;
				SRVDesc.Texture2DArray.ArraySize = TextureDesc.ArraySize;
			}
			else
			{
				SRVDesc.ViewDimension = ShaderResourceViewDimension;
				SRVDesc.Texture2D.MostDetailedMip = 0;
				SRVDesc.Texture2D.MipLevels = NumMips;
			}
			assert(S_OK == D3D11Device->CreateShaderResourceView(TextureResource.Get(), &SRVDesc, ShaderResourceView.GetAddressOf()));
		}
	}

	std::shared_ptr<FD3D11Texture2D> Texture2D = std::make_shared<FD3D11Texture2D>
	(
		SizeX,
		SizeY,
		SizeZ,
		NumMips,
		NumSamples, 
		(EPixelFormat)Format,
		bCubeTexture,
		Flags, 
		ClearBindingValue,
		TextureResource, 
		ShaderResourceView, 
		TextureDesc.ArraySize, 
		bCreatedRTVPerSlice,
		RenderTargetViews,
		DepthStencilViews
	);

	return Texture2D;
}

ComPtr<ID3D11ShaderResourceView> RHICreateShaderResourceView(FD3D11Texture2D* Texture2DRHI, uint16 MipLevel)
{
	//todo 2darray/3d
// 	if(Texture2DRHI->)
// 	{
// 		FD3D11Texture2D* Texture2DArray = (FD3D11Texture2D*)Texture2DRHI;
// 
// 		D3D11_TEXTURE2D_DESC TextureDesc;
// 		Texture2DArray->GetResource()->GetDesc(&TextureDesc);
// 
// 		bool bSRGB = (Texture2DArray->GetFlags() & TexCreate_SRGB) != 0;
// 		const DXGI_FORMAT PlatformShaderResourceFormat = FindShaderResourceDXGIFormat(TextureDesc.Format, bSRGB);
// 
// 		// Create a Shader Resource View
// 		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
// 		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
// 		SRVDesc.Texture2DArray.MostDetailedMip = MipLevel;
// 		SRVDesc.Texture2DArray.MipLevels = 1;
// 		SRVDesc.Texture2DArray.FirstArraySlice = 0;
// 		SRVDesc.Texture2DArray.ArraySize = TextureDesc.ArraySize;
// 
// 		SRVDesc.Format = PlatformShaderResourceFormat;
// 		ComPtr<ID3D11ShaderResourceView> ShaderResourceView;
// 		assert(S_OK == D3D11Device->CreateShaderResourceView(Texture2DArray->GetResource(), &SRVDesc, ShaderResourceView.GetAddressOf()));
// 
// 		return ShaderResourceView;
// 	}
// 
	
	if (Texture2DRHI->IsCube())
	{
		FD3D11Texture2D*  TextureCube = (FD3D11Texture2D*)Texture2DRHI;

		D3D11_TEXTURE2D_DESC TextureDesc;
		TextureCube->GetResource()->GetDesc(&TextureDesc);

		bool bSRGB = (TextureCube->GetFlags() & TexCreate_SRGB) != 0;
		const DXGI_FORMAT PlatformShaderResourceFormat = FindShaderResourceDXGIFormat(TextureDesc.Format, bSRGB);

		// Create a Shader Resource View
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		SRVDesc.TextureCube.MostDetailedMip = MipLevel;
		SRVDesc.TextureCube.MipLevels = 1;

		SRVDesc.Format = PlatformShaderResourceFormat;
		ComPtr<ID3D11ShaderResourceView> ShaderResourceView;
		assert(S_OK == D3D11Device->CreateShaderResourceView(TextureCube->GetResource(), &SRVDesc, ShaderResourceView.GetAddressOf()));
		return ShaderResourceView;
	}
	else
	{
		FD3D11Texture2D* Texture2D = (FD3D11Texture2D*)Texture2DRHI;

		D3D11_TEXTURE2D_DESC TextureDesc;
		Texture2D->GetResource()->GetDesc(&TextureDesc);

		bool bSRGB = (Texture2D->GetFlags() & TexCreate_SRGB) != 0;
		const DXGI_FORMAT PlatformShaderResourceFormat = FindShaderResourceDXGIFormat(TextureDesc.Format, bSRGB);

		// Create a Shader Resource View
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MostDetailedMip = MipLevel;
		SRVDesc.Texture2D.MipLevels = 1;

		SRVDesc.Format = PlatformShaderResourceFormat;
		ComPtr<ID3D11ShaderResourceView> ShaderResourceView;
		assert(S_OK == D3D11Device->CreateShaderResourceView(Texture2D->GetResource(), &SRVDesc, ShaderResourceView.GetAddressOf()));

		return ShaderResourceView;
	}
}

ComPtr<ID3D11ShaderResourceView> RHICreateShaderResourceView(ID3D11Buffer* VertexBuffer, UINT Stride, DXGI_FORMAT Format)
{
	D3D11_BUFFER_DESC BufferDesc;
	VertexBuffer->GetDesc(&BufferDesc);

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	ZeroMemory(&SRVDesc, sizeof(SRVDesc));
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Buffer.FirstElement = 0;
	SRVDesc.Format = Format;
	SRVDesc.Buffer.NumElements = BufferDesc.ByteWidth / Stride;
	ComPtr<ID3D11ShaderResourceView> ShaderResourceView;
	assert(S_OK == D3D11Device->CreateShaderResourceView(VertexBuffer, &SRVDesc, ShaderResourceView.GetAddressOf()));

	return ShaderResourceView;
}

ComPtr<ID3D11ShaderResourceView> RHICreateShaderResourceView(FD3D11Texture3D* Texture2DRHI, uint16 MipLevel)
{
	{
		FD3D11Texture3D* Texture3D = (FD3D11Texture3D*)Texture2DRHI;

		D3D11_TEXTURE3D_DESC TextureDesc;
		Texture3D->GetResource()->GetDesc(&TextureDesc);

		bool bSRGB = (Texture3D->GetFlags() & TexCreate_SRGB) != 0;
		const DXGI_FORMAT PlatformShaderResourceFormat = FindShaderResourceDXGIFormat(TextureDesc.Format, bSRGB);

		// Create a Shader Resource View
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		SRVDesc.Texture3D.MostDetailedMip = MipLevel;
		SRVDesc.Texture3D.MipLevels = 1;

		SRVDesc.Format = PlatformShaderResourceFormat;
		ComPtr<ID3D11ShaderResourceView> ShaderResourceView;
		assert(S_OK == D3D11Device->CreateShaderResourceView(Texture3D->GetResource(), &SRVDesc, (ID3D11ShaderResourceView**)ShaderResourceView.GetAddressOf()));

		return ShaderResourceView;

	}
}

std::shared_ptr<FD3D11Texture3D> CreateD3D11Texture3D(uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, uint32 Flags, FClearValueBinding ClearBindingValue /*= FClearValueBinding::Transparent*/, const void* BulkData /*= nullptr*/, uint32 BulkDataSize /*= 0*/)
{
	const bool bSRGB = (Flags & TexCreate_SRGB) != 0;

	const DXGI_FORMAT PlatformResourceFormat = (DXGI_FORMAT)GPixelFormats[Format].PlatformFormat;
	const DXGI_FORMAT PlatformShaderResourceFormat = FindShaderResourceDXGIFormat(PlatformResourceFormat, bSRGB);
	const DXGI_FORMAT PlatformRenderTargetFormat = FindShaderResourceDXGIFormat(PlatformResourceFormat, bSRGB);

	// Describe the texture.
	D3D11_TEXTURE3D_DESC TextureDesc;
	ZeroMemory(&TextureDesc, sizeof(D3D11_TEXTURE3D_DESC));
	TextureDesc.Width = SizeX;
	TextureDesc.Height = SizeY;
	TextureDesc.Depth = SizeZ;
	TextureDesc.MipLevels = NumMips;
	TextureDesc.Format = PlatformResourceFormat;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.MiscFlags = 0;

	if (Flags & TexCreate_GenerateMipCapable)
	{
		// Set the flag that allows us to call GenerateMips on this texture later
		TextureDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}

	if (Flags & TexCreate_UAV)
	{
		TextureDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	}

	bool bCreateRTV = false;

	if (Flags & TexCreate_RenderTargetable)
	{
		TextureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		bCreateRTV = true;
	}

	// Set up the texture bind flags.
	assert(!(Flags & TexCreate_DepthStencilTargetable));
	assert(!(Flags & TexCreate_ResolveTargetable));
	assert(Flags & TexCreate_ShaderResource);

	std::vector<D3D11_SUBRESOURCE_DATA> SubResourceData;

	if (BulkData)
	{
		uint8* Data = (uint8*)BulkData;
		SubResourceData.resize(NumMips);
		uint32 MipOffset = 0;
		for (uint32 MipIndex = 0; MipIndex < NumMips; ++MipIndex)
		{
			SubResourceData[MipIndex].pSysMem = &Data[MipOffset];
			SubResourceData[MipIndex].SysMemPitch = FMath::Max<uint32>(1, SizeX >> MipIndex) * GPixelFormats[Format].BlockBytes;
			SubResourceData[MipIndex].SysMemSlicePitch = FMath::Max<uint32>(1, SizeY >> MipIndex) * SubResourceData[MipIndex].SysMemPitch;
			MipOffset += FMath::Max<uint32>(1, SizeZ >> MipIndex) * SubResourceData[MipIndex].SysMemSlicePitch;
		}
	}

	ComPtr<ID3D11Texture3D> TextureResource;
	assert(S_OK == D3D11Device->CreateTexture3D(&TextureDesc, BulkData != NULL ? (const D3D11_SUBRESOURCE_DATA*)SubResourceData.data() : NULL, TextureResource.GetAddressOf()));

	// Create a shader resource view for the texture.
	ComPtr<ID3D11ShaderResourceView> ShaderResourceView;
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = PlatformShaderResourceFormat;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		SRVDesc.Texture3D.MipLevels = NumMips;
		SRVDesc.Texture3D.MostDetailedMip = 0;
		assert(S_OK == D3D11Device->CreateShaderResourceView(TextureResource.Get(), &SRVDesc, ShaderResourceView.GetAddressOf()));
	}

	ComPtr<ID3D11RenderTargetView> RenderTargetView;
	if (bCreateRTV)
	{
		// Create a render-target-view for the texture.
		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
		memset(&RTVDesc,0, sizeof(RTVDesc));
		RTVDesc.Format = PlatformRenderTargetFormat;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
		RTVDesc.Texture3D.MipSlice = 0;
		RTVDesc.Texture3D.FirstWSlice = 0;
		RTVDesc.Texture3D.WSize = SizeZ;

		assert(S_OK == D3D11Device->CreateRenderTargetView(TextureResource.Get(), &RTVDesc, RenderTargetView.GetAddressOf()));
	}

	std::vector<ComPtr<ID3D11RenderTargetView> > RenderTargetViews;
	RenderTargetViews.push_back(RenderTargetView);
	std::shared_ptr<FD3D11Texture3D> Texture3D = std::make_shared<FD3D11Texture3D>(TextureResource.Get(), ShaderResourceView.Get(), RenderTargetViews, SizeX, SizeY, SizeZ, NumMips, (EPixelFormat)Format, Flags, ClearBindingValue);
	return Texture3D;
}

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
		D3D11Device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &NumQualityLevels);

		extern HWND g_hWind;

		DXGI_SWAP_CHAIN_DESC SwapChainDesc;
		ZeroMemory(&SwapChainDesc, sizeof SwapChainDesc);
		SwapChainDesc.BufferCount = 1;
		SwapChainDesc.BufferDesc.Width = WindowWidth;
		SwapChainDesc.BufferDesc.Height = WindowHeight;
		SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
		SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
		SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
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

	{
		ComPtr<ID3D11Texture2D> BackBufferRHI = NULL;
		ComPtr<ID3D11RenderTargetView> BackBufferRTV = NULL;

		hr = DXGISwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)BackBufferRHI.GetAddressOf());
		if (FAILED(hr))
		{
			X_LOG("GetBuffer failed!");
			return false;
		}

		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT_UNKNOWN;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		RTVDesc.Texture2D.MipSlice = 0;
		assert(S_OK == D3D11Device->CreateRenderTargetView(BackBufferRHI.Get(), &RTVDesc, BackBufferRTV.GetAddressOf()));

		D3D11_TEXTURE2D_DESC TextureDesc;
		BackBufferRHI->GetDesc(&TextureDesc);

		ComPtr<ID3D11ShaderResourceView> BackBufferShaderResourceView;
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.MipLevels = 1;
		assert(S_OK == D3D11Device->CreateShaderResourceView(BackBufferRHI.Get(), &SRVDesc, BackBufferShaderResourceView.GetAddressOf()));

		std::vector<ComPtr<ID3D11RenderTargetView> > RenderTargetViews;
		RenderTargetViews.push_back(BackBufferRTV);

		BackBuffer = new FD3D11Texture2D
		(
			TextureDesc.Width,
			TextureDesc.Height,
			1,
			1,
			1,
			PF_B8G8R8A8,
			false,
			0,
			FClearValueBinding(),
			BackBufferRHI,
			BackBufferShaderResourceView,
			1,
			false,
			RenderTargetViews,
			nullptr
		);
	}

	GVolumeRasterizeVertexBuffer.InitRHI();

	GWindowViewport.InitRHI();

	// Initialize the platform pixel format map.
	GPixelFormats[PF_Unknown].PlatformFormat = DXGI_FORMAT_UNKNOWN;
	GPixelFormats[PF_A32B32G32R32F].PlatformFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	GPixelFormats[PF_B8G8R8A8].PlatformFormat = DXGI_FORMAT_B8G8R8A8_TYPELESS;
	GPixelFormats[PF_G8].PlatformFormat = DXGI_FORMAT_R8_UNORM;
	GPixelFormats[PF_G16].PlatformFormat = DXGI_FORMAT_R16_UNORM;
	GPixelFormats[PF_DXT1].PlatformFormat = DXGI_FORMAT_BC1_TYPELESS;
	GPixelFormats[PF_DXT3].PlatformFormat = DXGI_FORMAT_BC2_TYPELESS;
	GPixelFormats[PF_DXT5].PlatformFormat = DXGI_FORMAT_BC3_TYPELESS;
	GPixelFormats[PF_BC4].PlatformFormat = DXGI_FORMAT_BC4_UNORM;
	GPixelFormats[PF_UYVY].PlatformFormat = DXGI_FORMAT_UNKNOWN;		// TODO: Not supported in D3D11
#if DEPTH_32_BIT_CONVERSION
	GPixelFormats[PF_DepthStencil].PlatformFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
	GPixelFormats[PF_DepthStencil].BlockBytes = 5;
	GPixelFormats[PF_X24_G8].PlatformFormat = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
	GPixelFormats[PF_X24_G8].BlockBytes = 5;
#else
	GPixelFormats[PF_DepthStencil].PlatformFormat = DXGI_FORMAT_R24G8_TYPELESS;
	GPixelFormats[PF_DepthStencil].BlockBytes = 4;
	GPixelFormats[PF_X24_G8].PlatformFormat = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
	GPixelFormats[PF_X24_G8].BlockBytes = 4;
#endif
	GPixelFormats[PF_ShadowDepth].PlatformFormat = DXGI_FORMAT_R16_TYPELESS;
	GPixelFormats[PF_ShadowDepth].BlockBytes = 2;
	GPixelFormats[PF_R32_FLOAT].PlatformFormat = DXGI_FORMAT_R32_FLOAT;
	GPixelFormats[PF_G16R16].PlatformFormat = DXGI_FORMAT_R16G16_UNORM;
	GPixelFormats[PF_G16R16F].PlatformFormat = DXGI_FORMAT_R16G16_FLOAT;
	GPixelFormats[PF_G16R16F_FILTER].PlatformFormat = DXGI_FORMAT_R16G16_FLOAT;
	GPixelFormats[PF_G32R32F].PlatformFormat = DXGI_FORMAT_R32G32_FLOAT;
	GPixelFormats[PF_A2B10G10R10].PlatformFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	GPixelFormats[PF_A16B16G16R16].PlatformFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
	GPixelFormats[PF_D24].PlatformFormat = DXGI_FORMAT_R24G8_TYPELESS;
	GPixelFormats[PF_R16F].PlatformFormat = DXGI_FORMAT_R16_FLOAT;
	GPixelFormats[PF_R16F_FILTER].PlatformFormat = DXGI_FORMAT_R16_FLOAT;

	GPixelFormats[PF_FloatRGB].PlatformFormat = DXGI_FORMAT_R11G11B10_FLOAT;
	GPixelFormats[PF_FloatRGB].BlockBytes = 4;
	GPixelFormats[PF_FloatRGBA].PlatformFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	GPixelFormats[PF_FloatRGBA].BlockBytes = 8;

	GPixelFormats[PF_FloatR11G11B10].PlatformFormat = DXGI_FORMAT_R11G11B10_FLOAT;
	GPixelFormats[PF_FloatR11G11B10].BlockBytes = 4;
	GPixelFormats[PF_FloatR11G11B10].Supported = true;

	GPixelFormats[PF_V8U8].PlatformFormat = DXGI_FORMAT_R8G8_SNORM;
	GPixelFormats[PF_BC5].PlatformFormat = DXGI_FORMAT_BC5_UNORM;
	GPixelFormats[PF_A1].PlatformFormat = DXGI_FORMAT_R1_UNORM; // Not supported for rendering.
	GPixelFormats[PF_A8].PlatformFormat = DXGI_FORMAT_A8_UNORM;
	GPixelFormats[PF_R32_UINT].PlatformFormat = DXGI_FORMAT_R32_UINT;
	GPixelFormats[PF_R32_SINT].PlatformFormat = DXGI_FORMAT_R32_SINT;

	GPixelFormats[PF_R16_UINT].PlatformFormat = DXGI_FORMAT_R16_UINT;
	GPixelFormats[PF_R16_SINT].PlatformFormat = DXGI_FORMAT_R16_SINT;
	GPixelFormats[PF_R16G16B16A16_UINT].PlatformFormat = DXGI_FORMAT_R16G16B16A16_UINT;
	GPixelFormats[PF_R16G16B16A16_SINT].PlatformFormat = DXGI_FORMAT_R16G16B16A16_SINT;

	GPixelFormats[PF_R5G6B5_UNORM].PlatformFormat = DXGI_FORMAT_B5G6R5_UNORM;
	GPixelFormats[PF_R8G8B8A8].PlatformFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
	GPixelFormats[PF_R8G8B8A8_UINT].PlatformFormat = DXGI_FORMAT_R8G8B8A8_UINT;
	GPixelFormats[PF_R8G8B8A8_SNORM].PlatformFormat = DXGI_FORMAT_R8G8B8A8_SNORM;
	GPixelFormats[PF_R8G8].PlatformFormat = DXGI_FORMAT_R8G8_UNORM;
	GPixelFormats[PF_R32G32B32A32_UINT].PlatformFormat = DXGI_FORMAT_R32G32B32A32_UINT;
	GPixelFormats[PF_R16G16_UINT].PlatformFormat = DXGI_FORMAT_R16G16_UINT;

	GPixelFormats[PF_BC6H].PlatformFormat = DXGI_FORMAT_BC6H_UF16;
	GPixelFormats[PF_BC7].PlatformFormat = DXGI_FORMAT_BC7_TYPELESS;
	GPixelFormats[PF_R8_UINT].PlatformFormat = DXGI_FORMAT_R8_UINT;

	GPixelFormats[PF_R16G16B16A16_UNORM].PlatformFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
	GPixelFormats[PF_R16G16B16A16_SNORM].PlatformFormat = DXGI_FORMAT_R16G16B16A16_SNORM;

	{
		FColor Black(0, 0, 0, 0);
		GBlackTexture = CreateTexture2D(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, &Black);
		GBlackTextureSRV = CreateShaderResourceView2D(GBlackTexture, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0);
		GBlackTextureSamplerState = TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::GetRHI();
	}

	{
		FColor White(255, 255, 255, 255);
		GWhiteTexture = CreateTexture2D(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, &White);
		GWhiteTextureSRV = CreateShaderResourceView2D(GBlackTexture, DXGI_FORMAT_R8G8B8A8_UNORM,1, 0);
		GWhiteTextureSamplerState = TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::GetRHI();
	}

	{
		FColor Black(0, 0, 0, 0);
		GBlackVolumeTexture = CreateTexture3D(1, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, &Black);
		GBlackVolumeTextureSRV = CreateShaderResourceView3D(GBlackVolumeTexture, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0);
		GBlackVolumeTextureSamplerState = TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::GetRHI();
	}

	{
		FColor White[6] = { { 255, 255, 255, 255 } };
		GWhiteTextureCube = CreateTextureCube(1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, (uint8*)White);
		GWhiteTextureCubeSRV = CreateShaderResourceViewCube(GWhiteTextureCube, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0);
		GWhiteTextureCubeSamplerState = TStaticSamplerState<D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::GetRHI();
	}

	{
		FColor Black[6] = { { 0, 0, 0, 0 } };
		GBlackTextureDepthCube = RHICreateTextureCube(1, PF_ShadowDepth, 1, 0, FClearValueBinding::Transparent, Black, sizeof(Black));
	}
	{
		D3D11_BUFFER_DESC VertexDesc;
		ZeroMemory(&VertexDesc, sizeof(VertexDesc));
		VertexDesc.ByteWidth = 1024;
		VertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		VertexDesc.Usage = D3D11_USAGE_DYNAMIC;
		VertexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		VertexDesc.MiscFlags = 0;
		assert(S_OK == D3D11Device->CreateBuffer(&VertexDesc, NULL, DynamicVB.GetAddressOf()));
	}
	{
		D3D11_BUFFER_DESC IndexDesc;
		ZeroMemory(&IndexDesc, sizeof(IndexDesc));
		IndexDesc.Usage = D3D11_USAGE_DYNAMIC;
		IndexDesc.ByteWidth = 1024;
		IndexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		IndexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		IndexDesc.MiscFlags = 0;

		assert(S_OK == D3D11Device->CreateBuffer(&IndexDesc, NULL, DynamicIB.GetAddressOf()));
	}
	{
		uint32 Vertices[4];
		Vertices[0] = FColor(255, 255, 255, 255).DWColor();
		Vertices[1] = FColor(255, 255, 255, 255).DWColor();
		Vertices[2] = FColor(255, 255, 255, 255).DWColor();
		Vertices[3] = FColor(255, 255, 255, 255).DWColor();
		GNullColorVertexBuffer = RHICreateVertexBuffer(sizeof(uint32) * 4, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0 ,Vertices);
		GNullColorVertexBufferSRV = RHICreateShaderResourceView(GNullColorVertexBuffer.Get(), sizeof(FColor), DXGI_FORMAT_R8G8B8A8_UNORM);

	}
	return true;
}

//DO NOT USE THE STATIC FLINEARCOLORS TO INITIALIZE THIS STUFF.  
//Static init order is undefined and you will likely end up with bad values on some platforms.
const FClearValueBinding FClearValueBinding::None(EClearBinding::ENoneBound);
const FClearValueBinding FClearValueBinding::Black(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
const FClearValueBinding FClearValueBinding::White(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
const FClearValueBinding FClearValueBinding::Transparent(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
const FClearValueBinding FClearValueBinding::DepthOne(1.0f, 0);
const FClearValueBinding FClearValueBinding::DepthZero(0.0f, 0);
const FClearValueBinding FClearValueBinding::DepthNear((float)ERHIZBuffer::NearPlane, 0);
const FClearValueBinding FClearValueBinding::DepthFar((float)ERHIZBuffer::FarPlane, 0);
const FClearValueBinding FClearValueBinding::Green(FLinearColor(0.0f, 1.0f, 0.0f, 1.0f));
// Note: this is used as the default normal for DBuffer decals.  It must decode to a value of 0 in DecodeDBufferData.
const FClearValueBinding FClearValueBinding::DefaultNormal8Bit(FLinearColor(128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 1.0f));

char VSConstBuffer[4096];
uint32 VSBufferIndex;
char HSConstBuffer[4096];
uint32 HSBufferIndex;
char DSConstBuffer[4096];
uint32 DSBufferIndex;
char GSConstBuffer[4096];
uint32 GSBufferIndex;
char PSConstBuffer[4096];
uint32 PSBufferIndex;

FD3D11ConstantBuffer::FD3D11ConstantBuffer(uint32 InSize /*= 0*/, uint32 SubBuffers /*= 1*/)
	:MaxSize(InSize)
{
	CurrentUpdateSize = 0;
	TotalUpdateSize = 0;
}

FD3D11ConstantBuffer::~FD3D11ConstantBuffer()
{

}

void FD3D11ConstantBuffer::InitDynamicRHI()
{
	D3D11_BUFFER_DESC BufferDesc;
	ZeroMemory(&BufferDesc, sizeof(D3D11_BUFFER_DESC));
	// Verify constant buffer ByteWidth requirements
	//check(MaxSize <= D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT && (MaxSize % 16) == 0);
	BufferDesc.ByteWidth = MaxSize;

	BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.MiscFlags = 0;

	assert(S_OK == D3D11Device->CreateBuffer(&BufferDesc,NULL, ConstantBufferRHI.GetAddressOf()));

	ShadowData = (uint8*)_aligned_malloc(MaxSize,16);
}

void FD3D11ConstantBuffer::ReleaseDynamicRHI()
{
	_aligned_free(ShadowData);
}

bool FD3D11ConstantBuffer::CommitConstantsToDevice(bool bDiscardSharedConstants)
{
	if (CurrentUpdateSize == 0)
	{
		return false;
	}

	if (bDiscardSharedConstants)
	{
		// If we're discarding shared constants, just use constants that have been updated since the last Commit.
		TotalUpdateSize = CurrentUpdateSize;
	}
	else
	{
		// If we're re-using shared constants, use all constants.
		TotalUpdateSize = FMath::Max(CurrentUpdateSize, TotalUpdateSize);
	}

	D3D11DeviceContext->UpdateSubresource(ConstantBufferRHI.Get(), 0, NULL, (void*)ShadowData, MaxSize, MaxSize);

	CurrentUpdateSize = 0;
	return true;
}

std::shared_ptr<FD3D11ConstantBuffer> VSConstantBuffer;
std::shared_ptr<FD3D11ConstantBuffer> HSConstantBuffer;
std::shared_ptr<FD3D11ConstantBuffer> DSConstantBuffer;
std::shared_ptr<FD3D11ConstantBuffer> GSConstantBuffer;
std::shared_ptr<FD3D11ConstantBuffer> PSConstantBuffer;

void RHISetShaderParameter(ID3D11VertexShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue)
{
	assert(0 == BufferIndex);
	VSConstantBuffer->UpdateConstant((uint8*)NewValue, BaseIndex, NumBytes);
}
void RHISetShaderParameter(ID3D11HullShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue)
{
	assert(0 == BufferIndex);
	HSConstantBuffer->UpdateConstant((uint8*)NewValue, BaseIndex, NumBytes);
}
void RHISetShaderParameter(ID3D11DomainShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue)
{
	assert(0 == BufferIndex);
	DSConstantBuffer->UpdateConstant((uint8*)NewValue, BaseIndex, NumBytes);
}
void RHISetShaderParameter(ID3D11GeometryShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue)
{
	assert(0 == BufferIndex);
	GSConstantBuffer->UpdateConstant((uint8*)NewValue, BaseIndex, NumBytes);
}
void RHISetShaderParameter(ID3D11PixelShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue)
{
	assert(0 == BufferIndex);
	PSConstantBuffer->UpdateConstant((uint8*)NewValue, BaseIndex, NumBytes);
}

ComPtr<ID3D11Buffer> BoundUniformBuffers[6][14];

void InitConstantBuffers()
{
	VSConstantBuffer = std::make_shared<FD3D11ConstantBuffer>(4096);
	VSConstantBuffer->InitDynamicRHI();
	HSConstantBuffer = std::make_shared<FD3D11ConstantBuffer>(4096);
	HSConstantBuffer->InitDynamicRHI();
	DSConstantBuffer = std::make_shared<FD3D11ConstantBuffer>(4096);
	DSConstantBuffer->InitDynamicRHI();
	GSConstantBuffer = std::make_shared<FD3D11ConstantBuffer>(4096);
	GSConstantBuffer->InitDynamicRHI();
	PSConstantBuffer = std::make_shared<FD3D11ConstantBuffer>(4096);
	PSConstantBuffer->InitDynamicRHI();
}

void CommitNonComputeShaderConstants()
{
	if (VSConstantBuffer->CommitConstantsToDevice(false))
	{
		ID3D11Buffer* ConstantBuffer = VSConstantBuffer->GetConstantBuffer();
		D3D11DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
	}
	if (HSConstantBuffer->CommitConstantsToDevice(false))
	{
		ID3D11Buffer* ConstantBuffer = HSConstantBuffer->GetConstantBuffer();
		D3D11DeviceContext->HSSetConstantBuffers(0, 1, &ConstantBuffer);
	}
	if (DSConstantBuffer->CommitConstantsToDevice(false))
	{
		ID3D11Buffer* ConstantBuffer = DSConstantBuffer->GetConstantBuffer();
		D3D11DeviceContext->DSSetConstantBuffers(0, 1, &ConstantBuffer);
	}
	if (GSConstantBuffer->CommitConstantsToDevice(false))
	{
		ID3D11Buffer* ConstantBuffer = GSConstantBuffer->GetConstantBuffer();
		D3D11DeviceContext->GSSetConstantBuffers(0, 1, &ConstantBuffer);
	}
	if (PSConstantBuffer->CommitConstantsToDevice(false))
	{
		ID3D11Buffer* ConstantBuffer = PSConstantBuffer->GetConstantBuffer();
		D3D11DeviceContext->PSSetConstantBuffers(0, 1, &ConstantBuffer);
	}
}
uint32 PendingNumVertices;
uint32 PendingVertexDataStride;
D3D11_PRIMITIVE_TOPOLOGY PendingPrimitiveType;
uint32 PendingNumPrimitives;
uint32 PendingMinVertexIndex;
uint32 PendingNumIndices;
uint32 PendingIndexDataStride;

void RHIBeginDrawPrimitiveUP(D3D11_PRIMITIVE_TOPOLOGY PrimitiveType, uint32 NumPrimitives, uint32 NumVertices, uint32 VertexDataStride, void*& OutVertexData)
{
	assert(PendingNumVertices == 0);

	// Remember the parameters for the draw call.
	PendingPrimitiveType = PrimitiveType;
	PendingNumPrimitives = NumPrimitives;
	PendingNumVertices = NumVertices;
	PendingVertexDataStride = VertexDataStride;


	D3D11_MAPPED_SUBRESOURCE MapedSubresource;
	D3D11DeviceContext->Map(DynamicVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0,&MapedSubresource);
	OutVertexData = MapedSubresource.pData;
}

void RHIEndDrawPrimitiveUP()
{
	UINT Stride = sizeof(Vector4);
	UINT Offset = 0;
	D3D11DeviceContext->Unmap(DynamicVB.Get(),0);

	CommitNonComputeShaderConstants();

	D3D11DeviceContext->IASetVertexBuffers(0, 1, DynamicVB.GetAddressOf(), &Stride, &Offset);
	D3D11DeviceContext->IASetPrimitiveTopology(PendingPrimitiveType);
	D3D11DeviceContext->Draw(PendingNumVertices, 0);

	PendingPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	PendingNumPrimitives = 0;
	PendingNumVertices = 0;
	PendingVertexDataStride = 0;
}
inline uint32 GetVertexCountForPrimitiveCount(uint32 NumPrimitives, D3D11_PRIMITIVE_TOPOLOGY PrimitiveType)
{
	uint32 VertexCount = 0;
	switch (PrimitiveType)
	{
	case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST: VertexCount = NumPrimitives * 3; break;
	case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP: VertexCount = NumPrimitives + 2; break;
	case D3D11_PRIMITIVE_TOPOLOGY_LINELIST: VertexCount = NumPrimitives * 2; break;
	case D3D11_PRIMITIVE_TOPOLOGY_POINTLIST: VertexCount = NumPrimitives; break;
	//default: X_LOG("Unknown primitive type: %u", PrimitiveType);
	};
	return VertexCount;
}
inline void DrawPrimitiveUP(D3D11_PRIMITIVE_TOPOLOGY PrimitiveType, uint32 NumPrimitives, const void* VertexData, uint32 VertexDataStride)
{
	void* Buffer = NULL;
	assert(NumPrimitives > 0);
	const uint32 VertexCount = GetVertexCountForPrimitiveCount(NumPrimitives, PrimitiveType);
	RHIBeginDrawPrimitiveUP(PrimitiveType, NumPrimitives, VertexCount, VertexDataStride, Buffer);
	memcpy(Buffer, VertexData, VertexCount * VertexDataStride);
	RHIEndDrawPrimitiveUP();
}

void RHIBeginDrawIndexedPrimitiveUP(
	D3D11_PRIMITIVE_TOPOLOGY PrimitiveType,
	uint32 NumPrimitives, 
	uint32 NumVertices, 
	uint32 VertexDataStride, 
	void*& OutVertexData, 
	uint32 MinVertexIndex, 
	uint32 NumIndices, 
	uint32 IndexDataStride, 
	void*& OutIndexData)
{
	PendingPrimitiveType = PrimitiveType;
	PendingNumPrimitives = NumPrimitives;
	PendingMinVertexIndex = MinVertexIndex;
	PendingIndexDataStride = IndexDataStride;
	PendingNumVertices = NumVertices;
	PendingNumIndices = NumIndices;
	PendingVertexDataStride = VertexDataStride;

	// Map dynamic vertex and index buffers.
	D3D11_MAPPED_SUBRESOURCE VBMapedSubresource;
	D3D11DeviceContext->Map(DynamicVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &VBMapedSubresource);
	OutVertexData = VBMapedSubresource.pData;
	D3D11_MAPPED_SUBRESOURCE IBMapedSubresource;
	D3D11DeviceContext->Map(DynamicIB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &IBMapedSubresource);
	OutIndexData = IBMapedSubresource.pData;
}


void RHIEndDrawIndexedPrimitiveUP()
{
	D3D11DeviceContext->Unmap(DynamicVB.Get(), 0);
	D3D11DeviceContext->Unmap(DynamicIB.Get(), 0);

	CommitNonComputeShaderConstants();

	UINT Offset = 0;
	D3D11DeviceContext->IASetVertexBuffers(0, 1, DynamicVB.GetAddressOf(), &PendingVertexDataStride, &Offset);
	D3D11DeviceContext->IASetIndexBuffer(DynamicIB.Get(), PendingIndexDataStride == sizeof(uint16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
	D3D11DeviceContext->IASetPrimitiveTopology(PendingPrimitiveType);
	D3D11DeviceContext->DrawIndexed(PendingNumIndices, 0, 0);
	ClearRenderState();

	PendingPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	PendingNumPrimitives = 0;
	PendingMinVertexIndex = 0;
	PendingIndexDataStride = 0;
	PendingNumVertices = 0;
	PendingNumIndices = 0;
	PendingVertexDataStride = 0;
}

void DrawIndexedPrimitiveUP(D3D11_PRIMITIVE_TOPOLOGY PrimitiveType, uint32 MinVertexIndex, uint32 NumVertices, uint32 NumPrimitives, const void* IndexData, uint32 IndexDataStride, const void* VertexData, uint32 VertexDataStride)
{
	void* VertexBuffer = NULL;
	void* IndexBuffer = NULL;
	const uint32 NumIndices = GetVertexCountForPrimitiveCount(NumPrimitives, PrimitiveType);
	RHIBeginDrawIndexedPrimitiveUP(
		PrimitiveType,
		NumPrimitives,
		NumVertices,
		VertexDataStride,
		VertexBuffer,
		MinVertexIndex,
		NumIndices,
		IndexDataStride,
		IndexBuffer);
	memcpy(VertexBuffer, VertexData, NumVertices * VertexDataStride);
	memcpy(IndexBuffer, IndexData, NumIndices * IndexDataStride);
	RHIEndDrawIndexedPrimitiveUP();
}


ID3D11InputLayout* GetInputLayout(const std::vector<D3D11_INPUT_ELEMENT_DESC>* InputDecl, ID3DBlob* VSCode)
{
	assert(VSCode && InputDecl);

	static std::map<const std::vector<D3D11_INPUT_ELEMENT_DESC>*, std::map<ID3DBlob*, ComPtr<ID3D11InputLayout>>> InputLayoutCache;
	auto InternalMapIt = InputLayoutCache.find(InputDecl);
	if (InternalMapIt != InputLayoutCache.end())
	{
		std::map<ID3DBlob*, ComPtr<ID3D11InputLayout>>& InternalMap = InternalMapIt->second;
		auto It = InternalMap.find(VSCode);
		if (It != InternalMap.end())
		{
			return It->second.Get();
		}
	}
	
	{
		ComPtr<ID3D11InputLayout> InputLayout;
		D3D11Device->CreateInputLayout(InputDecl->data(), InputDecl->size(), VSCode->GetBufferPointer(), VSCode->GetBufferSize(), InputLayout.GetAddressOf());
		InputLayoutCache[InputDecl][VSCode] = InputLayout;
		return InputLayout.Get();
	}
}

void DrawRectangle(float X, float Y, float SizeX, float SizeY, float U, float V, float SizeU, float SizeV, FIntPoint TargetSize, FIntPoint TextureSize, FShader* VertexShader, uint32 InstanceCount)
{
	CommitNonComputeShaderConstants();

	FDrawRectangleParameters Parameters;
	Parameters.Constants.PosScaleBias = Vector4(SizeX, SizeY, X, Y);
	Parameters.Constants.UVScaleBias = Vector4(SizeU, SizeV, U, V);

	Parameters.Constants.InvTargetSizeAndTextureSize = Vector4(
		1.0f / TargetSize.X, 1.0f / TargetSize.Y,
		1.0f / TextureSize.X, 1.0f / TextureSize.Y);

	SetUniformBufferParameterImmediate(VertexShader->GetVertexShader(), VertexShader->GetUniformBufferParameter<FDrawRectangleParameters>(), Parameters);

	UINT Stride = sizeof(FilterVertex);
	UINT Offset = 0;
	D3D11DeviceContext->IASetVertexBuffers(0, 1, &GScreenRectangleVertexBuffer, &Stride, &Offset);
	D3D11DeviceContext->IASetIndexBuffer(GScreenRectangleIndexBuffer,DXGI_FORMAT_R16_UINT,0);

	D3D11DeviceContext->DrawIndexed(6, 0, 0);
	ClearRenderState();
}

int32 GMaxShadowDepthBufferSizeX = 2048;
int32 GMaxShadowDepthBufferSizeY = 2048;

void RHISetViewport(uint32 MinX, uint32 MinY, float MinZ, uint32 MaxX, uint32 MaxY, float MaxZ)
{
	D3D11_VIEWPORT VP = { (FLOAT)MinX ,(FLOAT)MinY ,(FLOAT)MaxX - (FLOAT)MinX,(FLOAT)MaxY - (FLOAT)MinY, MinZ ,MaxZ };
	D3D11DeviceContext->RSSetViewports(1, &VP);

	D3D11_RECT ScissorRect;
	ScissorRect.left = MinX;
	ScissorRect.right = MaxX;
	ScissorRect.top = MinY;
	ScissorRect.bottom = MaxY;
	D3D11DeviceContext->RSSetScissorRects(1,&ScissorRect);

}

void RHISetScissorRect(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY)
{
	if (bEnable)
	{
		D3D11_RECT ScissorRect;
		ScissorRect.left = MinX;
		ScissorRect.right = MaxX;
		ScissorRect.top = MinY;
		ScissorRect.bottom = MaxY;
		D3D11DeviceContext->RSSetScissorRects(1, &ScissorRect);
	}
	else
	{
		D3D11_RECT ScissorRect;
		ScissorRect.left = 0;
		ScissorRect.right = 2048;
		ScissorRect.top = 0;
		ScissorRect.bottom = 2048;
		D3D11DeviceContext->RSSetScissorRects(1, &ScissorRect);
	}
}

std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>>& GetVertexDeclarationFVector4()
{
	static std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>> GVector4VertexDeclaration = std::make_shared<std::vector<D3D11_INPUT_ELEMENT_DESC>, std::vector<D3D11_INPUT_ELEMENT_DESC>::size_type, D3D11_INPUT_ELEMENT_DESC>
		(
			1,{ "ATTRIBUTE",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0, D3D11_INPUT_PER_VERTEX_DATA,0 }
		);
	return GVector4VertexDeclaration;
}

std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>>& GetScreenVertexDeclaration()
{
	static std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>> GScreenVertexDeclaration = std::make_shared<std::vector<D3D11_INPUT_ELEMENT_DESC>, std::initializer_list<D3D11_INPUT_ELEMENT_DESC>>
		(
			{
				{ "ATTRIBUTE",	0,	DXGI_FORMAT_R32G32_FLOAT,	0, 0,   D3D11_INPUT_PER_VERTEX_DATA,0 },
				{ "ATTRIBUTE",	1,	DXGI_FORMAT_R32G32_FLOAT,	0, 8,	D3D11_INPUT_PER_VERTEX_DATA,0 },
			}
	);
	return GScreenVertexDeclaration;
}

void ClearQuadSetup(bool bClearColor, int32 NumClearColors, const FLinearColor* ClearColorArray, bool bClearDepth, float Depth, bool bClearStencil, uint32 Stencil)
{
	// Set new states
	ID3D11BlendState* BlendStateRHI;

	BlendStateRHI = bClearColor ? TStaticBlendState<>::GetRHI() : TStaticBlendStateWriteMask<0, 0, 0, 0, 0, 0, 0, 0>::GetRHI();

	ID3D11DepthStencilState* const DepthStencilStateRHI =
		(bClearDepth && bClearStencil)
			? TStaticDepthStencilState<
				true, D3D11_COMPARISON_ALWAYS,
				true, D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE,
				false, D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE,
				0xff, 0xff
					>::GetRHI()
					: bClearDepth
						? TStaticDepthStencilState<true, D3D11_COMPARISON_ALWAYS>::GetRHI()
						: bClearStencil
							? TStaticDepthStencilState<
									false, D3D11_COMPARISON_ALWAYS,
									true, D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE,
									false, D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE,
									0xff, 0xff
							>::GetRHI() : TStaticDepthStencilState<false, D3D11_COMPARISON_ALWAYS>::GetRHI();

	//FGraphicsPipelineStateInitializer GraphicsPSOInit;
	//RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

	D3D11DeviceContext->RSSetState(TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_NONE>::GetRHI());
	D3D11DeviceContext->OMSetBlendState(BlendStateRHI, NULL, 0xffffffff);
	D3D11DeviceContext->OMSetDepthStencilState(DepthStencilStateRHI, Stencil);

	auto ShaderMap = GetGlobalShaderMap();


	// Set the new shaders
	TShaderMapRef<TOneColorVS<true> > VertexShader(ShaderMap);

	FOneColorPS* PixelShader = NULL;

	// Set the shader to write to the appropriate number of render targets
	// On AMD PC hardware, outputting to a color index in the shader without a matching render target set has a significant performance hit
	if (NumClearColors <= 1)
	{
		TShaderMapRef<TOneColorPixelShaderMRT<1> > MRTPixelShader(ShaderMap);
		PixelShader = *MRTPixelShader;
	}
	else if (NumClearColors == 2)
	{
		TShaderMapRef<TOneColorPixelShaderMRT<2> > MRTPixelShader(ShaderMap);
		PixelShader = *MRTPixelShader;
	}
	else if (NumClearColors == 3)
	{
		TShaderMapRef<TOneColorPixelShaderMRT<3> > MRTPixelShader(ShaderMap);
		PixelShader = *MRTPixelShader;
	}
	else if (NumClearColors == 4)
	{
		TShaderMapRef<TOneColorPixelShaderMRT<4> > MRTPixelShader(ShaderMap);
		PixelShader = *MRTPixelShader;
	}
	else if (NumClearColors == 5)
	{
		TShaderMapRef<TOneColorPixelShaderMRT<5> > MRTPixelShader(ShaderMap);
		PixelShader = *MRTPixelShader;
	}
	else if (NumClearColors == 6)
	{
		TShaderMapRef<TOneColorPixelShaderMRT<6> > MRTPixelShader(ShaderMap);
		PixelShader = *MRTPixelShader;
	}
	else if (NumClearColors == 7)
	{
		TShaderMapRef<TOneColorPixelShaderMRT<7> > MRTPixelShader(ShaderMap);
		PixelShader = *MRTPixelShader;
	}
	else if (NumClearColors == 8)
	{
		TShaderMapRef<TOneColorPixelShaderMRT<8> > MRTPixelShader(ShaderMap);
		PixelShader = *MRTPixelShader;
	}

	ID3D11InputLayout* InputLayout = GetInputLayout(GetVertexDeclarationFVector4().get(), VertexShader->GetCode().Get());
	D3D11DeviceContext->IASetInputLayout(InputLayout);
	//GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
	//GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	//GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(PixelShader);
	//GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;

	D3D11DeviceContext->VSSetShader(VertexShader->GetVertexShader(), 0, 0);
	D3D11DeviceContext->PSSetShader(PixelShader->GetPixelShader(), 0, 0);
	D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
	//RHICmdList.SetStencilRef(Stencil);

	PixelShader->SetColors(ClearColorArray, NumClearColors);
}
void DrawClearQuadMRT(bool bClearColor, int32 NumClearColors, const FLinearColor* ClearColorArray, bool bClearDepth, float Depth, bool bClearStencil, uint32 Stencil)
{
	ClearQuadSetup(bClearColor, NumClearColors, ClearColorArray, bClearDepth, Depth, bClearStencil, Stencil);

	Vector4 Vertices[4];
	Vertices[0].Set(-1.0f, 1.0f, Depth, 1.0f);
	Vertices[1].Set(1.0f, 1.0f, Depth, 1.0f);
	Vertices[2].Set(-1.0f, -1.0f, Depth, 1.0f);
	Vertices[3].Set(1.0f, -1.0f, Depth, 1.0f);

	DrawPrimitiveUP(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, Vertices, sizeof(Vertices[0]));
}

void DrawClearQuadMRT(bool bClearColor, int32 NumClearColors, const FLinearColor* ClearColorArray, bool bClearDepth, float Depth, bool bClearStencil, uint32 Stencil, FIntPoint ViewSize, FIntRect ExcludeRect)
{

}


void RHIReadSurfaceFloatData(FD3D11Texture* TextureRHI, FIntRect InRect, std::vector<FFloat16Color>& OutData, ECubeFace CubeFace, int32 ArrayIndex, int32 MipIndex)
{
	FD3D11Texture2D* Texture = /*GetD3D11TextureFromRHITexture*/(FD3D11Texture2D*)(TextureRHI);

	uint32 SizeX = InRect.Width();
	uint32 SizeY = InRect.Height();

	// Check the format of the surface
	D3D11_TEXTURE2D_DESC TextureDesc;
	((ID3D11Texture2D*)Texture->GetResource())->GetDesc(&TextureDesc);

	assert(TextureDesc.Format == GPixelFormats[PF_FloatRGBA].PlatformFormat);

	// Allocate the output buffer.
	OutData.clear();
	OutData.resize(SizeX * SizeY);

	// Read back the surface data from defined rect
	D3D11_BOX	Rect;
	Rect.left = InRect.Min.X;
	Rect.top = InRect.Min.Y;
	Rect.right = InRect.Max.X;
	Rect.bottom = InRect.Max.Y;
	Rect.back = 1;
	Rect.front = 0;

	// create a temp 2d texture to copy render target to
	D3D11_TEXTURE2D_DESC Desc;
	ZeroMemory(&Desc, sizeof(D3D11_TEXTURE2D_DESC));
	Desc.Width = SizeX;
	Desc.Height = SizeY;
	Desc.MipLevels = 1;
	Desc.ArraySize = 1;
	Desc.Format = TextureDesc.Format;
	Desc.SampleDesc.Count = 1;
	Desc.SampleDesc.Quality = 0;
	Desc.Usage = D3D11_USAGE_STAGING;
	Desc.BindFlags = 0;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	Desc.MiscFlags = 0;
	ComPtr<ID3D11Texture2D> TempTexture2D;
	assert(S_OK == D3D11Device->CreateTexture2D(&Desc, NULL, TempTexture2D.GetAddressOf()));

	// Copy the data to a staging resource.
	uint32 Subresource = 0;
	if (TextureDesc.MiscFlags == D3D11_RESOURCE_MISC_TEXTURECUBE)
	{
		uint32 D3DFace = GetD3D11CubeFace(CubeFace);
		Subresource = D3D11CalcSubresource(MipIndex, ArrayIndex * 6 + D3DFace, TextureDesc.MipLevels);
	}
	D3D11DeviceContext->CopySubresourceRegion(TempTexture2D.Get(), 0, 0, 0, 0, Texture->GetResource(), Subresource, &Rect);

	// Lock the staging resource.
	D3D11_MAPPED_SUBRESOURCE LockedRect;
	assert(S_OK == D3D11DeviceContext->Map(TempTexture2D.Get(), 0, D3D11_MAP_READ, 0, &LockedRect));

	// Presize the array
	uint32 TotalCount = SizeX * SizeY;
	if (TotalCount >= OutData.size())
	{
		OutData.resize(TotalCount);
	}

	for (int32 Y = InRect.Min.Y; Y < InRect.Max.Y; Y++)
	{
		FFloat16Color* SrcPtr = (FFloat16Color*)((uint8*)LockedRect.pData + (Y - InRect.Min.Y) * LockedRect.RowPitch);
		int32 Index = (Y - InRect.Min.Y) * SizeX;
		assert(Index + ((int32)SizeX - 1) < (int32)OutData.size());
		FFloat16Color* DestColor = &OutData[Index];
		FFloat16* DestPtr = (FFloat16*)(DestColor);
		memcpy(DestPtr, SrcPtr, SizeX * sizeof(FFloat16) * 4);
	}

	D3D11DeviceContext->Unmap(TempTexture2D.Get(), 0);
}
static inline DXGI_FORMAT ConvertTypelessToUnorm(DXGI_FORMAT Format)
{
	// required to prevent 
	// D3D11: ERROR: ID3D11DeviceContext::ResolveSubresource: The Format (0x1b, R8G8B8A8_TYPELESS) is never able to resolve multisampled resources. [ RESOURCE_MANIPULATION ERROR #294: DEVICE_RESOLVESUBRESOURCE_FORMAT_INVALID ]
	// D3D11: **BREAK** enabled for the previous D3D11 message, which was: [ RESOURCE_MANIPULATION ERROR #294: DEVICE_RESOLVESUBRESOURCE_FORMAT_INVALID ]
	switch (Format)
	{
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		return DXGI_FORMAT_R8G8B8A8_UNORM;

	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		return DXGI_FORMAT_B8G8R8A8_UNORM;

	default:
		return Format;
	}
}
void RHICopyToResolveTarget(FD3D11Texture* SourceTextureRHI, FD3D11Texture* DestTextureRHI, const FResolveParams& ResolveParams)
{
	if (!SourceTextureRHI || !DestTextureRHI)
	{
		// no need to do anything (sliently ignored)
		return;
	}


	FD3D11Texture2D* SourceTexture2D = (FD3D11Texture2D*) SourceTextureRHI;
	FD3D11Texture2D* DestTexture2D = (FD3D11Texture2D*)DestTextureRHI;

	// 	FD3D11TextureCube* SourceTextureCube = static_cast<FD3D11TextureCube*>(SourceTextureRHI->GetTextureCube());
	// 	FD3D11TextureCube* DestTextureCube = static_cast<FD3D11TextureCube*>(DestTextureRHI->GetTextureCube());
	// 
	// 	FD3D11Texture3D* SourceTexture3D = static_cast<FD3D11Texture3D*>(SourceTextureRHI->GetTexture3D());
	// 	FD3D11Texture3D* DestTexture3D = static_cast<FD3D11Texture3D*>(DestTextureRHI->GetTexture3D());

	if (SourceTexture2D && DestTexture2D)
	{
		//assert(!SourceTextureCube && !DestTextureCube);
		if (SourceTexture2D != DestTexture2D)
		{
			if (DestTexture2D->GetDepthStencilView(FExclusiveDepthStencil::DepthWrite_StencilWrite)
				&& SourceTextureRHI->IsMultisampled()
				&& !DestTextureRHI->IsMultisampled())
			{
			}
			else
			{
				DXGI_FORMAT SrcFmt = (DXGI_FORMAT)GPixelFormats[SourceTextureRHI->GetFormat()].PlatformFormat;
				DXGI_FORMAT DstFmt = (DXGI_FORMAT)GPixelFormats[DestTexture2D->GetFormat()].PlatformFormat;

				DXGI_FORMAT Fmt = ConvertTypelessToUnorm((DXGI_FORMAT)GPixelFormats[DestTexture2D->GetFormat()].PlatformFormat);

				// Determine whether a MSAA resolve is needed, or just a copy.
				if (SourceTextureRHI->IsMultisampled() && !DestTexture2D->IsMultisampled())
				{
					D3D11DeviceContext->ResolveSubresource(
						DestTexture2D->GetResource(),
						ResolveParams.DestArrayIndex,
						SourceTexture2D->GetResource(),
						ResolveParams.SourceArrayIndex,
						Fmt
					);
				}
				else
				{
					if (ResolveParams.Rect.IsValid())
					{
						D3D11_BOX SrcBox;

						SrcBox.left = ResolveParams.Rect.X1;
						SrcBox.top = ResolveParams.Rect.Y1;
						SrcBox.front = 0;
						SrcBox.right = ResolveParams.Rect.X2;
						SrcBox.bottom = ResolveParams.Rect.Y2;
						SrcBox.back = 1;

						D3D11DeviceContext->CopySubresourceRegion(DestTexture2D->GetResource(), ResolveParams.DestArrayIndex, ResolveParams.DestRect.X1, ResolveParams.DestRect.Y1, 0, SourceTexture2D->GetResource(), ResolveParams.SourceArrayIndex, &SrcBox);
					}
					else
					{
						D3D11DeviceContext->CopyResource(DestTexture2D->GetResource(), SourceTexture2D->GetResource());
					}
				}
			}
		}
	}
}

void RHICopyTexture(FD3D11Texture* SourceTexture, FD3D11Texture* DestTexture, const FRHICopyTextureInfo& CopyInfo)
{
	const bool bIsCube = SourceTexture->GetTextureCube() != nullptr;
	const bool bAllCubeFaces = bIsCube && (CopyInfo.NumArraySlices % 6) == 0;
	const int32 NumArraySlices = bAllCubeFaces ? CopyInfo.NumArraySlices / 6 : CopyInfo.NumArraySlices;
	const int32 NumFaces = bAllCubeFaces ? 6 : 1;
	for (int32 ArrayIndex = 0; ArrayIndex < NumArraySlices; ++ArrayIndex)
	{
		int32 SourceArrayIndex = CopyInfo.SourceArraySlice + ArrayIndex;
		int32 DestArrayIndex = CopyInfo.DestArraySlice + ArrayIndex;
		for (int32 FaceIndex = 0; FaceIndex < NumFaces; ++FaceIndex)
		{
			FResolveParams ResolveParams(FResolveRect(),
				bIsCube ? (ECubeFace)FaceIndex : CubeFace_PosX,
				CopyInfo.MipIndex,
				SourceArrayIndex,
				DestArrayIndex
			);
			RHICopyToResolveTarget(SourceTexture, DestTexture, ResolveParams);
		}
	}
}

float GProjectionSignY = 1.0f;
EPixelFormat GRHIHDRDisplayOutputFormat = PF_FloatRGBA;
ID3D11ShaderResourceView* HighFrequencyNoiseTexture;