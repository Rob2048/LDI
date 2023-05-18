//----------------------------------------------------------------------------------------------------
// Debug primitives.
//----------------------------------------------------------------------------------------------------
void initDebugPrimitives(ldiDebugPrims* Prims) {
	Prims->lineGeometryVertData.clear();
	Prims->lineGeometryVertBufferSize = 0;
	Prims->lineGeometryVertCount = 0;

	if (Prims->lineGeometryVertexBuffer) {
		Prims->lineGeometryVertexBuffer->Release();
	}

	Prims->lineGeometryVertexBuffer = NULL;

	Prims->triGeometryVertData.clear();
	Prims->triGeometryVertBufferSize = 0;
	Prims->triGeometryVertCount = 0;
	
	if (Prims->triGeometryVertexBuffer) {
		Prims->triGeometryVertexBuffer->Release();
	}

	Prims->triGeometryVertexBuffer = NULL;
}

void _debugPrimitivesAllocateBuffers(ldiApp* AppContext, ldiDebugPrims* Prims) {
	if (Prims->lineGeometryVertData.size() > Prims->lineGeometryVertBufferSize) {
		if (Prims->lineGeometryVertexBuffer) {
			Prims->lineGeometryVertexBuffer->Release();
		}

		Prims->lineGeometryVertBufferSize = (int)Prims->lineGeometryVertData.size() + 10000;

		D3D11_BUFFER_DESC vbDesc;
		ZeroMemory(&vbDesc, sizeof(vbDesc));
		vbDesc.Usage = D3D11_USAGE_DYNAMIC;
		vbDesc.ByteWidth = sizeof(ldiSimpleVertex) * Prims->lineGeometryVertBufferSize;
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		AppContext->d3dDevice->CreateBuffer(&vbDesc, NULL, &Prims->lineGeometryVertexBuffer);
	}

	if (Prims->triGeometryVertData.size() > Prims->triGeometryVertBufferSize) {
		if (Prims->triGeometryVertexBuffer) {
			Prims->triGeometryVertexBuffer->Release();
		}

		Prims->triGeometryVertBufferSize = (int)Prims->triGeometryVertData.size() + 10000;

		D3D11_BUFFER_DESC vbDesc;
		ZeroMemory(&vbDesc, sizeof(vbDesc));
		vbDesc.Usage = D3D11_USAGE_DYNAMIC;
		vbDesc.ByteWidth = sizeof(ldiSimpleVertex) * Prims->triGeometryVertBufferSize;
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		AppContext->d3dDevice->CreateBuffer(&vbDesc, NULL, &Prims->triGeometryVertexBuffer);
	}
}

void beginDebugPrimitives(ldiDebugPrims* Prims) {
	Prims->lineGeometryVertCount = 0;
	Prims->lineGeometryVertData.clear();

	Prims->triGeometryVertCount = 0;
	Prims->triGeometryVertData.clear();
}

inline void pushDebugLine(ldiDebugPrims* Prims, vec3 A, vec3 B, vec3 Color) {
	Prims->lineGeometryVertData.push_back({ A, Color });
	Prims->lineGeometryVertData.push_back({ B, Color });
}

inline void pushDebugTri(ldiDebugPrims* Prims, vec3 A, vec3 B, vec3 C, vec3 Color) {
	Prims->triGeometryVertData.push_back({ A, Color });
	Prims->triGeometryVertData.push_back({ B, Color });
	Prims->triGeometryVertData.push_back({ C, Color });
}

inline void pushDebugQuad(ldiDebugPrims* Prims, vec3 Min, vec3 Max, vec3 Color) {
	vec3 v0(Min.x, Min.y, Min.z);
	vec3 v1(Max.x, Min.y, Min.z);
	vec3 v2(Max.x, Max.y, Min.z);
	vec3 v3(Min.x, Max.y, Min.z);

	Prims->triGeometryVertData.push_back({ v0, Color });
	Prims->triGeometryVertData.push_back({ v1, Color });
	Prims->triGeometryVertData.push_back({ v2, Color });

	Prims->triGeometryVertData.push_back({ v0, Color });
	Prims->triGeometryVertData.push_back({ v2, Color });
	Prims->triGeometryVertData.push_back({ v3, Color });
}

