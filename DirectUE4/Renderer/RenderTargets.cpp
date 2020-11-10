#include "RenderTargets.h"

RenderTargets& RenderTargets::Get()
{
	static RenderTargets Instance;
	return Instance;
}

void RenderTargets::BeginRenderingSceneColor()
{
	AllocSceneColor();
	D3D11DeviceContext->OMSetRenderTargets(1, &SceneColorRTV, NULL);
}

void RenderTargets::BeginRenderingPrePass(bool bClear)
{
	if (bClear)
	{
		D3D11DeviceContext->OMSetRenderTargets(0, NULL, SceneDepthDSV);
		D3D11DeviceContext->ClearDepthStencilView(SceneDepthDSV, D3D11_CLEAR_DEPTH, 0.f, 0);
	}
	else
	{
		D3D11DeviceContext->OMSetRenderTargets(0, NULL, SceneDepthDSV);
	}
}

void RenderTargets::FinishRenderingPrePass()
{

}

void RenderTargets::BeginRenderingGBuffer(bool bClearColor, const LinearColor& ClearColor /*= (0, 0, 0, 1)*/)
{
	AllocSceneColor();

	ID3D11RenderTargetView* MRT[8];
	int32 OutVelocityRTIndex;
	int MRTCount =  GetGBufferRenderTargets(MRT, OutVelocityRTIndex);

	D3D11DeviceContext->OMSetRenderTargets(MRTCount, MRT, SceneDepthDSV);

	D3D11DeviceContext->ClearDepthStencilView(SceneDepthDSV, D3D11_CLEAR_DEPTH, 0.f, 0);

	if (bClearColor)
	{
		for (int i = 0; i < MRTCount; ++i)
		{
			D3D11DeviceContext->ClearRenderTargetView(MRT[i], (FLOAT*)&ClearColor);
		}
	}
}

void RenderTargets::FinishRenderingGBuffer()
{

}

void RenderTargets::FinishRendering()
{
	D3D11DeviceContext->CopyResource(BackBuffer, SceneColorRT);
}

void RenderTargets::SetBufferSize(int32 InBufferSizeX, int32 InBufferSizeY)
{
	QuantizeSceneBufferSize(IntPoint(InBufferSizeX, InBufferSizeY), BufferSize);
}

void RenderTargets::Allocate()
{
	AllocRenderTargets();
	AllocGBuffer();

	ScreenSpaceAO = CreateTexture2D(WindowWidth, WindowHeight, DXGI_FORMAT_R8_UNORM, true, true, false);
}

void RenderTargets::AllocRenderTargets()
{
	AllocateCommonDepthTargets();
}

void RenderTargets::AllocSceneColor()
{
	if (!SceneColorRT)
	{
		SceneColorRT = CreateTexture2D(WindowWidth, WindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, true, true, false);
		SceneColorRTV = CreateRenderTargetView2D(SceneColorRT, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	}
}

void RenderTargets::AllocateCommonDepthTargets()
{
	if (!SceneDepthRT)
	{
		SceneDepthRT = CreateTexture2D(WindowWidth, WindowHeight, DXGI_FORMAT_R24G8_TYPELESS, false, true, true);
		SceneDepthDSV = CreateDepthStencilView2D(SceneDepthRT, DXGI_FORMAT_D24_UNORM_S8_UINT, 0);
	}
}

void RenderTargets::AllocGBuffer()
{
	if (GBufferART) return;

	GBufferART = CreateTexture2D(WindowWidth, WindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, true, true, false);//WorldNormal(3),PerObjectGBufferData(1)
	GBufferBRT = CreateTexture2D(WindowWidth, WindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, true, true, false);//metalic(1),specular(1),Roughness(1),ShadingModelID|SelectiveOutputMask(1)
	GBufferCRT = CreateTexture2D(WindowWidth, WindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, true, true, false);//BaseColor(3),GBufferAO(1)
// 	GBufferDRT = CreateTexture2D(WindowWidth, WindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, true, true, false);//CustomData(4)
// 	GBufferERT = CreateTexture2D(WindowWidth, WindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, true, true, false);//PrecomputedShadowFactors(4)
// 	VelocityRT = CreateTexture2D(WindowWidth, WindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, true, true, false);//Velocity(4)

	GBufferARTV = CreateRenderTargetView2D(GBufferART, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	GBufferBRTV = CreateRenderTargetView2D(GBufferBRT, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	GBufferCRTV = CreateRenderTargetView2D(GBufferCRT, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
// 	GBufferDRTV = CreateRenderTargetView2D(GBufferDRT, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
// 	GBufferERTV = CreateRenderTargetView2D(GBufferERT, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
// 	VelocityRTV = CreateRenderTargetView2D(VelocityRT, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
}

void RenderTargets::AllocLightAttenuation()
{

}

int RenderTargets::GetGBufferRenderTargets(ID3D11RenderTargetView* OutRenderTargets[8], int& OutVelocityRTIndex)
{
	int MRTCount = 0;
	OutRenderTargets[MRTCount++] = SceneColorRTV;
	OutRenderTargets[MRTCount++] = GBufferARTV;
	OutRenderTargets[MRTCount++] = GBufferBRTV;
	OutRenderTargets[MRTCount++] = GBufferCRTV;

	return MRTCount;
}
