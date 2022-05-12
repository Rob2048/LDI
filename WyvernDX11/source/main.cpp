#include <stdint.h>
#include <stdio.h>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

enum ldiTool {
	LDITOOL_NONE,
	LDITOOL_PLATFORM
};

struct ldiToolContext {
	ldiTool selectedTool = LDITOOL_NONE;
};

// Application and platform context.
struct ldiAppWin32 {
	HWND						hWnd;
	WNDCLASSEX					wc;

	ID3D11Device*				d3dDevice;
	ID3D11DeviceContext*		d3dDeviceContext;
	IDXGISwapChain*				SwapChain;
	ID3D11RenderTargetView*		mainRenderTargetView;

	ldiToolContext				tool;
};

ldiAppWin32 _appContext;

//----------------------------------------------------------------------------------------------------
// DX11.
//----------------------------------------------------------------------------------------------------
void _createPrimaryBackbuffer(ldiAppWin32* AppContext) {
	ID3D11Texture2D* pBackBuffer;
	AppContext->SwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	AppContext->d3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &AppContext->mainRenderTargetView);
	pBackBuffer->Release();
}

void _cleanupPrimaryBackbuffer(ldiAppWin32* AppContext) {
	if (AppContext->mainRenderTargetView) {
		AppContext->mainRenderTargetView->Release();
		AppContext->mainRenderTargetView = NULL;
	}
}

bool _createDeviceD3D(ldiAppWin32* AppContext) {
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = AppContext->hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	
	if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &AppContext->SwapChain, &AppContext->d3dDevice, &featureLevel, &AppContext->d3dDeviceContext) != S_OK)
		return false;

	_createPrimaryBackbuffer(AppContext);

	return true;
}

void _cleanupDeviceD3D(ldiAppWin32* AppContext) {
	_cleanupPrimaryBackbuffer(AppContext);
	if (AppContext->SwapChain) { AppContext->SwapChain->Release(); AppContext->SwapChain = NULL; }
	if (AppContext->d3dDeviceContext) { AppContext->d3dDeviceContext->Release(); AppContext->d3dDeviceContext = NULL; }
	if (AppContext->d3dDevice) { AppContext->d3dDevice->Release(); AppContext->d3dDevice = NULL; }
}

//----------------------------------------------------------------------------------------------------
// ImGUI helpers.
//----------------------------------------------------------------------------------------------------
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void _initImgui(ldiAppWin32* AppContext) {
	// Setup Dear ImGui context.
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = NULL;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(AppContext->hWnd);
	ImGui_ImplDX11_Init(AppContext->d3dDevice, AppContext->d3dDeviceContext);

	ImFont* font = io.Fonts->AddFontFromFileTTF("../assets/fonts/roboto/Roboto-Medium.ttf", 15.0f);
	IM_ASSERT(font != NULL);
}

