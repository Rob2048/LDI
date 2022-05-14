#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <vector>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <d3d11.h>
#include <d3dcompiler.h>

#include "glm.h"
#include "model.h"
#include "objLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

enum ldiTool {
	LDITOOL_NONE = 0,
	LDITOOL_PLATFORM = 1
};

struct ldiToolContext {
	ldiTool selectedTool = LDITOOL_NONE;
};

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

struct ldiSimpleVertex {
	vec3 position;
	vec3 color;
};

struct ldiBasicVertex {
	vec3 position;
	vec3 color;
	vec2 uv;
};

struct ldiCamera {
	vec3 position;
	vec3 rotation;
	mat4 viewMat;
};

struct ldiInput {
	bool	keyForward = false;
	bool	keyBackward = false;
	bool	keyLeft = false;
	bool	keyRight = false;
	bool	mouseRight = false;
	bool	mouseLeft = false;
	bool	mouseMiddle = false;
	bool	lockedMouse = false;

	int		indirectionUIMipLevel = 0;
	bool	purgeCache = false;
	bool	vtDebug = false;
};

// Application and platform context.
struct ldiApp {
	HWND						hWnd;
	WNDCLASSEX					wc;
	uint32_t					windowWidth;
	uint32_t					windowHeight;

	ImFont*						fontBig;

	ID3D11Device*				d3dDevice;
	ID3D11DeviceContext*		d3dDeviceContext;
	IDXGISwapChain*				SwapChain;
	ID3D11RenderTargetView*		mainRenderTargetView;

	ID3D11DepthStencilView*		depthStencilView;
	ID3D11Texture2D*			depthStencilBuffer;

	ldiToolContext				tool;

	bool						showPlatformWindow = true;
	bool						showDemoWindow = true;

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

	ldiInput					input;
	ldiCamera					camera;

	bool						primaryModelShowWireframe = false;
	bool						primaryModelShowShaded = false;

	float						pointWorldSize = 0.02f;
	float						pointScreenSize = 2.0f;
	float						pointScreenSpaceBlend = 0.0f;

	vec4						viewBackgroundColor = { 0.2f, 0.23f, 0.26f, 1.00f };
	vec4						gridColor = { 0.3f, 0.33f, 0.36f, 1.00f };
};

ldiApp _appContext;

#include "graphics.h"
#include "debugPrims.h"

//----------------------------------------------------------------------------------------------------
// Windowing helpers.
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
	io.IniFilename = NULL;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(AppContext->hWnd);
	ImGui_ImplDX11_Init(AppContext->d3dDevice, AppContext->d3dDeviceContext);

	ImFont* font = io.Fonts->AddFontFromFileTTF("../assets/fonts/roboto/Roboto-Medium.ttf", 15.0f);
	IM_ASSERT(font != NULL);

	AppContext->fontBig = io.Fonts->AddFontFromFileTTF("../assets/fonts/roboto/Roboto-Bold.ttf", 24.0f);
	IM_ASSERT(AppContext->fontBig != NULL);
}

