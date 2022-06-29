#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <vector>

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

#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

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

	ID3D11VertexShader*			imgCamVertexShader;
	ID3D11PixelShader*			imgCamPixelShader;

	ID3D11Buffer*				mvpConstantBuffer;
	ID3D11Buffer*				pointcloudConstantBuffer;

	ID3D11BlendState*			defaultBlendState;
	ID3D11BlendState*			alphaBlendState;
	ID3D11RasterizerState*		defaultRasterizerState;
	ID3D11RasterizerState*		wireframeRasterizerState;
	ID3D11DepthStencilState*	defaultDepthStencilState;

	ID3D11ShaderResourceView*	camResourceView;
	ID3D11SamplerState*			camSamplerState;
	ID3D11Texture2D*			camTex;
	
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
	bool						showImageInspector = true;
	bool						showModelInspector = true;
	bool						showSamplerTester = false;
	bool						showCamInspector = false;

	// TODO: Move to platform struct.
	bool						platformConnected = false;

	float						camImageFilterFactor = 0.6f;
	std::vector<vec2>			camImageMarkerCorners;
	std::vector<int>			camImageMarkerIds;
	std::vector<vec2>			camImageCharucoCorners;
	vec2						camImageCursorPos;
	uint8_t						camImagePixelValue;
	int							camImageShutterSpeed = 8000;
	int							camImageAnalogGain = 1;
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
#include "camInspector.h"

ldiApp				_appContext = {};
ldiModelInspector	_modelInspector = {};
ldiSamplerTester	_samplerTester = {};
ldiCamInspector		_camInspector = {};
ldiServer			_server = {};

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
// OpenCV functionality.
//----------------------------------------------------------------------------------------------------
using namespace cv;

cv::Ptr<cv::aruco::Dictionary> _dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_1000);
std::vector<cv::Ptr<cv::aruco::CharucoBoard>> _charucoBoards;

void CreateCharucos() {
	//for (int i = 0; i < 6; ++i) {
	//	cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(10, 10, 0.009f, 0.006f, _dictionary);
	//	charucoBoards.push_back(board);

	//	// NOTE: Shift ids.
	//	// board->ids.
	//	int offset = i * board->ids.size();

	//	for (int j = 0; j < board->ids.size(); ++j) {
	//		board->ids[j] = offset + j;
	//	}
	//}

	try {
		for (int i = 0; i < 6; ++i) {
			//cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(10, 10, 0.9f, 0.6f, _dictionary);
			cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(4, 4, 0.9f, 0.6f, _dictionary);
			_charucoBoards.push_back(board);

			// NOTE: Shift ids.
			int offset = i * board->ids.size();

			for (int j = 0; j < board->ids.size(); ++j) {
				board->ids[j] = offset + j;
			}

			// NOTE: Image output.
			cv::Mat markerImage;
			board->draw(cv::Size(1000, 1000), markerImage, 50, 1);
			char fileName[512];
			sprintf_s(fileName, "charuco_small_%d.png", i);
			cv::imwrite(fileName, markerImage);
		}

	}
	catch (cv::Exception e) {
		std::cout << "Exception: " << e.what() << "\n" << std::flush;
	}
}

