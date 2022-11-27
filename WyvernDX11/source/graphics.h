//----------------------------------------------------------------------------------------------------
// DirectX11 rendering.
//----------------------------------------------------------------------------------------------------

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

struct ldiRenderViewBuffers {
	ID3D11Texture2D*			mainViewTexture;
	ID3D11RenderTargetView*		mainViewRtView;
	ID3D11ShaderResourceView*	mainViewResourceView;
	ID3D11DepthStencilView*		depthStencilView;
	ID3D11Texture2D*			depthStencilBuffer;
};

struct ldiRenderModel {
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	int vertCount;
	int indexCount;
};

struct ldiRenderPointCloud {
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	int vertCount;
	int indexCount;
};

struct ldiRenderLines {
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	int vertCount;
	int indexCount;
};

ldiRenderPointCloud gfxCreateRenderPointCloud(ldiApp* AppContext, ldiPointCloud* PointCloud) {
	ldiRenderPointCloud result = {};

	// Pack point data into verts.
	int pointCount = (int)PointCloud->points.size();
	int vertCount = pointCount * 4;
	std::vector<ldiBasicVertex> packedVerts;
	packedVerts.resize(vertCount);

	for (int i = 0; i < pointCount; ++i) {
		ldiPointCloudVertex s = PointCloud->points[i];

		packedVerts[i * 4 + 0].position = s.position;
		packedVerts[i * 4 + 0].uv = vec2(-0.5f, -0.5f);
		packedVerts[i * 4 + 0].color = s.color;
		
		packedVerts[i * 4 + 1].position = s.position;
		packedVerts[i * 4 + 1].uv = vec2(0.5f, -0.5f);
		packedVerts[i * 4 + 1].color = s.color;

		packedVerts[i * 4 + 2].position = s.position;
		packedVerts[i * 4 + 2].uv = vec2(0.5f, 0.5f);
		packedVerts[i * 4 + 2].color = s.color;

		packedVerts[i * 4 + 3].position = s.position;
		packedVerts[i * 4 + 3].uv = vec2(-0.5f, 0.5f);
		packedVerts[i * 4 + 3].color = s.color;
	}

	int indexCount = pointCount * 6;
	std::vector<uint32_t> indices;
	indices.resize(indexCount);

	for (int i = 0; i < pointCount; ++i) {
		indices[i * 6 + 0] = i * 4 + 2;
		indices[i * 6 + 1] = i * 4 + 1;
		indices[i * 6 + 2] = i * 4 + 0;

		indices[i * 6 + 3] = i * 4 + 0;
		indices[i * 6 + 4] = i * 4 + 3;
		indices[i * 6 + 5] = i * 4 + 2;
	}

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.ByteWidth = sizeof(ldiBasicVertex) * vertCount;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = packedVerts.data();

	AppContext->d3dDevice->CreateBuffer(&vbDesc, &vbData, &result.vertexBuffer);

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.ByteWidth = sizeof(uint32_t) * indexCount;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices.data();

	AppContext->d3dDevice->CreateBuffer(&ibDesc, &ibData, &result.indexBuffer);

	result.vertCount = vertCount;
	result.indexCount = indexCount;

	return result;
}

void gfxRenderPointCloud(ldiApp* AppContext, ldiRenderPointCloud* PointCloud) {
	UINT lgStride = sizeof(ldiMeshVertex);
	UINT lgOffset = 0;

	AppContext->d3dDeviceContext->IASetInputLayout(AppContext->pointCloudInputLayout);
	AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &PointCloud->vertexBuffer, &lgStride, &lgOffset);
	AppContext->d3dDeviceContext->IASetIndexBuffer(PointCloud->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	AppContext->d3dDeviceContext->VSSetShader(AppContext->pointCloudVertexShader, 0, 0);
	ID3D11Buffer* constantBuffers[] = { AppContext->mvpConstantBuffer, AppContext->pointcloudConstantBuffer };
	AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 2, constantBuffers);
	AppContext->d3dDeviceContext->PSSetShader(AppContext->pointCloudPixelShader, 0, 0);
	AppContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
	AppContext->d3dDeviceContext->CSSetShader(NULL, NULL, 0);
	AppContext->d3dDeviceContext->OMSetBlendState(AppContext->defaultBlendState, NULL, 0xffffffff);
	AppContext->d3dDeviceContext->RSSetState(AppContext->defaultRasterizerState);
	AppContext->d3dDeviceContext->OMSetDepthStencilState(AppContext->defaultDepthStencilState, 0);

	AppContext->d3dDeviceContext->DrawIndexed(PointCloud->indexCount, 0, 0);
}

ldiRenderLines gfxCreateRenderQuadWireframe(ldiApp* AppContext, ldiQuadModel* ModelSource) {
	ldiRenderLines result = {};

	std::vector<ldiSimpleVertex> verts;
	verts.resize(ModelSource->verts.size());
	for (int i = 0; i < verts.size(); ++i) {
		verts[i].position = ModelSource->verts[i];
		verts[i].color = vec3(0.1, 0.1, 0.1);
	}

	int quadCount = (int)(ModelSource->indices.size() / 4);
	std::vector<uint32_t> indices;
	indices.resize(quadCount * 8);
	for (int i = 0; i < quadCount; ++i) {
		int v0 = ModelSource->indices[i * 4 + 0];
		int v1 = ModelSource->indices[i * 4 + 1];
		int v2 = ModelSource->indices[i * 4 + 2];
		int v3 = ModelSource->indices[i * 4 + 3];

		indices[i * 8 + 0] = v0;
		indices[i * 8 + 1] = v1;

		indices[i * 8 + 2] = v1;
		indices[i * 8 + 3] = v2;

		indices[i * 8 + 4] = v2;
		indices[i * 8 + 5] = v3;

		indices[i * 8 + 6] = v3;
		indices[i * 8 + 7] = v0;
	}

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.ByteWidth = (UINT)(sizeof(ldiSimpleVertex) * verts.size());
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = verts.data();

	AppContext->d3dDevice->CreateBuffer(&vbDesc, &vbData, &result.vertexBuffer);

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.ByteWidth = (UINT)(sizeof(uint32_t) * indices.size());
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices.data();

	AppContext->d3dDevice->CreateBuffer(&ibDesc, &ibData, &result.indexBuffer);

	result.vertCount = (int)verts.size();
	result.indexCount = (int)indices.size();

	return result;
}

