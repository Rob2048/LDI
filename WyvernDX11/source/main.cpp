#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <vector>

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

// TODO: Consider: Per frame, per object, per scene, etc...
struct ldiBasicConstantBuffer {
	vec4 screenSize;
	mat4 mvp;
	vec4 color;
	mat4 view;
	mat4 proj;
};

struct ldiPointCloudConstantBuffer {
	vec4 size;
};

struct ldiCamera {
	vec3 position;
	vec3 rotation;
	mat4 viewMat;
};

struct ldiTextInfo {
	vec2 position;
	std::string text;
};

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

	ID3D11VertexShader*			simpleVertexShader;
	ID3D11PixelShader*			simplePixelShader;
	ID3D11InputLayout*			simpleInputLayout;

	ID3D11VertexShader*			meshVertexShader;
	ID3D11PixelShader*			meshPixelShader;
	ID3D11InputLayout*			meshInputLayout;

	ID3D11VertexShader*			pointCloudVertexShader;
	ID3D11PixelShader*			pointCloudPixelShader;
	ID3D11InputLayout*			pointCloudInputLayout;

	ID3D11Buffer*				mvpConstantBuffer;
	ID3D11Buffer*				pointcloudConstantBuffer;

	ID3D11BlendState*			defaultBlendState;
	ID3D11BlendState*			alphaBlendState;
	ID3D11RasterizerState*		defaultRasterizerState;
	ID3D11RasterizerState*		wireframeRasterizerState;
	ID3D11DepthStencilState*	defaultDepthStencilState;
	
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

	// TODO: Move to platform struct.
	bool						platformConnected = false;
};

inline double _getTime(ldiApp* AppContext) {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	int64_t time = counter.QuadPart - AppContext->timeCounterStart;
	double result = (double)time / ((double)AppContext->timeFrequency);

	return result;
}

#include "graphics.h"
#include "debugPrims.h"
#include "modelInspector.h"
#include "samplerTester.h"

