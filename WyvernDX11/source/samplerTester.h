#pragma once

struct ldiSamplerTester {
	ldiApp*						appContext;

	int							mainViewWidth;
	int							mainViewHeight;

	ldiRenderViewBuffers		renderViewBuffers;

	vec4						viewBackgroundColor = { 0.2f, 0.23f, 0.26f, 1.00f };
	vec4						gridColor = { 0.3f, 0.33f, 0.36f, 1.00f };
	bool						showGrid = true;

	vec3						camOffset = { 0, 0, 1 };
	float						camScale = 0.01f;

	ldiRenderModel				samplePointModel;
};

int samplerTesterInit(ldiApp* AppContext, ldiSamplerTester* SamplerTester) {
	SamplerTester->appContext = AppContext;
	SamplerTester->mainViewWidth = 0;
	SamplerTester->mainViewHeight = 0;

	return 0;
}

void samplerTesterRender(ldiSamplerTester* SamplerTester, int Width, int Height, std::vector<ldiTextInfo>* TextBuffer) {
	ldiApp* appContext = SamplerTester->appContext;

	if (Width != SamplerTester->mainViewWidth || Height != SamplerTester->mainViewHeight) {
		SamplerTester->mainViewWidth = Width;
		SamplerTester->mainViewHeight = Height;
		gfxCreateRenderView(appContext, &SamplerTester->renderViewBuffers, Width, Height);
	}

	//----------------------------------------------------------------------------------------------------
	// Debug primitives.
	//----------------------------------------------------------------------------------------------------
	beginDebugPrimitives(appContext);
	
	if (SamplerTester->showGrid) {
		pushDebugLine(appContext, vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0));
		pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 1, 0));
		pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 0, 1));

		int gridCount = 10;
		float gridCellWidth = 1.0f;
		vec3 gridColor = SamplerTester->gridColor;
		vec3 gridHalfOffset = vec3(gridCellWidth * gridCount, gridCellWidth * gridCount, 0) * 0.5f;
		for (int i = 0; i < gridCount + 1; ++i) {
			pushDebugLine(appContext, vec3(i * gridCellWidth, 0, 0) - gridHalfOffset, vec3(i * gridCellWidth, gridCount * gridCellWidth, 0) - gridHalfOffset, gridColor);
			pushDebugLine(appContext, vec3(0, i * gridCellWidth, 0) - gridHalfOffset, vec3(gridCount * gridCellWidth, i * gridCellWidth, 0) - gridHalfOffset, gridColor);
		}
	}

	//----------------------------------------------------------------------------------------------------
	// Update camera.
	//----------------------------------------------------------------------------------------------------
	// TODO: Update inline with input for specific viewport, before rendering.
	vec3 camPos = SamplerTester->camOffset;
	mat4 camViewMat = glm::translate(mat4(1.0f), -camPos);

	float viewWidth = SamplerTester->mainViewWidth * SamplerTester->camScale;
	float viewHeight = SamplerTester->mainViewHeight * SamplerTester->camScale;
	mat4 projMat = glm::orthoRH_ZO(0.0f, viewWidth, viewHeight, 0.0f, 0.01f, 100.0f);
	//mat4 projMat = glm::perspectiveFovRH_ZO(glm::radians(50.0f), (float)SamplerTester->mainViewWidth, (float)SamplerTester->mainViewHeight, 0.01f, 100.0f);
	
	mat4 projViewModelMat = projMat * camViewMat;

	//----------------------------------------------------------------------------------------------------
	// Rendering.
	//----------------------------------------------------------------------------------------------------
	appContext->d3dDeviceContext->OMSetRenderTargets(1, &SamplerTester->renderViewBuffers.mainViewRtView, SamplerTester->renderViewBuffers.depthStencilView);

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = SamplerTester->mainViewWidth;
	viewport.Height = SamplerTester->mainViewHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	appContext->d3dDeviceContext->RSSetViewports(1, &viewport);
	appContext->d3dDeviceContext->ClearRenderTargetView(SamplerTester->renderViewBuffers.mainViewRtView, (float*)&SamplerTester->viewBackgroundColor);
	appContext->d3dDeviceContext->ClearDepthStencilView(SamplerTester->renderViewBuffers.depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

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
	// Render point samples.
	//----------------------------------------------------------------------------------------------------
	if (SamplerTester->samplePointModel.vertCount > 0) {
		UINT lgStride = sizeof(ldiMeshVertex);
		UINT lgOffset = 0;

		appContext->d3dDeviceContext->IASetInputLayout(appContext->basicInputLayout);
		appContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &SamplerTester->samplePointModel.vertexBuffer, &lgStride, &lgOffset);
		appContext->d3dDeviceContext->IASetIndexBuffer(SamplerTester->samplePointModel.indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		appContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		appContext->d3dDeviceContext->VSSetShader(appContext->basicVertexShader, 0, 0);
		appContext->d3dDeviceContext->VSSetConstantBuffers(0, 2, &appContext->mvpConstantBuffer);
		appContext->d3dDeviceContext->PSSetShader(appContext->basicPixelShader, 0, 0);
		appContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &appContext->mvpConstantBuffer);
		appContext->d3dDeviceContext->CSSetShader(NULL, NULL, 0);
		appContext->d3dDeviceContext->OMSetBlendState(appContext->defaultBlendState, NULL, 0xffffffff);
		appContext->d3dDeviceContext->RSSetState(appContext->defaultRasterizerState);
		appContext->d3dDeviceContext->OMSetDepthStencilState(appContext->defaultDepthStencilState, 0);

		appContext->d3dDeviceContext->DrawIndexed(SamplerTester->samplePointModel.indexCount, 0, 0);
	}
}