void gfxReleaseSurfelRenderModel(ldiRenderModel* Model) {
	if (Model->indexBuffer != 0) {
		Model->indexBuffer->Release();
		Model->indexBuffer = 0;
	}

	if (Model->vertexBuffer != 0) {
		Model->vertexBuffer->Release();
		Model->vertexBuffer = 0;
	}
}

ldiRenderModel gfxCreateSurfelRenderModel(ldiApp* AppContext, std::vector<ldiSurfel>* Surfels) {
	ldiRenderModel result = {};

	int quadCount = (int)Surfels->size();
	int vertCount = quadCount * 4;
	int triCount = quadCount * 2;
	int indexCount = triCount * 3;

	ldiBasicVertex* verts = new ldiBasicVertex[vertCount];
	uint32_t* indices = new uint32_t[indexCount];

	float surfelSize = 0.0075f;
	float hSize = surfelSize / 2.0f;
	//float hSize = surfelSize * 4.0f;
	float normalAdjust = 0.001f;
	
	for (int i = 0; i < quadCount; ++i) {
		ldiSurfel* s = &(*Surfels)[i];

		vec3 upVec(0, 1, 0);

		if (s->normal == upVec || s->normal == -upVec) {
			upVec = vec3(1, 0, 0);
		}

		vec3 tangent = glm::cross(s->normal, upVec);
		tangent = glm::normalize(tangent);

		vec3 bitangent = glm::cross(s->normal, tangent);
		bitangent = glm::normalize(bitangent);

		//s->position += s->normal * 20.0f;

		vec3 p0 = s->position - tangent * hSize - bitangent * hSize + s->normal * normalAdjust;
		vec3 p1 = s->position + tangent * hSize - bitangent * hSize + s->normal * normalAdjust;
		vec3 p2 = s->position + tangent * hSize + bitangent * hSize + s->normal * normalAdjust;
		vec3 p3 = s->position - tangent * hSize + bitangent * hSize + s->normal * normalAdjust;

		ldiBasicVertex* v0 = &verts[i * 4 + 0];
		ldiBasicVertex* v1 = &verts[i * 4 + 1];
		ldiBasicVertex* v2 = &verts[i * 4 + 2];
		ldiBasicVertex* v3 = &verts[i * 4 + 3];

		//vec3 color = s->normal * 0.5f + 0.5f;
		vec3 color = s->color;

		v0->position = p0;
		v0->color = color;
		v0->uv = vec2(0, 0);

		v1->position = p1;
		v1->color = color;
		v1->uv = vec2(1, 0);

		v2->position = p2;
		v2->color = color;
		v2->uv = vec2(1, 1);

		v3->position = p3;
		v3->color = color;
		v3->uv = vec2(0, 1);

		indices[i * 6 + 0] = i * 4 + 2;
		indices[i * 6 + 1] = i * 4 + 1;
		indices[i * 6 + 2] = i * 4 + 0;
		indices[i * 6 + 3] = i * 4 + 0;
		indices[i * 6 + 4] = i * 4 + 3;
		indices[i * 6 + 5] = i * 4 + 2;
	}

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.ByteWidth = sizeof(ldiBasicVertex) * vertCount;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = verts;

	AppContext->d3dDevice->CreateBuffer(&vbDesc, &vbData, &result.vertexBuffer);

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.ByteWidth = sizeof(uint32_t) * indexCount;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;

	AppContext->d3dDevice->CreateBuffer(&ibDesc, &ibData, &result.indexBuffer);

	result.vertCount = vertCount;
	result.indexCount = indexCount;

	delete[] verts;
	delete[] indices;

	return result;
}

