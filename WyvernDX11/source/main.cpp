#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <thread>

// NOTE: Must be included before any Windows includes.
#include "network.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <d3d11.h>
#include <d3dcompiler.h>

#include "glm.h"
#include "model.h"
#include "objLoader.h"
#include "plyLoader.h"
#include "stlLoader.h"
#include "image.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

#define _USE_MATH_DEFINES
#include <math.h>

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

// TODO: Consider: Per frame, per object, per scene, etc...
struct ldiBasicConstantBuffer {
	vec4 screenSize;
	mat4 mvp;
	vec4 color;
	mat4 view;
	mat4 proj;
};

struct ldiCamImagePixelConstants {
	vec4 params;
};

struct ldiPointCloudConstantBuffer {
	vec4 size;
};

struct ldiCamera {
	vec3 position;
	vec3 rotation;
	mat4 viewMat;
	mat4 projMat;
	mat4 invProjMat;
	mat4 projViewMat;
	float fov;
	int viewWidth;
	int viewHeight;
};

struct ldiTextInfo {
	vec2 position;
	std::string text;
	vec4 color;
};

struct ldiPlatformTransform {
	float x;
	float y;
	float z;
	float a;
	float b;
};

struct ldiPhysics;
struct ldiImageInspector;

// Application and platform context.
struct ldiApp {
	HWND						hWnd;
	WNDCLASSEX					wc;
	uint32_t					windowWidth;
	uint32_t					windowHeight;

	int64_t						timeFrequency;
	int64_t						timeCounterStart;

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

	ID3D11Buffer*				mvpConstantBuffer;
	ID3D11Buffer*				pointcloudConstantBuffer;

	ID3D11BlendState*			defaultBlendState;
	ID3D11BlendState*			alphaBlendState;
	ID3D11RasterizerState*		defaultRasterizerState;
	ID3D11RasterizerState*		doubleSidedRasterizerState;
	ID3D11RasterizerState*		wireframeRasterizerState;
	ID3D11DepthStencilState*	defaultDepthStencilState;
	ID3D11DepthStencilState*	noDepthState;
	ID3D11DepthStencilState*	wireframeDepthStencilState;
	ID3D11DepthStencilState*	nowriteDepthStencilState;

	ID3D11SamplerState*			defaultPointSamplerState;
	ID3D11SamplerState*			defaultLinearSamplerState;
	ID3D11SamplerState*			wrapLinearSamplerState;

	ID3D11Texture2D*			dotTexture;
	ID3D11ShaderResourceView*	dotShaderResourceView;
	ID3D11SamplerState*			dotSamplerState;
	
	std::vector<ldiSimpleVertex>	lineGeometryVertData;
	int								lineGeometryVertMax;
	int								lineGeometryVertCount;
	ID3D11Buffer*					lineGeometryVertexBuffer;

	std::vector<ldiSimpleVertex>	triGeometryVertData;
	int								triGeometryVertMax;
	int								triGeometryVertCount;
	ID3D11Buffer*					triGeometryVertexBuffer;

	ImFont*						fontBig;

	bool						showPlatformWindow = false;
	bool						showDemoWindow = false;
	bool						showImageInspector = false;
	bool						showModelInspector = true;
	bool						showSamplerTester = false;
	bool						showVisionSimulator = false;
	bool						showModelEditor = false;

	ldiServer					server = {};
	ldiPhysics*					physics = 0;
	ldiImageInspector*			imageInspector = 0;
};

double _getTime(ldiApp* AppContext) {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	int64_t time = counter.QuadPart - AppContext->timeCounterStart;
	double result = (double)time / ((double)AppContext->timeFrequency);

	return result;
}

// TODO: Move to another location.
void updateCamera3dBasicFps(ldiCamera* Camera, float ViewWidth, float ViewHeight) {
	mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(Camera->rotation.y), vec3Right);
	viewRotMat = glm::rotate(viewRotMat, glm::radians(Camera->rotation.x), vec3Up);
	
	Camera->viewMat = viewRotMat * glm::translate(mat4(1.0f), -Camera->position);
	Camera->projMat = glm::perspectiveFovRH_ZO(glm::radians(Camera->fov), ViewWidth, ViewHeight, 0.01f, 100.0f);
	Camera->invProjMat = inverse(Camera->projMat);
	Camera->projViewMat = Camera->projMat * Camera->viewMat;
	Camera->viewWidth = ViewWidth;
	Camera->viewHeight = ViewHeight;
}

#include "serialPort.h"
#include "physics.h"
#include "graphics.h"
#include "debugPrims.h"
#include "computerVision.h"
#include "modelInspector.h"
#include "samplerTester.h"
#include "platform.h"
#include "visionSimulator.h"
#include "modelEditor.h"

#include "imageInspector.h"

