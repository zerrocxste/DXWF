// DXWF.cpp : Определяет функции для статической библиотеки.
//
#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <assert.h>
#include <map>

#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")

#include "Include/d3dx9.h"
#pragma comment(lib, "Lib/x86/d3dx9.lib")

#include "framework.h"

typedef void (*callback_wndproc)(HWND, UINT, WPARAM, LPARAM);
typedef void (*callback)();

LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
D3DPRESENT_PARAMETERS    g_d3dpp{};
LPDIRECT3D9 pD3D = NULL;

std::string pszWindowName;
HWND phWindow;
HINSTANCE phInstance;

std::map<int, callback_wndproc>mWndProcCallbacks;
std::map<int, callback>mRenderCallbacks;

void DXWFWndProcCallbacks(DWORD pWndProcAttr, callback_wndproc cCallbackFunction)
{
	switch (pWndProcAttr)
	{
	case DXWF_WNDPROC_WNDPROCHANDLER_:
		mWndProcCallbacks[DXWF_WNDPROC_WNDPROCHANDLER_] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_SIZE_:
		mWndProcCallbacks[DXWF_WNDPROC_WM_SIZE_] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_MOUSEMOVE_:
		mWndProcCallbacks[DXWF_WNDPROC_WM_MOUSEMOVE_] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_LBUTTONDOWN_:
		mWndProcCallbacks[DXWF_WNDPROC_WM_LBUTTONDOWN_] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_LBUTTONUP_:
		mWndProcCallbacks[DXWF_WNDPROC_WM_LBUTTONUP_] = cCallbackFunction;
		break;
	default:
		std::cout << "DXWF: Error #1 " << __FUNCTION__ << " () -> Unknown arg\n";
		break;
	}
}

void DXWFRendererCallbacks(DWORD pWndProcAttr, callback cCallbackFunction)
{
	switch (pWndProcAttr)
	{
	case DXWF_RENDERER_LOOP_:
		mRenderCallbacks[DXWF_RENDERER_LOOP_] = cCallbackFunction;
		break;
	case DXWF_RENDERER_BEGIN_SCENE_LOOP_:
		mRenderCallbacks[DXWF_RENDERER_BEGIN_SCENE_LOOP_] = cCallbackFunction;
		break;
	case DXWF_RENDERER_RESET_:
		mRenderCallbacks[DXWF_RENDERER_RESET_] = cCallbackFunction;
		break;
	default:
		std::cout << "DXWF: Error #1 " << __FUNCTION__ << " () -> Unknown arg\n";
		break;
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (mWndProcCallbacks[DXWF_WNDPROC_WNDPROCHANDLER_] != nullptr)
		mWndProcCallbacks[DXWF_WNDPROC_WNDPROCHANDLER_](hWnd, message, wParam, lParam);

	switch (message)
	{
	case WM_SIZE:
		if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			g_d3dpp.BackBufferWidth = LOWORD(lParam);
			g_d3dpp.BackBufferHeight = HIWORD(lParam);

			if (mRenderCallbacks[DXWF_RENDERER_RESET_] != nullptr)
				mRenderCallbacks[DXWF_RENDERER_RESET_]();

			HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
			if (hr == D3DERR_INVALIDCALL)
				assert(0);

			if (mWndProcCallbacks[DXWF_WNDPROC_WM_SIZE_] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_SIZE_](hWnd, message, wParam, lParam);
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU)
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_MOUSEMOVE:
	{
		if (mWndProcCallbacks[DXWF_WNDPROC_WM_MOUSEMOVE_] != nullptr)
			mWndProcCallbacks[DXWF_WNDPROC_WM_MOUSEMOVE_](hWnd, message, wParam, lParam);
		break;
	}
	case WM_LBUTTONDOWN:
	{
		if (mWndProcCallbacks[DXWF_WNDPROC_WM_LBUTTONDOWN_] != nullptr)
			mWndProcCallbacks[DXWF_WNDPROC_WM_LBUTTONDOWN_](hWnd, message, wParam, lParam);
		break;
	}
	case WM_LBUTTONUP:
	{
		if (mWndProcCallbacks[DXWF_WNDPROC_WM_LBUTTONUP_] != nullptr)
			mWndProcCallbacks[DXWF_WNDPROC_WM_LBUTTONUP_](hWnd, message, wParam, lParam);
		break;
	}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void DXWFInitialization(
	HINSTANCE hInstance)
{
	phInstance = hInstance;
}

BOOL DXWFCreateWindow(
	LPCSTR szWindowName,
	const int iWindowPositionX, const int iWindowPositionY,
	const int iWindowSizeX, const int iWindowSizeY,
	DWORD dwWindowArg/*,
	const int pIcon = 0*/)
{
	pszWindowName = szWindowName;

	WNDCLASS wcWindowClass = { 0 };
	wcWindowClass.lpfnWndProc = (WNDPROC)WndProc;
	wcWindowClass.style = CS_HREDRAW | CS_VREDRAW;
	wcWindowClass.hInstance = phInstance;
	//wcWindowClass.hIcon = LoadIcon(phInstance, MAKEINTRESOURCE(0));
	wcWindowClass.lpszClassName = szWindowName;
	wcWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcWindowClass.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE;
	RegisterClass(&wcWindowClass);

	HWND hWnd = CreateWindow
	(
		szWindowName, _T(szWindowName), dwWindowArg,
		iWindowPositionX, iWindowPositionY,
		iWindowSizeX + 16, iWindowSizeY + 20,
		NULL, NULL, phInstance, NULL
	);

	phWindow = hWnd;

	if (!hWnd)
	{
		std::cout << "DXWF: Error #2 " << __FUNCTION__ << " () -> Failed to create window\n";
		return FALSE;
	}


	if ((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
	{
		std::cout << "DXWF: Error #3 " << __FUNCTION__ << " () -> Failed to create d3d context\n";
		UnregisterClass(szWindowName, phInstance);
		return FALSE;
	}

	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
	{
		pD3D->Release();
		UnregisterClass(szWindowName, phInstance);
		std::cout << "DXWF: Error #4 " << __FUNCTION__ << " () -> Failed to create device\n";
		return FALSE;
	}

	return TRUE;
}

HWND DXWFGetHWND()
{
	return phWindow;
}

LPDIRECT3DDEVICE9 DXWFGetD3DDevice()
{
	return g_pd3dDevice;
}

void DXWFRenderLoop()
{
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	ShowWindow(phWindow, SW_SHOWDEFAULT);
	UpdateWindow(phWindow);

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}

		if (mRenderCallbacks[DXWF_RENDERER_LOOP_] != nullptr)
			mRenderCallbacks[DXWF_RENDERER_LOOP_]();

		g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, false);
		g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
		g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);

		g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);

		if (g_pd3dDevice->BeginScene() >= 0)
		{
			if (mRenderCallbacks[DXWF_RENDERER_BEGIN_SCENE_LOOP_] != nullptr)
				mRenderCallbacks[DXWF_RENDERER_BEGIN_SCENE_LOOP_]();
			g_pd3dDevice->EndScene();
		}

		HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

		if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		{
			if (mRenderCallbacks[DXWF_RENDERER_RESET_] != nullptr)
				mRenderCallbacks[DXWF_RENDERER_RESET_]();
			HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
			if (hr == D3DERR_INVALIDCALL)
				assert(0);
		}
	}
}

void DXWFTerminate()
{
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
	if (pD3D) { pD3D->Release(); pD3D = NULL; }
	DestroyWindow(phWindow);
	UnregisterClass(pszWindowName.c_str(), phInstance);
}