/*
ldiRenderModel gfxCreateCoveragePointRenderModel(ldiApp* AppContext, std::vector<ldiSurfel>* Surfels) {
	ldiRenderModel result = {};

	int quadCount = (int)Surfels->size();
	int vertCount = quadCount * 4;
	int triCount = quadCount * 2;
	int indexCount = triCount * 3;

	ldiCoveragePointVertex* verts = new ldiCoveragePointVertex[vertCount];
	uint32_t* indices = new uint32_t[indexCount];

	float surfelSize = 0.0075f;
	float hSize = surfelSize / 2.0f;
	//float hSize = surfelSize * 4.0f;
	float normalAdjust = 0.001f;

	for (int i = 0; i < quadCount; ++i) {
		ldiSurfel* s = &(*Surfels)[i];

		vec3 upVec(0, 1, 0);

		if (s->normal == upVec || s->normal == -upVec) {
			upVec = vec3(1, 0, 0);
		}

		vec3 tangent = glm::cross(s->normal, upVec);
		tangent = glm::normalize(tangent);

		vec3 bitangent = glm::cross(s->normal, tangent);
		bitangent = glm::normalize(bitangent);

		vec3 p0 = s->position - tangent * hSize - bitangent * hSize + s->normal * normalAdjust;
		vec3 p1 = s->position + tangent * hSize - bitangent * hSize + s->normal * normalAdjust;
		vec3 p2 = s->position + tangent * hSize + bitangent * hSize + s->normal * normalAdjust;
		vec3 p3 = s->position - tangent * hSize + bitangent * hSize + s->normal * normalAdjust;

		ldiCoveragePointVertex* v0 = &verts[i * 4 + 0];
		ldiCoveragePointVertex* v1 = &verts[i * 4 + 1];
		ldiCoveragePointVertex* v2 = &verts[i * 4 + 2];
		ldiCoveragePointVertex* v3 = &verts[i * 4 + 3];

		v0->position = p0;
		v0->id = i;
		v0->normal = s->normal;

		v1->position = p1;
		v1->id = i;
		v1->normal = s->normal;
		
		v2->position = p2;
		v2->id = i;
		v2->normal = s->normal;
		
		v3->position = p3;
		v3->id = i;
		v3->normal = s->normal;
		
		indices[i * 6 + 0] = i * 4 + 2;
		indices[i * 6 + 1] = i * 4 + 1;
		indices[i * 6 + 2] = i * 4 + 0;
		indices[i * 6 + 3] = i * 4 + 0;
		indices[i * 6 + 4] = i * 4 + 3;
		indices[i * 6 + 5] = i * 4 + 2;
	}

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.ByteWidth = sizeof(ldiCoveragePointVertex) * vertCount;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = verts;

	AppContext->d3dDevice->CreateBuffer(&vbDesc, &vbData, &result.vertexBuffer);

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.ByteWidth = sizeof(uint32_t) * indexCount;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;

	AppContext->d3dDevice->CreateBuffer(&ibDesc, &ibData, &result.indexBuffer);

	result.vertCount = vertCount;
	result.indexCount = indexCount;

	delete[] verts;
	delete[] indices;

	return result;
}
*/

ldiRenderModel gfxCreateCoveragePointRenderModel(ldiApp* AppContext, std::vector<ldiSurfel>* Surfels, ldiQuadModel* ModelSource) {
	ldiRenderModel result = {};

	// Unique quads with unique verts per quad.

	int quadCount = (int)(ModelSource->indices.size() / 4);
	int vertCount = quadCount * 4;
	int triCount = quadCount * 2;
	int indexCount = triCount * 3;

	ldiCoveragePointVertex* verts = new ldiCoveragePointVertex[vertCount];
	uint32_t* indices = new uint32_t[indexCount];

	for (int i = 0; i < quadCount; ++i) {
		vec3 p0 = ModelSource->verts[ModelSource->indices[i * 4 + 0]];
		vec3 p1 = ModelSource->verts[ModelSource->indices[i * 4 + 1]];
		vec3 p2 = ModelSource->verts[ModelSource->indices[i * 4 + 2]];
		vec3 p3 = ModelSource->verts[ModelSource->indices[i * 4 + 3]];

		float s0 = glm::length(p0 - p1);
		float s1 = glm::length(p1 - p2);
		float s2 = glm::length(p2 - p3);
		float s3 = glm::length(p3 - p0);

		// NOTE: Calculate normal.
		vec3 normal = (*Surfels)[i].normal;

		ldiCoveragePointVertex* v0 = &verts[i * 4 + 0];
		ldiCoveragePointVertex* v1 = &verts[i * 4 + 1];
		ldiCoveragePointVertex* v2 = &verts[i * 4 + 2];
		ldiCoveragePointVertex* v3 = &verts[i * 4 + 3];

		v0->position = p0;
		v0->id = i;
		v0->normal = normal;

		v1->position = p1;
		v1->id = i;
		v1->normal = normal;

		v2->position = p2;
		v2->id = i;
		v2->normal = normal;

		v3->position = p3;
		v3->id = i;
		v3->normal = normal;

		indices[i * 6 + 0] = i * 4 + 2;
		indices[i * 6 + 1] = i * 4 + 1;
		indices[i * 6 + 2] = i * 4 + 0;
		indices[i * 6 + 3] = i * 4 + 0;
		indices[i * 6 + 4] = i * 4 + 3;
		indices[i * 6 + 5] = i * 4 + 2;
	}

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.ByteWidth = sizeof(ldiCoveragePointVertex) * vertCount;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = verts;

	AppContext->d3dDevice->CreateBuffer(&vbDesc, &vbData, &result.vertexBuffer);

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.ByteWidth = sizeof(uint32_t) * indexCount;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;

	AppContext->d3dDevice->CreateBuffer(&ibDesc, &ibData, &result.indexBuffer);

	result.vertCount = vertCount;
	result.indexCount = indexCount;

	delete[] verts;
	delete[] indices;

	return result;
}

