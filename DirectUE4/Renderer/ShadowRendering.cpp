#include "ShadowRendering.h"
#include "Scene.h"
#include "DeferredShading.h"
#include "StaticMesh.h"
#include "RenderTargets.h"

struct ShadowMapView
{
	FMatrix View;
	FMatrix Projection;
};

FMatrix ShadowMapProjectionMatrix;
ID3D11Buffer* ShadowMapProjectionUniformBuffer;

FMatrix ScreenToShadowMatrix;

ID3D11InputLayout* ShadowProjectionInputLayout;

ID3DBlob* ShadowPassVSBytecode;
ID3DBlob* ShadowPassPSBytecode;
ID3D11Texture2D* ShadowPassRT;
ID3D11DepthStencilView* ShadowPassDSV;
ID3D11VertexShader* ShadowPassVS;
ID3D11PixelShader* ShadowPassPS;
ID3D11RasterizerState* ShadowPassRasterizerState;
ID3D11BlendState* ShadowPassBlendState;
ID3D11DepthStencilState* ShadowPassDepthStencilState;

ID3D11ShaderResourceView* ShadowPassDepthSRV;
ID3D11SamplerState* ShadowPassDepthSamplerState;
ID3DBlob* ShadowProjectionVSBytecode;
ID3DBlob* ShadowProjectionPSBytecode;
ID3D11Texture2D* ShadowProjectionRT;
ID3D11RenderTargetView* ShadowProjectionRTV;
ID3D11VertexShader* ShadowProjectionVS;
ID3D11PixelShader* ShadowProjectionPS;
ID3D11RasterizerState* ShadowProjectionRasterizerState;
ID3D11BlendState* ShadowProjectionBlendState;
ID3D11DepthStencilState* ShadowProjectionDepthStencilState;

ID3D11Buffer* DirecianlLightShadowMapViewUniformBuffer;
ID3D11Buffer* DynamicVertexBuffer;

extern ID3D11ShaderResourceView* LightPassSceneDepthSRV;
extern ID3D11SamplerState* LightPassSceneDepthSamplerState;
extern ID3D11InputLayout* PositionOnlyMeshInputLayout;

std::map<std::string, ParameterAllocation> ShadowPassVSParams;
std::map<std::string, ParameterAllocation> ShadowPassPSParams;

std::map<std::string, ParameterAllocation> ShadowProjectionVSParams;
std::map<std::string, ParameterAllocation> ShadowProjectionPSParams;


