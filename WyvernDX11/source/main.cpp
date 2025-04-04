#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <thread>

// NOTE: Must be included before any Windows includes.
#include "network.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

#define _USE_MATH_DEFINES
#include <math.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <d3d11.h>
#include <d3dcompiler.h>

#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>

#include "glm.h"
#include "utilities.h"
#include "circleFit.h"
#include "model.h"
#include "objLoader.h"
#include "plyLoader.h"
#include "stlLoader.h"
#include "image.h"
#include "threadSafeQueue.h"
#include "camera.h"
#include "ui.h"
#include "calibrationJob.h"

struct ldiBasicConstantBuffer {
	vec4 screenSize;
	mat4 mvp;
	vec4 color;
	mat4 view;
	mat4 proj;
	mat4 world;
};

struct ldiCamImagePixelConstants {
	vec4 params;
};

struct ldiPointCloudConstantBuffer {
	vec4 size;
};

struct ldiPhysics;
struct ldiImageInspector;
struct ldiProjectContext;
struct ldiPlatform;

struct ldiDebugPrims {
	std::vector<ldiSimpleVertex>	lineGeometryVertData;
	int								lineGeometryVertBufferSize;
	int								lineGeometryVertCount;
	ID3D11Buffer*					lineGeometryVertexBuffer;

	std::vector<ldiSimpleVertex>	triGeometryVertData;
	int								triGeometryVertBufferSize;
	int								triGeometryVertCount;
	ID3D11Buffer*					triGeometryVertexBuffer;
};

// Application and platform context.
struct ldiApp {
	HWND						hWnd;
	WNDCLASSEX					wc;
	uint32_t					windowWidth;
	uint32_t					windowHeight;

	std::string					currentWorkingDir;
	
	ID3D11Device*				d3dDevice;
	ID3D11DeviceContext*		d3dDeviceContext;
	IDXGISwapChain*				SwapChain;
	ID3D11RenderTargetView*		mainRenderTargetView;

	ID3D11VertexShader*			basicVertexShader;
	ID3D11PixelShader*			basicPixelShader;
	ID3D11InputLayout*			basicInputLayout;	

	ID3D11VertexShader*			dotVertexShader;
	ID3D11PixelShader*			dotPixelShader;
	ID3D11InputLayout*			dotInputLayout;

	ID3D11VertexShader*			simpleVertexShader;
	ID3D11PixelShader*			simplePixelShader;
	ID3D11InputLayout*			simpleInputLayout;

	ID3D11VertexShader*			meshVertexShader;
	ID3D11PixelShader*			meshPixelShader;
	ID3D11InputLayout*			meshInputLayout;

	ID3D11VertexShader*			litMeshVertexShader;
	ID3D11PixelShader*			litMeshPixelShader;
	ID3D11InputLayout*			litMeshInputLayout;

	ID3D11VertexShader*			pointCloudVertexShader;
	ID3D11PixelShader*			pointCloudPixelShader;
	ID3D11InputLayout*			pointCloudInputLayout;

	ID3D11VertexShader*			surfelVertexShader;
	ID3D11PixelShader*			surfelPixelShader;
	ID3D11InputLayout*			surfelInputLayout;

	ID3D11VertexShader*			imgCamVertexShader;
	ID3D11PixelShader*			imgCamPixelShader;
	ID3D11InputLayout*			imgCamInputLayout;

	ID3D11ComputeShader*		simImgComputeShader;

	ID3D11VertexShader*			surfelCoverageVertexShader;
	ID3D11PixelShader*			surfelCoveragePixelShader;
	ID3D11InputLayout*			surfelCoverageInputLayout;

	ID3D11Buffer*				mvpConstantBuffer;
	ID3D11Buffer*				pointcloudConstantBuffer;
	
	ID3D11BlendState*			defaultBlendState;
	ID3D11BlendState*			alphaBlendState;
	ID3D11BlendState*			multiplyBlendState;
	ID3D11RasterizerState*		defaultRasterizerState;
	ID3D11RasterizerState*		doubleSidedRasterizerState;
	ID3D11RasterizerState*		wireframeRasterizerState;
	ID3D11DepthStencilState*	defaultDepthStencilState;
	ID3D11DepthStencilState*	replaceDepthStencilState;
	ID3D11DepthStencilState*	noDepthState;
	ID3D11DepthStencilState*	wireframeDepthStencilState;
	ID3D11DepthStencilState*	nowriteDepthStencilState;
	ID3D11DepthStencilState*	rayMarchDepthStencilState;

