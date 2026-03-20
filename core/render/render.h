#pragma once
#include <imgui.h>
#include <d3d11.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui_settings.hpp>

class Render {
private:
	static inline ID3D11Device* g_pd3dDevice = nullptr;
	static inline ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
	static inline IDXGISwapChain* g_pSwapChain = nullptr;
	static inline bool g_SwapChainOccluded = false;
	static inline UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
	static inline ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
public:
	static void Initialize();
	static bool CreateDeviceD3D(HWND hWnd);
	static void CleanupDeviceD3D();
	static void CreateRenderTarget();
	static void CleanupRenderTarget();

	static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};