//---------------------------------------------------------------------------------------------------------------
// Image space helper functions.
//---------------------------------------------------------------------------------------------------------------
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

struct SamplePoint {
	vec2 pos;
	float value;
	float scale;
	float radius;
	int nextSampleId;
};

float GammaToLinear(float In) {
	float linearRGBLo = In / 12.92f;
	float linearRGBHi = pow(max(abs((In + 0.055f) / 1.055f), 1.192092896e-07f), 2.4f);

	return (In <= 0.04045) ? linearRGBLo : linearRGBHi;
}

float LinearToGamma(float In) {
	float sRGBLo = In * 12.92f;
	float sRGBHi = (pow(max(abs(In), 1.192092896e-07f), 1.0f / 2.4f) * 1.055f) - 0.055f;
	return (In <= 0.0031308f) ? sRGBLo : sRGBHi;
}


// Get intensity value from source texture at x,y, 0.0 to 1.0.
float GetSourceValue(float* Data, int Width, int Height, float X, float Y) {
	int pX = (int)(X * Width);
	int pY = (int)(Y * Height);
	int idx = pY * Width + pX;

	return Data[idx];
}

int clamp(int Value, int Min, int Max) {
	return min(max(Value, Min), Max);
}

void samplerTesterInitPointModel(ldiSamplerTester* SamplerTester, ldiRenderModel* Model, std::vector<SamplePoint>* SamplePoints) {
	ldiApp* appContext = SamplerTester->appContext;

	if (Model->vertexBuffer) {
		Model->vertexBuffer->Release();
		Model->vertexBuffer = NULL;
	}

	if (Model->indexBuffer) {
		Model->indexBuffer->Release();
		Model->indexBuffer = NULL;
	}

	// Create vert buffer.
	int sampleCount = SamplePoints->size();
	int quadCount = sampleCount;
	int triCount = quadCount * 2;
	int vertCount = quadCount * 4;

	ldiBasicVertex* verts = new ldiBasicVertex[vertCount];

	vec3 basicColor(0, 0, 0);

	for (int i = 0; i < quadCount; ++i) {
		SamplePoint* sample = &(*SamplePoints)[i];

		float hS = sample->scale * 0.5f;

		verts[i * 4 + 0].position = vec3(-hS + sample->pos.x, -hS + sample->pos.y, 0);
		verts[i * 4 + 0].color = basicColor;
		verts[i * 4 + 0].uv = vec2(0, 0);

		verts[i * 4 + 1].position = vec3(hS + sample->pos.x, -hS + sample->pos.y, 0);
		verts[i * 4 + 1].color = basicColor;
		verts[i * 4 + 1].uv = vec2(0, 0);

		verts[i * 4 + 2].position = vec3(hS + sample->pos.x, hS + sample->pos.y, 0);
		verts[i * 4 + 2].color = basicColor;
		verts[i * 4 + 2].uv = vec2(0, 0);

		verts[i * 4 + 3].position = vec3(-hS + sample->pos.x, hS + sample->pos.y, 0);
		verts[i * 4 + 3].color = basicColor;
		verts[i * 4 + 3].uv = vec2(0, 0);
	}

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.ByteWidth = sizeof(ldiBasicVertex) * vertCount;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = verts;

	appContext->d3dDevice->CreateBuffer(&vbDesc, &vbData, &Model->vertexBuffer);

	delete[] verts;

	// Create index buffer.
	int indexCount = triCount * 3;

	uint32_t* indices = new uint32_t[indexCount];

	for (int i = 0; i < quadCount; ++i) {
		indices[i * 6 + 0] = i * 4 + 0;
		indices[i * 6 + 1] = i * 4 + 1;
		indices[i * 6 + 2] = i * 4 + 2;

		indices[i * 6 + 3] = i * 4 + 0;
		indices[i * 6 + 4] = i * 4 + 2;
		indices[i * 6 + 5] = i * 4 + 3;
	}

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.ByteWidth = sizeof(uint32_t) * indexCount;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;

	appContext->d3dDevice->CreateBuffer(&ibDesc, &ibData, &Model->indexBuffer);

	delete[] indices;

	Model->vertCount = vertCount;
	Model->indexCount = indexCount;
}