void InitShadowDepthMapsPass()
{
	/*
	//shadow pass
	ShadowPassVSBytecode = CompileVertexShader(TEXT("./Shaders/ShadowDepthVertexShader.hlsl"), "VS_Main");
	ShadowPassPSBytecode = CompilePixelShader(TEXT("./Shaders/ShadowDepthPixelShader.hlsl"), "PS_Main");
	GetShaderParameterAllocations(ShadowPassVSBytecode, ShadowPassVSParams);
	GetShaderParameterAllocations(ShadowPassPSBytecode, ShadowPassPSParams);
	ShadowPassVS = CreateVertexShader(ShadowPassVSBytecode);
	ShadowPassPS = CreatePixelShader(ShadowPassPSBytecode);

	ShadowPassRT = CreateTexture2D(WindowWidth, WindowHeight, DXGI_FORMAT_R32_TYPELESS, false, true, true);
	ShadowPassDSV = CreateDepthStencilView2D(ShadowPassRT, DXGI_FORMAT_D32_FLOAT, 0);
	ShadowPassDepthSRV = CreateShaderResourceView2D(ShadowPassRT, DXGI_FORMAT_R32_FLOAT, 1, 0);

	ShadowPassRasterizerState = TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_BACK, FALSE>::GetRHI();
	ShadowPassBlendState = TStaticBlendState<>::GetRHI();
	ShadowPassDepthStencilState = TStaticDepthStencilState<true, D3D11_COMPARISON_LESS>::GetRHI();

	ShadowProjectionVSBytecode = CompileVertexShader(TEXT("./Shaders/ShadowProjectionVertexShader.hlsl"), "Main");
	ShadowProjectionPSBytecode = CompilePixelShader(TEXT("./Shaders/ShadowProjectionPixelShader.hlsl"), "Main");
	GetShaderParameterAllocations(ShadowProjectionVSBytecode, ShadowProjectionVSParams);
	GetShaderParameterAllocations(ShadowProjectionPSBytecode, ShadowProjectionPSParams);

	D3D11_INPUT_ELEMENT_DESC ShadowProjectionInputDesc[] =
	{
		{ "ATTRIBUTE",	0,	DXGI_FORMAT_R32G32_FLOAT,	0, 0,  D3D11_INPUT_PER_VERTEX_DATA,0 },
	};
	ShadowProjectionInputLayout = CreateInputLayout(ShadowProjectionInputDesc, sizeof(ShadowProjectionInputDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC), ShadowProjectionVSBytecode);

	ShadowProjectionRT = CreateTexture2D(WindowWidth, WindowHeight, DXGI_FORMAT_R32G32B32A32_FLOAT, true, true, false);
	ShadowProjectionRTV = CreateRenderTargetView2D(ShadowProjectionRT, DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
	ShadowProjectionVS = CreateVertexShader(ShadowProjectionVSBytecode);
	ShadowProjectionPS = CreatePixelShader(ShadowProjectionPSBytecode);

	extern float ViewZNear;
	extern float ViewZFar;
	Matrix DirectionalLightViewRotationMatrix = Matrix::DXLookToLH(DirLight.Direction);
	Frustum ViewFrustum(3.1415926f / 3.f, (float)WindowWidth / WindowHeight, ViewZNear, ViewZFar);
	Box FrumstumBounds = ViewFrustum.GetBounds();
	Vector FrumstumWorldPosition = MainCamera.GetPosition() + MainCamera.FaceDir * (ViewZFar - ViewZNear);
	Vector FrumstumViewPosition = DirectionalLightViewRotationMatrix.Transform(FrumstumWorldPosition);
	Matrix DirectionalLightProjectionMatrix = Matrix::DXFromOrthognalLH(FrumstumBounds.Max.X, FrumstumBounds.Min.X, FrumstumBounds.Max.Y, FrumstumBounds.Min.Y, FrumstumBounds.Max.Z, FrumstumBounds.Min.Z);
	//DirectionalLightProjectionMatrix = ProjectionMatrix;
	//ShadowMapProjectionMatrix = DirectionalLightViewRotationMatrix * Matrix::DXFromTranslation(-FrumstumViewPosition) * DirectionalLightProjectionMatrix;
	ShadowMapProjectionMatrix = Matrix::DXFromTranslation(-FrumstumWorldPosition) * DirectionalLightViewRotationMatrix *  DirectionalLightProjectionMatrix;
	//Matrix  InvViewProjectionMatrix = VMs.GetInvViewProjectionMatrix();
	//ScreenToShadowMatrix =  InvViewProjectionMatrix * ShadowMapProjectionMatrix;
	//ScreenToShadowMatrix.Transpose();

	//UE4 ShadowRendering.cpp GetScreenToShadowMatrix
	uint32 TileOffsetX = 0; uint32 TileOffsetY = 0;
	uint32 BorderSize = 0;
	float InvMaxSubjectDepth = 1.0f / 1.0f;
	const Vector2 ShadowBufferResolution((float)WindowWidth, (float)WindowHeight);
	uint32 TileResolutionX = WindowWidth;
	uint32 TileResolutionY = WindowHeight;
	const float InvBufferResolutionX = 1.0f / (float)ShadowBufferResolution.X;
	const float ShadowResolutionFractionX = 0.5f * (float)TileResolutionX * InvBufferResolutionX;
	const float InvBufferResolutionY = 1.0f / (float)ShadowBufferResolution.Y;
	const float ShadowResolutionFractionY = 0.5f * (float)TileResolutionY * InvBufferResolutionY;
	// Calculate the matrix to transform a screenspace position into shadow map space
	ScreenToShadowMatrix =
		// Z of the position being transformed is actually view space Z, 
		// Transform it into post projection space by applying the projection matrix,
		// Which is the required space before applying View.InvTranslatedViewProjectionMatrix
		Matrix(
			Plane(1, 0, 0, 0),
			Plane(0, 1, 0, 0),
			Plane(0, 0, VMs.GetProjectionMatrix().M[2][2], 1),
			Plane(0, 0, VMs.GetProjectionMatrix().M[3][2], 0)) *
		// Transform the post projection space position into translated world space
		// Translated world space is normal world space translated to the view's origin, 
		// Which prevents floating point imprecision far from the world origin.
		VMs.GetInvTranslatedViewProjectionMatrix() *
		// Translate to the origin of the shadow's translated world space
		//FTranslationMatrix(PreShadowTranslation - View.ViewMatrices.GetPreViewTranslation()) *
		// Transform into the shadow's post projection space
		// This has to be the same transform used to render the shadow depths
		ShadowMapProjectionMatrix *
		// Scale and translate x and y to be texture coordinates into the ShadowInfo's rectangle in the shadow depth buffer
		// Normalize z by MaxSubjectDepth, as was done when writing shadow depths
		Matrix(
			Plane(ShadowResolutionFractionX, 0, 0, 0),
			Plane(0, -ShadowResolutionFractionY, 0, 0),
			Plane(0, 0, InvMaxSubjectDepth, 0),
			Plane(
			(TileOffsetX + BorderSize) * InvBufferResolutionX + ShadowResolutionFractionX,
				(TileOffsetY + BorderSize) * InvBufferResolutionY + ShadowResolutionFractionY,
				0,
				1
			)
		);
	ScreenToShadowMatrix.Transpose();

	ShadowMapProjectionMatrix.Transpose();
	//ShadowMapProjectionUniformBuffer = CreateConstantBuffer(false, sizeof(ShadowMapProjectionMatrix), &ShadowMapProjectionMatrix);
	*/
}

