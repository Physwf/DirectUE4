#include "Viewport.h"
#include "Camera.h"
#include "Actor.h"
#include "World.h"
#include "DeferredShading.h"

void Viewport::OnKeyDown(unsigned int KeyCode)
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

void Viewport::OnKeyUp(unsigned int KeyCode)
{
	//MainCamera.StopMove();
}

void Viewport::OnMouseDown(int X, int Y)
{
// 	if (SelectedActor)
// 	{
// 		SelectedActor->StartDrag(X, Y);
// 	}
}

void Viewport::OnMouseUp(int X, int Y)
{
// 	if (SelectedActor)
// 	{
// 		SelectedActor->StopDrag(X,Y);
// 	}
}

void Viewport::OnRightMouseDown(int X, int Y)
{
	//MainCamera.StartDrag(X, Y);
}

void Viewport::OnRightMouseUp(int X, int Y)
{
	//MainCamera.StopDrag(X, Y);

}

void Viewport::OnMouseMove(int X, int Y)
{
// 	MainCamera.Drag(X, Y);
// 	if (SelectedActor)
// 	{
// 		SelectedActor->Drag(X, Y);
// 	}
}

void Viewport::Draw(bool bShouldPresent /*= true*/)
{
	SceneViewFamily ViewFamily;
	ViewFamily.Scene = GWorld.Scene;
	for (Camera* C : GWorld.GetCameras())
	{
		FSceneView* View = C->CalcSceneView(ViewFamily,*this);
	}
	SceneRenderer Renderer(ViewFamily);
	Renderer.Render();
	if (bShouldPresent)
	{
		DXGISwapChain->Present(0, 0);
	}
}

Viewport GWindowViewport;