ldiRenderModel gfxCreateRenderQuadModelDebug(ldiApp* AppContext, ldiQuadModel* ModelSource) {
	ldiRenderModel result = {};

	// Unique quads with unique verts per quad.

	int quadCount = (int)(ModelSource->indices.size() / 4);
	int vertCount = quadCount * 4;
	int triCount = quadCount * 2;
	int indexCount = triCount * 3;

	ldiSimpleVertex* verts = new ldiSimpleVertex[vertCount];
	uint32_t* indices = new uint32_t[indexCount];

	double totalArea = 0.0;

	const double dotSize = 0.0075;
	
	for (int i = 0; i < quadCount; ++i) {
		vec3 p0 = ModelSource->verts[ModelSource->indices[i * 4 + 0]];
		vec3 p1 = ModelSource->verts[ModelSource->indices[i * 4 + 1]];
		vec3 p2 = ModelSource->verts[ModelSource->indices[i * 4 + 2]];
		vec3 p3 = ModelSource->verts[ModelSource->indices[i * 4 + 3]];

		float s0 = glm::length(p0 - p1);
		float s1 = glm::length(p1 - p2);
		float s2 = glm::length(p2 - p3);
		float s3 = glm::length(p3 - p0);

		// Find biggest error;
		float error = 0.0f;
		float target = (float)dotSize;

		/*error = max(error, abs(s0 - target));
		error = max(error, abs(s1 - target));
		error = max(error, abs(s2 - target));
		error = max(error, abs(s3 - target));*/

		double area = (s0 * s1) * 0.5 + (s2 * s3) * 0.5;

		totalArea += area;

		error = area - (target * target);
		//float colErr = 0.5 + (error * 3000.0);
		float colErr = 0.75 + (error * 4000.0);
		vec3 color(colErr, colErr, colErr);

		// NOTE: Calculate normal.
		vec3 normal = glm::normalize(glm::cross(p1 - p0, p3 - p0));

		color.x = normal.x * 0.5 + 0.5;
		color.y = normal.y * 0.5 + 0.5;
		color.z = normal.z * 0.5 + 0.5;

		vec3 centerPoint = vec3(0.0f, 7.5f, 0.0f);
		float maxDist = 7.5f;
		float dist0 = glm::length(p0 - centerPoint);
		float dist1 = glm::length(p1 - centerPoint);
		float dist2 = glm::length(p2 - centerPoint);
		float dist3 = glm::length(p3 - centerPoint);

		if (dist0 > maxDist || dist1 > maxDist || dist2 > maxDist || dist3 > maxDist) {
			color.x = 1.0f;
			color.y = 0;
			color.z = 0;
		}

		//color.x *= colErr;
		//color.y *= colErr;
		//color.z *= colErr;

		// NOTE: Unique color for each quad.		
		/*int scaledIdx = i * (0xFFFFFF / quadCount);
		color.x = ((scaledIdx >> 0) & 0xFF) / 255.0f;
		color.y = ((scaledIdx >> 8) & 0xFF) / 255.0f;
		color.z = ((scaledIdx >> 16) & 0xFF) / 255.0f;*/

		// NOTE: Colormap for quad ID.
		static vec3 colorMap[] = {
			{0, 0, 0},
			
			{0, 0, 1},
			{1, 0, 1},
			{1, 0, 0},
			{1, 1, 0},
			{0, 1, 0},
			{0, 1, 1},
			
			/*{0, 0, 1},
			{1, 0, 1},
			{1, 0, 0},
			{1, 1, 0},
			{0, 1, 0},
			{0, 1, 1},

			{0, 0, 1},
			{1, 0, 1},
			{1, 0, 0},
			{1, 1, 0},
			{0, 1, 0},
			{0, 1, 1},

			{0, 0, 1},
			{1, 0, 1},
			{1, 0, 0},
			{1, 1, 0},
			{0, 1, 0},
			{0, 1, 1},*/

			{1, 1, 1},
		};

		int colorMapEntries = (sizeof(colorMap) / sizeof(vec3)) - 1;
		float idNorm = ((float)i / (float)quadCount) * colorMapEntries;
		int colorMapSection = idNorm;
		float colorMapT = idNorm - colorMapSection;

		vec3 cA = colorMap[colorMapSection + 0];
		vec3 cB = colorMap[colorMapSection + 1];
		
		//color = cA + (cB - cA) * colorMapT;

		ldiSimpleVertex* v0 = &verts[i * 4 + 0];
		ldiSimpleVertex* v1 = &verts[i * 4 + 1];
		ldiSimpleVertex* v2 = &verts[i * 4 + 2];
		ldiSimpleVertex* v3 = &verts[i * 4 + 3];

		v0->position = p0;
		v0->color = color;
		
		v1->position = p1;
		v1->color = color;
		
		v2->position = p2;
		v2->color = color;
		
		v3->position = p3;
		v3->color = color;

		indices[i * 6 + 0] = i * 4 + 2;
		indices[i * 6 + 1] = i * 4 + 1;
		indices[i * 6 + 2] = i * 4 + 0;
		indices[i * 6 + 3] = i * 4 + 0;
		indices[i * 6 + 4] = i * 4 + 3;
		indices[i * 6 + 5] = i * 4 + 2;
	}

	double singleDotArea = dotSize * dotSize;
	double dotsInArea = totalArea / singleDotArea;

	std::cout << "Total area: " << totalArea << " cm2 Dots: " << (int)dotsInArea << "\n";

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.ByteWidth = sizeof(ldiSimpleVertex) * vertCount;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = verts;

	AppContext->d3dDevice->CreateBuffer(&vbDesc, &vbData, &result.vertexBuffer);

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.ByteWidth = sizeof(uint32_t) * indexCount;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;

	AppContext->d3dDevice->CreateBuffer(&ibDesc, &ibData, &result.indexBuffer);

	result.vertCount = vertCount;
	result.indexCount = indexCount;

	delete[] verts;
	delete[] indices;

	return result;
}

ldiRenderModel gfxCreateRenderQuadModel(ldiApp* AppContext, ldiModel* ModelSource) {
	ldiRenderModel result = {};

	int quadCount = (int)(ModelSource->indices.size() / 4);
	int vertCount = (int)ModelSource->verts.size();
	int triCount = quadCount * 2;
	int indexCount = triCount * 3;

	uint32_t* indices = new uint32_t[indexCount];

	for (int i = 0; i < quadCount; ++i) {
		int i0 = ModelSource->indices[i * 4 + 0];
		int i1 = ModelSource->indices[i * 4 + 1];
		int i2 = ModelSource->indices[i * 4 + 2];
		int i3 = ModelSource->indices[i * 4 + 3];

		indices[i * 6 + 0] = i2;
		indices[i * 6 + 1] = i1;
		indices[i * 6 + 2] = i0;
		indices[i * 6 + 3] = i2;
		indices[i * 6 + 4] = i0;
		indices[i * 6 + 5] = i3;
	}

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.ByteWidth = sizeof(ldiMeshVertex) * vertCount;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = ModelSource->verts.data();

	AppContext->d3dDevice->CreateBuffer(&vbDesc, &vbData, &result.vertexBuffer);

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.ByteWidth = sizeof(uint32_t) * indexCount;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;

	AppContext->d3dDevice->CreateBuffer(&ibDesc, &ibData, &result.indexBuffer);

	result.vertCount = vertCount;
	result.indexCount = indexCount;

	delete[] indices;

	return result;
}

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

