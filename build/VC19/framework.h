//DirectX Window Framwork
//author: zerrocxste

#pragma once

#define DXWF_WNDPROC_WNDPROCHANDLER_ 0
#define DXWF_WNDPROC_WM_SIZE_ 1
#define DXWF_WNDPROC_WM_MOUSEMOVE_ 3
#define DXWF_WNDPROC_WM_LBUTTONDOWN_ 4
#define DXWF_WNDPROC_WM_LBUTTONUP_ 5

#define DXWF_RENDERER_LOOP_ 0
#define DXWF_RENDERER_BEGIN_SCENE_LOOP_ 1
#define DXWF_RENDERER_RESET_ 2

typedef void (*callback_wndproc)(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
typedef void (*callback)();

void DXWFInitialization(
	HINSTANCE hInstance);

void DXWFWndProcCallbacks(
	DWORD pWndProcAttr,
	callback_wndproc a);

void DXWFRendererCallbacks(
	DWORD pWndProcAttr, 
	callback cCallbackFunction);

BOOL DXWFCreateWindow(
	LPCSTR szWindowName,
	const int iWindowPositionX, const int iWindowPositionY,
	const int iWindowSizeX, const int iWindowSizeY,
	DWORD dwWindowArg/*,
	const int pIcon = 0*/);

HWND DXWFGetHWND();

LPDIRECT3DDEVICE9 DXWFGetD3DDevice();

void DXWFRenderLoop();

void DXWFTerminate();


