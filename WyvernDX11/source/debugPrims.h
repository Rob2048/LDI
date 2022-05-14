//----------------------------------------------------------------------------------------------------
// Debug primitives.
//----------------------------------------------------------------------------------------------------
void initDebugPrimitives(ldiApp* AppContext) {
	{
		AppContext->triGeometryVertMax = 64000;

		D3D11_BUFFER_DESC vbDesc;
		ZeroMemory(&vbDesc, sizeof(vbDesc));
		vbDesc.Usage = D3D11_USAGE_DYNAMIC;
		vbDesc.ByteWidth = sizeof(ldiSimpleVertex) * AppContext->triGeometryVertMax;
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		AppContext->d3dDevice->CreateBuffer(&vbDesc, NULL, &AppContext->triGeometryVertexBuffer);
	}

	{
		AppContext->lineGeometryVertMax = 16000;

		D3D11_BUFFER_DESC vbDesc;
		ZeroMemory(&vbDesc, sizeof(vbDesc));
		vbDesc.Usage = D3D11_USAGE_DYNAMIC;
		vbDesc.ByteWidth = sizeof(ldiSimpleVertex) * AppContext->lineGeometryVertMax;
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		AppContext->d3dDevice->CreateBuffer(&vbDesc, NULL, &AppContext->lineGeometryVertexBuffer);
	}
}

void beginDebugPrimitives(ldiApp* AppContext) {
	AppContext->lineGeometryVertCount = 0;
	AppContext->lineGeometryVertData.clear();

	AppContext->triGeometryVertCount = 0;
	AppContext->triGeometryVertData.clear();
}

inline void pushDebugLine(ldiApp* AppContext, vec3 A, vec3 B, vec3 Color) {
	AppContext->lineGeometryVertCount += 2;
	AppContext->lineGeometryVertData.push_back({ A, Color });
	AppContext->lineGeometryVertData.push_back({ B, Color });
}

inline void pushDebugTri(ldiApp* AppContext, vec3 A, vec3 B, vec3 C, vec3 Color) {
	AppContext->triGeometryVertCount += 3;
	AppContext->triGeometryVertData.push_back({ A, Color });
	AppContext->triGeometryVertData.push_back({ B, Color });
	AppContext->triGeometryVertData.push_back({ C, Color });
}

inline void pushDebugBox(ldiApp* AppContext, vec3 Origin, vec3 Size, vec3 Color) {
	vec3 halfSize = Size * 0.5f;
	vec3 v0 = Origin + vec3(-halfSize.x, -halfSize.y, -halfSize.z);
	vec3 v1 = Origin + vec3(-halfSize.x, -halfSize.y, halfSize.z);
	vec3 v2 = Origin + vec3(halfSize.x, -halfSize.y, halfSize.z);
	vec3 v3 = Origin + vec3(halfSize.x, -halfSize.y, -halfSize.z);

	vec3 v4 = Origin + vec3(-halfSize.x, halfSize.y, -halfSize.z);
	vec3 v5 = Origin + vec3(-halfSize.x, halfSize.y, halfSize.z);
	vec3 v6 = Origin + vec3(halfSize.x, halfSize.y, halfSize.z);
	vec3 v7 = Origin + vec3(halfSize.x, halfSize.y, -halfSize.z);

	pushDebugLine(AppContext, v0, v1, Color);
	pushDebugLine(AppContext, v1, v2, Color);
	pushDebugLine(AppContext, v2, v3, Color);
	pushDebugLine(AppContext, v3, v0, Color);

	pushDebugLine(AppContext, v4, v5, Color);
	pushDebugLine(AppContext, v5, v6, Color);
	pushDebugLine(AppContext, v6, v7, Color);
	pushDebugLine(AppContext, v7, v4, Color);

	pushDebugLine(AppContext, v0, v4, Color);
	pushDebugLine(AppContext, v1, v5, Color);
	pushDebugLine(AppContext, v2, v6, Color);
	pushDebugLine(AppContext, v3, v7, Color);
}