	ID3D11SamplerState*			defaultPointSamplerState;
	ID3D11SamplerState*			defaultLinearSamplerState;
	ID3D11SamplerState*			wrapLinearSamplerState;
	ID3D11SamplerState*			defaultAnisotropicSamplerState;

	ID3D11Texture2D*			dotTexture;
	ID3D11ShaderResourceView*	dotShaderResourceView;
	ID3D11SamplerState*			dotSamplerState;

	ldiDebugPrims				defaultDebug;

	ImFont*						fontBig;

	bool						showPlatformWindow = true;
	bool						showDemoWindow = false;
	bool						showImageInspector = false;
	bool						showModelInspector = false;
	bool						showSamplerTester = false;
	bool						showModelEditor = false;
	bool						showGalvoInspector = false;
	bool						showProjectInspector = true;

	ldiServer					server = {};
	ldiPhysics*					physics = 0;
	ldiImageInspector*			imageInspector = 0;
	ldiPlatform*				platform = 0;
	
	ldiProjectContext*			projectContext;
	ldiCalibrationJob			calibJob;
};

//----------------------------------------------------------------------------------------------------
// Time.
//----------------------------------------------------------------------------------------------------
int64_t	_timeFrequency;
int64_t _timeCounterStart;

double getTime() {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	int64_t time = counter.QuadPart - _timeCounterStart;
	double result = (double)time / ((double)_timeFrequency);

	return result;
}

void _initTiming() {
	LARGE_INTEGER freq;
	LARGE_INTEGER counter;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&counter);
	_timeCounterStart = counter.QuadPart;
	_timeFrequency = freq.QuadPart;
}

//----------------------------------------------------------------------------------------------------
// Primary systems.
//----------------------------------------------------------------------------------------------------
#include "computerVision.h"
#include "serialPort.h"
#include "graphics.h"
#include "debugPrims.h"
#include "spatialGrid.h"
#include "physics.h"
#include "verletPhysics.h"
#include "antOptimizer.h"
#include "hawk.h"
#include "horse.h"
#include "analogScope.h"
#include "panther.h"
#include "calibration.h"
#include "scan.h"
#include "project.h"
#include "modelInspector.h"
#include "platform.h"
#include "samplerTester.h"
#include "imageInspector.h"
#include "modelEditor.h"
#include "galvoInspector.h"

ldiPhysics*				_physics = new ldiPhysics();
ldiApp*					_appContext = new ldiApp();
ldiModelInspector*		_modelInspector = new ldiModelInspector();
ldiSamplerTester*		_samplerTester = new ldiSamplerTester();
ldiPlatform*			_platform = new ldiPlatform();
ldiImageInspector*		_imageInspector = new ldiImageInspector();
ldiModelEditor*			_modelEditor = new ldiModelEditor();
ldiGalvoInspector*		_galvoInspector = new ldiGalvoInspector();
ldiProjectContext*		_projectContext = new ldiProjectContext();

//----------------------------------------------------------------------------------------------------
// Windowing and GUI helpers.
//----------------------------------------------------------------------------------------------------
vec2 getMousePosition(ldiApp* AppContext) {
	POINT p;
	GetCursorPos(&p);
	ScreenToClient(AppContext->hWnd, &p);
	return vec2((float)p.x, (float)p.y);
}

void setMousePosition(ldiApp* AppContext, vec2 Position) {
	POINT p;
	p.x = (LONG)Position.x;
	p.y = (LONG)Position.y;
	ClientToScreen(AppContext->hWnd, &p);
	SetCursorPos(p.x, p.y);
}

