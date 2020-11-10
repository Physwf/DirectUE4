#include "D3D11RHI.h"
#include <D3Dcompiler.h>
#include <stdio.h>
#include "log.h"

namespace GSTest
{
	ID3D11InputLayout* InputLayout = NULL;
	ID3D11Buffer* VertexBuffer = NULL;
	ID3D11Buffer* IndexBuffer = NULL;
	ID3D11VertexShader* VertexShader = NULL;
	ID3D11GeometryShader* GeometryShader = NULL;
	ID3D11PixelShader* PixelShader = NULL;
}

using namespace GSTest;

struct TestVertex
{
	float x, y, z;
	float r, g, b, a;
};

void CreateTestBuffer()
{
	HRESULT hr;

	//create vertex buffer


	TestVertex TriangleVertices[] =
	{
		{ 0.0f, 0.5f, 0.0f,			1.0f, 0.0f, 0.0f, 1.0f },
	{ 0.45f, -0.5, 0.0f,		0.0f, 1.0f, 0.0f, 1.0f },
	{ -0.45f, -0.5f, 0.0f,		0.0f, 0.0f, 1.0f, 1.0f }
	};

	D3D11_BUFFER_DESC VertexDesc;
	ZeroMemory(&VertexDesc, sizeof(VertexDesc));
	VertexDesc.Usage = D3D11_USAGE_DEFAULT;
	VertexDesc.ByteWidth = sizeof(TestVertex) * 3;
	VertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VertexDesc.CPUAccessFlags = 0;
	VertexDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = TriangleVertices;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	hr = D3D11Device->CreateBuffer(&VertexDesc, &InitData, &VertexBuffer);
	if (FAILED(hr))
	{
		X_LOG("CreateBuffer for vertex failed!");
		return;
	}

	//create index buffer
	unsigned int Indices[] = { 0,1,2 };
	D3D11_BUFFER_DESC IndexDesc;
	ZeroMemory(&IndexDesc, sizeof(IndexDesc));
	IndexDesc.Usage = D3D11_USAGE_DEFAULT;
	IndexDesc.ByteWidth = sizeof(unsigned int) * 3;
	IndexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	IndexDesc.CPUAccessFlags = 0;
	IndexDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA IndexData;
	ZeroMemory(&IndexData, sizeof(IndexData));
	IndexData.pSysMem = Indices;
	IndexData.SysMemPitch = 0;
	IndexData.SysMemSlicePitch = 0;

	hr = D3D11Device->CreateBuffer(&IndexDesc, &IndexData, &IndexBuffer);
	if (FAILED(hr))
	{
		X_LOG("CreateBuffer for index failed!");
		return;
	}

	//create vertex shader
	ID3DBlob* VSBytecode;
	ID3DBlob* GSBytecode;
	ID3DBlob* PSBytecode;
	ID3DBlob* OutErrorMsg;
	LPCSTR VSTarget = D3D11Device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0 ? "vs_5_0" : "vs_4_0";
	UINT VSFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;

	hr = D3DCompileFromFile(TEXT("GeometryTest.hlsl"), NULL, NULL, "VS_Main", VSTarget, VSFlags, 0, &VSBytecode, &OutErrorMsg);
	if (FAILED(hr))
	{
		X_LOG("D3DCompileFromFile failed! %s", (const char*)OutErrorMsg->GetBufferPointer());
		return;
	}

	hr = D3D11Device->CreateVertexShader(VSBytecode->GetBufferPointer(), VSBytecode->GetBufferSize(), NULL, &VertexShader);
	if (FAILED(hr))
	{
		X_LOG("CreateVertexShader failed! %s", (const char*)OutErrorMsg->GetBufferPointer());
		return;
	}
	if (OutErrorMsg)
	{
		OutErrorMsg->Release();
	}

	//create geometry shader
	LPCSTR GSTarget = D3D11Device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0 ? "gs_5_0" : "gs_4_0";
	UINT GSFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;

	hr = D3DCompileFromFile(TEXT("GeometryTest.hlsl"), NULL, NULL, "GS_Main", GSTarget, GSFlags, 0, &GSBytecode, &OutErrorMsg);
	if (FAILED(hr))
	{
		X_LOG("D3DCompileFromFile failed! %s", (const char*)OutErrorMsg->GetBufferPointer());
		return;
	}

	hr = D3D11Device->CreateGeometryShader(GSBytecode->GetBufferPointer(), GSBytecode->GetBufferSize(), NULL, &GeometryShader);
	if (FAILED(hr))
	{
		X_LOG("CreateGeometryShader failed! %s", (const char*)OutErrorMsg->GetBufferPointer());
		return;
	}
	if (OutErrorMsg)
	{
		OutErrorMsg->Release();
	}

	//create pixel shader
	LPCSTR PSTarget = D3D11Device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0 ? "ps_5_0" : "ps_4_0";
	UINT PSFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;

	hr = D3DCompileFromFile(TEXT("GeometryTest.hlsl"), NULL, NULL, "PS_Main", PSTarget, PSFlags, 0, &PSBytecode, &OutErrorMsg);
	if (FAILED(hr))
	{
		X_LOG("D3DCompileFromFile failed! %s", (const char*)OutErrorMsg->GetBufferPointer());
		return;
	}

	hr = D3D11Device->CreatePixelShader(PSBytecode->GetBufferPointer(), PSBytecode->GetBufferSize(), NULL, &PixelShader);
	if (FAILED(hr))
	{
		X_LOG("CreatePixelShader failed!");
		return;
	}
	if (OutErrorMsg)
	{
		OutErrorMsg->Release();
	}

	//create input layout
	D3D11_INPUT_ELEMENT_DESC InputDesc[] =
	{
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0, 0, D3D11_INPUT_PER_VERTEX_DATA,0 },
	{ "COLOR",		0,	DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 12,D3D11_INPUT_PER_VERTEX_DATA,0 },
	};
	UINT NumElements = ARRAYSIZE(InputDesc);
	hr = D3D11Device->CreateInputLayout(InputDesc, NumElements, VSBytecode->GetBufferPointer(), VSBytecode->GetBufferSize(), &InputLayout);
	if (FAILED(hr))
	{
		X_LOG("CreateInputLayout failed!");
		return;
	}

	//VSBytecode->Release();
	//PSBytecode->Release();


}

void RenderTest()
{
	D3D11DeviceContext->IASetInputLayout(InputLayout);
	D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	UINT Stride = sizeof(TestVertex);
	UINT Offset = 0;
	D3D11DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
	//D3D11DeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	D3D11DeviceContext->VSSetShader(VertexShader, 0, 0);
	D3D11DeviceContext->GSSetShader(GeometryShader, 0, 0);
	D3D11DeviceContext->PSSetShader(PixelShader, 0, 0);

	//D3D11DeviceContext->DrawIndexed(3, 0, 0);
	D3D11DeviceContext->Draw(3, 0);
}