ldiRenderModel gfxCreateRenderModelMutableVerts(ldiApp* AppContext, ldiModel* ModelSource) {
	ldiRenderModel result = {};

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_DYNAMIC;
	vbDesc.ByteWidth = sizeof(ldiMeshVertex) * ModelSource->verts.size();
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

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

ldiRenderModel gfxCreateRenderModelBasic(ldiApp* AppContext, std::vector<ldiBasicVertex>* Verts, std::vector<uint32_t>* Indices) {
	ldiRenderModel result = {};

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.ByteWidth = sizeof(ldiBasicVertex) * Verts->size();
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = Verts->data();

	AppContext->d3dDevice->CreateBuffer(&vbDesc, &vbData, &result.vertexBuffer);

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.ByteWidth = sizeof(uint32_t) * Indices->size();
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = Indices->data();

	AppContext->d3dDevice->CreateBuffer(&ibDesc, &ibData, &result.indexBuffer);

	result.vertCount = Verts->size();
	result.indexCount = Indices->size();

	return result;
}

void gfxMapConstantBuffer(ldiApp* AppContext, ID3D11Buffer* Dst, void* Src, int Size) {
	D3D11_MAPPED_SUBRESOURCE ms;
	AppContext->d3dDeviceContext->Map(Dst, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	memcpy(ms.pData, Src, Size);
	AppContext->d3dDeviceContext->Unmap(Dst, 0);
}

void gfxRenderModel(ldiApp* AppContext, ldiRenderModel* Model, bool Wireframe = false, ID3D11ShaderResourceView* ResourceView = NULL, ID3D11SamplerState* Sampler = NULL) {
	UINT lgStride = sizeof(ldiMeshVertex);
	UINT lgOffset = 0;

	AppContext->d3dDeviceContext->IASetInputLayout(AppContext->meshInputLayout);
	AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &Model->vertexBuffer, &lgStride, &lgOffset);
	AppContext->d3dDeviceContext->IASetIndexBuffer(Model->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	AppContext->d3dDeviceContext->VSSetShader(AppContext->meshVertexShader, 0, 0);
	AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
	AppContext->d3dDeviceContext->PSSetShader(AppContext->meshPixelShader, 0, 0);
	AppContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
	AppContext->d3dDeviceContext->CSSetShader(NULL, NULL, 0);
	
	AppContext->d3dDeviceContext->OMSetBlendState(AppContext->defaultBlendState, NULL, 0xffffffff);

	if (Wireframe) {
		AppContext->d3dDeviceContext->RSSetState(AppContext->wireframeRasterizerState);
	} else {
		AppContext->d3dDeviceContext->RSSetState(AppContext->defaultRasterizerState);
	}

	AppContext->d3dDeviceContext->OMSetDepthStencilState(AppContext->defaultDepthStencilState, 0);

	if (ResourceView != NULL && Sampler != NULL) {
		AppContext->d3dDeviceContext->PSSetShaderResources(0, 1, &ResourceView);
		AppContext->d3dDeviceContext->PSSetSamplers(0, 1, &Sampler);
	}

	AppContext->d3dDeviceContext->DrawIndexed(Model->indexCount, 0, 0);
}

void gfxRenderModel(ldiApp* AppContext, ldiRenderModel* Model, ID3D11RasterizerState* RasterizerState, ID3D11VertexShader* VertexShader, ID3D11PixelShader* PixelShader, ID3D11InputLayout* InputLayout, ID3D11ShaderResourceView* ResourceView = NULL, ID3D11SamplerState* Sampler = NULL) {
	UINT lgStride = sizeof(ldiMeshVertex);
	UINT lgOffset = 0;

	AppContext->d3dDeviceContext->IASetInputLayout(InputLayout);
	AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &Model->vertexBuffer, &lgStride, &lgOffset);
	AppContext->d3dDeviceContext->IASetIndexBuffer(Model->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	AppContext->d3dDeviceContext->VSSetShader(VertexShader, 0, 0);
	AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
	AppContext->d3dDeviceContext->PSSetShader(PixelShader, 0, 0);
	AppContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
	AppContext->d3dDeviceContext->CSSetShader(NULL, NULL, 0);

	AppContext->d3dDeviceContext->OMSetBlendState(AppContext->defaultBlendState, NULL, 0xffffffff);

	AppContext->d3dDeviceContext->RSSetState(RasterizerState);

	AppContext->d3dDeviceContext->OMSetDepthStencilState(AppContext->defaultDepthStencilState, 0);

	if (ResourceView != NULL && Sampler != NULL) {
		AppContext->d3dDeviceContext->PSSetShaderResources(0, 1, &ResourceView);
		AppContext->d3dDeviceContext->PSSetSamplers(0, 1, &Sampler);
	}

	AppContext->d3dDeviceContext->DrawIndexed(Model->indexCount, 0, 0);
}

void gfxRenderBasicModel(ldiApp* AppContext, ldiRenderModel* Model, ID3D11ShaderResourceView* ResourceView = NULL, ID3D11SamplerState* Sampler = NULL) {
	UINT lgStride = sizeof(ldiBasicVertex);
	UINT lgOffset = 0;

	AppContext->d3dDeviceContext->IASetInputLayout(AppContext->basicInputLayout);
	AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &Model->vertexBuffer, &lgStride, &lgOffset);
	AppContext->d3dDeviceContext->IASetIndexBuffer(Model->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	AppContext->d3dDeviceContext->VSSetShader(AppContext->basicVertexShader, 0, 0);
	AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
	AppContext->d3dDeviceContext->PSSetShader(AppContext->basicPixelShader, 0, 0);
	AppContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
	AppContext->d3dDeviceContext->CSSetShader(NULL, NULL, 0);

	AppContext->d3dDeviceContext->OMSetBlendState(AppContext->defaultBlendState, NULL, 0xffffffff);
	AppContext->d3dDeviceContext->RSSetState(AppContext->defaultRasterizerState);

	AppContext->d3dDeviceContext->OMSetDepthStencilState(AppContext->defaultDepthStencilState, 0);

	if (ResourceView != NULL && Sampler != NULL) {
		AppContext->d3dDeviceContext->PSSetShaderResources(0, 1, &ResourceView);
		AppContext->d3dDeviceContext->PSSetSamplers(0, 1, &Sampler);
	}

	AppContext->d3dDeviceContext->DrawIndexed(Model->indexCount, 0, 0);
}

void gfxRenderBasicModel(ldiApp* AppContext, ldiRenderModel* Model, ID3D11InputLayout* InputLayout, ID3D11VertexShader* VertexShader, ID3D11PixelShader* PixelShader, ID3D11DepthStencilState* DepthState) {
	UINT lgStride = sizeof(ldiBasicVertex);
	UINT lgOffset = 0;

	AppContext->d3dDeviceContext->IASetInputLayout(InputLayout);
	AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &Model->vertexBuffer, &lgStride, &lgOffset);
	AppContext->d3dDeviceContext->IASetIndexBuffer(Model->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	AppContext->d3dDeviceContext->VSSetShader(VertexShader, 0, 0);
	AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
	AppContext->d3dDeviceContext->PSSetShader(PixelShader, 0, 0);
	AppContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
	AppContext->d3dDeviceContext->CSSetShader(NULL, NULL, 0);
	AppContext->d3dDeviceContext->OMSetBlendState(AppContext->defaultBlendState, NULL, 0xffffffff);
	AppContext->d3dDeviceContext->RSSetState(AppContext->defaultRasterizerState);
	AppContext->d3dDeviceContext->OMSetDepthStencilState(DepthState, 0);
	AppContext->d3dDeviceContext->DrawIndexed(Model->indexCount, 0, 0);
}

void gfxRenderSurfelModel(ldiApp* AppContext, ldiRenderModel* Model, ID3D11ShaderResourceView* ResourceView = NULL, ID3D11SamplerState* Sampler = NULL) {
	UINT lgStride = sizeof(ldiBasicVertex);
	UINT lgOffset = 0;

	AppContext->d3dDeviceContext->IASetInputLayout(AppContext->surfelInputLayout);
	AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &Model->vertexBuffer, &lgStride, &lgOffset);
	AppContext->d3dDeviceContext->IASetIndexBuffer(Model->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	AppContext->d3dDeviceContext->VSSetShader(AppContext->surfelVertexShader, 0, 0);
	AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
	AppContext->d3dDeviceContext->PSSetShader(AppContext->surfelPixelShader, 0, 0);
	AppContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
	AppContext->d3dDeviceContext->CSSetShader(NULL, NULL, 0);

	AppContext->d3dDeviceContext->OMSetBlendState(AppContext->defaultBlendState, NULL, 0xffffffff);
	AppContext->d3dDeviceContext->RSSetState(AppContext->defaultRasterizerState);

	AppContext->d3dDeviceContext->OMSetDepthStencilState(AppContext->defaultDepthStencilState, 0);

	if (ResourceView != NULL && Sampler != NULL) {
		AppContext->d3dDeviceContext->PSSetShaderResources(0, 1, &ResourceView);
		AppContext->d3dDeviceContext->PSSetSamplers(0, 1, &Sampler);
	}

	AppContext->d3dDeviceContext->DrawIndexed(Model->indexCount, 0, 0);
}

void gfxRenderWireModel(ldiApp* AppContext, ldiRenderLines* Model) {
	UINT lgStride = sizeof(ldiSimpleVertex);
	UINT lgOffset = 0;

	AppContext->d3dDeviceContext->IASetInputLayout(AppContext->simpleInputLayout);
	AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &Model->vertexBuffer, &lgStride, &lgOffset);
	AppContext->d3dDeviceContext->IASetIndexBuffer(Model->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	AppContext->d3dDeviceContext->VSSetShader(AppContext->simpleVertexShader, 0, 0);
	AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
	AppContext->d3dDeviceContext->PSSetShader(AppContext->simplePixelShader, 0, 0);
	AppContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
	AppContext->d3dDeviceContext->CSSetShader(NULL, NULL, 0);

	AppContext->d3dDeviceContext->OMSetBlendState(AppContext->defaultBlendState, NULL, 0xffffffff);
	AppContext->d3dDeviceContext->RSSetState(AppContext->wireframeRasterizerState);

	AppContext->d3dDeviceContext->OMSetDepthStencilState(AppContext->wireframeDepthStencilState, 0);

	AppContext->d3dDeviceContext->DrawIndexed(Model->indexCount, 0, 0);
}

void gfxRenderDebugModel(ldiApp* AppContext, ldiRenderModel* Model) {
	UINT lgStride = sizeof(ldiSimpleVertex);
	UINT lgOffset = 0;

	AppContext->d3dDeviceContext->IASetInputLayout(AppContext->simpleInputLayout);
	AppContext->d3dDeviceContext->IASetVertexBuffers(0, 1, &Model->vertexBuffer, &lgStride, &lgOffset);
	AppContext->d3dDeviceContext->IASetIndexBuffer(Model->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	AppContext->d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	AppContext->d3dDeviceContext->VSSetShader(AppContext->simpleVertexShader, 0, 0);
	AppContext->d3dDeviceContext->VSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
	AppContext->d3dDeviceContext->PSSetShader(AppContext->simplePixelShader, 0, 0);
	AppContext->d3dDeviceContext->PSSetConstantBuffers(0, 1, &AppContext->mvpConstantBuffer);
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
}

void gfxCleanupPrimaryBackbuffer(ldiApp* AppContext) {
	if (AppContext->mainRenderTargetView) {
		AppContext->mainRenderTargetView->Release();
		AppContext->mainRenderTargetView = NULL;
	}
}

void gfxCreateRenderView(ldiApp* AppContext, ldiRenderViewBuffers* ViewBuffers, int Width, int Height) {
	if (ViewBuffers->mainViewTexture) {
		ViewBuffers->mainViewTexture->Release();
		ViewBuffers->mainViewTexture = NULL;
	}

	if (ViewBuffers->mainViewRtView) {
		ViewBuffers->mainViewRtView->Release();
		ViewBuffers->mainViewRtView = NULL;
	}

	if (ViewBuffers->mainViewResourceView) {
		ViewBuffers->mainViewResourceView->Release();
		ViewBuffers->mainViewResourceView = NULL;
	}

	if (ViewBuffers->depthStencilBuffer) {
		ViewBuffers->depthStencilBuffer->Release();
		ViewBuffers->depthStencilBuffer = NULL;
	}

	if (ViewBuffers->depthStencilView) {
		ViewBuffers->depthStencilView->Release();
		ViewBuffers->depthStencilView = NULL;
	}

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = Width;
	texDesc.Height = Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	AppContext->d3dDevice->CreateTexture2D(&texDesc, NULL, &ViewBuffers->mainViewTexture);

	D3D11_RENDER_TARGET_VIEW_DESC rtDesc = {};
	rtDesc.Format = texDesc.Format;
	rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	AppContext->d3dDevice->CreateRenderTargetView(ViewBuffers->mainViewTexture, &rtDesc, &ViewBuffers->mainViewRtView);

	AppContext->d3dDevice->CreateShaderResourceView(ViewBuffers->mainViewTexture, NULL, &ViewBuffers->mainViewResourceView);

	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = Width;
	depthStencilDesc.Height = Height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	AppContext->d3dDevice->CreateTexture2D(&depthStencilDesc, NULL, &ViewBuffers->depthStencilBuffer);
	AppContext->d3dDevice->CreateDepthStencilView(ViewBuffers->depthStencilBuffer, NULL, &ViewBuffers->depthStencilView);
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
	//const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0 };

	if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 1, D3D11_SDK_VERSION, &sd, &AppContext->SwapChain, &AppContext->d3dDevice, &featureLevel, &AppContext->d3dDeviceContext) != S_OK)
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

void gfxBeginPrimaryViewport(ldiApp* AppContext) {
	AppContext->d3dDeviceContext->OMSetRenderTargets(1, &AppContext->mainRenderTargetView, NULL);

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = AppContext->windowWidth;
	viewport.Height = AppContext->windowHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	AppContext->d3dDeviceContext->RSSetViewports(1, &viewport);

	const float clearColor[4] = { 0.07f, 0.08f, 0.09f, 1.00f };
	AppContext->d3dDeviceContext->ClearRenderTargetView(AppContext->mainRenderTargetView, clearColor);
}

void gfxEndPrimaryViewport(ldiApp* AppContext) {
	AppContext->SwapChain->Present(1, 0);
}

ID3DBlob* gfxCompileShader(LPCWSTR Filename, const char* EntryPoint, LPCSTR Target) {
	ID3DBlob* errorBlob = NULL;
	ID3DBlob* shaderBlob = NULL;
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

	HRESULT result = D3DCompileFromFile(Filename, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint, Target, flags, 0, &shaderBlob, &errorBlob);

	if (result < 0) {
		std::cout << "Failed to compile shader: " << std::hex << "0x" << (uint32_t)result << " " << std::dec << "\n";
		
		if (result == 0x80070002) {
			std::cout << "File not found\n";
		}

		if (errorBlob != NULL) {
			std::cout << (const char*)errorBlob->GetBufferPointer() << "\n";
			errorBlob->Release();
		}

		return 0;
	}

	return shaderBlob;
}

bool gfxCreateVertexShader(ldiApp* AppContext, LPCWSTR Filename, const char* EntryPoint, ID3D11VertexShader** Shader, D3D11_INPUT_ELEMENT_DESC* LayoutDesc, int LayoutElements, ID3D11InputLayout** InputLayout) {
	ID3DBlob* shaderBlob = gfxCompileShader(Filename, EntryPoint, "vs_5_0");
	
	if (!shaderBlob) {
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

bool gfxCreateVertexShader(ldiApp* AppContext, LPCWSTR Filename, const char* EntryPoint, ID3D11VertexShader** Shader) {
	ID3DBlob* shaderBlob = gfxCompileShader(Filename, EntryPoint, "vs_5_0");

	if (!shaderBlob) {
		return false;
	}

	if (AppContext->d3dDevice->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, Shader) != S_OK) {
		shaderBlob->Release();
		return false;
	}

	shaderBlob->Release();

	return true;
}

bool gfxCreatePixelShader(ldiApp* AppContext, LPCWSTR Filename, const char* EntryPoint, ID3D11PixelShader** Shader) {
	ID3DBlob* shaderBlob = gfxCompileShader(Filename, EntryPoint, "ps_5_0");

	if (!shaderBlob) {
		return false;
	}

	if (AppContext->d3dDevice->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, Shader) != S_OK) {
		shaderBlob->Release();
		return false;
	}

	shaderBlob->Release();

	return true;
}

bool gfxCreateHullShader(ldiApp* AppContext, LPCWSTR Filename, const char* EntryPoint, ID3D11HullShader** Shader) {
	ID3DBlob* shaderBlob = gfxCompileShader(Filename, EntryPoint, "hs_5_0");

	if (!shaderBlob) {
		return false;
	}

	if (AppContext->d3dDevice->CreateHullShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, Shader) != S_OK) {
		shaderBlob->Release();
		return false;
	}

	shaderBlob->Release();

	return true;
}

bool gfxCreateDomainShader(ldiApp* AppContext, LPCWSTR Filename, const char* EntryPoint, ID3D11DomainShader** Shader) {
	ID3DBlob* shaderBlob = gfxCompileShader(Filename, EntryPoint, "ds_5_0");
	
	if (!shaderBlob) {
		return false;
	}

	if (AppContext->d3dDevice->CreateDomainShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, Shader) != S_OK) {
		shaderBlob->Release();
		return false;
	}

	shaderBlob->Release();

	return true;
}