void _initImgui(ldiApp* AppContext) {
	// Setup Dear ImGui context.
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.IniFilename = NULL;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;
	//io.ConfigViewportsNoDefaultParent = true;
	//io.ConfigDockingAlwaysTabBar = true;
	//io.ConfigDockingTransparentPayload = true;

	ImGui::StyleColorsDark();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Create main GUI style.
	//ImGuiStyle& style = ImGui::GetStyle();
	style.DisplaySafeAreaPadding = ImVec2(0, 0);
	style.TabRounding = 0.0f;
	style.ScrollbarRounding = 0.0f;
	style.ScrollbarSize = 16.0f;

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.18f, 0.21f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.21f, 0.21f, 0.25f, 0.50f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.41f, 0.26f, 0.98f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.47f, 0.26f, 0.98f, 0.67f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.13f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.19f, 0.19f, 0.21f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.27f, 0.28f, 0.32f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.36f, 0.37f, 0.43f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.36f, 0.41f, 0.51f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.39f, 0.26f, 0.98f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.43f, 0.24f, 0.88f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.36f, 0.53f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.16f, 0.18f, 0.21f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.41f, 0.26f, 0.98f, 0.40f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.47f, 0.26f, 0.98f, 0.67f);
	//colors[ImGuiCol_Header] = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.30f, 0.29f, 0.42f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.29f, 0.28f, 0.32f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.28f, 0.43f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.11f, 0.12f, 0.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.38f, 0.34f, 0.57f, 0.80f);
	colors[ImGuiCol_TabActive] = ImVec4(0.34f, 0.35f, 0.53f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.21f, 0.21f, 0.24f, 1.00f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.40f, 0.98f, 0.70f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.41f, 0.26f, 0.98f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);

	ImGui_ImplWin32_Init(AppContext->hWnd);
	ImGui_ImplDX11_Init(AppContext->d3dDevice, AppContext->d3dDeviceContext);

	ImFont* font = io.Fonts->AddFontFromFileTTF("../../assets/fonts/roboto/Roboto-Regular.ttf", 14.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("../../assets/fonts/inter/Inter-Medium.ttf", 15.0f);
	IM_ASSERT(font != NULL);

	AppContext->fontBig = io.Fonts->AddFontFromFileTTF("../../assets/fonts/roboto/Roboto-Bold.ttf", 24.0f);
	IM_ASSERT(AppContext->fontBig != NULL);
}