inline void pushDebugBoxSolid(ldiApp* AppContext, vec3 Origin, vec3 Size, vec3 Color) {
	vec3 halfSize = Size * 0.5f;
	vec3 v0 = Origin + vec3(-halfSize.x, -halfSize.y, -halfSize.z);
	vec3 v1 = Origin + vec3(-halfSize.x, -halfSize.y, halfSize.z);
	vec3 v2 = Origin + vec3(halfSize.x, -halfSize.y, halfSize.z);
	vec3 v3 = Origin + vec3(halfSize.x, -halfSize.y, -halfSize.z);

	vec3 v4 = Origin + vec3(-halfSize.x, halfSize.y, -halfSize.z);
	vec3 v5 = Origin + vec3(-halfSize.x, halfSize.y, halfSize.z);
	vec3 v6 = Origin + vec3(halfSize.x, halfSize.y, halfSize.z);
	vec3 v7 = Origin + vec3(halfSize.x, halfSize.y, -halfSize.z);

	pushDebugTri(AppContext, v0, v1, v2, Color);
	pushDebugTri(AppContext, v0, v2, v3, Color);

	pushDebugTri(AppContext, v6, v5, v4, Color);
	pushDebugTri(AppContext, v7, v6, v4, Color);

	pushDebugTri(AppContext, v0, v4, v1, Color);
	pushDebugTri(AppContext, v4, v5, v1, Color);

	pushDebugTri(AppContext, v1, v5, v2, Color);
	pushDebugTri(AppContext, v5, v6, v2, Color);

	pushDebugTri(AppContext, v2, v6, v3, Color);
	pushDebugTri(AppContext, v6, v7, v3, Color);

	pushDebugTri(AppContext, v3, v7, v0, Color);
	pushDebugTri(AppContext, v7, v4, v0, Color);
}

void renderDebugPrimitives(ldiApp* AppContext) {
	const float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
	AppContext->d3dDeviceContext->OMSetBlendState(AppContext->alphaBlendState, blendFactor, 0xffffffff);
	AppContext->d3dDeviceContext->RSSetState(AppContext->defaultRasterizerState);

	//----------------------------------------------------------------------------------------------------
	// Render line geometry.
	//----------------------------------------------------------------------------------------------------
	if (AppContext->lineGeometryVertCount > 0) {
		D3D11_MAPPED_SUBRESOURCE ms;
		AppContext->d3dDeviceContext->Map(AppContext->lineGeometryVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		memcpy(ms.pData, AppContext->lineGeometryVertData.data(), sizeof(ldiSimpleVertex) * AppContext->lineGeometryVertCount);
		AppContext->d3dDeviceContext->Unmap(AppContext->lineGeometryVertexBuffer, 0);

		AppContext->d3dDeviceContext->VSSetShader(AppContext->simpleVertexShader, 0, 0);
		AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
		AppContext->d3dDeviceContext->PSSetShader(AppContext->simplePixelShader, 0, 0);
		AppContext->d3dDeviceContext->IASetInputLayout(AppContext->simpleInputLayout);

		UINT lgStride = sizeof(ldiSimpleVertex);
		UINT lgOffset = 0;
		AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &AppContext->lineGeometryVertexBuffer, &lgStride, &lgOffset);
		AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		AppContext->d3dDeviceContext->Draw(AppContext->lineGeometryVertCount, 0);
	}

	//----------------------------------------------------------------------------------------------------
	// Render tri geometry.
	//----------------------------------------------------------------------------------------------------
	if (AppContext->triGeometryVertCount > 0) {
		D3D11_MAPPED_SUBRESOURCE ms;
		AppContext->d3dDeviceContext->Map(AppContext->triGeometryVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		memcpy(ms.pData, AppContext->triGeometryVertData.data(), sizeof(ldiSimpleVertex) * AppContext->triGeometryVertCount);
		AppContext->d3dDeviceContext->Unmap(AppContext->triGeometryVertexBuffer, 0);

		AppContext->d3dDeviceContext->VSSetShader(AppContext->simpleVertexShader, 0, 0);
		AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
		AppContext->d3dDeviceContext->PSSetShader(AppContext->simplePixelShader, 0, 0);
		AppContext->d3dDeviceContext->IASetInputLayout(AppContext->simpleInputLayout);

		UINT lgStride = sizeof(ldiSimpleVertex);
		UINT lgOffset = 0;
		AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &AppContext->triGeometryVertexBuffer, &lgStride, &lgOffset);
		AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		AppContext->d3dDeviceContext->Draw(AppContext->triGeometryVertCount, 0);
	}
}