bool gfxCreateComputeShader(ldiApp* AppContext, LPCWSTR Filename, const char* EntryPoint, ID3D11ComputeShader** Shader) {
	ID3DBlob* shaderBlob = gfxCompileShader(Filename, EntryPoint, "cs_5_0");

	if (!shaderBlob) {
		return false;
	}

	if (AppContext->d3dDevice->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, Shader) != S_OK) {
		shaderBlob->Release();
		return false;
	}

	shaderBlob->Release();

	return true;
}

bool gfxCreateStructuredBuffer(ldiApp* AppContext, int ElementSize, int ElementCount, void* Data, ID3D11Buffer** Buffer) {
	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc.ByteWidth = ElementSize * ElementCount;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = ElementSize;

	if (Data) {
		D3D11_SUBRESOURCE_DATA data = {};
		data.pSysMem = Data;

		if (AppContext->d3dDevice->CreateBuffer(&desc, &data, Buffer) != S_OK) {
			return false;
		}
	} else {
		if (AppContext->d3dDevice->CreateBuffer(&desc, 0, Buffer) != S_OK) {
			return false;
		}
	}

	return true;
}

bool gfxCreateBufferShaderResourceView(ldiApp* AppContext, ID3D11Buffer* Resource, ID3D11ShaderResourceView** View) {
	// NOTE: Creates view for raw or structured buffers.
	
	HRESULT result = AppContext->d3dDevice->CreateShaderResourceView(Resource, 0, View);

	if (result != S_OK) {
		return false;
	}

	return true;
}

