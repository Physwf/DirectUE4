#include "Viewport.h"
#include "Camera.h"
#include "Actor.h"
#include "World.h"
#include "DeferredShading.h"
#include "Scene.h"

const std::shared_ptr<FD3D11Texture2D>& FRenderTarget::GetRenderTargetTexture() const
{
	return RenderTargetTextureRHI;
}

float FRenderTarget::GetDisplayGamma() const
{
	return 2.2f;
}

void FViewport::OnKeyDown(unsigned int KeyCode)
{
// 	switch(KeyCode)
// 	{
// 	case 0x41:
// 		//MainCamera.Side(1.0f);
// 		MainCamera.StartMove(Vector(1.f,0,0));
// 		break;
// 	case 0x44:
// 		//MainCamera.Side(-1.0f);
// 		MainCamera.StartMove(Vector(-1.f, 0, 0));
// 		break;
// 	case 0x57:
// 		//MainCamera.Walk(1.0f);
// 		MainCamera.StartMove(Vector(0.f, 0, 1.f));
// 		break;
// 	case 0x53:
// 		//MainCamera.Walk(-1.0f);
// 		MainCamera.StartMove(Vector(0.f, 0, -1.f));
// 		break;
// 	case VK_SPACE:
// 		//MainCamera.Lift(1.0f);
// 		MainCamera.StartMove(Vector(0.f, 1, 0.f));
// 		break;
// 	}
}

void FViewport::OnKeyUp(unsigned int KeyCode)
{
	//MainCamera.StopMove();
}

void FViewport::OnMouseDown(int X, int Y)
{
// 	if (SelectedActor)
// 	{
// 		SelectedActor->StartDrag(X, Y);
// 	}
}

void FViewport::OnMouseUp(int X, int Y)
{
// 	if (SelectedActor)
// 	{
// 		SelectedActor->StopDrag(X,Y);
// 	}
}

void FViewport::OnRightMouseDown(int X, int Y)
{
	//MainCamera.StartDrag(X, Y);
}

void FViewport::OnRightMouseUp(int X, int Y)
{
	//MainCamera.StopDrag(X, Y);

}

void FViewport::OnMouseMove(int X, int Y)
{
// 	MainCamera.Drag(X, Y);
// 	if (SelectedActor)
// 	{
// 		SelectedActor->Drag(X, Y);
// 	}
}

void FViewport::Draw(bool bShouldPresent /*= true*/)
{
	FSceneViewFamily ViewFamily;
	ViewFamily.Scene = GWorld.Scene;
	ViewFamily.RenderTarget = this;
	ViewFamily.bResolveScene = true;

	for (Camera* C : GWorld.GetCameras())
	{
		FSceneView* View = C->CalcSceneView(ViewFamily,*this);
	}
	GWorld.SendAllEndOfFrameUpdates();

	ViewFamily.Scene->IncrementFrameNumber();
	ViewFamily.FrameNumber = ViewFamily.Scene->GetFrameNumber();

	FSceneRenderer Renderer(ViewFamily);
	GFrameNumberRenderThread++;
	GFrameNumber++;
	Renderer.Render();

	FSceneRenderTargets& SceneContex = FSceneRenderTargets::Get();
	SceneContex.FinishRendering();

	if (bShouldPresent)
	{
		DXGISwapChain->Present(0, 0);
	}
	FViewInfo::DestroyAllSnapshots();
}

void FViewport::InitRHI()
{
	RenderTargetTextureRHI.reset(BackBuffer);
}

void FViewport::ReleaseRHI()
{
	RenderTargetTextureRHI.reset();
}

FViewport GWindowViewport;