inline void pushDebugCirlcle(ldiDebugPrims* Prims, vec3 Origin, float Radius, vec3 Color, int Segs, vec3 XAxis, vec3 YAxis) {
	float divInc = glm::radians(360.0f / Segs);

	vec3 lastPoint = XAxis * (float)sin(0) * Radius + YAxis * (float)cos(0) * Radius + Origin;

	for (int i = 0; i < Segs; ++i) {
		float rad = divInc * (i + 1);
		vec3 newPoint = XAxis * (float)sin(rad) * Radius + YAxis * (float)cos(rad) * Radius + Origin;
		pushDebugLine(Prims, lastPoint, newPoint, Color);
		lastPoint = newPoint;
	}
}

inline void pushDebugCircleHorz(ldiDebugPrims* Prims, vec3 Origin, float Radius, vec3 Color, int Segs) {
	pushDebugCirlcle(Prims, Origin, Radius, Color, Segs, vec3Right, vec3Forward);
}

inline void pushDebugCircleVert(ldiDebugPrims* Prims, vec3 Origin, float Radius, vec3 Color, int Segs) {
	pushDebugCirlcle(Prims, Origin, Radius, Color, Segs, vec3Right, vec3Up);
}

inline void pushDebugElipse(ldiDebugPrims* Prims, vec3 Origin, float Width, float Height, vec3 Color, int Segs, vec3 XAxis, vec3 YAxis) {
	float divInc = glm::radians(360.0f / Segs);

	vec3 lastPoint;

	for (int i = 0; i < Segs + 1; ++i) {
		float rad = divInc * i;
		vec3 newPoint = XAxis * (float)sin(rad) * Width + YAxis * (float)cos(rad) * Height + Origin;

		if (i > 0) {
			pushDebugLine(Prims, lastPoint, newPoint, Color);
		}

		lastPoint = newPoint;
	}
}

inline void pushDebugSphere(ldiDebugPrims* Prims, vec3 Origin, float Radius, vec3 Color, int Segs) {
	pushDebugCirlcle(Prims, Origin, Radius, Color, Segs, vec3Right, vec3Up);
	pushDebugCirlcle(Prims, Origin, Radius, Color, Segs, vec3Forward, vec3Up);
	pushDebugCirlcle(Prims, Origin, Radius, Color, Segs, vec3Right, vec3Forward);
}

inline void pushDebugPlane(ldiDebugPrims* Prims, vec3 Origin, vec3 Normal, float Size, vec3 Color) {
	vec3 up = vec3Up;

	if (up == Normal) {
		up = vec3Right;
	}

	vec3 side = glm::cross(Normal, up);
	side = glm::normalize(side);
	up = glm::cross(Normal, side);
	up = glm::normalize(up);

	//pushDebugSphere(Prims, Origin, 0.01f, vec3(1, 0, 1), 6);

	pushDebugLine(Prims, Origin, Origin + up, vec3(1, 0, 0));
	pushDebugLine(Prims, Origin, Origin + side, vec3(0, 1, 0));
	pushDebugLine(Prims, Origin, Origin + Normal, vec3(0, 0, 1));

	float hSize = Size * 0.5f;

	vec3 v0 = -side * hSize - up * hSize + Origin;
	vec3 v1 = side * hSize - up * hSize + Origin;
	vec3 v2 = side * hSize + up * hSize + Origin;
	vec3 v3 = -side * hSize + up * hSize + Origin;

	pushDebugLine(Prims, v0, v1, Color);
	pushDebugLine(Prims, v1, v2, Color);
	pushDebugLine(Prims, v2, v3, Color);
	pushDebugLine(Prims, v3, v0, Color);
}

