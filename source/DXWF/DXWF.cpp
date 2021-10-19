// DXWF.cpp : Определяет функции для статической библиотеки.
//

//DXWF: simple library for creating directx window
//PASTED by zerrocxste
//last upd 15.03.21
//added: show window post frame (fix spermi na ekrane)

#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <assert.h>
#include <map>
#include <time.h>

#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")

#include <dwmapi.h>

#include "framework.h"

typedef void (*callback_wndproc)(HWND, UINT, WPARAM, LPARAM);
typedef void (*callback)();

LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
D3DPRESENT_PARAMETERS    g_d3dpp{};
LPDIRECT3D9 pD3D = NULL;

std::string pszWindowName;
HWND phWindow;
HINSTANCE phInstance;
my_color blur_col;

std::map<int, callback_wndproc>mWndProcCallbacks;
std::map<int, callback>mRenderCallbacks;

DWORD pDXWindowFlags;

DWORD iMaxFPS = NULL;

namespace helpers_func
{
	DWORD color_to_argb(my_color color)
	{
		return (DWORD)((color.a << 24) | (color.b << 16) | (color.g << 8) | (color.r));
	}

	int argb_to_abgr(int argbColor) {
		int r = (argbColor >> 16) & 0xFF;
		int b = argbColor & 0xFF;
		return (argbColor & 0xFF00FF00) | (b << 16) | r;
	}
}

const DWORD WCA_ACCENT_POLICY = 19;

typedef enum 
{
	ACCENT_DISABLED = 0,
	ACCENT_ENABLE_GRADIENT = 1,
	ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
	ACCENT_ENABLE_BLURBEHIND = 3,
	ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
	_ACCENT_STATE_SIZE = 0xFFFFFFFF
} ACCENT_STATE;

typedef enum
{
	FLAGS_DISABLED,
	FILL_WINDOW = 2,
	FILL_WINDOW_AND_RIGHT_BOTTOM = 4,
	FILL_ALL = 6
} ACCENT_FLAGS;

typedef struct 
{
	ACCENT_STATE accentState;
	ACCENT_FLAGS accentFlags;
	int gradientColor;
	int invalidState;
} DWMACCENTPOLICY;

typedef struct _WINCOMPATTR_DATA 
{
	DWMACCENTPOLICY AccentPolicy;
} WINCOMPATTR_DATA;

typedef struct tagWINCOMPATTR
{
	DWORD attribute; // the attribute to query
	WINCOMPATTR_DATA* pData; // buffer to store the result
	ULONG dataSize; // size of the pData buffer
} WINCOMPATTR;

using fSetWindowCompositionAttribute = HRESULT(WINAPI*)(HWND, WINCOMPATTR*);
fSetWindowCompositionAttribute pfSetWindowCompositionAttribute = NULL;

using fDwmExtendFrameIntoClientArea = HRESULT(WINAPI*)(HWND, const MARGINS*);
fDwmExtendFrameIntoClientArea pfDwmExtendFrameIntoClientArea = NULL;

BOOL is_initialized = FALSE;

void DXWFWndProcCallbacks(DWORD pWndProcAttr, callback_wndproc cCallbackFunction)
{
	if (is_initialized == FALSE)
	{
		std::cout << "DXWF: Error #5 " << __FUNCTION__ << " () -> DXWF not initialized\n";
		return;
	}

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
	case DXWF_WNDPROC_WM_SYSKEYDOWN_:
		mWndProcCallbacks[DXWF_WNDPROC_WM_SYSKEYDOWN_] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_SYSCHAR_:
		mWndProcCallbacks[DXWF_WNDPROC_WM_SYSCHAR_] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_SYSKEYUP_:
		mWndProcCallbacks[DXWF_WNDPROC_WM_SYSKEYUP_] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_KEYDOWN_:
		mWndProcCallbacks[DXWF_WNDPROC_WM_KEYDOWN_] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_KEYUP_:
		mWndProcCallbacks[DXWF_WNDPROC_WM_KEYUP_] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_CHAR_:
		mWndProcCallbacks[DXWF_WNDPROC_WM_CHAR_] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_PAINT_:
		mWndProcCallbacks[DXWF_WNDPROC_WM_PAINT_] = cCallbackFunction;
		break;
	default:
		std::cout << "DXWF: Error #1 " << __FUNCTION__ << " () -> Unknown arg\n";
		break;
	}
}