bool gfxCreateTexture2DUav(ldiApp* AppContext, ID3D11Texture2D* Resource, ID3D11UnorderedAccessView** View) {
	D3D11_TEXTURE2D_DESC texDesc = {};
	Resource->GetDesc(&texDesc);

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.Format = texDesc.Format;
	desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice = 0;
	
	HRESULT result = AppContext->d3dDevice->CreateUnorderedAccessView(Resource, &desc, View);

	if (result != S_OK) {
		return false;
	}

	return true;
}

bool gfxCreateTexture3DUav(ldiApp* AppContext, ID3D11Texture3D* Resource, ID3D11UnorderedAccessView** View) {
	D3D11_TEXTURE3D_DESC texDesc = {};
	Resource->GetDesc(&texDesc);

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.Format = texDesc.Format;
	desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	desc.Texture3D.MipSlice = 0;
	desc.Texture3D.FirstWSlice = 0;
	desc.Texture3D.WSize = texDesc.Depth;

	HRESULT result = AppContext->d3dDevice->CreateUnorderedAccessView(Resource, &desc, View);

	if (result != S_OK) {
		return false;
	}

	return true;
}

bool gfxCreateBufferUav(ldiApp* AppContext, ID3D11Buffer* Buffer, ID3D11UnorderedAccessView** View) {
	// NOTE: Creates view for raw or structured buffers.

	D3D11_BUFFER_DESC descBuf = {};
	Buffer->GetDesc(&descBuf);

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	desc.Buffer.FirstElement = 0;

	if (descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS) {
		// This is a Raw Buffer.
		desc.Format = DXGI_FORMAT_R32_TYPELESS;
		desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
		desc.Buffer.NumElements = descBuf.ByteWidth / 4;

	} else if (descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) {
		// This is a Structured Buffer.
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.Buffer.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;
	} else {
		return false;
	}

	HRESULT result = AppContext->d3dDevice->CreateUnorderedAccessView(Buffer, &desc, View);

	if (result != S_OK) {
		return false;
	}

	return true;
}

void gfxRunComputeShader(ldiApp* AppContext, ID3D11ComputeShader* Shader) {
	AppContext->d3dDeviceContext->CSSetShader(Shader, 0, 0);

	//AppContext->d3dDeviceContext->CSSetShaderResources(0, nNumViews, pShaderResourceViews);
	//AppContext->d3dDeviceContext->CSSetUnorderedAccessViews(0, 1, &pUnorderedAccessView, 0);

	//AppContext->d3dDeviceContext->Dispatch(X, Y, Z);
		//AppContext->d3dDeviceContext->Dispatch(num, 1, 1);

	AppContext->d3dDeviceContext->CSSetShader(0, 0, 0);

	// TODO: Consider unbinding all resources after dispatch.
}