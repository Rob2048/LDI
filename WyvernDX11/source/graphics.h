//----------------------------------------------------------------------------------------------------
// DirectX11 rendering.
//----------------------------------------------------------------------------------------------------
struct ldiRenderModel {
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	int vertCount;
	int indexCount;
};

ldiRenderModel gfxCreateRenderModel(ldiApp* AppContext, ldiModel* ModelSource) {
	ldiRenderModel result = {};

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.ByteWidth = sizeof(ldiMeshVertex) * ModelSource->verts.size();
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = ModelSource->verts.data();

	AppContext->d3dDevice->CreateBuffer(&vbDesc, &vbData, &result.vertexBuffer);

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.ByteWidth = sizeof(uint32_t) * ModelSource->indices.size();
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = ModelSource->indices.data();
	
	AppContext->d3dDevice->CreateBuffer(&ibDesc, &ibData, &result.indexBuffer);

	result.vertCount = ModelSource->verts.size();
	result.indexCount = ModelSource->indices.size();

	return result;
}

void gfxRenderModel(ldiApp* AppContext, ldiRenderModel* Model) {
	UINT lgStride = sizeof(ldiMeshVertex);
	UINT lgOffset = 0;

	AppContext->d3dDeviceContext->IASetInputLayout(AppContext->meshInputLayout);
	AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &Model->vertexBuffer, &lgStride, &lgOffset);
	AppContext->d3dDeviceContext->IASetIndexBuffer(Model->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	AppContext->d3dDeviceContext->VSSetShader(AppContext->meshVertexShader, 0, 0);
	AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
	AppContext->d3dDeviceContext->PSSetShader(AppContext->meshPixelShader, 0, 0);
	AppContext->d3dDeviceContext->CSSetShader(NULL, NULL, 0);
	
	AppContext->d3dDeviceContext->OMSetBlendState(AppContext->defaultBlendState, NULL, 0xffffffff);
	AppContext->d3dDeviceContext->RSSetState(AppContext->defaultRasterizerState);
	AppContext->d3dDeviceContext->OMSetDepthStencilState(AppContext->defaultDepthStencilState, 0);

	AppContext->d3dDeviceContext->DrawIndexed(Model->indexCount, 0, 0);
}

void gfxCreatePrimaryBackbuffer(ldiApp* AppContext) {
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

void gfxCleanupPrimaryBackbuffer(ldiApp* AppContext) {
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

bool gfxCreateDeviceD3D(ldiApp* AppContext) {
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

	gfxCreatePrimaryBackbuffer(AppContext);

	return true;
}

void gfxCleanupDeviceD3D(ldiApp* AppContext) {
	gfxCleanupPrimaryBackbuffer(AppContext);
	if (AppContext->SwapChain) { AppContext->SwapChain->Release(); AppContext->SwapChain = NULL; }
	if (AppContext->d3dDeviceContext) { AppContext->d3dDeviceContext->Release(); AppContext->d3dDeviceContext = NULL; }
	if (AppContext->d3dDevice) { AppContext->d3dDevice->Release(); AppContext->d3dDevice = NULL; }
}

bool gfxCreateVertexShader(ldiApp* AppContext, LPCWSTR Filename, const char* EntryPoint, ID3D11VertexShader** Shader, D3D11_INPUT_ELEMENT_DESC* LayoutDesc, int LayoutElements, ID3D11InputLayout** InputLayout) {
	ID3DBlob* errorBlob;
	ID3DBlob* shaderBlob;

	if (FAILED(D3DCompileFromFile(Filename, NULL, NULL, EntryPoint, "vs_5_0", 0, 0, &shaderBlob, &errorBlob))) {
		std::cout << "Failed to compile shader\n";
		if (errorBlob != NULL) {
			std::cout << (const char*)errorBlob->GetBufferPointer() << "\n";
			errorBlob->Release();
		}
		return false;
	}

	if (AppContext->d3dDevice->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, Shader) != S_OK) {
		shaderBlob->Release();
		return false;
	}

	if (AppContext->d3dDevice->CreateInputLayout(LayoutDesc, LayoutElements, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), InputLayout) != S_OK) {
		shaderBlob->Release();
		return false;
	}

	shaderBlob->Release();

	return true;
}

bool gfxCreatePixelShader(ldiApp* AppContext, LPCWSTR Filename, const char* EntryPoint, ID3D11PixelShader** Shader) {
	ID3DBlob* errorBlob;
	ID3DBlob* shaderBlob;

	if (FAILED(D3DCompileFromFile(Filename, NULL, NULL, EntryPoint, "ps_5_0", 0, 0, &shaderBlob, &errorBlob))) {
		std::cout << "Failed to compile shader\n";
		if (errorBlob != NULL) {
			std::cout << (const char*)errorBlob->GetBufferPointer() << "\n";
			errorBlob->Release();
		}
		return false;
	}

	if (AppContext->d3dDevice->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, Shader) != S_OK) {
		shaderBlob->Release();
		return false;
	}

	shaderBlob->Release();

	return true;
}