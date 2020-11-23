#include "SceneFilterRendering.h"
#include "PostProcessing.h"

ID3D11Buffer* GScreenRectangleVertexBuffer;
ID3D11Buffer* GScreenRectangleIndexBuffer;
ID3D11InputLayout* GFilterInputLayout;
ID3D11VertexShader* GCommonPostProcessVS;

FDrawRectangleParameters DrawRectangleParameters;

void InitScreenRectangleResources()
{
	FilterVertex Vertices[] =
	{
		{ Vector4(1,  1,	0,	1) ,Vector2(1,	1) },
		{ Vector4(0,  1,	0,	1) ,Vector2(0,	1) },
		{ Vector4(1,  0,	0,	1) ,Vector2(1,	0) },
		{ Vector4(0,  0,	0,	1) ,Vector2(0,	0) },
	};
	GScreenRectangleVertexBuffer = CreateVertexBuffer(false, sizeof(Vertices), Vertices);

	uint16 Indices[] = { 0, 1, 2, 2, 1, 3};
	GScreenRectangleIndexBuffer = CreateIndexBuffer((void*)Indices, sizeof(Indices));

	TShaderMapRef<FPostProcessVS> VertexShader(GetGlobalShaderMap());
	ID3DBlob* VSBytecode = VertexShader->GetCode().Get();
	D3D11_INPUT_ELEMENT_DESC InputDesc[] =
	{
		{ "ATTRIBUTE",	0,	DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 0,   D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "ATTRIBUTE",	1,	DXGI_FORMAT_R32G32_FLOAT,		0, 16,  D3D11_INPUT_PER_VERTEX_DATA,0 },
	};
	GFilterInputLayout = CreateInputLayout(InputDesc, sizeof(InputDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC), VSBytecode);
	GCommonPostProcessVS = CreateVertexShader(VSBytecode);
}

