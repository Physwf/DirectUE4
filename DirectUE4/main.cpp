#include <windows.h>
#include "D3D11RHI.h"
#include "Viewport.h"
#include "World.h"
#include "DeferredShading.h"
#include "log.h"

void OutputDebug(const char* Format)
{
	OutputDebugStringA(Format);
}

LRESULT CALLBACK WindowProc(HWND hWnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam);

HWND g_hWind = NULL;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof WNDCLASSEX;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = L"WindowClass1";

	RegisterClassEx(&wc);

	RECT wr = { 0,0,WindowWidth,WindowHeight };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	g_hWind = CreateWindowEx
	(
		NULL,
		L"WindowClass1",
		L"dx11demo",
		WS_OVERLAPPEDWINDOW,
		300,
		300,
		wr.right - wr.left,
		wr.bottom - wr.top,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	ShowWindow(g_hWind, nCmdShow);

	if (!InitRHI())
	{
		return 1;
	}

	InitShading();
	GWorld.InitWorld();
	GWindowViewport.SetSizeXY(WindowWidth, WindowHeight);

	MSG msg;
	
	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				break;
			}
			continue;
		}
		{

			static DWORD LastTickCount = 0;
			DWORD TimeEclipse = GetTickCount() - LastTickCount;
			if (TimeEclipse > 30) TimeEclipse = 30;
			LastTickCount = GetTickCount();

			GWorld.Tick(TimeEclipse / 1000.f);
			GWindowViewport.Draw();

			Sleep(1);
		}

	}

	return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	case WM_KEYDOWN:
	{
		X_LOG("WM_KEYDOWN\n");
		GWindowViewport.OnKeyDown(wParam);
		break;
	}
	case WM_KEYUP:
	{
		GWindowViewport.OnKeyUp(wParam);
		break;
	}
	case WM_LBUTTONDOWN:
	{
		GWindowViewport.OnMouseDown(LOWORD(lParam), HIWORD(lParam));
		break;
	}
	case WM_LBUTTONUP:
	{
		GWindowViewport.OnMouseUp(LOWORD(lParam), HIWORD(lParam));
		break;
	}
	case WM_RBUTTONDOWN:
	{
		GWindowViewport.OnRightMouseDown(LOWORD(lParam), HIWORD(lParam));
		break;
	}
	case WM_RBUTTONUP:
	{
		GWindowViewport.OnRightMouseUp(LOWORD(lParam), HIWORD(lParam));
		break;
	}
	case WM_MOUSEMOVE:
	{
		GWindowViewport.OnMouseMove(LOWORD(lParam), HIWORD(lParam));
		break;
	}
	}

	return	DefWindowProc(hWnd, message, wParam, lParam);
}