void _renderImgui(ldiApp* AppContext) {
	gfxBeginPrimaryViewport(AppContext);
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Update and Render additional Platform Windows
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	gfxEndPrimaryViewport(AppContext);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg) {
		case WM_SIZE: {
			uint32_t width = (UINT)LOWORD(lParam);
			uint32_t height = (UINT)HIWORD(lParam);
			//std::cout << "Resize: " << width << ", " << height << "\n";

			_appContext->windowWidth = width;
			_appContext->windowHeight = height;

			if (_appContext->d3dDevice != NULL && wParam != SIZE_MINIMIZED) {
				gfxCleanupPrimaryBackbuffer(_appContext);
				_appContext->SwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
				gfxCreatePrimaryBackbuffer(_appContext);
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

void _createWindow(ldiApp* AppContext) {
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

	AppContext->hWnd = CreateWindow(AppContext->wc.lpszClassName, _T("Wyvern DX11"), WS_OVERLAPPEDWINDOW, 100, 100, 1920, 1080, NULL, NULL, AppContext->wc.hInstance, NULL);
}

void _unregisterWindow(ldiApp* AppContext) {
	UnregisterClass(AppContext->wc.lpszClassName, AppContext->wc.hInstance);
}

bool _handleWindowLoop() {
	bool quit = false;

	MSG msg;
	while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (msg.message == WM_QUIT) {
			quit = true;
		}
	}

	return quit;
}

//----------------------------------------------------------------------------------------------------
// Application.
//----------------------------------------------------------------------------------------------------
bool _initResources(ldiApp* AppContext) {
	std::cout << "Initializing resources\n";

	//----------------------------------------------------------------------------------------------------
	// Vertex attribute layouts.
	//----------------------------------------------------------------------------------------------------
	D3D11_INPUT_ELEMENT_DESC basicLayout[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0,	DXGI_FORMAT_R32G32_FLOAT,		0,	24,	D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D3D11_INPUT_ELEMENT_DESC simpleLayout[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D3D11_INPUT_ELEMENT_DESC meshLayout[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0,	DXGI_FORMAT_R32G32_FLOAT,		0,	24,	D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D3D11_INPUT_ELEMENT_DESC pointcloudLayout[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0,	DXGI_FORMAT_R32G32_FLOAT,		0,	24,	D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D3D11_INPUT_ELEMENT_DESC imguiUiLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	D3D11_INPUT_ELEMENT_DESC coveragePointLayout[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0,	DXGI_FORMAT_R32_UINT,			0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	16,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	//----------------------------------------------------------------------------------------------------
	// Basic shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/basic.hlsl", "mainVs", &AppContext->basicVertexShader, basicLayout, 3, &AppContext->basicInputLayout)) {
		return false;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/basic.hlsl", "mainPs", &AppContext->basicPixelShader)) {
		return false;
	}

	//----------------------------------------------------------------------------------------------------
	// Dot shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/dot.hlsl", "mainVs", &AppContext->dotVertexShader, basicLayout, 3, &AppContext->dotInputLayout)) {
		return false;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/dot.hlsl", "mainPs", &AppContext->dotPixelShader)) {
		return false;
	}

	//----------------------------------------------------------------------------------------------------
	// Surfel shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/surfel.hlsl", "mainVs", &AppContext->surfelVertexShader, basicLayout, 3, &AppContext->surfelInputLayout)) {
		return false;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/surfel.hlsl", "mainPs", &AppContext->surfelPixelShader)) {
		return false;
	}

	//----------------------------------------------------------------------------------------------------
	// Simple shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/simple.hlsl", "mainVs", &AppContext->simpleVertexShader, simpleLayout, 2, &AppContext->simpleInputLayout)) {
		return false;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/simple.hlsl", "mainPs", &AppContext->simplePixelShader)) {
		return false;
	}

	//----------------------------------------------------------------------------------------------------
	// Mesh shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/mesh.hlsl", "mainVs", &AppContext->meshVertexShader, meshLayout, 3, &AppContext->meshInputLayout)) {
		return false;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/mesh.hlsl", "mainPs", &AppContext->meshPixelShader)) {
		return false;
	}

	//----------------------------------------------------------------------------------------------------
	// Lit mesh shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/litMesh.hlsl", "mainVs", &AppContext->litMeshVertexShader, meshLayout, 3, &AppContext->litMeshInputLayout)) {
		return false;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/litMesh.hlsl", "mainPs", &AppContext->litMeshPixelShader)) {
		return false;
	}

	//----------------------------------------------------------------------------------------------------
	// Pointcloud shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/pointcloud.hlsl", "mainVs", &AppContext->pointCloudVertexShader, pointcloudLayout, 3, &AppContext->pointCloudInputLayout)) {
		return false;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/pointcloud.hlsl", "mainPs", &AppContext->pointCloudPixelShader)) {
		return false;
	}

	//----------------------------------------------------------------------------------------------------
	// Image cam shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/imgCam.hlsl", "mainVs", &AppContext->imgCamVertexShader, imguiUiLayout, 3, &AppContext->imgCamInputLayout)) {
		return false;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/imgCam.hlsl", "mainPs", &AppContext->imgCamPixelShader)) {
		return false;
	}

	//----------------------------------------------------------------------------------------------------
	// Surfel coverage view shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateVertexShader(AppContext, L"../../assets/shaders/surfelCoverageView.hlsl", "mainVs", &AppContext->surfelCoverageVertexShader, coveragePointLayout, 3, &AppContext->surfelCoverageInputLayout)) {
		return false;
	}

	if (!gfxCreatePixelShader(AppContext, L"../../assets/shaders/surfelCoverageView.hlsl", "mainPs", &AppContext->surfelCoveragePixelShader)) {
		return false;
	}

	//----------------------------------------------------------------------------------------------------
	// Test compute shader.
	//----------------------------------------------------------------------------------------------------
	if (!gfxCreateComputeShader(AppContext, L"../../assets/shaders/computeTest.hlsl", "mainCs", &AppContext->simImgComputeShader)) {
		return false;
	}

	//----------------------------------------------------------------------------------------------------
	// Basic constant buffer.
	//----------------------------------------------------------------------------------------------------
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(ldiBasicConstantBuffer);
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		AppContext->d3dDevice->CreateBuffer(&desc, NULL, &AppContext->mvpConstantBuffer);
	}

	//----------------------------------------------------------------------------------------------------
	// Point cloud constant buffer.
	//----------------------------------------------------------------------------------------------------
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(ldiPointCloudConstantBuffer);
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		AppContext->d3dDevice->CreateBuffer(&desc, NULL, &AppContext->pointcloudConstantBuffer);
	}

	//----------------------------------------------------------------------------------------------------
	// Model render state.
	//----------------------------------------------------------------------------------------------------
	{
		D3D11_BLEND_DESC desc = {};
		desc.AlphaToCoverageEnable = false;
		desc.RenderTarget[0].BlendEnable = true;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		AppContext->d3dDevice->CreateBlendState(&desc, &AppContext->alphaBlendState);
	}

	{
		D3D11_BLEND_DESC desc = {};
		desc.AlphaToCoverageEnable = false;
		desc.RenderTarget[0].BlendEnable = false;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
		desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		AppContext->d3dDevice->CreateBlendState(&desc, &AppContext->defaultBlendState);
	}

	{
		D3D11_BLEND_DESC desc = {};
		desc.AlphaToCoverageEnable = false;
		desc.RenderTarget[0].BlendEnable = true;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
		desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		AppContext->d3dDevice->CreateBlendState(&desc, &AppContext->multiplyBlendState);

		//rtBlend.SrcBlend = D3D11_BLEND_DEST_COLOR;
		//rtBlend.DestBlend = D3D11_BLEND_ZERO;
		//rtBlend.BlendOp = D3D11_BLEND_OP_ADD;
		//rtBlend.SrcBlendAlpha = D3D11_BLEND_ZERO;// D3D11_BLEND_ONE;
		//rtBlend.DestBlendAlpha = D3D11_BLEND_ONE;// D3D11_BLEND_ZERO;
		//rtBlend.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		//rtBlend.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	}

	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_BACK;
		desc.ScissorEnable = false;
		desc.DepthClipEnable = true;
		AppContext->d3dDevice->CreateRasterizerState(&desc, &AppContext->defaultRasterizerState);
	}

	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.ScissorEnable = false;
		desc.DepthClipEnable = true;
		AppContext->d3dDevice->CreateRasterizerState(&desc, &AppContext->doubleSidedRasterizerState);
	}

	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D11_FILL_WIREFRAME;
		desc.CullMode = D3D11_CULL_NONE;
		desc.ScissorEnable = false;
		/*desc.DepthClipEnable = true;
		desc.DepthBias = -256;
		desc.DepthBiasClamp = -0.00001f;*/
				
		/*desc.DepthClipEnable = true;
		desc.DepthBias = -1;
		desc.DepthBiasClamp = 1.0f;
		desc.SlopeScaledDepthBias = -1.0f;*/
		AppContext->d3dDevice->CreateRasterizerState(&desc, &AppContext->wireframeRasterizerState);
	}

	{
		D3D11_DEPTH_STENCIL_DESC desc = {};
		desc.DepthEnable = true;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		desc.StencilEnable = false;
		desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		desc.BackFace = desc.FrontFace;
		/*desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		desc.BackFace = desc.FrontFace;*/
		AppContext->d3dDevice->CreateDepthStencilState(&desc, &AppContext->rayMarchDepthStencilState);
	}	

	{
		D3D11_DEPTH_STENCIL_DESC desc = {};
		desc.DepthEnable = true;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		desc.StencilEnable = false;
		/*desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		desc.BackFace = desc.FrontFace;*/
		AppContext->d3dDevice->CreateDepthStencilState(&desc, &AppContext->nowriteDepthStencilState);
	}

	{
		D3D11_DEPTH_STENCIL_DESC desc = {};
		desc.DepthEnable = true;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		desc.StencilEnable = false;
		AppContext->d3dDevice->CreateDepthStencilState(&desc, &AppContext->wireframeDepthStencilState);
	}

	{
		D3D11_DEPTH_STENCIL_DESC desc = {};
		desc.DepthEnable = true;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_LESS;
		desc.StencilEnable = false;
		/*desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		desc.BackFace = desc.FrontFace;*/
		AppContext->d3dDevice->CreateDepthStencilState(&desc, &AppContext->defaultDepthStencilState);
	}

	{
		D3D11_DEPTH_STENCIL_DESC desc = {};
		desc.DepthEnable = true;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		desc.StencilEnable = false;
		AppContext->d3dDevice->CreateDepthStencilState(&desc, &AppContext->replaceDepthStencilState);
	}
	
	{
		D3D11_DEPTH_STENCIL_DESC desc = {};
		desc.DepthEnable = false;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_LESS;
		desc.StencilEnable = false;
		/*desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		desc.BackFace = desc.FrontFace;*/
		AppContext->d3dDevice->CreateDepthStencilState(&desc, &AppContext->noDepthState);
	}

	// Default point sampler.
	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		AppContext->d3dDevice->CreateSamplerState(&samplerDesc, &AppContext->defaultPointSamplerState);
	}

	// Default linear sampler.
	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		AppContext->d3dDevice->CreateSamplerState(&samplerDesc, &AppContext->defaultLinearSamplerState);
	}

	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		AppContext->d3dDevice->CreateSamplerState(&samplerDesc, &AppContext->wrapLinearSamplerState);
	}

	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		AppContext->d3dDevice->CreateSamplerState(&samplerDesc, &AppContext->defaultAnisotropicSamplerState);
	}

	//----------------------------------------------------------------------------------------------------
	// Dot texture.
	//----------------------------------------------------------------------------------------------------
	{
		int x, y, n;
		uint8_t* imageRawPixels = imageLoadRgba("../../assets/images/dot.png", &x, &y, &n);

		D3D11_SUBRESOURCE_DATA texData = {};
		texData.pSysMem = imageRawPixels;
		texData.SysMemPitch = x * 4;

		D3D11_TEXTURE2D_DESC tex2dDesc = {};
		tex2dDesc.Width = x;
		tex2dDesc.Height = y;
		tex2dDesc.MipLevels = 0;
		tex2dDesc.ArraySize = 1;
		tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		tex2dDesc.SampleDesc.Count = 1;
		tex2dDesc.SampleDesc.Quality = 0;
		tex2dDesc.Usage = D3D11_USAGE_DEFAULT; // D3D11_USAGE_IMMUTABLE;
		tex2dDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		tex2dDesc.CPUAccessFlags = 0;
		tex2dDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

		if (FAILED(AppContext->d3dDevice->CreateTexture2D(&tex2dDesc, NULL, &AppContext->dotTexture))) {
			std::cout << "Texture failed to create\n";
			return false;
		}

		AppContext->d3dDeviceContext->UpdateSubresource(AppContext->dotTexture, 0, NULL, imageRawPixels, x * 4, 0);

		imageFree(imageRawPixels);

		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
		viewDesc.Format = tex2dDesc.Format;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MostDetailedMip = 0;
		viewDesc.Texture2D.MipLevels = -1;

		if (AppContext->d3dDevice->CreateShaderResourceView(AppContext->dotTexture, &viewDesc, &AppContext->dotShaderResourceView) != S_OK) {
			std::cout << "CreateShaderResourceView failed\n";
			return false;
		}

		AppContext->d3dDeviceContext->GenerateMips(AppContext->dotShaderResourceView);

		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		if (FAILED(AppContext->d3dDevice->CreateSamplerState(&samplerDesc, &AppContext->dotSamplerState))) {
			std::cout << "CreateSamplerState failed\n";
			return false;
		}
	}
	
	return true;
}