inline void pushDebugBox(ldiDebugPrims* Prims, vec3 Origin, vec3 Size, vec3 Color) {
	vec3 halfSize = Size * 0.5f;
	vec3 v0 = Origin + vec3(-halfSize.x, -halfSize.y, -halfSize.z);
	vec3 v1 = Origin + vec3(-halfSize.x, -halfSize.y, halfSize.z);
	vec3 v2 = Origin + vec3(halfSize.x, -halfSize.y, halfSize.z);
	vec3 v3 = Origin + vec3(halfSize.x, -halfSize.y, -halfSize.z);

	vec3 v4 = Origin + vec3(-halfSize.x, halfSize.y, -halfSize.z);
	vec3 v5 = Origin + vec3(-halfSize.x, halfSize.y, halfSize.z);
	vec3 v6 = Origin + vec3(halfSize.x, halfSize.y, halfSize.z);
	vec3 v7 = Origin + vec3(halfSize.x, halfSize.y, -halfSize.z);

	pushDebugLine(Prims, v0, v1, Color);
	pushDebugLine(Prims, v1, v2, Color);
	pushDebugLine(Prims, v2, v3, Color);
	pushDebugLine(Prims, v3, v0, Color);

	pushDebugLine(Prims, v4, v5, Color);
	pushDebugLine(Prims, v5, v6, Color);
	pushDebugLine(Prims, v6, v7, Color);
	pushDebugLine(Prims, v7, v4, Color);

	pushDebugLine(Prims, v0, v4, Color);
	pushDebugLine(Prims, v1, v5, Color);
	pushDebugLine(Prims, v2, v6, Color);
	pushDebugLine(Prims, v3, v7, Color);
}

inline void pushDebugBoxMinMax(ldiDebugPrims* Prims, vec3 Min, vec3 Max, vec3 Color) {
	vec3 v0 = vec3(Min.x, Min.y, Min.z);
	vec3 v1 = vec3(Min.x, Min.y, Max.z);
	vec3 v2 = vec3(Max.x, Min.y, Max.z);
	vec3 v3 = vec3(Max.x, Min.y, Min.z);

	vec3 v4 = vec3(Min.x, Max.y, Min.z);
	vec3 v5 = vec3(Min.x, Max.y, Max.z);
	vec3 v6 = vec3(Max.x, Max.y, Max.z);
	vec3 v7 = vec3(Max.x, Max.y, Min.z);

	pushDebugLine(Prims, v0, v1, Color);
	pushDebugLine(Prims, v1, v2, Color);
	pushDebugLine(Prims, v2, v3, Color);
	pushDebugLine(Prims, v3, v0, Color);

	pushDebugLine(Prims, v4, v5, Color);
	pushDebugLine(Prims, v5, v6, Color);
	pushDebugLine(Prims, v6, v7, Color);
	pushDebugLine(Prims, v7, v4, Color);

	pushDebugLine(Prims, v0, v4, Color);
	pushDebugLine(Prims, v1, v5, Color);
	pushDebugLine(Prims, v2, v6, Color);
	pushDebugLine(Prims, v3, v7, Color);
}

inline void pushDebugBoxSolid(ldiDebugPrims* Prims, vec3 Origin, vec3 Size, vec3 Color) {
	vec3 halfSize = Size * 0.5f;
	vec3 v0 = Origin + vec3(-halfSize.x, -halfSize.y, -halfSize.z);
	vec3 v1 = Origin + vec3(-halfSize.x, -halfSize.y, halfSize.z);
	vec3 v2 = Origin + vec3(halfSize.x, -halfSize.y, halfSize.z);
	vec3 v3 = Origin + vec3(halfSize.x, -halfSize.y, -halfSize.z);

	vec3 v4 = Origin + vec3(-halfSize.x, halfSize.y, -halfSize.z);
	vec3 v5 = Origin + vec3(-halfSize.x, halfSize.y, halfSize.z);
	vec3 v6 = Origin + vec3(halfSize.x, halfSize.y, halfSize.z);
	vec3 v7 = Origin + vec3(halfSize.x, halfSize.y, -halfSize.z);

	pushDebugTri(Prims, v0, v1, v2, Color);
	pushDebugTri(Prims, v0, v2, v3, Color);

	pushDebugTri(Prims, v6, v5, v4, Color);
	pushDebugTri(Prims, v7, v6, v4, Color);

	pushDebugTri(Prims, v0, v4, v1, Color);
	pushDebugTri(Prims, v4, v5, v1, Color);

	pushDebugTri(Prims, v1, v5, v2, Color);
	pushDebugTri(Prims, v5, v6, v2, Color);

	pushDebugTri(Prims, v2, v6, v3, Color);
	pushDebugTri(Prims, v6, v7, v3, Color);

	pushDebugTri(Prims, v3, v7, v0, Color);
	pushDebugTri(Prims, v7, v4, v0, Color);
}