ldiApp				_appContext = {};
ldiModelInspector	_modelInspector = {};
ldiSamplerTester	_samplerTester = {};

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

	ImFont* font = io.Fonts->AddFontFromFileTTF("../assets/fonts/roboto/Roboto-Medium.ttf", 15.0f);
	IM_ASSERT(font != NULL);

	AppContext->fontBig = io.Fonts->AddFontFromFileTTF("../assets/fonts/roboto/Roboto-Bold.ttf", 24.0f);
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

	AppContext->hWnd = CreateWindow(AppContext->wc.lpszClassName, _T("Wyvern DX11"), WS_OVERLAPPEDWINDOW, 100, 100, 1600, 900, NULL, NULL, AppContext->wc.hInstance, NULL);
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
	std::cout << "Compiling shaders\n";

	//----------------------------------------------------------------------------------------------------
	// Basic shader.
	//----------------------------------------------------------------------------------------------------
	D3D11_INPUT_ELEMENT_DESC basicLayout[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0,	DXGI_FORMAT_R32G32_FLOAT,		0,	24,	D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	if (!gfxCreateVertexShader(AppContext, L"../assets/shaders/basic.hlsl", "mainVs", &AppContext->basicVertexShader, basicLayout, 3, &AppContext->basicInputLayout)) {
		return false;
	}

	if (!gfxCreatePixelShader(AppContext, L"../assets/shaders/basic.hlsl", "mainPs", &AppContext->basicPixelShader)) {
		return false;
	}

	//----------------------------------------------------------------------------------------------------
	// Simple shader.
	//----------------------------------------------------------------------------------------------------
	D3D11_INPUT_ELEMENT_DESC simpleLayout[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	if (!gfxCreateVertexShader(AppContext, L"../assets/shaders/simple.hlsl", "mainVs", &AppContext->simpleVertexShader, simpleLayout, 2, &AppContext->simpleInputLayout)) {
		return false;
	}

	if (!gfxCreatePixelShader(AppContext, L"../assets/shaders/simple.hlsl", "mainPs", &AppContext->simplePixelShader)) {
		return false;
	}

	//----------------------------------------------------------------------------------------------------
	// Mesh shader.
	//----------------------------------------------------------------------------------------------------
	D3D11_INPUT_ELEMENT_DESC meshLayout[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0,	DXGI_FORMAT_R32G32_FLOAT,		0,	24,	D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	if (!gfxCreateVertexShader(AppContext, L"../assets/shaders/mesh.hlsl", "mainVs", &AppContext->meshVertexShader, meshLayout, 3, &AppContext->meshInputLayout)) {
		return false;
	}

	if (!gfxCreatePixelShader(AppContext, L"../assets/shaders/mesh.hlsl", "mainPs", &AppContext->meshPixelShader)) {
		return false;
	}

	//----------------------------------------------------------------------------------------------------
	// Pointcloud shader.
	//----------------------------------------------------------------------------------------------------
	D3D11_INPUT_ELEMENT_DESC pointcloudLayout[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0,	DXGI_FORMAT_R32G32_FLOAT,		0,	24,	D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	if (!gfxCreateVertexShader(AppContext, L"../assets/shaders/pointcloud.hlsl", "mainVs", &AppContext->pointCloudVertexShader, pointcloudLayout, 3, &AppContext->pointCloudInputLayout)) {
		return false;
	}

	if (!gfxCreatePixelShader(AppContext, L"../assets/shaders/pointcloud.hlsl", "mainPs", &AppContext->pointCloudPixelShader)) {
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
		desc.FillMode = D3D11_FILL_WIREFRAME;
		desc.CullMode = D3D11_CULL_BACK;
		desc.ScissorEnable = false;
		desc.DepthClipEnable = true;
		desc.DepthBias = -1;
		desc.DepthBiasClamp = 1.0f;
		desc.SlopeScaledDepthBias = -1.0f;
		AppContext->d3dDevice->CreateRasterizerState(&desc, &AppContext->wireframeRasterizerState);
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
	
	return true;
}

//----------------------------------------------------------------------------------------------------
// Application main.
//----------------------------------------------------------------------------------------------------
int main() {
	std::cout << "Starting WyvernDX11\n";

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

	ldiModelInspector* modelInspector = &_modelInspector;
	if (modelInspectorInit(appContext, modelInspector) != 0) {
		std::cout << "Could not init model inspector\n";
		return 1;
	}

	ldiSamplerTester* samplerTester = &_samplerTester;
	if (samplerTesterInit(appContext, samplerTester) != 0) {
		std::cout << "Could not init sampler tester\n";
		return 1;
	}
	
	if (!_initResources(appContext)) {
		return 1;
	}

	initDebugPrimitives(appContext);

	std::cout << "Vector size: " << sizeof(vec3) << "\n";

	/*ImGui::GetID
	ImGui::DockBuilderDockWindow();
	ImGui::DockBuilderFinish();*/

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
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		ImGui::DockSpaceOverViewport();

		if (appContext->showDemoWindow) {
			ImGui::ShowDemoWindow(&appContext->showDemoWindow);
		}

		if (appContext->showPlatformWindow) {
			static float f = 0.0f;
			static int counter = 0;

			//ImGui::SetNextWindowSize(ImVec2(400, ImGui::GetMainViewport()->WorkSize.y));
			//ImGui::SetNextWindowPos(ImVec2(ImGui::GetMainViewport()->WorkSize.x, ImGui::GetMainViewport()->WorkPos.y), 0, ImVec2(1, 0));

			//ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Once);
			ImGui::Begin("Platform controls", &appContext->showPlatformWindow, ImGuiWindowFlags_NoCollapse);

			ImGui::Text("Connection");

			ImGui::BeginDisabled(appContext->platformConnected);
			char ipBuff[] = "192.168.0.50";
			int port = 5000;
			ImGui::InputText("Address", ipBuff, sizeof(ipBuff));
			ImGui::InputInt("Port", &port);
			ImGui::EndDisabled();

			if (appContext->platformConnected) {
				if (ImGui::Button("Disconnect", ImVec2(-1, 0))) {
					appContext->platformConnected = false;
				};
				ImGui::Text("Status: Connected");
			} else {
				if (ImGui::Button("Connect", ImVec2(-1, 0))) {
					appContext->platformConnected = true;
				};
				ImGui::Text("Status: Disconnected");
			}

			ImGui::Separator();

			ImGui::BeginDisabled(!appContext->platformConnected);
			ImGui::Text("Position");
			ImGui::PushFont(appContext->fontBig);
			
			float startX = ImGui::GetCursorPosX();
			float availX = ImGui::GetContentRegionAvail().x;
			ImGui::SetCursorPosX(startX);
			ImGui::TextColored(ImVec4(0.921f, 0.125f, 0.231f, 1.0f), "X: 0.00");
			ImGui::SameLine();
			ImGui::SetCursorPosX(startX + availX / 3);
			ImGui::TextColored(ImVec4(0.164f, 0.945f, 0.266f, 1.0f), "Y: 0.00");
			ImGui::SameLine();
			ImGui::SetCursorPosX(startX + availX / 3 * 2);
			ImGui::TextColored(ImVec4(0.227f, 0.690f, 1.000f, 1.0f), "Z: 0.00");
			ImGui::PopFont();

			ImGui::Button("Find home", ImVec2(-1, 0));
			ImGui::Button("Go home", ImVec2(-1, 0));

			float distance = 1;
			ImGui::InputFloat("Distance", &distance);
			
			ImGui::Button("-X", ImVec2(32, 32));
			ImGui::SameLine();
			ImGui::Button("-Y", ImVec2(32, 32));
			ImGui::SameLine();
			ImGui::Button("-Z", ImVec2(32, 32));

			ImGui::Button("+X", ImVec2(32, 32));
			ImGui::SameLine();
			ImGui::Button("+Y", ImVec2(32, 32));
			ImGui::SameLine();
			ImGui::Button("+Z", ImVec2(32, 32));
			
			ImGui::Separator();
			ImGui::Text("Laser");
			ImGui::Button("Start laser preview", ImVec2(-1, 0));
			ImGui::EndDisabled();

			ImGui::End();
		}

		if (appContext->showImageInspector) {
			//ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_Once);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(700, 600));
			ImGui::Begin("Image inspector", 0, ImGuiWindowFlags_NoCollapse);

				ImGui::BeginChild("ImageChild", ImVec2(530, 0));
					ImGui::Text("Image");
					//ID3D11ShaderResourceView*
					//ImTextureID my_tex_id = io.Fonts->TexID;
					ImVec2 pos = ImGui::GetCursorScreenPos();
					ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
					ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
					ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
					ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
		
					//ImGui::Image(shaderResourceViewTest, ImVec2(512, 512), uv_min, uv_max, tint_col, border_col);
				ImGui::EndChild();

				ImGui::SameLine();
				ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x, 0), false);
					
					ImGui::Text("Display channel");
					ImGui::SameLine();
					ImGui::Button("C");
					ImGui::SameLine();
					ImGui::Button("M");

				ImGui::EndChild();

			ImGui::End();
			ImGui::PopStyleVar();
		}

		if (appContext->showModelInspector) {
			ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Once);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::Begin("Model inspector", &appContext->showModelInspector, ImGuiWindowFlags_NoCollapse);

			ImVec2 viewSize = ImGui::GetContentRegionAvail();
			ImVec2 startPos = ImGui::GetCursorPos();
			ImVec2 screenStartPos = ImGui::GetCursorScreenPos();
			
			// NOTE: ImDrawList API uses screen coordinates!
			/*ImVec2 tempStartPos = ImGui::GetCursorScreenPos();
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			draw_list->AddRectFilled(tempStartPos, ImVec2(tempStartPos.x + 500, tempStartPos.y + 500), IM_COL32(200, 200, 200, 255));*/

			// This will catch our interactions.
			ImGui::InvisibleButton("__mainViewButton", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
			const bool isHovered = ImGui::IsItemHovered(); // Hovered
			const bool isActive = ImGui::IsItemActive();   // Held
			//const ImVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y); // Lock scrolled origin
			//const ImVec2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

			{
				vec3 camMove(0, 0, 0);
				ldiCamera* camera = &modelInspector->camera;
				mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(camera->rotation.y), vec3Right);
				viewRotMat = glm::rotate(viewRotMat, glm::radians(camera->rotation.x), vec3Up);

				if (isActive && (ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle))) {
					ImGui::SetMouseCursor(ImGuiMouseCursor_None);
				
					if (ImGui::IsKeyDown(ImGuiKey_W)) {
						camMove += vec3(vec4(vec3Forward, 0.0f) * viewRotMat);
					}

					if (ImGui::IsKeyDown(ImGuiKey_S)) {
						camMove -= vec3(vec4(vec3Forward, 0.0f) * viewRotMat);
					}

					if (ImGui::IsKeyDown(ImGuiKey_A)) {
						camMove -= vec3(vec4(vec3Right, 0.0f) * viewRotMat);
					}

					if (ImGui::IsKeyDown(ImGuiKey_D)) {
						camMove += vec3(vec4(vec3Right, 0.0f) * viewRotMat);
					}
				}
				
				if (glm::length(camMove) > 0.0f) {
					camMove = glm::normalize(camMove);
					float cameraSpeed = 10.0f * ImGui::GetIO().DeltaTime;
					camera->position += camMove * cameraSpeed;
				}
			}

			if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
				vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
				mouseDelta *= 0.15f;
				modelInspector->camera.rotation += vec3(mouseDelta.x, mouseDelta.y, 0);
			}

			if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
				vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
				mouseDelta *= 0.025f;

				ldiCamera* camera = &modelInspector->camera;
				mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(camera->rotation.y), vec3Right);
				viewRotMat = glm::rotate(viewRotMat, glm::radians(camera->rotation.x), vec3Up);

				camera->position += vec3(vec4(vec3Right, 0.0f) * viewRotMat) * -mouseDelta.x;
				camera->position += vec3(vec4(vec3Up, 0.0f) * viewRotMat) * mouseDelta.y;
			}

			ImGui::SetCursorPos(startPos);
			std::vector<ldiTextInfo> textBuffer;
			modelInspectorRender(modelInspector, viewSize.x, viewSize.y, &textBuffer);
			ImGui::Image(modelInspector->renderViewBuffers.mainViewResourceView, viewSize);

			// NOTE: ImDrawList API uses screen coordinates!
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			for (int i = 0; i < textBuffer.size(); ++i) {
				ldiTextInfo* info = &textBuffer[i];
				drawList->AddText(ImVec2(screenStartPos.x + info->position.x, screenStartPos.y + info->position.y), IM_COL32(255, 255, 255, 255), info->text.c_str());
			}

			// Viewport overlay.
			// TODO: Use command buffer of text locations/strings.
			/*{
				const ImGuiViewport* viewport = ImGui::GetMainViewport();

				ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always, ImVec2(0, 0));
				ImGui::SetNextWindowSize(viewport->Size);
				ImGui::Begin("__viewport", NULL, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMouseInputs);

				ImDrawList* drawList = ImGui::GetWindowDrawList();

				if (clipPos.z > 0) {
					drawList->AddText(ImVec2(screenPos.x, screenPos.y), IM_COL32(255, 255, 255, 255), "X");
				}

				ImGui::End();
			}*/

			{	
				// Viewport overlay widgets.
				ImGui::SetCursorPos(ImVec2(startPos.x + 10, startPos.y + 10));
				ImGui::BeginChild("_simpleOverlayMainView", ImVec2(300, 0), false, ImGuiWindowFlags_NoScrollbar);

				ImGui::Text("Time: (%f)", ImGui::GetTime());
				ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

				ImGui::Separator();
				ImGui::Text("Primary model");
				ImGui::Checkbox("Shaded", &modelInspector->primaryModelShowShaded);
				ImGui::Checkbox("Wireframe", &modelInspector->primaryModelShowWireframe);

				ImGui::Separator();
				ImGui::Text("Quad model");
				ImGui::Checkbox("Area debug", &modelInspector->quadMeshShowDebug);
				ImGui::Checkbox("Quad wireframe", &modelInspector->quadMeshShowWireframe);

				ImGui::Separator();
				ImGui::Text("Point cloud");
				ImGui::Checkbox("Show", &modelInspector->showPointCloud);
				ImGui::SliderFloat("World size", &modelInspector->pointWorldSize, 0.0f, 1.0f);
				ImGui::SliderFloat("Screen size", &modelInspector->pointScreenSize, 0.0f, 32.0f);
				ImGui::SliderFloat("Screen blend", &modelInspector->pointScreenSpaceBlend, 0.0f, 1.0f);

				ImGui::Separator();
				ImGui::Text("Viewport");
				ImGui::ColorEdit3("Background", (float*)&modelInspector->viewBackgroundColor);
				ImGui::ColorEdit3("Grid", (float*)&modelInspector->gridColor);

				ImGui::Separator();
				ImGui::Text("Processing");
				if (ImGui::Button("Process model")) {
					modelInspectorVoxelize(modelInspector);
					modelInspectorCreateQuadMesh(modelInspector);
				}

				if (ImGui::Button("Voxelize")) {
					modelInspectorVoxelize(modelInspector);
				}

				if (ImGui::Button("Quadralize")) {
					modelInspectorCreateQuadMesh(modelInspector);
				}

				ImGui::EndChild();
			}

			ImGui::End();
			ImGui::PopStyleVar();
		}

		if (appContext->showSamplerTester) {
			ImGui::Begin("Sampler tester controls");
			ImGui::Checkbox("Grid", &samplerTester->showGrid);
			if (ImGui::Button("Run sample test")) {
				samplerTesterRunTest(samplerTester);
			}
			ImGui::End();

			ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Once);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::Begin("Sampler tester", &appContext->showSamplerTester, ImGuiWindowFlags_NoCollapse);

			ImVec2 viewSize = ImGui::GetContentRegionAvail();
			ImVec2 startPos = ImGui::GetCursorPos();
			ImVec2 screenStartPos = ImGui::GetCursorScreenPos();

			// This will catch our interactions.
			ImGui::InvisibleButton("__mainViewButton", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
			const bool isHovered = ImGui::IsItemHovered(); // Hovered
			const bool isActive = ImGui::IsItemActive();   // Held
			//ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
			//const ImVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y); // Lock scrolled origin
			ImVec2 mousePos = ImGui::GetIO().MousePos;
			const ImVec2 mouseCanvasPos(mousePos.x - screenStartPos.x, mousePos.y - screenStartPos.y);

			// Convert canvas pos to world pos.
			vec2 worldPos;
			worldPos.x = mouseCanvasPos.x;
			worldPos.y = mouseCanvasPos.y;
			worldPos *= samplerTester->camScale;
			worldPos += vec2(samplerTester->camOffset);
			//std::cout << worldPos.x << ", " << worldPos.y << "\n";

			/*{
				vec3 camMove(0, 0, 0);
				ldiCamera* camera = &modelInspector->camera;
				mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(camera->rotation.y), vec3Right);
				viewRotMat = glm::rotate(viewRotMat, glm::radians(camera->rotation.x), vec3Up);

				if (isActive && (ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle))) {
					ImGui::SetMouseCursor(ImGuiMouseCursor_None);

					if (ImGui::IsKeyDown(ImGuiKey_W)) {
						camMove += vec3(vec4(vec3Forward, 0.0f) * viewRotMat);
					}

					if (ImGui::IsKeyDown(ImGuiKey_S)) {
						camMove -= vec3(vec4(vec3Forward, 0.0f) * viewRotMat);
					}

					if (ImGui::IsKeyDown(ImGuiKey_A)) {
						camMove -= vec3(vec4(vec3Right, 0.0f) * viewRotMat);
					}

					if (ImGui::IsKeyDown(ImGuiKey_D)) {
						camMove += vec3(vec4(vec3Right, 0.0f) * viewRotMat);
					}
				}

				if (glm::length(camMove) > 0.0f) {
					camMove = glm::normalize(camMove);
					float cameraSpeed = 10.0f * ImGui::GetIO().DeltaTime;
					camera->position += camMove * cameraSpeed;
				}
			}

			if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
				vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
				mouseDelta *= 0.15f;
				modelInspector->camera.rotation += vec3(mouseDelta.x, mouseDelta.y, 0);
			}*/

			if (isHovered) {
				float wheel = ImGui::GetIO().MouseWheel;

				if (wheel) {
					samplerTester->camScale -= wheel * 0.1f * samplerTester->camScale;

					vec2 newWorldPos;
					newWorldPos.x = mouseCanvasPos.x;
					newWorldPos.y = mouseCanvasPos.y;
					newWorldPos *= samplerTester->camScale;
					newWorldPos += vec2(samplerTester->camOffset);

					vec2 deltaWorldPos = newWorldPos - worldPos;

					samplerTester->camOffset.x -= deltaWorldPos.x;
					samplerTester->camOffset.y -= deltaWorldPos.y;
				}
			}

			if (isActive && (ImGui::IsMouseDragging(ImGuiMouseButton_Left) || ImGui::IsMouseDragging(ImGuiMouseButton_Right))) {
				vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
				mouseDelta *= samplerTester->camScale;

				samplerTester->camOffset -= vec3(mouseDelta.x, mouseDelta.y, 0);
			}

			ImGui::SetCursorPos(startPos);
			std::vector<ldiTextInfo> textBuffer;
			samplerTesterRender(samplerTester, viewSize.x, viewSize.y, &textBuffer);
			ImGui::Image(samplerTester->renderViewBuffers.mainViewResourceView, viewSize);

			//{
			//	// Viewport overlay widgets.
			//	ImGui::SetCursorPos(ImVec2(startPos.x + 10, startPos.y + 10));
			//	ImGui::BeginChild("_simpleOverlayMainView", ImVec2(300, 0), false, ImGuiWindowFlags_NoScrollbar);

			//	ImGui::Text("Time: (%f)", ImGui::GetTime());
			//	ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			//	ImGui::Separator();
			//	ImGui::Text("Primary model");
			//	ImGui::Checkbox("Shaded", &modelInspector->primaryModelShowShaded);
			//	ImGui::Checkbox("Wireframe", &modelInspector->primaryModelShowWireframe);

			//	ImGui::Separator();
			//	ImGui::Text("Point cloud");
			//	ImGui::Checkbox("Show", &modelInspector->showPointCloud);
			//	ImGui::SliderFloat("World size", &modelInspector->pointWorldSize, 0.0f, 1.0f);
			//	ImGui::SliderFloat("Screen size", &modelInspector->pointScreenSize, 0.0f, 32.0f);
			//	ImGui::SliderFloat("Screen blend", &modelInspector->pointScreenSpaceBlend, 0.0f, 1.0f);

			//	ImGui::Separator();
			//	ImGui::Text("Viewport");
			//	ImGui::ColorEdit3("Background", (float*)&modelInspector->viewBackgroundColor);
			//	ImGui::ColorEdit3("Grid", (float*)&modelInspector->gridColor);

			//	ImGui::Separator();
			//	ImGui::Text("Processing");
			//	if (ImGui::Button("Process model")) {
			//		modelInspectorVoxelize(modelInspector);
			//		modelInspectorCreateQuadMesh(modelInspector);
			//	}

			//	if (ImGui::Button("Voxelize")) {
			//		modelInspectorVoxelize(modelInspector);
			//	}

			//	if (ImGui::Button("Quadralize")) {
			//		modelInspectorCreateQuadMesh(modelInspector);
			//	}

			//	ImGui::EndChild();
			//}

			ImGui::End();
			ImGui::PopStyleVar();
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

	return 0;
}