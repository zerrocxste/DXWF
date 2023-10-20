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
#include <windowsx.h>

#if TEST_OPENGL == 0
#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")
#include "framework.h"
#else
#include "framework.h"
#include "gl3w/glcorearb.h"
#include "gl3w/gl3winit.h"
#include "gl3w/gl3w.h"

#include <gl\GL.h>
#pragma comment(lib, "OpenGL32.lib")

#include <gl/Glu.h>
#pragma comment (lib, "Glu32.lib")

extern "C"
{
	WINGDIAPI void APIENTRY glTexCoord2f(GLfloat s, GLfloat t);
	WINGDIAPI void APIENTRY glPushMatrix(void);
	WINGDIAPI void APIENTRY glPopMatrix(void);
	WINGDIAPI void APIENTRY glBegin(GLenum mode);
	WINGDIAPI void APIENTRY glVertex2f(GLfloat x, GLfloat y);
	WINGDIAPI void APIENTRY glEnableClientState(GLenum array);
	WINGDIAPI void APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
	WINGDIAPI void APIENTRY glColor3f(GLfloat red, GLfloat green, GLfloat blue);
	WINGDIAPI void APIENTRY glDisableClientState(GLenum array);
	WINGDIAPI void APIENTRY glEnd(void);
	WINGDIAPI void APIENTRY glMatrixMode(GLenum mode);
	WINGDIAPI void APIENTRY glLoadIdentity(void);
	WINGDIAPI void APIENTRY glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	WINGDIAPI void APIENTRY glShadeModel(GLenum mode);
}

#define GL_BGR_EXT                        0x80E0

#define GL_MODELVIEW                      0x1700
#define GL_PROJECTION                     0x1701
#define GL_TEXTURE                        0x1702

#define GL_FLAT                           0x1D00
#define GL_SMOOTH                         0x1D01

#define GL_ALPHA_TEST                     0x0BC0

#define GL_CLAMP                          0x2900
#define GL_REPEAT                         0x2901
#endif // TEST_OPENGL == 0

#include <dwmapi.h>

typedef void (*callback_wndproc)(HWND, UINT, WPARAM, LPARAM);
typedef void (*callback)();

#if TEST_OPENGL == 0
LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
D3DPRESENT_PARAMETERS    g_d3dpp{};
LPDIRECT3D9				 pD3D = NULL;
#endif // TEST_OPENGL == 0

std::string szWindowName;
HWND phWindow;
HINSTANCE phInstance;
dxwf_color blur_col;

std::map<int, callback_wndproc>mWndProcCallbacks;
std::map<int, callback>mRenderCallbacks;

DWORD UserWindowFlags;
int iTitleBarYSize = 0;
int iMaxDontTrackLeftTopSize = 0;
int iMaxDontTrackRightTopSize = 0;
DWORD iMaxFPS = NULL;

HDC hDC;
HGLRC hRC;