void renderDebugPrimitives(ldiApp* AppContext, ldiDebugPrims* Prims, bool Depth = true) {
	Prims->lineGeometryVertCount = (int)Prims->lineGeometryVertData.size();
	Prims->triGeometryVertCount = (int)Prims->triGeometryVertData.size();

	_debugPrimitivesAllocateBuffers(AppContext, Prims);

	const float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };

	if (Depth) {
		AppContext->d3dDeviceContext->OMSetDepthStencilState(AppContext->defaultDepthStencilState, 0);
	} else {
		AppContext->d3dDeviceContext->OMSetDepthStencilState(AppContext->noDepthState, 0);
	}

	AppContext->d3dDeviceContext->OMSetBlendState(AppContext->alphaBlendState, blendFactor, 0xffffffff);
	AppContext->d3dDeviceContext->RSSetState(AppContext->defaultRasterizerState);

	//----------------------------------------------------------------------------------------------------
	// Render line geometry.
	//----------------------------------------------------------------------------------------------------
	if (Prims->lineGeometryVertCount > 0) {
		D3D11_MAPPED_SUBRESOURCE ms;
		AppContext->d3dDeviceContext->Map(Prims->lineGeometryVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		memcpy(ms.pData, Prims->lineGeometryVertData.data(), sizeof(ldiSimpleVertex) * Prims->lineGeometryVertCount);
		AppContext->d3dDeviceContext->Unmap(Prims->lineGeometryVertexBuffer, 0);

		UINT lgStride = sizeof(ldiSimpleVertex);
		UINT lgOffset = 0;

		AppContext->d3dDeviceContext->IASetInputLayout(AppContext->simpleInputLayout);
		AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &Prims->lineGeometryVertexBuffer, &lgStride, &lgOffset);
		AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		AppContext->d3dDeviceContext->VSSetShader(AppContext->simpleVertexShader, 0, 0);
		AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
		AppContext->d3dDeviceContext->PSSetShader(AppContext->simplePixelShader, 0, 0);
		AppContext->d3dDeviceContext->Draw(Prims->lineGeometryVertCount, 0);
	}

	//----------------------------------------------------------------------------------------------------
	// Render tri geometry.
	//----------------------------------------------------------------------------------------------------
	if (Prims->triGeometryVertCount > 0) {
		D3D11_MAPPED_SUBRESOURCE ms;
		AppContext->d3dDeviceContext->Map(Prims->triGeometryVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		memcpy(ms.pData, Prims->triGeometryVertData.data(), sizeof(ldiSimpleVertex) * Prims->triGeometryVertCount);
		AppContext->d3dDeviceContext->Unmap(Prims->triGeometryVertexBuffer, 0);

		UINT lgStride = sizeof(ldiSimpleVertex);
		UINT lgOffset = 0;

		AppContext->d3dDeviceContext->IASetInputLayout(AppContext->simpleInputLayout);
		AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &Prims->triGeometryVertexBuffer, &lgStride, &lgOffset);
		AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		AppContext->d3dDeviceContext->VSSetShader(AppContext->simpleVertexShader, 0, 0);
		AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
		AppContext->d3dDeviceContext->PSSetShader(AppContext->simplePixelShader, 0, 0);
		AppContext->d3dDeviceContext->Draw(Prims->triGeometryVertCount, 0);
	}
}