//----------------------------------------------------------------------------------------------------
// Application main.
//----------------------------------------------------------------------------------------------------
int main() {
	std::cout << "Starting WyvernDX11\n";
	_initTiming();

	char dirBuff[512];
	GetCurrentDirectory(sizeof(dirBuff), dirBuff);
	_appContext->currentWorkingDir = std::string(dirBuff);
	std::cout << "Working directory: " << _appContext->currentWorkingDir << "\n";

	_createWindow(_appContext);

	// Initialize Direct3D.
	if (!gfxCreateDeviceD3D(_appContext)) {
		gfxCleanupDeviceD3D(_appContext);
		_unregisterWindow(_appContext);
		return 1;
	}

	ShowWindow(_appContext->hWnd, SW_SHOWDEFAULT);
	UpdateWindow(_appContext->hWnd);

	_initImgui(_appContext);

	// TODO: Move to callibration machine vision setup.
	createCharucos(false);
	cameraCalibCreateTarget(9, 6, 1.0f, 64);
	
	if (!_initResources(_appContext)) {
		return 1;
	}

	initDebugPrimitives(&_appContext->defaultDebug);

	_appContext->projectContext = _projectContext;

	_appContext->physics = _physics;
	if (physicsInit(_appContext, _physics) != 0) {
		std::cout << "PhysX init failed\n";
		return 1;
	}

	if (modelInspectorInit(_appContext, _modelInspector) != 0) {
		std::cout << "Could not init model inspector\n";
		return 1;
	}

	if (modelInspectorLoad(_appContext, _modelInspector) != 0) {
		std::cout << "Could not load model inspector\n";
		return 1;
	}

	if (samplerTesterInit(_appContext, _samplerTester) != 0) {
		std::cout << "Could not init sampler tester\n";
		return 1;
	}

	_appContext->platform = _platform;
	if (platformInit(_appContext, _platform) != 0) {
		std::cout << "Could not init platform\n";
		return 1;
	}

	_appContext->imageInspector = _imageInspector;
	if (imageInspectorInit(_appContext, _imageInspector) != 0) {
		std::cout << "Could not init image inspector\n";
		return 1;
	}

	if (modelEditorInit(_appContext, _modelEditor) != 0) {
		std::cout << "Could not init model editor\n";
		return 1;
	}

	if (galvoInspectorInit(_appContext, _galvoInspector) != 0) {
		std::cout << "Could not init galvo inspector\n";
		return 1;
	}

	//if (modelEditorLoad(_appContext, _modelEditor) != 0) {
	//	std::cout << "Could not load model editor\n";
	//	// TODO: Platform should be detroyed properly when ANY system failed to load.
	//	//platformDestroy(platform);
	//	return 1;
	//}

	/*ImGui::GetID
	ImGui::DockBuilderDockWindow();
	ImGui::DockBuilderFinish();*/

	//if (!networkInit(_appContext->server, "20000")) {
	//	std::cout << "Networking failure\n";
	//	return 1;
	//}

	// Default project.
	//projectInit(_appContext, _appContext->projectContext);
	if (!projectLoad(_appContext, _appContext->projectContext, "C:\\Projects\\LDI\\WyvernDX11\\bin\\projects\\taryk.prj")) {
		std::cout << "Project failed to load\n";
		return 1;
	}

	if (!projectProcess(_appContext, _appContext->projectContext)) {
		std::cout << "Failed to process\n";
		return 1;
	}

	if (!calibLoadCalibJob("C:\\Projects\\LDI\\WyvernDX11\\bin\\cache\\vol1.cal", &_appContext->calibJob)) {
		std::cout << "Failed to load calibration job\n";
		return 1;
	}

	// Main loop
	bool running = true;
	while (running) {
		
		if (_handleWindowLoop()) {
			break;
		}
		
		//----------------------------------------------------------------------------------------------------
		// Build interface.
		//----------------------------------------------------------------------------------------------------
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("New project")) {
					projectInit(_appContext, _appContext->projectContext);
				}

				if (ImGui::MenuItem("Open project")) {
					std::string filePath;
					if (showOpenFileDialog(_appContext->hWnd, _appContext->currentWorkingDir, filePath, L"Project file", L"*.prj")) {
						projectLoad(_appContext, _appContext->projectContext, filePath);
					}
				}

				if (ImGui::MenuItem("Save project")) {
					if (_appContext->projectContext->path.empty()) {
						std::string filePath;
						if (showSaveFileDialog(_appContext->hWnd, _appContext->currentWorkingDir, filePath, L"Project file", L"*.prj", L"prj")) {
							_appContext->projectContext->path = filePath;
							projectSave(_appContext, _appContext->projectContext);
						}
					} else {
						projectSave(_appContext, _appContext->projectContext);
					}
				}

				if (ImGui::MenuItem("Save project as...")) {
					std::string filePath;
					if (showSaveFileDialog(_appContext->hWnd, _appContext->currentWorkingDir, filePath, L"Project file", L"*.prj", L"prj")) {
						_appContext->projectContext->path = filePath;
						projectSave(_appContext, _appContext->projectContext);
					}
				}

				ImGui::Separator();
				if (ImGui::MenuItem("Quit", "Alt+F4")) {
					running = false;
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Platform")) {
				if (ImGui::MenuItem("New calibration")) {}
				if (ImGui::MenuItem("Open calibration")) {}
				if (ImGui::MenuItem("Save calibration")) {}
				if (ImGui::MenuItem("Save calibration as...")) {}
				ImGui::EndMenu();
			}
			
			/*if (ImGui::BeginMenu("Workspace")) {
				if (ImGui::MenuItem("Platform")) {}
				ImGui::Separator();
				if (ImGui::MenuItem("Scan")) {}
				if (ImGui::MenuItem("Process")) {}
				if (ImGui::MenuItem("Run")) {}
				ImGui::Separator();
				if (ImGui::MenuItem("Model inspector")) {}
				if (ImGui::MenuItem("Image inspector")) {}
				ImGui::EndMenu();
			}*/

			if (ImGui::BeginMenu("Window")) {
				if (ImGui::MenuItem("ImGUI demo window", NULL, _appContext->showDemoWindow)) {
					_appContext->showDemoWindow = !_appContext->showDemoWindow;
				}
				
				ImGui::Separator();				
				if (ImGui::MenuItem("Project inspector", NULL, _appContext->showProjectInspector)) {
					_appContext->showProjectInspector = !_appContext->showProjectInspector;
				}

				if (ImGui::MenuItem("Platform controls", NULL, _appContext->showPlatformWindow)) {
					_appContext->showPlatformWindow = !_appContext->showPlatformWindow;
				}
				
				if (ImGui::MenuItem("Model inspector", NULL, _appContext->showModelInspector)) {
					_appContext->showModelInspector = !_appContext->showModelInspector;
				}
				
				if (ImGui::MenuItem("Sampler tester", NULL, _appContext->showSamplerTester)) {
					_appContext->showSamplerTester = !_appContext->showSamplerTester;
				}
				
				if (ImGui::MenuItem("Image inspector", NULL, _appContext->showImageInspector)) {
					_appContext->showImageInspector = !_appContext->showImageInspector;
				}

				if (ImGui::MenuItem("Model editor", NULL, _appContext->showModelEditor)) {
					_appContext->showModelEditor = !_appContext->showModelEditor;
				}

				if (ImGui::MenuItem("Galvo inspector", NULL, _appContext->showGalvoInspector)) {
					_appContext->showGalvoInspector = !_appContext->showGalvoInspector;
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		ImGui::DockSpaceOverViewport();

		if (_appContext->showDemoWindow) {
			ImGui::ShowDemoWindow(&_appContext->showDemoWindow);
		}

		if (_appContext->showPlatformWindow) {
			platformShowUi(_platform);
		}

		if (_appContext->showImageInspector) {
			imageInspectorShowUi(_appContext->imageInspector);
		}

		if (_appContext->showModelInspector) {
			modelInspectorShowUi(_modelInspector);
		}

		if (_appContext->showSamplerTester) {
			samplerTesterShowUi(_samplerTester);
		}

		if (_appContext->showModelEditor) {
			modelEditorShowUi(_modelEditor);
		}

		if (_appContext->showGalvoInspector) {
			galvoInspectorShowUi(_galvoInspector);
		}

		if (_appContext->showProjectInspector) {
			projectShowUi(_appContext, _appContext->projectContext, &_appContext->platform->mainCamera);
		}

		_renderImgui(_appContext);
	}

	// TODO: Release all the things. (Probably never going to happen).
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	gfxCleanupDeviceD3D(_appContext);
	DestroyWindow(_appContext->hWnd);
	_unregisterWindow(_appContext);

	platformDestroy(_platform);

	return 0;
}