namespace helpers_func
{
	DWORD color_to_argb(dxwf_color color)
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

void DrawScene()
{
	if (mRenderCallbacks[DXWF_RENDERER_LOOP] != nullptr)
		mRenderCallbacks[DXWF_RENDERER_LOOP]();

#if TEST_OPENGL == 0
	g_pd3dDevice->Clear(0, 0, D3DCLEAR_TARGET, 0, 1.0f, 0);

	if (g_pd3dDevice->BeginScene() >= 0)
	{
		if (mRenderCallbacks[DXWF_RENDERER_BEGIN_SCENE_LOOP] != nullptr)
			mRenderCallbacks[DXWF_RENDERER_BEGIN_SCENE_LOOP]();
		g_pd3dDevice->EndScene();
	}

	if (g_pd3dDevice->Present(NULL, NULL, NULL, NULL) == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
	{
		if (mRenderCallbacks[DXWF_RENDERER_RESET] != nullptr)
			mRenderCallbacks[DXWF_RENDERER_RESET]();
		HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
		if (hr == D3DERR_INVALIDCALL)
			assert(0);
	}
#else
	if (mRenderCallbacks[DXWF_RENDERER_BEGIN_SCENE_LOOP] != nullptr)
		mRenderCallbacks[DXWF_RENDERER_BEGIN_SCENE_LOOP]();

	SwapBuffers(hDC);
#endif // TEST_OPENGL == 0
}

void DXWFWndProcCallbacks(DXWF_WNDPROC_CALLBACKS WndProcCallbackNum, callback_wndproc cCallbackFunction)
{
	if (is_initialized == FALSE)
		return;

	switch (WndProcCallbackNum)
	{
	case DXWF_WNDPROC_WNDPROCHANDLER:
		mWndProcCallbacks[DXWF_WNDPROC_WNDPROCHANDLER] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_SIZE:
		mWndProcCallbacks[DXWF_WNDPROC_WM_SIZE] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_MOUSEMOVE:
		mWndProcCallbacks[DXWF_WNDPROC_WM_MOUSEMOVE] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_LBUTTONDOWN:
		mWndProcCallbacks[DXWF_WNDPROC_WM_LBUTTONDOWN] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_LBUTTONUP:
		mWndProcCallbacks[DXWF_WNDPROC_WM_LBUTTONUP] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_SYSKEYDOWN:
		mWndProcCallbacks[DXWF_WNDPROC_WM_SYSKEYDOWN] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_SYSCHAR:
		mWndProcCallbacks[DXWF_WNDPROC_WM_SYSCHAR] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_SYSKEYUP:
		mWndProcCallbacks[DXWF_WNDPROC_WM_SYSKEYUP] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_KEYDOWN:
		mWndProcCallbacks[DXWF_WNDPROC_WM_KEYDOWN] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_KEYUP:
		mWndProcCallbacks[DXWF_WNDPROC_WM_KEYUP] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_CHAR:
		mWndProcCallbacks[DXWF_WNDPROC_WM_CHAR] = cCallbackFunction;
		break;
	case DXWF_WNDPROC_WM_PAINT:
		mWndProcCallbacks[DXWF_WNDPROC_WM_PAINT] = cCallbackFunction;
		break;
	default:
		break;
	}
}

void DXWFRendererCallbacks(DXWF_RENDER_CALLBACKS RenderCallbackNum, callback cCallbackFunction)
{
	if (is_initialized == FALSE)
		return;

	switch (RenderCallbackNum)
	{
	case DXWF_RENDERER_LOOP:
		mRenderCallbacks[DXWF_RENDERER_LOOP] = cCallbackFunction;
		break;
	case DXWF_RENDERER_BEGIN_SCENE_LOOP:
		mRenderCallbacks[DXWF_RENDERER_BEGIN_SCENE_LOOP] = cCallbackFunction;
		break;
	case DXWF_RENDERER_RESET:
		mRenderCallbacks[DXWF_RENDERER_RESET] = cCallbackFunction;
		break;
	default:
		break;
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (mWndProcCallbacks[DXWF_WNDPROC_WNDPROCHANDLER] != nullptr)
		mWndProcCallbacks[DXWF_WNDPROC_WNDPROCHANDLER](hWnd, message, wParam, lParam);

	switch (message)
	{
		case WM_PAINT:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_PAINT] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_PAINT](hWnd, message, wParam, lParam);
			DrawScene();
			break;
		case WM_SIZE:
#if TEST_OPENGL == 0
			if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
			{
				g_d3dpp.BackBufferWidth = LOWORD(lParam);
				g_d3dpp.BackBufferHeight = HIWORD(lParam);

				if (mRenderCallbacks[DXWF_RENDERER_RESET] != nullptr)
					mRenderCallbacks[DXWF_RENDERER_RESET]();

				HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
				if (hr == D3DERR_INVALIDCALL)
					assert(0);

				if (mWndProcCallbacks[DXWF_WNDPROC_WM_SIZE] != nullptr)
					mWndProcCallbacks[DXWF_WNDPROC_WM_SIZE](hWnd, message, wParam, lParam);
			}
			return 0;
#endif // TEST_OPENGL == 0
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU)
				return 0;
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_MOUSEMOVE:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_MOUSEMOVE] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_MOUSEMOVE](hWnd, message, wParam, lParam);
			break;	
		case WM_LBUTTONDOWN:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_LBUTTONDOWN] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_LBUTTONDOWN](hWnd, message, wParam, lParam);
			break;
		case WM_LBUTTONUP:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_LBUTTONUP] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_LBUTTONUP](hWnd, message, wParam, lParam);
			break;
		case WM_SYSKEYDOWN:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_SYSKEYDOWN] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_SYSKEYDOWN](hWnd, message, wParam, lParam);
			break;
		case WM_SYSCHAR:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_SYSCHAR] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_SYSCHAR](hWnd, message, wParam, lParam);
			break;
		case WM_SYSKEYUP:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_SYSKEYUP] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_SYSKEYUP](hWnd, message, wParam, lParam);
			break;
		case WM_KEYDOWN:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_KEYDOWN] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_KEYDOWN](hWnd, message, wParam, lParam);
			break;
		case WM_KEYUP:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_KEYUP] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_KEYUP](hWnd, message, wParam, lParam);
			break;
		case WM_CHAR:
			if (mWndProcCallbacks[DXWF_WNDPROC_WM_CHAR] != nullptr)
				mWndProcCallbacks[DXWF_WNDPROC_WM_CHAR](hWnd, message, wParam, lParam);
			break;
		case WM_DWMCOLORIZATIONCOLORCHANGED:
			if (UserWindowFlags & USER_DXWF_FLAGS::ENABLE_BLUR_SYSTEM_COLORIZATION)
			{
				DWORD color = 0;
				BOOL opaque = FALSE;

				HRESULT hr = DwmGetColorizationColor(&color, &opaque);

				if (SUCCEEDED(hr))
				{
					color = helpers_func::argb_to_abgr(color);

					if (blur_col.a != 0)
					{
						std::uint8_t* color_arr = (std::uint8_t*)&color;
						color_arr[3] = blur_col.a;
					}

					ACCENT_STATE blur_type = (UserWindowFlags & USER_DXWF_FLAGS::ENABLE_BLUR_ACRYLIC) ? ACCENT_ENABLE_ACRYLICBLURBEHIND : ACCENT_ENABLE_BLURBEHIND;

					DWMACCENTPOLICY policy = { blur_type, FILL_WINDOW, color, 0 };

					WINCOMPATTR data = { WCA_ACCENT_POLICY, (WINCOMPATTR_DATA*)&policy, sizeof(WINCOMPATTR_DATA) };

					pfSetWindowCompositionAttribute(hWnd, &data);
				}
			}
			break;
		case WM_NCHITTEST:
			if (UserWindowFlags & ENABLE_POPUP_RESIZE)
			{
				POINT cursor{};
				cursor.x = GET_X_LPARAM(lParam);
				cursor.y = GET_Y_LPARAM(lParam);

				const POINT border{
					::GetSystemMetrics(SM_CXFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER),
					::GetSystemMetrics(SM_CYFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER)
				};
				RECT window;
				if (!::GetWindowRect(phWindow, &window)) {
					return HTNOWHERE;
				}

				enum region_mask {
					client = 0b0000,
					left = 0b0001,
					right = 0b0010,
					top = 0b0100,
					bottom = 0b1000,
				};

				const auto result =
					left * (cursor.x < (window.left + border.x)) |
					right * (cursor.x >= (window.right - border.x)) |
					top * (cursor.y < (window.top + border.y)) |
					bottom * (cursor.y >= (window.bottom - border.y));

				auto IsDeadZoneLeftTop = iMaxDontTrackLeftTopSize > 0 ? cursor.x - window.left >= iMaxDontTrackLeftTopSize : true;
				auto IsDeadZoneRightTop = iMaxDontTrackRightTopSize > 0 ? abs(cursor.x - window.right) >= iMaxDontTrackRightTopSize : true;

				if (IsDeadZoneLeftTop && IsDeadZoneRightTop && iTitleBarYSize > 0 && cursor.y - window.top <= iTitleBarYSize)
					return HTCAPTION;

				switch (result) {
				case left: 
					return HTLEFT;
				case right:
					return HTRIGHT;
				case top: 
					return HTTOP;
				case bottom: 
					return HTBOTTOM;
				case top | left:
					return HTTOPLEFT;
				case top | right: 
					return HTTOPRIGHT;
				case bottom | left:
					return HTBOTTOMLEFT;
				case bottom | right:
					return HTBOTTOMRIGHT;
				default: break;
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
		return is_initialized;
		
	if (!dwm_api)
		return is_initialized;

	auto set_window_composition_attribute_address = (fSetWindowCompositionAttribute)GetProcAddress(user32, "SetWindowCompositionAttribute");

	if (!set_window_composition_attribute_address)
		return is_initialized;

	auto dwm_extend_frame_into_client_area_address = (fDwmExtendFrameIntoClientArea)GetProcAddress(dwm_api, "DwmExtendFrameIntoClientArea");

	if (!dwm_extend_frame_into_client_area_address)
		return is_initialized;

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

HRESULT EnableBlurWin10(HWND hwnd, bool enable_system_colorization, bool enable_acrylic, dxwf_color blur_color)
{
	DWORD color = 0;
	BOOL opaque = FALSE;

	if (enable_system_colorization)
	{
		DwmGetColorizationColor(&color, &opaque);
		color = helpers_func::argb_to_abgr(color);
		
		if (blur_color.a != 0)
		{
			std::uint8_t* color_arr = (std::uint8_t*)&color;
			color_arr[3] = blur_color.a;
		}
	}
	else
		color = helpers_func::color_to_argb(blur_color);

	ACCENT_STATE blur_type = enable_acrylic ? ACCENT_ENABLE_ACRYLICBLURBEHIND : ACCENT_ENABLE_BLURBEHIND;

	DWMACCENTPOLICY policy = { blur_type, FILL_WINDOW, color, 0 };

	WINCOMPATTR data = { WCA_ACCENT_POLICY, (WINCOMPATTR_DATA*)&policy, sizeof(WINCOMPATTR_DATA) };

	return pfSetWindowCompositionAttribute(hwnd, &data);
}

HRESULT EnableBlurBehind(HWND hwnd, bool enable_blur, bool enable_system_colorization, bool enable_acrylic, dxwf_color blur_color)
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

void DXWFEnableBlur(DWORD blur_flags, dxwf_color blur_color)
{
	if (EnableBlurBehind(phWindow,
		blur_flags & USER_DXWF_FLAGS::ENABLE_WINDOW_BLUR,
		blur_flags & USER_DXWF_FLAGS::ENABLE_BLUR_SYSTEM_COLORIZATION,
		blur_flags & USER_DXWF_FLAGS::ENABLE_BLUR_ACRYLIC,
		blur_color) == S_OK)
		DXWFEnableTransparentWindow();
}

void DXWFDisableBlur()
{
	DWMACCENTPOLICY policy = { ACCENT_DISABLED, FILL_WINDOW, 0, 0 };
	WINCOMPATTR data = { WCA_ACCENT_POLICY, (WINCOMPATTR_DATA*)&policy, sizeof(WINCOMPATTR_DATA) };
	pfSetWindowCompositionAttribute(phWindow, &data);
}

void DXWFSetBlurColor(dxwf_color blur_color)
{
	if (!phWindow)
	{
		blur_col = blur_color;
		return;
	}

	DXWFEnableBlur(UserWindowFlags, blur_color);
}

void DXWFSetDragTitlebarYSize(int YSize)
{
	iTitleBarYSize = YSize;
}

void DXWFSetDragDeadZoneXOffset(int iTopLeftDeadZone, int iTopRightDeadZone)
{
	iMaxDontTrackLeftTopSize = iTopLeftDeadZone;
	iMaxDontTrackRightTopSize = iTopRightDeadZone;
}

BOOL DXWFCreateWindow(
	LPCSTR pszWindowName,
	const int iWindowPositionX, const int iWindowPositionY,
	const int iWindowSizeX, const int iWindowSizeY,
	DWORD dwWindowArg,
	DWORD dwExStyle,
	DWORD dx_window_flags,
	int pIcon)
{
	if (is_initialized == FALSE)
		return FALSE;

	UserWindowFlags = dx_window_flags;
	szWindowName = pszWindowName;

	WNDCLASS wcWindowClass = { 0 };
	wcWindowClass.lpfnWndProc = (WNDPROC)WndProc;
	wcWindowClass.style = CS_HREDRAW | CS_VREDRAW;
	wcWindowClass.hInstance = phInstance;
	wcWindowClass.hIcon = LoadIcon(phInstance, MAKEINTRESOURCE(pIcon));
	wcWindowClass.lpszClassName = pszWindowName;
	wcWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcWindowClass.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));

	RegisterClass(&wcWindowClass);

	phWindow = CreateWindowEx
	(
		dwExStyle,
		pszWindowName,
		_T(pszWindowName),
		dwWindowArg,
		iWindowPositionX, iWindowPositionY,
		iWindowSizeX, iWindowSizeY,
		NULL,
		NULL,
		NULL,
		NULL
	);

	if (!phWindow)
		return FALSE;

#if TEST_OPENGL == 0
	if ((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
	{
		UnregisterClass(pszWindowName, phInstance);
		return FALSE;
	}
#endif // TEST_OPENGL == 0

	EnableBlurBehind(phWindow, 
		UserWindowFlags & ENABLE_WINDOW_BLUR,
		UserWindowFlags & ENABLE_BLUR_SYSTEM_COLORIZATION,
		UserWindowFlags & ENABLE_BLUR_ACRYLIC,
		blur_col);

	if (UserWindowFlags & ENABLE_WINDOW_ALPHA)
		DXWFEnableTransparentWindow();
	
#if TEST_OPENGL == 0
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = (UserWindowFlags & ENABLE_WINDOW_ALPHA) ? D3DFMT_A8R8G8B8 : D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	HRESULT ret = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, phWindow, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice);

	if (ret < 0)
	{
		pD3D->Release();
		UnregisterClass(pszWindowName, phInstance);
		return FALSE;
	}
#else
	PIXELFORMATDESCRIPTOR pfd = { 0 };
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	hDC = GetDC(phWindow);
	int pixelFormat = ChoosePixelFormat(hDC, &pfd);
	SetPixelFormat(hDC, pixelFormat, &pfd);
	hRC = wglCreateContext(hDC);
	wglMakeCurrent(hDC, hRC);

	GL3::Initialize();

	glViewport(0.f, 0.f, iWindowSizeX, iWindowSizeY);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, iWindowSizeX, 0, iWindowSizeY);
#endif // TEST_OPENGL == 0

	return TRUE;
}

HWND DXWFGetHWND()
{
	return phWindow;
}

#if TEST_OPENGL == 0
LPDIRECT3DDEVICE9 DXWFGetD3DDevice()
{
	return g_pd3dDevice;
}
#endif // TEST_OPENGL == 0

void DXWFSetFramerateLimit(const DWORD iMaxFPs)
{
	iMaxFPS = iMaxFPs;
}

void DXWFRenderLoop()
{
	if (is_initialized == FALSE)
		return;

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

#if TEST_OPENGL == 1
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
#endif // TEST_OPENGL == 0

		DrawScene();

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
		return;

#if TEST_OPENGL == 0
	if (g_pd3dDevice) 
	{ 
		g_pd3dDevice->Release(); 
		g_pd3dDevice = NULL; 
	}

	if (pD3D) 
	{
		pD3D->Release(); pD3D = NULL; 
	}
#endif // TEST_OPENGL == 0

	iMaxFPS = 0;
	UserWindowFlags = 0;
	DestroyWindow(phWindow);
	UnregisterClass(szWindowName.c_str(), phInstance);
	phWindow = NULL;
}

