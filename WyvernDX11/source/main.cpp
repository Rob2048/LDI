#include <stdint.h>
#include <stdio.h>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <d3dcompiler.h>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

using namespace glm;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

// NOTE: Using openGL default co-ords.
const vec3 vec3Forward(0, 0, -1);
const vec3 vec3Right(1, 0, 0);
const vec3 vec3Up(0, 1, 0);
const vec3 vec3Zero(0, 0, 0);
const vec3 vec3One(1, 1, 1);

enum ldiTool {
	LDITOOL_NONE = 0,
	LDITOOL_PLATFORM = 1
};

struct ldiToolContext {
	ldiTool selectedTool = LDITOOL_NONE;
};

struct ldiBasicVertexConstantBuffer {
	mat4 mvp;
};

struct ldiBasicVertex {
	vec3 position;
	vec3 color;
	vec2 uv;
};

// Application and platform context.
struct ldiAppWin32 {
	HWND						hWnd;
	WNDCLASSEX					wc;
	uint32_t					windowWidth;
	uint32_t					windowHeight;

	ID3D11Device*				d3dDevice;
	ID3D11DeviceContext*		d3dDeviceContext;
	IDXGISwapChain*				SwapChain;
	ID3D11RenderTargetView*		mainRenderTargetView;

	ID3D11DepthStencilView*		depthStencilView;
	ID3D11Texture2D*			depthStencilBuffer;

	ldiToolContext				tool;

	bool						showPlatformWindow = false;

	ID3D11VertexShader*			basicVertexShader;
	ID3D11PixelShader*			basicPixelShader;
	ID3D11InputLayout*			basicInputLayout;
	ID3D11Buffer*				basicVertexConstantBuffer;

	ID3D11Buffer*				testShapeVertexBuffer;
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


	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = AppContext->windowWidth;
	depthStencilDesc.Height = AppContext->windowHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	AppContext->d3dDevice->CreateTexture2D(&depthStencilDesc, NULL, &AppContext->depthStencilBuffer);
	AppContext->d3dDevice->CreateDepthStencilView(AppContext->depthStencilBuffer, NULL, &AppContext->depthStencilView);
}

