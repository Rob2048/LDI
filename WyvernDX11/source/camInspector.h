#pragma once

struct ldiCamInspector {
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

int camInspectorInit(ldiApp* AppContext, ldiCamInspector* Tool) {
	Tool->appContext = AppContext;
	Tool->mainViewWidth = 0;
	Tool->mainViewHeight = 0;

	return 0;
}

void camInspectorRender(ldiCamInspector* Tool, int Width, int Height, std::vector<ldiTextInfo>* TextBuffer) {
	ldiApp* appContext = Tool->appContext;

	if (Width != Tool->mainViewWidth || Height != Tool->mainViewHeight) {
		Tool->mainViewWidth = Width;
		Tool->mainViewHeight = Height;
		gfxCreateRenderView(appContext, &Tool->renderViewBuffers, Width, Height);
	}

	//----------------------------------------------------------------------------------------------------
	// Debug primitives.
	//----------------------------------------------------------------------------------------------------
	beginDebugPrimitives(appContext);
	
	if (Tool->showGrid) {
		pushDebugLine(appContext, vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0));
		pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 1, 0));
		pushDebugLine(appContext, vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 0, 1));

		int gridCount = 10;
		float gridCellWidth = 1.0f;
		vec3 gridColor = Tool->gridColor;
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
	vec3 camPos = Tool->camOffset;
	mat4 camViewMat = glm::translate(mat4(1.0f), -camPos);

	float viewWidth = Tool->mainViewWidth * Tool->camScale;
	float viewHeight = Tool->mainViewHeight * Tool->camScale;
	mat4 projMat = glm::orthoRH_ZO(0.0f, viewWidth, viewHeight, 0.0f, 0.01f, 100.0f);
	//mat4 projMat = glm::perspectiveFovRH_ZO(glm::radians(50.0f), (float)SamplerTester->mainViewWidth, (float)SamplerTester->mainViewHeight, 0.01f, 100.0f);
	
	mat4 projViewModelMat = projMat * camViewMat;

	//----------------------------------------------------------------------------------------------------
	// Rendering.
	//----------------------------------------------------------------------------------------------------
	appContext->d3dDeviceContext->OMSetRenderTargets(1, &Tool->renderViewBuffers.mainViewRtView, Tool->renderViewBuffers.depthStencilView);

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Tool->mainViewWidth;
	viewport.Height = Tool->mainViewHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	appContext->d3dDeviceContext->RSSetViewports(1, &viewport);
	appContext->d3dDeviceContext->ClearRenderTargetView(Tool->renderViewBuffers.mainViewRtView, (float*)&Tool->viewBackgroundColor);
	appContext->d3dDeviceContext->ClearDepthStencilView(Tool->renderViewBuffers.depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

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
}