ldiPhysics			_physics = {};
ldiApp				_appContext = {};
ldiModelInspector	_modelInspector = {};
ldiSamplerTester	_samplerTester = {};
ldiPlatform			_platform = {};
ldiVisionSimulator	_visionSimulator = {};
ldiImageInspector	_imageInspector = {};
ldiModelEditor		_modelEditor = {};

//----------------------------------------------------------------------------------------------------
// Windowing helpers.
//----------------------------------------------------------------------------------------------------
void _initTiming(ldiApp* AppContext) {
	LARGE_INTEGER freq;
	LARGE_INTEGER counter;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&counter);
	AppContext->timeCounterStart = counter.QuadPart;
	AppContext->timeFrequency = freq.QuadPart;
}

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
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
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
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
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

			_appContext.windowWidth = width;
			_appContext.windowHeight = height;

			if (_appContext.d3dDevice != NULL && wParam != SIZE_MINIMIZED) {
				gfxCleanupPrimaryBackbuffer(&_appContext);
				_appContext.SwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
				gfxCreatePrimaryBackbuffer(&_appContext);
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
	// Layouts.
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
		desc.CullMode = D3D11_CULL_BACK;
		desc.ScissorEnable = false;
		desc.DepthClipEnable = false;
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
		AppContext->d3dDevice->CreateDepthStencilState(&desc, &AppContext->nowriteDepthStencilState);
	}

	{
		D3D11_DEPTH_STENCIL_DESC desc = {};
		desc.DepthEnable = true;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		desc.StencilEnable = false;
		/*desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		desc.BackFace = desc.FrontFace;*/
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
// Network command handling.
//----------------------------------------------------------------------------------------------------
void _processPacket(ldiApp* AppContext, ldiPacketView* PacketView) {
	ldiProtocolHeader* packetHeader = (ldiProtocolHeader*)PacketView->data;
	//std::cout << "Opcode: " << packetHeader->opcode << "\n";

	if (packetHeader->opcode == 0) {
		imageInspectorHandlePacket(AppContext->imageInspector, PacketView);
	} else if (packetHeader->opcode == 1) {
		imageInspectorHandlePacket(AppContext->imageInspector, PacketView);
	} else {
		std::cout << "Error: Got unknown opcode (" << packetHeader->opcode << ") from packet\n";
	}
}

//----------------------------------------------------------------------------------------------------
// Application main.
//----------------------------------------------------------------------------------------------------
int main() {
	std::cout << "Starting WyvernDX11\n";

	createCharucos(false);

	char dirBuff[512];
	GetCurrentDirectory(sizeof(dirBuff), dirBuff);
	std::cout << "Working directory: " << dirBuff << "\n";

	ldiApp* appContext = &_appContext;

	_initTiming(appContext);
	_createWindow(appContext);

	// Initialize Direct3D.
	if (!gfxCreateDeviceD3D(appContext)) {
		gfxCleanupDeviceD3D(appContext);
		_unregisterWindow(appContext);
		return 1;
	}

	ShowWindow(appContext->hWnd, SW_SHOWDEFAULT);
	UpdateWindow(appContext->hWnd);

	_initImgui(appContext);

	if (!_initResources(appContext)) {
		return 1;
	}

	initDebugPrimitives(appContext);

	appContext->physics = &_physics;
	if (physicsInit(appContext, &_physics) != 0) {
		std::cout << "PhysX init failed\n";
		return 1;
	}

	ldiModelInspector* modelInspector = &_modelInspector;
	if (modelInspectorInit(appContext, modelInspector) != 0) {
		std::cout << "Could not init model inspector\n";
		return 1;
	}

	if (modelInspectorLoad(appContext, modelInspector) != 0) {
		std::cout << "Could not load model inspector\n";
		return 1;
	}

	ldiSamplerTester* samplerTester = &_samplerTester;
	if (samplerTesterInit(appContext, samplerTester) != 0) {
		std::cout << "Could not init sampler tester\n";
		return 1;
	}

	ldiPlatform* platform = &_platform;
	if (platformInit(appContext, platform) != 0) {
		std::cout << "Could not init platform\n";
		return 1;
	}

	appContext->imageInspector = &_imageInspector;
	if (imageInspectorInit(appContext, &_imageInspector) != 0) {
		std::cout << "Could not init image inspector\n";
		return 1;
	}

	ldiVisionSimulator* visionSimulator = &_visionSimulator;
	if (visionSimulatorInit(appContext, visionSimulator) != 0) {
		std::cout << "Could not init vision simulator\n";
		return 1;
	}

	ldiModelEditor* modelEditor = &_modelEditor;
	if (modelEditorInit(appContext, modelEditor) != 0) {
		std::cout << "Could not init model editor\n";
		return 1;
	}

	//if (modelEditorLoad(appContext, modelEditor) != 0) {
	//	std::cout << "Could not load model editor\n";
	//	// TODO: Platform should be detroyed properly when ANY system failed to load.
	//	platformDestroy(platform);
	//	return 1;
	//}

	/*ImGui::GetID
	ImGui::DockBuilderDockWindow();
	ImGui::DockBuilderFinish();*/

	if (!networkInit(&appContext->server, "20000")) {
		std::cout << "Networking failure\n";
		return 1;
	}

	// Create main style.
	ImGuiStyle& style = ImGui::GetStyle();
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
	colors[ImGuiCol_Header] = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.29f, 0.28f, 0.32f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.28f, 0.43f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.11f, 0.12f, 0.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.38f, 0.34f, 0.57f, 0.80f);
	colors[ImGuiCol_TabActive] = ImVec4(0.34f, 0.35f, 0.53f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.21f, 0.21f, 0.24f, 1.00f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.40f, 0.98f, 0.70f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.41f, 0.26f, 0.98f, 1.00f);

	// Main loop
	bool running = true;
	while (running) {
		
		if (_handleWindowLoop()) {
			break;
		}

		while (true) {
			ldiPacketView packetView;
			int updateResult = networkUpdate(&appContext->server, &packetView);

			if (updateResult == 1) {
				// Got packet.
				_processPacket(appContext, &packetView);
			} else if (updateResult == 0) {
				// Would block.
				break;
			} else {
				// Critical error.
			}
		}
		
		//----------------------------------------------------------------------------------------------------
		// Build interface.
		//----------------------------------------------------------------------------------------------------
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("New project")) {}
				if (ImGui::MenuItem("Open project")) {}
				if (ImGui::MenuItem("Save project")) {}
				if (ImGui::MenuItem("Save project as...")) {}
				ImGui::Separator();
				if (ImGui::MenuItem("Import model")) {}
				ImGui::Separator();
				if (ImGui::MenuItem("Quit", "Alt+F4")) {
					running = false;
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Platform")) {
				if (ImGui::MenuItem("New configuration")) {}
				if (ImGui::MenuItem("Open configuration")) {}
				if (ImGui::MenuItem("Save configuration")) {}
				if (ImGui::MenuItem("Save configuration as...")) {}
				ImGui::EndMenu();
			}
			
			if (ImGui::BeginMenu("Workspace")) {
				if (ImGui::MenuItem("Platform")) {}
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
				if (ImGui::MenuItem("ImGUI demo window", NULL, appContext->showDemoWindow)) {
					appContext->showDemoWindow = !appContext->showDemoWindow;
				}
				
				ImGui::Separator();				
				if (ImGui::MenuItem("Platform controls", NULL, appContext->showPlatformWindow)) {
					appContext->showPlatformWindow = !appContext->showPlatformWindow;
				}
				
				if (ImGui::MenuItem("Model inspector", NULL, appContext->showModelInspector)) {
					appContext->showModelInspector = !appContext->showModelInspector;
				}
				
				if (ImGui::MenuItem("Sampler tester", NULL, appContext->showSamplerTester)) {
					appContext->showSamplerTester = !appContext->showSamplerTester;
				}
				
				if (ImGui::MenuItem("Image inspector", NULL, appContext->showImageInspector)) {
					appContext->showImageInspector = !appContext->showImageInspector;
				}

				if (ImGui::MenuItem("Vision simulator", NULL, appContext->showVisionSimulator)) {
					appContext->showVisionSimulator = !appContext->showVisionSimulator;
				}

				if (ImGui::MenuItem("Model editor", NULL, appContext->showModelEditor)) {
					appContext->showModelEditor = !appContext->showModelEditor;
				}
				
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		ImGui::DockSpaceOverViewport();

		if (appContext->showDemoWindow) {
			ImGui::ShowDemoWindow(&appContext->showDemoWindow);
		}

		if (appContext->showPlatformWindow) {
			platformShowUi(platform);
		}

		if (appContext->showImageInspector) {
			imageInspectorShowUi(appContext->imageInspector);
		}

		if (appContext->showModelInspector) {
			modelInspectorShowUi(modelInspector);
		}

		if (appContext->showSamplerTester) {
			samplerTesterShowUi(samplerTester);
		}

		if (appContext->showVisionSimulator) {
			visionSimulatorShowUi(visionSimulator);
		}

		if (appContext->showModelEditor) {
			modelEditorShowUi(modelEditor);
		}

		_renderImgui(&_appContext);
	}

	// TODO: Release all the things.
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	gfxCleanupDeviceD3D(&_appContext);
	DestroyWindow(_appContext.hWnd);
	_unregisterWindow(&_appContext);

	platformDestroy(platform);

	return 0;
}