void _renderImgui(ldiApp* AppContext) {
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
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

		case WM_KEYDOWN: {
			int32_t key = (int32_t)wParam;
			
			if (key == 87) _appContext.input.keyForward = true;
			if (key == 83) _appContext.input.keyBackward = true;
			if (key == 65) _appContext.input.keyLeft = true;
			if (key == 68) _appContext.input.keyRight = true;

			break;
		}

		case WM_KEYUP: {
			int32_t key = (int32_t)wParam;
			
			if (key == 87) _appContext.input.keyForward = false;
			if (key == 83) _appContext.input.keyBackward = false;
			if (key == 65) _appContext.input.keyLeft = false;
			if (key == 68) _appContext.input.keyRight = false;

			break;
		}

		case WM_RBUTTONDOWN: {
			_appContext.input.mouseRight = true;
			break;
		}

		case WM_RBUTTONUP: {
			_appContext.input.mouseRight = false;
			break;
		}

		case WM_MBUTTONDOWN: {
			_appContext.input.mouseMiddle = true;
			break;
		}

		case WM_MBUTTONUP: {
			_appContext.input.mouseMiddle = false;
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
		desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
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

	appContext->camera.position = vec3(0, 0, 10);
	appContext->camera.rotation = vec3(0, 0, 0);

	if (!_initResources(appContext)) {
		return 1;
	}

	initDebugPrimitives(appContext);

	ldiModel dergnModel = objLoadModel("../assets/models/dergn.obj");
	ldiRenderModel dergnRenderModel = gfxCreateRenderModel(appContext, &dergnModel);

	std::vector<ldiSimpleVertex> pointCloudPoints;
	float pointSpacing = 0.1f;

	for (int iZ = 0; iZ < 1; ++iZ) {
		for (int iY = 0; iY < 1; ++iY) {
			for (int iX = 0; iX < 1; ++iX) {
				pointCloudPoints.push_back({ vec3(iX * pointSpacing, iY * pointSpacing, iZ * pointSpacing), vec3(iX / 100.0f, iY / 100.0f, iZ / 100.0f) });
			}
		}
	}

	ldiRenderPointCloud pointCloud = gfxCreateRenderPointCloud(appContext, &pointCloudPoints);

	int x, y, n;
	uint8_t *imageRawPixels = stbi_load("../assets/models/dergnTexture.png", &x, &y, &n, 0);
	std::cout << "Load texture: " << x << ", " << y << " (" << n << ")\n";

	uint8_t* packedTexData = new uint8_t[x * y * 4];
	for (int i = 0; i < x * y; ++i) {
		packedTexData[i * 4 + 0] = imageRawPixels[i * n + 0];
		packedTexData[i * 4 + 1] = imageRawPixels[i * n + 1];
		packedTexData[i * 4 + 2] = imageRawPixels[i * n + 2];
		packedTexData[i * 4 + 3] = 255;
	}

	stbi_image_free(imageRawPixels);

	D3D11_SUBRESOURCE_DATA texData = {};
	texData.pSysMem = packedTexData;
	texData.SysMemPitch = x * 4;

	D3D11_TEXTURE2D_DESC tex2dDesc = {};
	tex2dDesc.Width = x;
	tex2dDesc.Height = y;
	tex2dDesc.MipLevels = 1;
	tex2dDesc.ArraySize = 1;
	tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tex2dDesc.SampleDesc.Count = 1;
	tex2dDesc.SampleDesc.Quality = 0;
	tex2dDesc.Usage = D3D11_USAGE_IMMUTABLE;
	tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	tex2dDesc.CPUAccessFlags = 0;
	tex2dDesc.MiscFlags = 0;

	ID3D11Texture2D* texResource;
	if (appContext->d3dDevice->CreateTexture2D(&tex2dDesc, &texData, &texResource) != S_OK) {
		std::cout << "Texture failed to create\n";
	}

	delete[] packedTexData;

	ID3D11ShaderResourceView* shaderResourceViewTest;
	appContext->d3dDevice->CreateShaderResourceView(texResource, NULL, &shaderResourceViewTest);

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	
	ID3D11SamplerState* texSamplerState;
	appContext->d3dDevice->CreateSamplerState(&samplerDesc, &texSamplerState);

	// Main loop
	bool running = true;
	while (running) {
		
		if (_handleWindowLoop()) {
			break;
		}

		//----------------------------------------------------------------------------------------------------
		// Initial debug primitives.
		//----------------------------------------------------------------------------------------------------
		beginDebugPrimitives(appContext);
		pushDebugLine(appContext, vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0));
		pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 1, 0));
		pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 0, 1));

		//pushDebugBox(appContext, vec3(0, 0, 0), vec3(2, 2, 2), vec3(1, 0, 1));
		//pushDebugBoxSolid(appContext, vec3(2, 0, 0), vec3(0.1, 0.1, 0.1), vec3(0.5f, 0, 0.5f));

		// Grid.
		int gridCount = 10;
		float gridCellWidth = 1.0f;
		vec3 gridColor = appContext->gridColor;
		vec3 gridHalfOffset = vec3(gridCellWidth * gridCount, 0, gridCellWidth * gridCount) * 0.5f;
		for (int i = 0; i < gridCount + 1; ++i) {
			pushDebugLine(appContext, vec3(i * gridCellWidth, 0, 0) - gridHalfOffset, vec3(i * gridCellWidth, 0, gridCount * gridCellWidth) - gridHalfOffset, gridColor);
			pushDebugLine(appContext, vec3(0, 0, i * gridCellWidth) - gridHalfOffset, vec3(gridCount * gridCellWidth, 0, i * gridCellWidth) - gridHalfOffset, gridColor);
		}

		//----------------------------------------------------------------------------------------------------
		// Update input.
		//----------------------------------------------------------------------------------------------------
		if (!ImGui::GetIO().WantCaptureKeyboard) {
			ldiCamera* camera = &appContext->camera;
			mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(camera->rotation.y), vec3Right);
			viewRotMat = glm::rotate(viewRotMat, glm::radians(camera->rotation.x), vec3Up);
			
			vec3 camMove(0, 0, 0);
			if (appContext->input.keyForward) camMove += vec3(vec4(vec3Forward, 0.0f) * viewRotMat);
			if (appContext->input.keyBackward) camMove -= vec3(vec4(vec3Forward, 0.0f) * viewRotMat);
			if (appContext->input.keyRight) camMove += vec3(vec4(vec3Right, 0.0f) * viewRotMat);
			if (appContext->input.keyLeft) camMove -= vec3(vec4(vec3Right, 0.0f) * viewRotMat);

			if (glm::length(camMove) > 0.0f) {
				camMove = glm::normalize(camMove);
				float cameraSpeed = 10.0f * ImGui::GetIO().DeltaTime;
				camera->position += camMove * cameraSpeed;
			}
		}

		if (!ImGui::GetIO().WantCaptureMouse) {
			if (appContext->input.mouseRight) {
				ImGui::SetWindowFocus(NULL);

				vec2 mouseCenter = vec2(appContext->windowWidth / 2, appContext->windowHeight / 2);

				if (!appContext->input.lockedMouse) {
					appContext->input.lockedMouse = true;
					ShowCursor(false);
					setMousePosition(appContext, mouseCenter);
				} else {
					vec2 mouseDelta = getMousePosition(appContext) - mouseCenter;
					mouseDelta *= 0.15f;
					appContext->camera.rotation += vec3(mouseDelta.x, mouseDelta.y, 0);
					setMousePosition(appContext, mouseCenter);
				}
			} else if (appContext->input.mouseMiddle) {
				ImGui::SetWindowFocus(NULL);

				vec2 mouseCenter = vec2(appContext->windowWidth / 2, appContext->windowHeight / 2);

				if (!appContext->input.lockedMouse) {
					appContext->input.lockedMouse = true;
					ShowCursor(false);
					setMousePosition(appContext, mouseCenter);
				} else {
					vec2 mouseDelta = getMousePosition(appContext) - mouseCenter;
					mouseDelta *= 0.025f;

					ldiCamera* camera = &appContext->camera;
					mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(camera->rotation.y), vec3Right);
					viewRotMat = glm::rotate(viewRotMat, glm::radians(camera->rotation.x), vec3Up);

					camera->position += vec3(vec4(vec3Right, 0.0f) * viewRotMat) * -mouseDelta.x;
					camera->position += vec3(vec4(vec3Up, 0.0f) * viewRotMat) * mouseDelta.y;

					setMousePosition(appContext, mouseCenter);
				}
			} else {
				if (appContext->input.lockedMouse)
				{
					appContext->input.lockedMouse = false;
					ShowCursor(true);
				}
			}
		}

		//----------------------------------------------------------------------------------------------------
		// Update camera.
		//----------------------------------------------------------------------------------------------------
		mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(appContext->camera.rotation.y), vec3Right);
		viewRotMat = glm::rotate(viewRotMat, glm::radians(appContext->camera.rotation.x), vec3Up);
		mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -appContext->camera.position);

		mat4 projMat = glm::perspectiveFovRH_ZO(glm::radians(50.0f), (float)appContext->windowWidth, (float)appContext->windowHeight, 0.01f, 100.0f);
		mat4 invProjMat = inverse(projMat);
		mat4 modelMat = mat4(1.0f);

		mat4 projViewModelMat = projMat * camViewMat * modelMat;

		//----------------------------------------------------------------------------------------------------
		// Rendering.
		//----------------------------------------------------------------------------------------------------
		gfxBeginPrimaryViewport(appContext);

		//----------------------------------------------------------------------------------------------------
		// Render debug primitives.
		//----------------------------------------------------------------------------------------------------
		{
			D3D11_MAPPED_SUBRESOURCE ms;
			appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
			ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
			constantBuffer->mvp = projViewModelMat;
			constantBuffer->color = vec4(1, 1, 1, 1);
			appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);
		}

		renderDebugPrimitives(appContext);

		//----------------------------------------------------------------------------------------------------
		// Render models.
		//----------------------------------------------------------------------------------------------------
		if (appContext->primaryModelShowShaded) {
			gfxRenderModel(appContext, &dergnRenderModel, false, shaderResourceViewTest, texSamplerState);
		}

		if (appContext->primaryModelShowWireframe) {
			D3D11_MAPPED_SUBRESOURCE ms;
			appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
			ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
			constantBuffer->mvp = projViewModelMat;
			constantBuffer->color = vec4(0, 0, 0, 1);
			appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);
			
			gfxRenderModel(appContext, &dergnRenderModel, true);
		}

		{
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				appContext->d3dDeviceContext->Map(appContext->mvpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
				ldiBasicConstantBuffer* constantBuffer = (ldiBasicConstantBuffer*)ms.pData;
				constantBuffer->screenSize = vec4(appContext->windowWidth, appContext->windowHeight, 0, 0);
				constantBuffer->mvp = projViewModelMat;
				constantBuffer->color = vec4(0, 0, 0, 1);
				constantBuffer->view = camViewMat;
				constantBuffer->proj = projMat;
				appContext->d3dDeviceContext->Unmap(appContext->mvpConstantBuffer, 0);
			}
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				appContext->d3dDeviceContext->Map(appContext->pointcloudConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
				ldiPointCloudConstantBuffer* constantBuffer = (ldiPointCloudConstantBuffer*)ms.pData;
				//constantBuffer->size = vec4(0.1f, 0.1f, 0, 0);
				//constantBuffer->size = vec4(16, 16, 1, 0);
				constantBuffer->size = vec4(appContext->pointWorldSize, appContext->pointScreenSize, appContext->pointScreenSpaceBlend, 0);
				appContext->d3dDeviceContext->Unmap(appContext->pointcloudConstantBuffer, 0);
			}

			gfxRenderPointCloud(appContext, &pointCloud);
		}

		//----------------------------------------------------------------------------------------------------
		// Build interface.
		//----------------------------------------------------------------------------------------------------
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// Viewport overlay.
		{
			mat4 viewRotMat = glm::rotate(mat4(1.0f), glm::radians(appContext->camera.rotation.y), vec3Right);
			viewRotMat = glm::rotate(viewRotMat, glm::radians(appContext->camera.rotation.x), vec3Up);
			mat4 camViewMat = viewRotMat * glm::translate(mat4(1.0f), -appContext->camera.position);

			mat4 projMat = glm::perspectiveFovRH_ZO(glm::radians(50.0f), (float)appContext->windowWidth, (float)appContext->windowHeight, 0.01f, 100.0f);
			mat4 invProjMat = inverse(projMat);
			mat4 modelMat = mat4(1.0f);

			mat4 projViewModelMat = projMat * camViewMat * modelMat;

			vec4 worldPos = vec4(1, 0, 0, 1);
			vec4 clipPos = projViewModelMat * worldPos;
			clipPos.x /= clipPos.w;
			clipPos.y /= clipPos.w;

			vec2 screenPos;			
			screenPos.x = (clipPos.x * 0.5 + 0.5) * appContext->windowWidth;
			screenPos.y = (1.0f - (clipPos.y * 0.5 + 0.5)) * appContext->windowHeight;

			const ImGuiViewport* viewport = ImGui::GetMainViewport();

			ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always, ImVec2(0, 0));
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::Begin("__viewport", NULL, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMouseInputs);

			ImDrawList* drawList = ImGui::GetWindowDrawList();

			if (clipPos.z > 0) {
				drawList->AddText(ImVec2(screenPos.x, screenPos.y), IM_COL32(255, 255, 255, 255), "X");
			}

			ImGui::End();
		}

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
				if (ImGui::MenuItem("ImGUI demo window", NULL, appContext->showDemoWindow)) {
					appContext->showDemoWindow = !appContext->showDemoWindow;
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

			ImGui::Separator();
			ImGui::Text("Primary model");
			ImGui::Checkbox("Shaded", &appContext->primaryModelShowShaded);
			ImGui::Checkbox("Wireframe", &appContext->primaryModelShowWireframe);

			ImGui::Separator();
			ImGui::Text("Point cloud");
			ImGui::SliderFloat("World size", &appContext->pointWorldSize, 0.0f, 1.0f);
			ImGui::SliderFloat("Screen size", &appContext->pointScreenSize, 0.0f, 32.0f);
			ImGui::SliderFloat("Screen blend", &appContext->pointScreenSpaceBlend, 0.0f, 1.0f);

			ImGui::Separator();
			ImGui::Text("Viewport");
			ImGui::ColorEdit3("Background", (float*)&appContext->viewBackgroundColor);
			ImGui::ColorEdit3("Grid", (float*)&appContext->gridColor);

			ImGui::End();
		}

		if (appContext->showDemoWindow) {
			ImGui::ShowDemoWindow(&appContext->showDemoWindow);
		}

		if (appContext->showPlatformWindow) {
			static float f = 0.0f;
			static int counter = 0;

			ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Once);
			ImGui::Begin("Platform controls", &appContext->showPlatformWindow);

			ImGui::Text("Connection");
			char ipBuff[] = "192.168.0.50";
			int port = 5000;
			ImGui::InputText("Address", ipBuff, sizeof(ipBuff));
			ImGui::InputInt("Port", &port);
			ImGui::Button("Connect", ImVec2(-1, 0));
			ImGui::Text("Status: Disconnected");

			ImGui::Separator();

			ImGui::BeginDisabled();
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

		{
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
		
					ImGui::Image(shaderResourceViewTest, ImVec2(512, 512), uv_min, uv_max, tint_col, border_col);
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

		_renderImgui(&_appContext);

		gfxEndPrimaryViewport(appContext);
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