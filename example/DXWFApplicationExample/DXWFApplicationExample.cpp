#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <Windows.h>

#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")

#include "Include/d3dx9.h"
#pragma comment(lib, "Lib/x86/d3dx9.lib")

#include "DXWF.h"
#pragma comment (lib, "DXWF.lib")

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_internal.h"

namespace console
{
	bool isOpen = false;
	static void attach(LPCSTR pszConsoleName)
	{
		if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
			AllocConsole();
			isOpen = true;
		}
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
		SetConsoleTitle(pszConsoleName);
		SetWindowPos(GetConsoleWindow(), 0, 10, 10, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}

	static void detach()
	{
		if (isOpen)
			FreeConsole();
	}
}

void initializeImGui();

LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hprevInst, LPSTR, int)
{
	//console::attach("debug");

	DXWFWndProcCallbacks(DXWF_WNDPROC_WNDPROCHANDLER_,
		[](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
			ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);
		}
	);

	DXWFRendererCallbacks(DXWF_RENDERER_LOOP_,
		[]() {
			ImGui_ImplDX9_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			ImGui::ShowTestWindow();

			ImGui::EndFrame();
		}
	);

	DXWFRendererCallbacks(DXWF_RENDERER_BEGIN_SCENE_LOOP_,
		[]() {			
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		}
	);

	DXWFRendererCallbacks(DXWF_RENDERER_RESET_,
		[]() {
			ImGui_ImplDX9_InvalidateDeviceObjects();
		}
	);

	DXWFInitialization(hInst);
	
	if (DXWFCreateWindow("test window", 
		100, 100, 
		1100, 700, 
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_SIZEBOX | WS_MAXIMIZEBOX))
	{
		initializeImGui();
		DXWFRenderLoop();	
	}	

	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	
	DXWFTerminate();

	//console::detach();

	return 0;
}

void initializeImGui()
{
	ImGui::CreateContext();

	auto io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Tahoma.ttf", 13.9f, NULL, io.Fonts->GetGlyphRangesCyrillic());

	ImGui_ImplWin32_Init(DXWFGetHWND());
	ImGui_ImplDX9_Init(DXWFGetD3DDevice());

	ImGui::StyleColorsLight();
	ImGui::GetStyle().WindowRounding = 0.f;
}