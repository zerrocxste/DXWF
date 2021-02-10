// DXWF.cpp : Определяет функции для статической библиотеки.
//
#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <assert.h>
#include <map>

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

std::map<int, callback_wndproc>mWndProcCallbacks;
std::map<int, callback>mRenderCallbacks;

DWORD pDXWindowFlags;

const DWORD WCA_ACCENT_POLICY = 19;

typedef enum 
{
	ACCENT_DISABLED = 0,
	ACCENT_ENABLE_GRADIENT = 1,
	ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
	ACCENT_ENABLE_BLURBEHIND = 3,
	ACCENT_INVALID_STATE = 4,
	_ACCENT_STATE_SIZE = 0xFFFFFFFF
} ACCENT_STATE;

typedef struct 
{
	ACCENT_STATE accentState;
	int accentFlags;
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

using fDwmEnableBlurBehindWindow = HRESULT(WINAPI*)(HWND, const DWM_BLURBEHIND*);
fDwmEnableBlurBehindWindow pfDwmEnableBlurBehindWindow = NULL;

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
		return FALSE;
	}
		
	if (!dwm_api)
	{
		std::cout << "DXWF: Error #5 " << __FUNCTION__ << " () -> Failed to get dwmapi.dll handle\n";
		return FALSE;
	}

	auto set_window_composition_attribute_address = (fSetWindowCompositionAttribute)GetProcAddress(user32, "SetWindowCompositionAttribute");

	if (!set_window_composition_attribute_address)
	{
		std::cout << "DXWF: Error #5 " << __FUNCTION__ << " () -> Failed to get SetWindowCompositionAttribute address\n";
		return FALSE;
	}

	auto dwm_extend_frame_into_client_area_address = (fDwmExtendFrameIntoClientArea)GetProcAddress(dwm_api, "DwmExtendFrameIntoClientArea");

	if (!dwm_extend_frame_into_client_area_address)
	{
		std::cout << "DXWF: Error #5 " << __FUNCTION__ << " () -> Failed to get DwmExtendFrameIntoClientArea address\n";
		return FALSE;
	}

	auto dwm_enable_blur_behind_window_address = (fDwmEnableBlurBehindWindow)GetProcAddress(dwm_api, "DwmEnableBlurBehindWindow");
	
	if (!dwm_enable_blur_behind_window_address)
	{
		std::cout << "DXWF: Error #5 " << __FUNCTION__ << " () -> Failed to get DwmEnableBlurBehindWindow address\n";
		return FALSE;
	}

	pfSetWindowCompositionAttribute = set_window_composition_attribute_address;
	pfDwmExtendFrameIntoClientArea = dwm_extend_frame_into_client_area_address;
	pfDwmEnableBlurBehindWindow = dwm_enable_blur_behind_window_address;

	is_initialized = TRUE;

	return TRUE;
}

HRESULT EnableBlurBehind(HWND hwnd)
{
	HRESULT hr = S_OK;

	DWM_BLURBEHIND bb = { 0 };
	bb.dwFlags = DWM_BB_ENABLE;
	bb.fEnable = true;
	bb.hRgnBlur = NULL;
	hr = pfDwmEnableBlurBehindWindow(hwnd, &bb);

	DWMACCENTPOLICY policy = { ACCENT_ENABLE_BLURBEHIND, 0, 0, 0 };
	WINCOMPATTR data = { WCA_ACCENT_POLICY, (WINCOMPATTR_DATA*)&policy, sizeof(WINCOMPATTR_DATA) };
	hr = pfSetWindowCompositionAttribute(hwnd, &data);

	return hr;
}

BOOL DXWFCreateWindow(
	LPCSTR szWindowName,
	const int iWindowPositionX, const int iWindowPositionY,
	const int iWindowSizeX, const int iWindowSizeY,
	DWORD dwWindowArg,
	DWORD dwExStyle,
	DWORD dx_window_flags,
	int pIcon)
{
	if (is_initialized == FALSE)
	{
		std::cout << "DXWF: Error #5 " << __FUNCTION__ << " () -> DXWF not initialized\n";
		return FALSE;
	}

	pDXWindowFlags = dx_window_flags;
	pszWindowName = szWindowName;

	WNDCLASS wcWindowClass = { 0 };
	wcWindowClass.lpfnWndProc = (WNDPROC)WndProc;
	wcWindowClass.style = CS_HREDRAW | CS_VREDRAW;
	wcWindowClass.hInstance = phInstance;
	wcWindowClass.hIcon = LoadIcon(phInstance, MAKEINTRESOURCE(pIcon));
	wcWindowClass.lpszClassName = szWindowName;
	wcWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcWindowClass.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE;

	RegisterClass(&wcWindowClass);

	phWindow = CreateWindowEx(
		dwExStyle,
		szWindowName,
		_T(szWindowName),
		dwWindowArg,
		iWindowPositionX, iWindowPositionY,
		iWindowSizeX + 16, iWindowSizeY + 20,
		0,
		0,
		0,
		0);

	if (pDXWindowFlags & user_dxwf_flags::ENABLE_WINDOW_ALPHA)
	{
		SetLayeredWindowAttributes(phWindow, 0, 1.0f, LWA_ALPHA);
		SetLayeredWindowAttributes(phWindow, 0, RGB(0, 0, 0), LWA_COLORKEY);
	}

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

	if (pDXWindowFlags & user_dxwf_flags::ENABLE_WINDOW_BLUR)
	{
		EnableBlurBehind(phWindow);
	}

	if (pDXWindowFlags & user_dxwf_flags::ENABLE_WINDOW_ALPHA)
	{
		MARGINS margins = { -1 };
		pfDwmExtendFrameIntoClientArea(phWindow, &margins);
	}
	
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = (pDXWindowFlags & user_dxwf_flags::ENABLE_WINDOW_ALPHA) ? D3DFMT_A8R8G8B8 : D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, phWindow, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
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

LPDIRECT3DDEVICE9& DXWFGetD3DDevice()
{
	return g_pd3dDevice;
}

void DXWFRenderLoop()
{
	if (is_initialized == FALSE)
	{
		std::cout << "DXWF: Error #5 " << __FUNCTION__ << " () -> DXWF not initialized\n";
		return;
	}

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

		if (pDXWindowFlags & user_dxwf_flags::ENABLE_WINDOW_ALPHA)
		{
			g_pd3dDevice->SetPixelShader(NULL);
			g_pd3dDevice->SetVertexShader(NULL);
			g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, false);
			g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, false);
			g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
			g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, false);
			g_pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, true);
			g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
			g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
			g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		}
		else
		{
			g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, false);
			g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
		}

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
	DestroyWindow(phWindow);
	UnregisterClass(pszWindowName.c_str(), phInstance);
}

