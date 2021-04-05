//DirectX Window Framwork
//author: zerrocxste

#pragma once

#define DXWF_WNDPROC_WNDPROCHANDLER_ 0
#define DXWF_WNDPROC_WM_SIZE_ 1
#define DXWF_WNDPROC_WM_MOUSEMOVE_ 3
#define DXWF_WNDPROC_WM_LBUTTONDOWN_ 4
#define DXWF_WNDPROC_WM_LBUTTONUP_ 5
#define DXWF_WNDPROC_WM_SYSKEYDOWN_ 6
#define DXWF_WNDPROC_WM_SYSCHAR_ 7
#define DXWF_WNDPROC_WM_SYSKEYUP_ 8
#define DXWF_WNDPROC_WM_KEYDOWN_ 9
#define DXWF_WNDPROC_WM_KEYUP_ 10
#define DXWF_WNDPROC_WM_CHAR_ 11
#define DXWF_WNDPROC_WM_PAINT_ 12

#define DXWF_RENDERER_LOOP_ 0
#define DXWF_RENDERER_BEGIN_SCENE_LOOP_ 1
#define DXWF_RENDERER_RESET_ 2

enum user_dxwf_flags
{
	NONE = 0,
	ENABLE_WINDOW_ALPHA = 1 << 0,
	ENABLE_WINDOW_BLUR = 1 << 1,
	ENABLE_BLUR_ACRYLIC = 1 << 2,
	ENABLE_BLUR_SYSTEM_COLORIZATION = 1 << 3
};

struct my_color
{
	my_color(int R, int G, int B, int A) : r(R), g(G), b(B), a(A) {};
	int r, g, b, a;
};

typedef void (*callback_wndproc)(HWND, UINT, WPARAM, LPARAM);
typedef void (*callback)();

BOOL DXWFInitialization(
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
	DWORD dwWindowArg,
	DWORD dwExStyle,
	DWORD dx_window_flags,
	my_color blur_color,
	int pIcon);

HWND DXWFGetHWND();

LPDIRECT3DDEVICE9& DXWFGetD3DDevice();

void DXWFSetFramerateLimit(
	const DWORD iMaxFPs);

void DXWFRenderLoop();

void DXWFTerminate();