void RenderShadowProjection()
{
	/*
	D3D11DeviceContext->OMSetRenderTargets(1, &ShadowProjectionRTV, NULL);
	//D3D11DeviceContext->OMSetDepthStencilState(NULL, 0);
	const FLOAT ClearColor[] = { 0.f,0.f,0.0f,1.f };
	D3D11DeviceContext->ClearRenderTargetView(ShadowProjectionRTV, ClearColor);

	D3D11DeviceContext->RSSetState(TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_NONE, FALSE, FALSE>::GetRHI());
	//D3D11DeviceContext->OMSetBlendState(TStaticBlendState<>::GetRHI(),NULL,0);
	D3D11DeviceContext->RSSetViewports(1, &GViewport);
	D3D11DeviceContext->OMSetDepthStencilState(TStaticDepthStencilState<false, D3D11_COMPARISON_ALWAYS>::GetRHI(), 0);

	D3D11DeviceContext->IASetInputLayout(ShadowProjectionInputLayout);
	D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	const ParameterAllocation& ShadowDepthTexture = ShadowProjectionPSParams.at("ShadowDepthTexture");
	const ParameterAllocation& ShadowDepthTextureSampler = ShadowProjectionPSParams.at("ShadowDepthTextureSampler");
	const ParameterAllocation& SceneDepthTextureParam = ShadowProjectionPSParams.at("SceneTexturesStruct_SceneDepthTexture");
	const ParameterAllocation& SceneDepthTextureSamplerParam = ShadowProjectionPSParams.at("SceneTexturesStruct_SceneDepthTextureSampler");
	const ParameterAllocation& ViewParam = ShadowProjectionPSParams.at("View");
	D3D11DeviceContext->PSSetShaderResources(ShadowDepthTexture.BaseIndex, ShadowDepthTexture.Size, &ShadowPassDepthSRV);
	ShadowPassDepthSamplerState = TStaticSamplerState<>::GetRHI();
	D3D11DeviceContext->PSSetSamplers(ShadowDepthTextureSampler.BaseIndex, ShadowDepthTextureSampler.Size, &ShadowPassDepthSamplerState);
	D3D11DeviceContext->PSSetShaderResources(SceneDepthTextureParam.BaseIndex, SceneDepthTextureParam.Size, &LightPassSceneDepthSRV);
	D3D11DeviceContext->PSSetSamplers(SceneDepthTextureSamplerParam.BaseIndex, SceneDepthTextureSamplerParam.Size, &LightPassSceneDepthSamplerState);
	D3D11DeviceContext->PSSetConstantBuffers(ViewParam.BufferIndex, 1, &ViewUniformBuffer);

	const ParameterAllocation& ShadowFadeFractionParam = ShadowProjectionPSParams.at("ShadowFadeFraction");
	const ParameterAllocation& ShadowSharpenParam = ShadowProjectionPSParams.at("ShadowSharpen");
	const ParameterAllocation& ScreenToShadowMatrixParam = ShadowProjectionPSParams.at("ScreenToShadowMatrix");
	float fShadowFadeFraction = 0.8f;
	float fShadowSharpen = 0.9f;

	memcpy(GlobalConstantBufferData + ShadowFadeFractionParam.BaseIndex, &fShadowFadeFraction, ShadowFadeFractionParam.Size);
	memcpy(GlobalConstantBufferData + ShadowSharpenParam.BaseIndex, &fShadowSharpen, ShadowSharpenParam.Size);
	memcpy(GlobalConstantBufferData + ScreenToShadowMatrixParam.BaseIndex, &ScreenToShadowMatrix, ScreenToShadowMatrixParam.Size);

	//https://gamedev.stackexchange.com/questions/184702/direct3d-constant-buffer-not-updating
	//https://docs.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-updatesubresource
	D3D11DeviceContext->UpdateSubresource(GlobalConstantBuffer, 0, NULL, GlobalConstantBufferData, 0, 0);
	D3D11DeviceContext->PSSetConstantBuffers(ShadowFadeFractionParam.BufferIndex, 1, &GlobalConstantBuffer);

	D3D11DeviceContext->VSSetShader(ShadowProjectionVS, 0, 0);
	D3D11DeviceContext->PSSetShader(ShadowProjectionPS, 0, 0);

	DynamicVertexBuffer = CreateVertexBuffer(true, 4 * sizeof(Vector4));

	Vector4 Verts[4] =
	{
		Vector4(-1.0f, 1.0f, 0.0f),
		Vector4(1.0f, 1.0f, 0.0f),
		Vector4(-1.0f, -1.0f, 0.0f),
		Vector4(1.0f, -1.0f, 0.0f),
	};
	//https://docs.microsoft.com/en-us/windows/win32/direct3d11/how-to--use-dynamic-resources
	D3D11_MAPPED_SUBRESOURCE SubResource;
	ZeroMemory(&SubResource, sizeof(SubResource));
	D3D11DeviceContext->Map(DynamicVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubResource);
	memcpy(SubResource.pData, Verts, sizeof(Verts));
	D3D11DeviceContext->Unmap(DynamicVertexBuffer, 0);
	UINT Stride = sizeof(Vector4);
	UINT Offset = 0;
	D3D11DeviceContext->IASetVertexBuffers(0, 1, &DynamicVertexBuffer, &Stride, &Offset);

	D3D11DeviceContext->Draw(4, 0);
	//reset
	ID3D11ShaderResourceView* SRV = NULL;
	ID3D11SamplerState* Sampler = NULL;
	D3D11DeviceContext->PSSetShaderResources(ShadowDepthTexture.BaseIndex, ShadowDepthTexture.Size, &SRV);
	D3D11DeviceContext->PSSetSamplers(ShadowDepthTextureSampler.BaseIndex, ShadowDepthTextureSampler.Size, &Sampler);
	D3D11DeviceContext->PSSetShaderResources(SceneDepthTextureParam.BaseIndex, SceneDepthTextureParam.Size, &SRV);
	D3D11DeviceContext->PSSetSamplers(SceneDepthTextureSamplerParam.BaseIndex, SceneDepthTextureSamplerParam.Size, &Sampler);
	*/
}