void _cleanupPrimaryBackbuffer(ldiAppWin32* AppContext) {
	if (AppContext->mainRenderTargetView) {
		AppContext->mainRenderTargetView->Release();
		AppContext->mainRenderTargetView = NULL;
	}

	if (AppContext->depthStencilView) {
		AppContext->depthStencilView->Release();
		AppContext->depthStencilView = NULL;
	}

	if (AppContext->depthStencilBuffer) {
		AppContext->depthStencilBuffer->Release();
		AppContext->depthStencilBuffer = NULL;
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
			uint32_t width = (UINT)LOWORD(lParam);
			uint32_t height = (UINT)HIWORD(lParam);
			//std::cout << "Resize: " << width << ", " << height << "\n";

			_appContext.windowWidth = width;
			_appContext.windowHeight = height;

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

	AppContext->hWnd = CreateWindow(AppContext->wc.lpszClassName, _T("Wyvern DX11"), WS_OVERLAPPEDWINDOW, 100, 100, 1600, 900, NULL, NULL, AppContext->wc.hInstance, NULL);
}

void _unregisterWindow(ldiAppWin32* AppContext) {
	UnregisterClass(AppContext->wc.lpszClassName, AppContext->wc.hInstance);
}

//----------------------------------------------------------------------------------------------------
// Graphics.
//----------------------------------------------------------------------------------------------------
bool initRendering(ldiAppWin32* AppContext) {
	std::cout << "Compiling shaders\n";

	ID3DBlob* errorBlob;
	ID3DBlob* shaderBlob;

	if (FAILED(D3DCompileFromFile(L"../assets/shaders/basic.vs", NULL, NULL, "main", "vs_5_0", 0, 0, &shaderBlob, &errorBlob))) {
		std::cout << "Failed to compile shader\n";
		if (errorBlob != NULL) {
			std::cout << (const char*)errorBlob->GetBufferPointer() << "\n";
			errorBlob->Release();
		}
		return false;
	}

	if (AppContext->d3dDevice->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &AppContext->basicVertexShader) != S_OK) {
		shaderBlob->Release();
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC basicLayout[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0,	DXGI_FORMAT_R32G32_FLOAT,		0,	24,	D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	if (AppContext->d3dDevice->CreateInputLayout(basicLayout, 3, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), &AppContext->basicInputLayout) != S_OK) {
		shaderBlob->Release();
		return false;
	}

	shaderBlob->Release();

	// TODO: Failure at this point means many resources to cleanup.

	if (FAILED(D3DCompileFromFile(L"../assets/shaders/basic.ps", NULL, NULL, "main", "ps_5_0", 0, 0, &shaderBlob, &errorBlob))) {
		std::cout << "Failed to compile shader\n";
		if (errorBlob != NULL) {
			std::cout << (const char*)errorBlob->GetBufferPointer() << "\n";
			errorBlob->Release();
		}
		return false;
	}

	if (AppContext->d3dDevice->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &AppContext->basicPixelShader) != S_OK) {
		shaderBlob->Release();
		return false;
	}

	shaderBlob->Release();

	// Create basic constant buffer.
	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = sizeof(ldiBasicVertexConstantBuffer);
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	AppContext->d3dDevice->CreateBuffer(&desc, NULL, &AppContext->basicVertexConstantBuffer);

	// Create vertex buffer for test shape.
	ldiBasicVertex testShapeVerts[] = {
		{ vec3(0.0f, 0.2f, -10.0f),		vec3(1.0f, 0.0f, 0.0f),		vec2(0.0f, 0.0f) },
		{ vec3(0.5f, 1.0f, -10.0f),		vec3(0.0f, 1.0f, 0.0f),		vec2(0.0f, 0.0f) },
		{ vec3(1.0f, 0.0f, -10.0f),		vec3(0.0f, 0.0f, 1.0f),		vec2(0.0f, 0.0f) },

		{ vec3(0.0f, 0.0f, -10.1f),		vec3(0.0f, 1.0f, 1.0f),		vec2(0.0f, 0.0f) },
		{ vec3(0.25f, 0.5f, -10.1f),		vec3(1.0f, 1.0f, 0.0f),		vec2(0.0f, 0.0f) },
		{ vec3(1.0f, 0.0f, -10.1f),		vec3(1.0f, 0.0f, 1.0f),		vec2(0.0f, 0.0f) },
	};

	D3D11_BUFFER_DESC vbDesc;
	ZeroMemory(&vbDesc, sizeof(vbDesc));
	vbDesc.Usage = D3D11_USAGE_DYNAMIC;
	vbDesc.ByteWidth = sizeof(testShapeVerts);
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	AppContext->d3dDevice->CreateBuffer(&vbDesc, NULL, &AppContext->testShapeVertexBuffer);

	D3D11_MAPPED_SUBRESOURCE ms;
	AppContext->d3dDeviceContext->Map(AppContext->testShapeVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	memcpy(ms.pData, testShapeVerts, sizeof(testShapeVerts));
	AppContext->d3dDeviceContext->Unmap(AppContext->testShapeVertexBuffer, 0);

	return true;
}

void render(ldiAppWin32* AppContext) {
	// Camera.
	vec3 camRot(20, 0, 0);
	vec3 camPos(0, 0, 0);

	mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(camRot.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(camRot.x), vec3Up);
	mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -camPos);

	mat4 projMat = perspectiveFovRH_ZO(radians(50.0f), (float)AppContext->windowWidth, (float)AppContext->windowHeight, 0.01f, 100.0f);
	mat4 invProjMat = inverse(projMat);
	mat4 modelMat = mat4(1.0f);

	mat4 projViewModelMat = projMat * camViewMat * modelMat;

	D3D11_MAPPED_SUBRESOURCE ms;
	if (AppContext->d3dDeviceContext->Map(AppContext->basicVertexConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms) != S_OK)
		return;

	ldiBasicVertexConstantBuffer* constantBuffer = (ldiBasicVertexConstantBuffer*)ms.pData;
	memcpy(&constantBuffer->mvp, &projViewModelMat, sizeof(projViewModelMat));
	AppContext->d3dDeviceContext->Unmap(AppContext->basicVertexConstantBuffer, 0);

	AppContext->d3dDeviceContext->VSSetShader(AppContext->basicVertexShader, 0, 0);
	AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &AppContext->basicVertexConstantBuffer);
	AppContext->d3dDeviceContext->PSSetShader(AppContext->basicPixelShader, 0, 0);
	AppContext->d3dDeviceContext->IASetInputLayout(AppContext->basicInputLayout);

	UINT stride = sizeof(ldiBasicVertex);
	UINT offset = 0;
	AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &AppContext->testShapeVertexBuffer, &stride, &offset);
	AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	AppContext->d3dDeviceContext->Draw(6, 0);
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
	ldiAppWin32* appContext = &_appContext;
	bool showDemoWindow = false;

	// D3D setup.
	if (!initRendering(&_appContext)) {
		return 1;
	}

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
		_appContext.d3dDeviceContext->OMSetRenderTargets(1, &_appContext.mainRenderTargetView, _appContext.depthStencilView);

		D3D11_VIEWPORT viewport;
		ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = _appContext.windowWidth;
		viewport.Height = _appContext.windowHeight;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		_appContext.d3dDeviceContext->RSSetViewports(1, &viewport);

		const float clearColor[4] = { 0.2f, 0.23f, 0.26f, 1.00f };
		_appContext.d3dDeviceContext->ClearRenderTargetView(_appContext.mainRenderTargetView, clearColor);
		_appContext.d3dDeviceContext->ClearDepthStencilView(_appContext.depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		render(&_appContext);

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

			if (appContext->tool.selectedTool == LDITOOL_PLATFORM) {
				if (ImGui::BeginMenu("Platform")) {
					if (ImGui::MenuItem("New configuration")) {}
					if (ImGui::MenuItem("Open configuration")) {}
					if (ImGui::MenuItem("Save configuration")) {}
					if (ImGui::MenuItem("Save configuration as...")) {}
					ImGui::EndMenu();
				}
			}
			
			if (ImGui::BeginMenu("Workspace")) {
				if (ImGui::MenuItem("Platform")) {
					appContext->tool.selectedTool = LDITOOL_PLATFORM;
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
				if (ImGui::MenuItem("ImGUI demo window", NULL, showDemoWindow)) {
					showDemoWindow = !showDemoWindow;
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Platform controls", NULL, appContext->showPlatformWindow)) {
					appContext->showPlatformWindow = !appContext->showPlatformWindow;
				}
				ImGui::EndMenu();
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
			window_pos.x = work_pos.x + 10;
			window_pos.y = work_pos.y + 10;
			window_pos_pivot.x = 0.0f;
			window_pos_pivot.y = 0.0f;
			ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
			
			ImGui::SetNextWindowBgAlpha(0.35f);
			ImGui::Begin("Example: Simple overlay", NULL, window_flags);

			ImGui::Text("Time: (%f)", ImGui::GetTime());
			ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			bool showWire = true;
			if (ImGui::Checkbox("Wireframe", &showWire)) {}
			ImGui::End();
		}

		if (showDemoWindow) {
			ImGui::ShowDemoWindow(&showDemoWindow);
		}

		if (appContext->showPlatformWindow) {
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Platform controls", &appContext->showPlatformWindow);

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			//ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

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

	// TODO: Release all the things.
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	_cleanupDeviceD3D(&_appContext);
	DestroyWindow(_appContext.hWnd);
	_unregisterWindow(&_appContext);

	return 0;
}