void _findCharuco(uint8_t* Data) {
	double t0 = _getTime(&_appContext);

	int offset = 1;

	int width = 1600;
	int height = 1300;
	int id = -1;

	try {
		Mat image(Size(width, height), CV_8UC1, Data);
		//Mat image;
		//cv::flip(imageSrc, image, 0);
		Mat imageDebug;
		cv::cvtColor(image, imageDebug, cv::COLOR_GRAY2RGB);

		std::vector<int> markerIds;
		std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCandidates;
		cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
		// TODO: Enable subpixel corner refinement.
		// WTF is this doing?
		//parameters->cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
		cv::aruco::detectMarkers(image, _dictionary, markerCorners, markerIds, parameters, rejectedCandidates);
		// Use refine strategy to detect more markers.
		//cv::Ptr<cv::aruco::Board> board = charucoBoard.staticCast<aruco::Board>();
		//aruco::refineDetectedMarkers(image, board, markerCorners, markerIds, rejectedCandidates);

		if (id != -1) {
			aruco::drawDetectedMarkers(imageDebug, markerCorners, markerIds);
		}

		std::vector<std::vector<Point2f>> charucoCorners(6);
		std::vector<std::vector<int>> charucoIds(6);

		_appContext.camImageMarkerCorners.clear();
		_appContext.camImageCharucoCorners.clear();
		_appContext.camImageMarkerIds = markerIds;

		for (int i = 0; i < markerCorners.size(); ++i) {
			for (int j = 0; j < markerCorners[i].size(); ++j) {
				_appContext.camImageMarkerCorners.push_back(vec2(markerCorners[i][j].x, markerCorners[i][j].y));
			}
		}

		// Interpolate charuco corners for each possible board.
		for (int i = 0; i < 6; ++i) {
			if (markerCorners.size() > 0) {
				aruco::interpolateCornersCharuco(markerCorners, markerIds, image, _charucoBoards[i], charucoCorners[i], charucoIds[i]);
			}

			if (charucoCorners[i].size() > 0) {
				if (id != -1) {
					aruco::drawDetectedCornersCharuco(imageDebug, charucoCorners[i], charucoIds[i]);
				}
			}
		}

		int boardCount = charucoIds.size();
		//send(Client, (const char*)&boardCount, 4, 0);
		std::cout << "Boards: " << boardCount << "\n";

		for (int i = 0; i < boardCount; ++i) {
			int markerCount = charucoIds[i].size();
			std::cout << "Board " << i << " markers: " << markerCount << "\n";
			//send(Client, (const char*)&markerCount, 4, 0);

			for (int j = 0; j < markerCount; ++j) {
				_appContext.camImageCharucoCorners.push_back(vec2(charucoCorners[i][j].x, charucoCorners[i][j].y));
				std::cout << charucoCorners[i][j].x << ", " << charucoCorners[i][j].y << "\n";
				/*send(Client, (const char*)&charucoIds[i][j], 4, 0);
				send(Client, (const char*)&charucoCorners[i][j], 8, 0);*/
			}
		}

		if (id != -1) {
			char buf[256];
			sprintf_s(buf, "debug_%d.png", id);
			cv::imwrite(buf, imageDebug);
		}

		t0 = _getTime(&_appContext) - t0;
		std::cout << "Find charuco: " << (t0 * 1000.0) << " ms\n";
	}
	catch (Exception e) {
		std::cout << "Exception: " << e.what() << "\n" << std::flush;
	}

	//std::cout << "Done finding charuco\n" << std::flush;
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
	// Image cam shader.
	//----------------------------------------------------------------------------------------------------
	ID3D11InputLayout* imguiUiLayoutOb;
	D3D11_INPUT_ELEMENT_DESC imguiUiLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	if (!gfxCreateVertexShader(AppContext, L"../assets/shaders/imgCam.hlsl", "mainVs", &AppContext->imgCamVertexShader, imguiUiLayout, 3, &imguiUiLayoutOb)) {
		return false;
	}

	if (!gfxCreatePixelShader(AppContext, L"../assets/shaders/imgCam.hlsl", "mainPs", &AppContext->imgCamPixelShader)) {
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

	// Prime camera texture.
	{
		D3D11_TEXTURE2D_DESC tex2dDesc = {};
		tex2dDesc.Width = 1600;
		tex2dDesc.Height = 1300;
		tex2dDesc.MipLevels = 1;
		tex2dDesc.ArraySize = 1;
		tex2dDesc.Format = DXGI_FORMAT_R8_UNORM;
		tex2dDesc.SampleDesc.Count = 1;
		tex2dDesc.SampleDesc.Quality = 0;
		tex2dDesc.Usage = D3D11_USAGE_DYNAMIC;
		tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		tex2dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		tex2dDesc.MiscFlags = 0;

		if (AppContext->d3dDevice->CreateTexture2D(&tex2dDesc, NULL, &AppContext->camTex) != S_OK) {
			std::cout << "Texture failed to create\n";
		}

		AppContext->d3dDevice->CreateShaderResourceView(AppContext->camTex, NULL, &AppContext->camResourceView);

		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		AppContext->d3dDevice->CreateSamplerState(&samplerDesc, &AppContext->camSamplerState);
	}
	
	return true;
}

void _imageInspectorSetStateCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
	//AddDrawCmd ??
	_appContext.d3dDeviceContext->PSSetSamplers(0, 1, &_appContext.camSamplerState);
	_appContext.d3dDeviceContext->PSSetShader(_appContext.imgCamPixelShader, NULL, 0);
	_appContext.d3dDeviceContext->VSSetShader(_appContext.imgCamVertexShader, NULL, 0);
}

//----------------------------------------------------------------------------------------------------
// Network command handling.
//----------------------------------------------------------------------------------------------------
static float _pixels[1600 * 1300] = {};
static uint8_t _pixelsFinal[1600 * 1300] = {};