void _renderImgui(ldiAppWin32* AppContext) {
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

//----------------------------------------------------------------------------------------------------
// Win32 window handling.
//----------------------------------------------------------------------------------------------------
// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg) {
		case WM_SIZE: {
			if (_appContext.d3dDevice != NULL && wParam != SIZE_MINIMIZED) {
				_cleanupPrimaryBackbuffer(&_appContext);
				_appContext.SwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
				_createPrimaryBackbuffer(&_appContext);
			}
			return 0;
		}

		case WM_SYSCOMMAND: {
			// Disable ALT application menu
			if ((wParam & 0xfff0) == SC_KEYMENU) {
				return 0;
			}
			break;
		}

		case WM_DESTROY: {
			PostQuitMessage(0);
			return 0;
		}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void _createWindow(ldiAppWin32* AppContext) {
	//ImGui_ImplWin32_EnableDpiAwareness();
	AppContext->wc = {
		sizeof(WNDCLASSEX),
		CS_CLASSDC,
		WndProc,
		0L,
		0L,
		GetModuleHandle(NULL),
		NULL,
		NULL,
		NULL, NULL,
		_T("WyvernDX11"),
		NULL
	};

	RegisterClassEx(&AppContext->wc);

	AppContext->hWnd = CreateWindow(AppContext->wc.lpszClassName, _T("Wyvern DX11"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720, NULL, NULL, AppContext->wc.hInstance, NULL);
}

void _unregisterWindow(ldiAppWin32* AppContext) {
	UnregisterClass(AppContext->wc.lpszClassName, AppContext->wc.hInstance);
}

//----------------------------------------------------------------------------------------------------
// Application main.
//----------------------------------------------------------------------------------------------------
int main() {
	std::cout << "Starting WyvernDX11\n";

	char dirBuff[512];
	GetCurrentDirectory(sizeof(dirBuff), dirBuff);
	std::cout << "Working directory: " << dirBuff << "\n";

	_createWindow(&_appContext);

	// Initialize Direct3D
	if (!_createDeviceD3D(&_appContext)) {
		_cleanupDeviceD3D(&_appContext);
		_unregisterWindow(&_appContext);
		return 1;
	}

	ShowWindow(_appContext.hWnd, SW_SHOWDEFAULT);
	UpdateWindow(_appContext.hWnd);

	_initImgui(&_appContext);

	// Application state.
	bool show_demo_window = true;
	bool showPlatformTool = false;

	ImVec4 clear_color = ImVec4(0.2f, 0.23f, 0.26f, 1.00f);

	// Main loop
	bool done = false;
	while (!done) {
		
		MSG msg;
		while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT) {
				done = true;
			}
		}

		if (done) {
			break;
		}

		// Render 3d viewport.
		// ...

		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		_appContext.d3dDeviceContext->OMSetRenderTargets(1, &_appContext.mainRenderTargetView, NULL);
		_appContext.d3dDeviceContext->ClearRenderTargetView(_appContext.mainRenderTargetView, clear_color_with_alpha);

		// Build IMGUI UI.
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// Viewport overlay.
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();

			ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always, ImVec2(0, 0));
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::Begin("__viewport", NULL, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMouseInputs);

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddText(ImVec2(100 + 50 * sin(ImGui::GetTime()), 100 + 50 * cos(ImGui::GetTime())), IM_COL32(255, 255, 255, 255), "This is some long text for testing purposes");

			ImGui::End();
		}

		if (ImGui::BeginMainMenuBar()) {

			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("New project")) {}
				if (ImGui::MenuItem("Open project")) {}
				if (ImGui::MenuItem("Save project")) {}
				if (ImGui::MenuItem("Save project as...")) {}
				ImGui::Separator();
				if (ImGui::MenuItem("Quit", "Alt+F4")) {
					done = true;
				}

				ImGui::EndMenu();
			}
			
			if (ImGui::BeginMenu("Workspace")) {
				if (ImGui::MenuItem("Platform")) {
					showPlatformTool = true;
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Scan")) {}
				if (ImGui::MenuItem("Process")) {}
				if (ImGui::MenuItem("Run")) {}
				ImGui::Separator();
				if (ImGui::MenuItem("Model inspector")) {}
				if (ImGui::MenuItem("Test bench")) {}
				if (ImGui::MenuItem("Image inspector")) {}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View")) {
				if (ImGui::MenuItem("Platform controls")) {}
				ImGui::Separator();
			}

			ImGui::EndMainMenuBar();
		}

		{
			// Viewport widgets.
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImVec2 work_pos = viewport->WorkPos;
			ImVec2 work_size = viewport->WorkSize;
			ImVec2 window_pos, window_pos_pivot;
			window_pos.x = work_pos.x;
			window_pos.y = work_pos.y;
			window_pos_pivot.x = 0.0f;
			window_pos_pivot.y = 0.0f;
			ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
			//ImGui::SetNextWindowSize(work_size);

			ImGui::SetNextWindowBgAlpha(0.35f);
			ImGui::Begin("Example: Simple overlay", NULL, window_flags);

			ImGui::Text("Time: (%f)", ImGui::GetTime());
			ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			bool showWire = true;
			if (ImGui::Checkbox("Wireframe", &showWire)) {}
			ImGui::End();
		}

		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;

			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		_renderImgui(&_appContext);

		_appContext.SwapChain->Present(1, 0);
	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	_cleanupDeviceD3D(&_appContext);
	DestroyWindow(_appContext.hWnd);
	_unregisterWindow(&_appContext);

	return 0;
}