void DXWFRendererCallbacks(DWORD pWndProcAttr, callback cCallbackFunction)
{
	if (is_initialized == FALSE)
	{
		std::cout << "DXWF: Error #5 " << __FUNCTION__ << " () -> DXWF not initialized\n";
		return;
	}

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
		case WM_PAINT:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_PAINT_] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_PAINT_](hWnd, message, wParam, lParam);
			break;
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
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_MOUSEMOVE_] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_MOUSEMOVE_](hWnd, message, wParam, lParam);
			break;	
		case WM_LBUTTONDOWN:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_LBUTTONDOWN_] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_LBUTTONDOWN_](hWnd, message, wParam, lParam);
			break;
		case WM_LBUTTONUP:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_LBUTTONUP_] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_LBUTTONUP_](hWnd, message, wParam, lParam);
			break;
		case WM_SYSKEYDOWN:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_SYSKEYDOWN_] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_SYSKEYDOWN_](hWnd, message, wParam, lParam);
			break;
		case WM_SYSCHAR:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_SYSCHAR_] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_SYSCHAR_](hWnd, message, wParam, lParam);
			break;
		case WM_SYSKEYUP:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_SYSKEYUP_] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_SYSKEYUP_](hWnd, message, wParam, lParam);
			break;
		case WM_KEYDOWN:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_KEYDOWN_] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_KEYDOWN_](hWnd, message, wParam, lParam);
			break;
		case WM_KEYUP:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_KEYUP_] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_KEYUP_](hWnd, message, wParam, lParam);
			break;
		case WM_CHAR:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_CHAR_] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_CHAR_](hWnd, message, wParam, lParam);
			break;
		case WM_DWMCOLORIZATIONCOLORCHANGED:
			if (pDXWindowFlags & user_dxwf_flags::ENABLE_BLUR_SYSTEM_COLORIZATION)
			{
				DWORD color = 0;
				BOOL opaque = FALSE;

				HRESULT hr = DwmGetColorizationColor(&color, &opaque);

				if (SUCCEEDED(hr))
				{
					color = helpers_func::argb_to_abgr(color);

					ACCENT_STATE blur_type = (pDXWindowFlags & user_dxwf_flags::ENABLE_BLUR_ACRYLIC) ? ACCENT_ENABLE_ACRYLICBLURBEHIND : ACCENT_ENABLE_BLURBEHIND;

					DWMACCENTPOLICY policy = { blur_type, FILL_WINDOW, color, 0 };

					WINCOMPATTR data = { WCA_ACCENT_POLICY, (WINCOMPATTR_DATA*)&policy, sizeof(WINCOMPATTR_DATA) };

					pfSetWindowCompositionAttribute(hWnd, &data);
				}
			}
			break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

BOOL DXWFInitialization(
	HINSTANCE hInstance)
{
	phInstance = hInstance;

	auto user32 = GetModuleHandle("user32.dll");
	auto dwm_api = GetModuleHandle("Dwmapi.dll");

	if (!user32)
	{
		std::cout << "DXWF: Error #5 " << __FUNCTION__ << " () -> Failed to get user32.dll handle\n";
		return is_initialized;
	}
		
	if (!dwm_api)
	{
		std::cout << "DXWF: Error #5 " << __FUNCTION__ << " () -> Failed to get dwmapi.dll handle\n";
		return is_initialized;
	}

	auto set_window_composition_attribute_address = (fSetWindowCompositionAttribute)GetProcAddress(user32, "SetWindowCompositionAttribute");

	if (!set_window_composition_attribute_address)
	{
		std::cout << "DXWF: Error #5 " << __FUNCTION__ << " () -> Failed to get SetWindowCompositionAttribute address\n";
		return is_initialized;
	}

	auto dwm_extend_frame_into_client_area_address = (fDwmExtendFrameIntoClientArea)GetProcAddress(dwm_api, "DwmExtendFrameIntoClientArea");

	if (!dwm_extend_frame_into_client_area_address)
	{
		std::cout << "DXWF: Error #5 " << __FUNCTION__ << " () -> Failed to get DwmExtendFrameIntoClientArea address\n";
		return is_initialized;
	}

	pfSetWindowCompositionAttribute = set_window_composition_attribute_address;
	pfDwmExtendFrameIntoClientArea = dwm_extend_frame_into_client_area_address;

	is_initialized = TRUE;

	return is_initialized;
}