void _processPacket(ldiApp* AppContext, ldiPacketView* PacketView) {
	ldiProtocolHeader* packetHeader = (ldiProtocolHeader*)PacketView->data;
	//std::cout << "Opcode: " << packetHeader->opcode << "\n";

	if (packetHeader->opcode == 0) {
		ldiProtocolSettings* packet = (ldiProtocolSettings*)PacketView->data;
		std::cout << "Got settings: " << packet->shutterSpeed << " " << packet->analogGain << "\n";

		AppContext->camImageShutterSpeed = packet->shutterSpeed;
		AppContext->camImageAnalogGain = packet->analogGain;

	} else if (packetHeader->opcode == 1) {
		ldiProtocolImageHeader* imageHeader = (ldiProtocolImageHeader*)PacketView->data;
		//std::cout << "Got image " << imageHeader->width << " " << imageHeader->height << "\n";

		uint8_t* frameData = PacketView->data + sizeof(ldiProtocolImageHeader);

		for (int i = 0; i < 1600 * 1300; ++i) {
			_pixels[i] = _pixels[i] * _appContext.camImageFilterFactor + frameData[i] * (1.0f - _appContext.camImageFilterFactor);
			_pixelsFinal[i] = _pixels[i];
		}
		
		D3D11_MAPPED_SUBRESOURCE ms;
		AppContext->d3dDeviceContext->Map(AppContext->camTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		uint8_t* pixelData = (uint8_t*)ms.pData;

		for (int i = 0; i < 1300; ++i) {
			// memcpy(pixelData + i * ms.RowPitch, (PacketView->data + sizeof(ldiProtocolImageHeader)) + i * 1600, 1600);
			memcpy(pixelData + i * ms.RowPitch, _pixelsFinal + i * 1600, 1600);
		}
		
		AppContext->d3dDeviceContext->Unmap(AppContext->camTex, 0);

	} else {
		std::cout << "Error: Got unknown opcode (" << packetHeader->opcode << ") from packet\n";
	}
}

//----------------------------------------------------------------------------------------------------
// Application main.
//----------------------------------------------------------------------------------------------------
int main() {
	std::cout << "Starting WyvernDX11\n";

	CreateCharucos();

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

	ldiCamInspector* camInspector = &_camInspector;
	if (camInspectorInit(appContext, camInspector) != 0) {
		std::cout << "Could not init cam inspector\n";
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

	if (!networkInit(&_server, "20000")) {
		std::cout << "Networking failure\n";
		return 1;
	}

	// Main loop
	bool running = true;
	while (running) {
		
		if (_handleWindowLoop()) {
			break;
		}

		while (true) {
			ldiPacketView packetView;
			int updateResult = networkUpdate(&_server, &packetView);

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
				if (ImGui::MenuItem("Cam inspector", NULL, appContext->showCamInspector)) {
					appContext->showCamInspector = !appContext->showCamInspector;
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
			ImGui::Begin("Image inspector controls");
			if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
				bool newSettings = false;

				if (ImGui::SliderInt("Shutter speed", &appContext->camImageShutterSpeed, 10, 100000)) {
					//std::cout << "Set shutter: " << appContext->camImageShutterSpeed << "\n";
					newSettings = true;
				}

				if (ImGui::SliderInt("Analog gain", &appContext->camImageAnalogGain, 0, 100)) {
					//std::cout << "Set shutter: " << appContext->camImageAnalogGain << "\n";
					newSettings = true;
				}

				if (newSettings) {
					ldiProtocolSettings settings;
					settings.header.packetSize = sizeof(ldiProtocolSettings) - 4;
					settings.header.opcode = 0;
					settings.shutterSpeed = appContext->camImageShutterSpeed;
					settings.analogGain = appContext->camImageAnalogGain;

					networkSend(&_server, (uint8_t*)&settings, sizeof(ldiProtocolSettings));
				}
			}

			static float imgScale = 1.0f;
			static vec2 imgOffset(0.0f, 0.0f);

			if (ImGui::CollapsingHeader("Image", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::SliderFloat("Scale", &imgScale, 0.1f, 10.0f);
				ImGui::SliderFloat("Filter factor", &appContext->camImageFilterFactor, 0.0f, 1.0f);
				//char strBuff[256];
				//sprintf_s(strBuff, sizeof(strBuff), 
				ImGui::Text("Cursor position: %.2f, %.2f", appContext->camImageCursorPos.x, appContext->camImageCursorPos.y);
				ImGui::Text("Cursor value: %d", appContext->camImagePixelValue);
			}

			if (ImGui::CollapsingHeader("Machine vision", ImGuiTreeNodeFlags_DefaultOpen)) {
				if (ImGui::Button("Find Charuco")) {
					_findCharuco(_pixelsFinal);
				}
			}

			//ImGui::Checkbox("Grid", &camInspector->showGrid);
			//if (ImGui::Button("Run sample test")) {
				//camInspectorRunTest(camInspector);
			//}
			ImGui::End();

			//ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_Once);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(700, 600));
			ImGui::Begin("Image inspector", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar);

			ImVec2 viewSize = ImGui::GetContentRegionAvail();
			ImVec2 startPos = ImGui::GetCursorPos();
			ImVec2 screenStartPos = ImGui::GetCursorScreenPos();

			// This will catch our interactions.
			ImGui::InvisibleButton("__imgInspecViewButton", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
			const bool isHovered = ImGui::IsItemHovered(); // Hovered
			const bool isActive = ImGui::IsItemActive();   // Held
			ImVec2 mousePos = ImGui::GetIO().MousePos;
			const ImVec2 mouseCanvasPos(mousePos.x - screenStartPos.x, mousePos.y - screenStartPos.y);

			// Convert canvas pos to world pos.
			vec2 worldPos;
			worldPos.x = mouseCanvasPos.x;
			worldPos.y = mouseCanvasPos.y;
			worldPos *= (1.0 / imgScale);
			worldPos -= imgOffset;

			if (isHovered) {
				float wheel = ImGui::GetIO().MouseWheel;

				if (wheel) {
					imgScale += wheel * 0.2f * imgScale;

					vec2 newWorldPos;
					newWorldPos.x = mouseCanvasPos.x;
					newWorldPos.y = mouseCanvasPos.y;
					newWorldPos *= (1.0 / imgScale);
					newWorldPos -= imgOffset;

					vec2 deltaWorldPos = newWorldPos - worldPos;

					imgOffset.x += deltaWorldPos.x;
					imgOffset.y += deltaWorldPos.y;
				}
			}

			if (isActive && (ImGui::IsMouseDragging(ImGuiMouseButton_Left) || ImGui::IsMouseDragging(ImGuiMouseButton_Right))) {
				vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
				mouseDelta *= (1.0 / imgScale);

				imgOffset += vec2(mouseDelta.x, mouseDelta.y);
			}

				//ImGui::BeginChild("ImageChild", ImVec2(530, 0));
					//ImGui::Text("Image");
					//ID3D11ShaderResourceView*
					//ImTextureID my_tex_id = io.Fonts->TexID;
					ImVec2 pos = ImGui::GetCursorScreenPos();
					ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
					ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
					ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
					ImVec4 border_col = ImVec4(0.5f, 0.5f, 0.5f, 0.0f); // 50% opaque white
		
					//ImGui::Image(shaderResourceViewTest, ImVec2(512, 512), uv_min, uv_max, tint_col, border_col);
					ImDrawList* draw_list = ImGui::GetWindowDrawList();
					draw_list->AddCallback(_imageInspectorSetStateCallback, 0);

					//window->DC.CursorPos
					//ImGui::Image(appContext->camResourceView, ImVec2(1600 * imgScale, 1300 * imgScale), uv_min, uv_max, tint_col, border_col);
					ImVec2 imgMin;
					imgMin.x = screenStartPos.x + imgOffset.x * imgScale;
					imgMin.y = screenStartPos.y + imgOffset.y * imgScale;

					ImVec2 imgMax;
					imgMax.x = imgMin.x + 1600 * imgScale;
					imgMax.y = imgMin.y + 1300 * imgScale;

					draw_list->AddImage(appContext->camResourceView, imgMin, imgMax, uv_min, uv_max, ImGui::GetColorU32(tint_col));

					draw_list->AddCallback(ImDrawCallback_ResetRenderState, 0);

					std::vector<vec2> markerCentroids;

					for (int i = 0; i < appContext->camImageMarkerCorners.size() / 4; ++i) {
						/*ImVec2 offset = pos;
						offset.x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale;
						offset.y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale;*/

						ImVec2 points[5];

						vec2 markerCentroid(0.0f, 0.0f);

						for (int j = 0; j < 4; ++j) {
							vec2 o = appContext->camImageMarkerCorners[i * 4 + j];

							points[j] = pos;
							points[j].x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale;
							points[j].y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale;

							markerCentroid.x += points[j].x;
							markerCentroid.y += points[j].y;
						}
						points[4] = points[0];

						markerCentroid /= 4.0f;
						markerCentroids.push_back(markerCentroid);

						draw_list->AddPolyline(points, 5, ImColor(0, 0, 255), 0, 1.0f);
						//draw_list->AddCircle(offset, 4.0f, ImColor(0, 0, 255));
					}

					//std::cout << appContext->camImageMarkerIds.size() << "\n";

					for (int i = 0; i < appContext->camImageMarkerIds.size(); ++i) {
						char strBuff[32];
						sprintf_s(strBuff, sizeof(strBuff), "%d", appContext->camImageMarkerIds[i]);
						draw_list->AddText(ImVec2(markerCentroids[i].x, markerCentroids[i].y), ImColor(52, 195, 235), strBuff);
					}

					for (int i = 0; i < appContext->camImageCharucoCorners.size(); ++i) {
						vec2 o = appContext->camImageCharucoCorners[i];

						// TODO: Do we need half pixel offset? Check debug drawing commands.
						ImVec2 offset = pos;
						offset.x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale;
						offset.y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale;

						draw_list->AddCircle(offset, 4.0f, ImColor(0, 255, 0));
					}

					for (int i = 0; i < appContext->camImageCharucoCorners.size(); ++i) {
						vec2 o = appContext->camImageCharucoCorners[i];

						ImVec2 offset = pos;
						offset.x = screenStartPos.x + (imgOffset.x + o.x + 0.5) * imgScale + 5;
						offset.y = screenStartPos.y + (imgOffset.y + o.y + 0.5) * imgScale - 15;

						char strBuf[256];
						sprintf_s(strBuf, 256, "%.2f, %.2f", o.x, o.y);

						draw_list->AddText(offset, ImColor(0, 200, 0), strBuf);
					}

					// Cursor.
					{
						vec2 pixelPos;
						pixelPos.x = (int)worldPos.x;
						pixelPos.y = (int)worldPos.y;

						appContext->camImageCursorPos = worldPos;

						if (pixelPos.x >= 0 && pixelPos.x < 1600) {
							if (pixelPos.y >= 0 && pixelPos.y < 1300) {
								appContext->camImagePixelValue = _pixelsFinal[(int)pixelPos.y * 1600 + (int)pixelPos.x];
							}
						}

						ImVec2 rMin;
						rMin.x = screenStartPos.x + (imgOffset.x + pixelPos.x) * imgScale;
						rMin.y = screenStartPos.y + (imgOffset.y + pixelPos.y) * imgScale;

						ImVec2 rMax = rMin;
						rMax.x += imgScale;
						rMax.y += imgScale;

						draw_list->AddRect(rMin, rMax, ImColor(255, 0, 255));
					}
					
					
				//ImGui::EndChild();

				/*ImGui::SameLine();
				ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x, 0), false);
					
					ImGui::Text("Display channel");
					ImGui::SameLine();
					ImGui::Button("C");
					ImGui::SameLine();
					ImGui::Button("M");

				ImGui::EndChild();*/

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
			ImGui::InvisibleButton("__sampleTestViewButton", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
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

		if (appContext->showCamInspector) {
			ImGui::Begin("Cam inspector controls");
			ImGui::Checkbox("Grid", &camInspector->showGrid);
			if (ImGui::Button("Run sample test")) {
				//camInspectorRunTest(camInspector);
			}
			ImGui::End();

			ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Once);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::Begin("Camera inspector", &appContext->showCamInspector, ImGuiWindowFlags_NoCollapse);

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
			worldPos *= camInspector->camScale;
			worldPos += vec2(camInspector->camOffset);
			//std::cout << worldPos.x << ", " << worldPos.y << "\n";

			if (isHovered) {
				float wheel = ImGui::GetIO().MouseWheel;

				if (wheel) {
					camInspector->camScale -= wheel * 0.1f * camInspector->camScale;

					vec2 newWorldPos;
					newWorldPos.x = mouseCanvasPos.x;
					newWorldPos.y = mouseCanvasPos.y;
					newWorldPos *= camInspector->camScale;
					newWorldPos += vec2(camInspector->camOffset);

					vec2 deltaWorldPos = newWorldPos - worldPos;

					camInspector->camOffset.x -= deltaWorldPos.x;
					camInspector->camOffset.y -= deltaWorldPos.y;
				}
			}

			if (isActive && (ImGui::IsMouseDragging(ImGuiMouseButton_Left) || ImGui::IsMouseDragging(ImGuiMouseButton_Right))) {
				vec2 mouseDelta = vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
				mouseDelta *= camInspector->camScale;

				camInspector->camOffset -= vec3(mouseDelta.x, mouseDelta.y, 0);
			}

			ImGui::SetCursorPos(startPos);
			std::vector<ldiTextInfo> textBuffer;
			camInspectorRender(camInspector, viewSize.x, viewSize.y, &textBuffer);
			ImGui::Image(camInspector->renderViewBuffers.mainViewResourceView, viewSize);

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