#include "SceneFilterRendering.h"
#include "PostProcessing.h"

ID3D11Buffer* GScreenRectangleVertexBuffer;
ID3D11Buffer* GScreenRectangleIndexBuffer;

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
}

std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>> GetFilterInputDelcaration()
{
	static std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>> GFilterInputDeclaration = std::make_shared<std::vector<D3D11_INPUT_ELEMENT_DESC>, std::initializer_list<D3D11_INPUT_ELEMENT_DESC>>
		(
			{
				{ "ATTRIBUTE",	0,	DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 0,   D3D11_INPUT_PER_VERTEX_DATA,0 },
				{ "ATTRIBUTE",	1,	DXGI_FORMAT_R32G32_FLOAT,		0, 16,  D3D11_INPUT_PER_VERTEX_DATA,0 },
			}
		);
	return GFilterInputDeclaration;
}

extern void DrawPostProcessPass(float X, float Y, float SizeX, float SizeY, float U, float V, float SizeU, float SizeV, FIntPoint TargetSize, FIntPoint TextureSize, class FShader* VertexShader/*, */ /*EStereoscopicPass StereoView,*/ /*bool bHasCustomMesh, */ /*EDrawRectangleFlags Flags = EDRF_Default */)
{
	DrawRectangle(X, Y, SizeX, SizeY, U, V, SizeU, SizeV, TargetSize, TextureSize, VertexShader/*, Flags*/);
}