HRESULT EnableBlurWin7(HWND hwnd)
{
	HRESULT hr = S_OK;
	DWM_BLURBEHIND bb = { 0 };
	bb.dwFlags = DWM_BB_ENABLE;
	bb.fEnable = true;
	bb.hRgnBlur = NULL;
	hr = DwmEnableBlurBehindWindow(hwnd, &bb);
	MARGINS margins = { -1 };
	hr = DwmExtendFrameIntoClientArea(hwnd, &margins);
	return hr;
}

HRESULT EnableBlurWin10(HWND hwnd, bool enable_system_colorization, bool enable_acrylic, my_color blur_color)
{
	DWORD color = 0;
	BOOL opaque = FALSE;

	if (enable_system_colorization)
	{
		DwmGetColorizationColor(&color, &opaque);
		color = helpers_func::argb_to_abgr(color);
	}
	else
		color = helpers_func::color_to_argb(blur_color);

	ACCENT_STATE blur_type = enable_acrylic ? ACCENT_ENABLE_ACRYLICBLURBEHIND : ACCENT_ENABLE_BLURBEHIND;

	DWMACCENTPOLICY policy = { blur_type, FILL_WINDOW, color, 0 };

	WINCOMPATTR data = { WCA_ACCENT_POLICY, (WINCOMPATTR_DATA*)&policy, sizeof(WINCOMPATTR_DATA) };

	return pfSetWindowCompositionAttribute(hwnd, &data);
}

HRESULT EnableBlurBehind(HWND hwnd, bool enable_blur, bool enable_system_colorization, bool enable_acrylic, my_color blur_color)
{
	if (enable_blur || enable_acrylic)
	{
		EnableBlurWin7(hwnd);

		EnableBlurWin10(hwnd, enable_system_colorization, enable_acrylic, blur_color);

		return S_OK;
	}
	return S_FALSE;
}

void DXWFSetWindowVisibleState(bool show)
{
	show ? ShowWindow(phWindow, SW_SHOW) : ShowWindow(phWindow, SW_MINIMIZE);
}

void DXWFSetWindowHideState(bool show)
{
	show ? ShowWindow(phWindow, SW_SHOW) : ShowWindow(phWindow, SW_HIDE);
}

void DXWFSetWindowPos(int x, int y)
{
	SetWindowPos(phWindow, NULL, x, y, 0, 0, SWP_NOSIZE);
}

void DXWFSetWindowSize(int x, int y)
{
	SetWindowPos(phWindow, NULL, 0, 0, x, y, SWP_NOMOVE);
}

void DXWFEnableTransparentWindow()
{
	SetLayeredWindowAttributes(phWindow, 0, 255, LWA_ALPHA);
	MARGINS margins = { -1 };
	pfDwmExtendFrameIntoClientArea(phWindow, &margins);
}

void DXWFDisableTransparentWindow()
{
	SetLayeredWindowAttributes(phWindow, 0, 0, LWA_ALPHA);
}

void DXWFEnableBlur(DWORD blur_flags, my_color blur_color)
{
	if (EnableBlurBehind(phWindow,
		blur_flags & user_dxwf_flags::ENABLE_WINDOW_BLUR,
		blur_flags & user_dxwf_flags::ENABLE_BLUR_SYSTEM_COLORIZATION,
		blur_flags & user_dxwf_flags::ENABLE_BLUR_ACRYLIC,
		blur_color) == S_OK)
		DXWFEnableTransparentWindow();
}

void DXWFDisableBlur()
{
	DWMACCENTPOLICY policy = { ACCENT_DISABLED, FILL_WINDOW, 0, 0 };
	WINCOMPATTR data = { WCA_ACCENT_POLICY, (WINCOMPATTR_DATA*)&policy, sizeof(WINCOMPATTR_DATA) };
	pfSetWindowCompositionAttribute(phWindow, &data);
}