void samplerTesterUpdatePointModel(ldiSamplerTester* SamplerTester, std::vector<SamplePoint>* SamplePoints) {

}

void samplerTesterRunTest(ldiSamplerTester* SamplerTester) {
	//---------------------------------------------------------------------------------------------------------------
	// Load image.
	//---------------------------------------------------------------------------------------------------------------
	double t0 = _getTime(SamplerTester->appContext);

	int sourceWidth, sourceHeight, sourceChannels;
	//uint8_t* sourcePixels = stbi_load("images\\dergn2.png", &sourceWidth, &sourceHeight, &sourceChannels, 0);
	uint8_t* sourcePixels = imageLoadRgba("../assets/images/imM.png", &sourceWidth, &sourceHeight, &sourceChannels);

	t0 = _getTime(SamplerTester->appContext) - t0;
	std::cout << "Image loaded: " << (t0 * 1000.0f) << " ms\n";

	std::cout << "Image stats: " << sourceWidth << ", " << sourceHeight << " " << sourceChannels << "\n";

	//---------------------------------------------------------------------------------------------------------------
	// Convert image to greyscale
	//---------------------------------------------------------------------------------------------------------------
	int sourceTotalPixels = sourceWidth * sourceHeight;

	float* sourceIntensity = new float[sourceTotalPixels];

	t0 = _getTime(SamplerTester->appContext);

	for (int i = 0; i < sourceTotalPixels; ++i) {
		uint8_t r = sourcePixels[i * sourceChannels + 0];
		//uint8_t g = sourcePixels[i * sourceChannels + 1];
		//uint8_t b = sourcePixels[i * sourceChannels + 2];

		float lum = r / 255.0f;// (r * 0.2126f + g * 0.7152f + b * 0.0722f) / 255.0f;
		//float lum = (r * 0.2126f + g * 0.7152f + b * 0.0722f) / 255.0f;

		sourceIntensity[i] = GammaToLinear(lum);
		//sourceIntensity[i] = lum;
	}

	t0 = _getTime(SamplerTester->appContext) - t0;
	std::cout << "Greyscale: " << (t0 * 1000.0f) << " ms\n";

	std::vector<SamplePoint> samplePoints;

	for (int i = 0; i < 200000; ++i) {
		SamplePoint point;
		point.scale = 0.01f;
		point.pos = vec2((rand() % 1000000) / 10000.0, (rand() % 1000000) / 10000.0);
		samplePoints.push_back(point);
	}

	samplerTesterInitPointModel(SamplerTester, &SamplerTester->samplePointModel, &samplePoints);
}