void SceneRenderer::RenderShadowDepthMaps()
{
	/*
	D3D11DeviceContext->OMSetRenderTargets(0, NULL, ShadowPassDSV);
	D3D11DeviceContext->ClearDepthStencilView(ShadowPassDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	const ParameterAllocation& ViewParams = ShadowPassVSParams.at("View");
	D3D11DeviceContext->VSSetConstantBuffers(ViewParams.BufferIndex, 1, &ViewUniformBuffer);

	const ParameterAllocation& ProjectionMatrixParams = ShadowPassVSParams.at("ProjectionMatrix");
	memcpy(GlobalConstantBufferData + ProjectionMatrixParams.BaseIndex, &ShadowMapProjectionMatrix, ProjectionMatrixParams.Size);
	D3D11DeviceContext->UpdateSubresource(GlobalConstantBuffer, 0, NULL, GlobalConstantBufferData, 0, 0);
	D3D11DeviceContext->VSSetConstantBuffers(ProjectionMatrixParams.BufferIndex, 1, &GlobalConstantBuffer);

	D3D11DeviceContext->RSSetState(ShadowPassRasterizerState);
	D3D11DeviceContext->OMSetBlendState(TSTaticBlendState<>::GetRHI(), NULL, 0xffffffff);
	D3D11DeviceContext->OMSetDepthStencilState(ShadowPassDepthStencilState, 0);

	const ParameterAllocation& PrimitiveParams = ShadowPassVSParams.at("Primitive");

	for (MeshBatch& MB : GScene->AllBatches)
	{
		D3D11DeviceContext->IASetInputLayout(PositionOnlyMeshInputLayout);
		D3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		UINT Stride = sizeof(PositionOnlyLocalVertex);
		UINT Offset = 0;
		D3D11DeviceContext->IASetVertexBuffers(0, 1, &MB.PositionOnlyVertexBuffer, &Stride, &Offset);
		D3D11DeviceContext->IASetIndexBuffer(MB.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		D3D11DeviceContext->VSSetShader(ShadowPassVS, 0, 0);
		D3D11DeviceContext->PSSetShader(ShadowPassPS, 0, 0);

		for (size_t Element = 0; Element < MB.Elements.size(); ++Element)
		{
			D3D11DeviceContext->VSSetConstantBuffers(PrimitiveParams.BufferIndex, 1, &MB.Elements[Element].PrimitiveUniformBuffer);
			D3D11DeviceContext->DrawIndexed(MB.Elements[Element].NumTriangles * 3, MB.Elements[Element].FirstIndex, 0);
		}
	}
	*/
}