BOOL DXWFCreateWindow(
	LPCSTR szWindowName,
	const int iWindowPositionX, const int iWindowPositionY,
	const int iWindowSizeX, const int iWindowSizeY,
	DWORD dwWindowArg,
	DWORD dwExStyle,
	DWORD dx_window_flags,
	my_color blur_color,
	int pIcon)
{
	if (is_initialized == FALSE)
	{
		std::cout << "DXWF: Error #5 " << __FUNCTION__ << " () -> DXWF not initialized\n";
		return FALSE;
	}

	pDXWindowFlags = dx_window_flags;
	pszWindowName = szWindowName;
	blur_col = blur_color;

	WNDCLASS wcWindowClass = { 0 };
	wcWindowClass.lpfnWndProc = (WNDPROC)WndProc;
	wcWindowClass.style = CS_HREDRAW | CS_VREDRAW;
	wcWindowClass.hInstance = phInstance;
	wcWindowClass.hIcon = LoadIcon(phInstance, MAKEINTRESOURCE(pIcon));
	wcWindowClass.lpszClassName = szWindowName;
	wcWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcWindowClass.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));

	RegisterClass(&wcWindowClass);

	phWindow = CreateWindowEx
	(
		dwExStyle,
		szWindowName,
		_T(szWindowName),
		dwWindowArg,
		iWindowPositionX, iWindowPositionY,
		iWindowSizeX, iWindowSizeY,
		NULL,
		NULL,
		NULL,
		NULL
	);

	if (!phWindow)
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

	EnableBlurBehind(phWindow, 
		pDXWindowFlags & user_dxwf_flags::ENABLE_WINDOW_BLUR,
		pDXWindowFlags & user_dxwf_flags::ENABLE_BLUR_SYSTEM_COLORIZATION, 
		pDXWindowFlags & user_dxwf_flags::ENABLE_BLUR_ACRYLIC, 
		blur_col);

	if (pDXWindowFlags & user_dxwf_flags::ENABLE_WINDOW_ALPHA)
		DXWFEnableTransparentWindow();
	
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = (pDXWindowFlags & user_dxwf_flags::ENABLE_WINDOW_ALPHA) ? D3DFMT_A8R8G8B8 : D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	HRESULT ret = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, phWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice);

	if (ret < 0)
	{
		pD3D->Release();
		UnregisterClass(szWindowName, phInstance);
		std::cout << "DXWF: Error #4 " << __FUNCTION__ << " () -> Failed to create device. CreateDevice code: " << std::hex << ret << std::dec << std::endl;
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

void DXWFSetFramerateLimit(const DWORD iMaxFPs)
{
	iMaxFPS = iMaxFPs;
}

void DXWFRenderLoop()
{
	if (is_initialized == FALSE)
	{
		std::cout << "DXWF: Error #5 " << __FUNCTION__ << " () -> DXWF not initialized\n";
		return;
	}

	static DWORD LastFrameTime = 0;

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

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

		g_pd3dDevice->Clear(0, 0, D3DCLEAR_TARGET, 0, 1.0f, 0);

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

		static auto show_post_frame = []()
		{
			ShowWindow(phWindow, SW_SHOWDEFAULT);
			UpdateWindow(phWindow);
			return true;
		}();

		if ((int)iMaxFPS != 0)
		{
			DWORD currentTime = timeGetTime();
			if ((currentTime - LastFrameTime) < (1000 / iMaxFPS))
			{
				Sleep(currentTime - LastFrameTime);
			}
			LastFrameTime = currentTime;
		}
	}
}

void DXWFTerminate()
{
	if (is_initialized == FALSE)
	{
		std::cout << "DXWF: Error #5 " << __FUNCTION__ << " () -> DXWF not initialized\n";
		return;
	}

	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
	if (pD3D) { pD3D->Release(); pD3D = NULL; }
	pDXWindowFlags = NULL;
	DestroyWindow(phWindow);
	UnregisterClass(pszWindowName.c_str(), phInstance);
	phWindow = NULL;
}

