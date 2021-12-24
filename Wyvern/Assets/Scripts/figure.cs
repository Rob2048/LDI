using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Diagnostics;
using System;
using UnityEngine.Rendering;

using Debug = UnityEngine.Debug;

public class ViewCache {
	public byte[] visibleSurfelMask;
	public int resultCount;
}

public class SurfelViewsBake {
	public int surfelCount;
	public byte[] workingSet;
	public ViewCache[] views;
}

public class TriangleRect {
	public Vector2 v0;
	public Vector2 v1;
	public Vector2 v2;
	public Rect rect;
	public int id;
}

public class MeshInfo {
	public Vector3[] vertPositions;
	public Vector3[] vertNormals;
	public Vector2[] vertUvs;
	public Vector2[] vertUvsUnique;
	public Color32[] vertColors;
	public int[] triangles;

	public MeshInfo Clone() {
		MeshInfo result = new MeshInfo();

		if (vertPositions != null) result.vertPositions = (Vector3[])vertPositions.Clone();
		if (vertNormals != null) result.vertNormals = (Vector3[])vertNormals.Clone();
		if (vertUvs != null) result.vertUvs = (Vector2[])vertUvs.Clone();
		if (vertUvsUnique != null) result.vertUvsUnique = (Vector2[])vertUvsUnique.Clone();
		if (vertColors != null) result.vertColors = (Color32[])vertColors.Clone();
		if (triangles != null) result.triangles = (int[])triangles.Clone();

		return result;
	}

	public void Scale(float scale) {
		for (int i = 0; i < vertPositions.Length; ++i) {
			vertPositions[i] *= scale;
		}
	}

	public void Translate(Vector3 translate) {
		for (int i = 0; i < vertPositions.Length; ++i) {
			vertPositions[i] += translate;
		}
	}

	public void BuildUniqueTriangleIds() {
		int triCount = triangles.Length / 3;

		if (triCount >= 255 * 255 * 255) {
			Debug.LogError("Can't build unique triangles for mesh, too many faces.");
		}

		vertColors = new Color32[triCount * 3];

		for (int i = 0; i < triCount; ++i) {
			Color32 col = new Color32((byte)(i & 0xFF), (byte)((i >> 8) & 0xFF), (byte)((i >> 16) & 0xFF), 255);
			vertColors[i * 3 + 0] = col;
			vertColors[i * 3 + 1] = col;
			vertColors[i * 3 + 2] = col;
		}
	}
}

public class Figure {

	public MeshInfo initialMeshInfo;
	public MeshInfo processedMeshInfo;
	public Texture2D texture;
	public RenderTexture coverageResultRt;

	public GameObject previewGob;
	public GameObject wireGob;

	public GameObject modelPreviewRootGob;

	public void SetMesh(MeshInfo mesh) {
		initialMeshInfo = mesh;
	}

	public void SetTexture(Texture2D texture) {
		this.texture = texture;
	}

	public Mesh GenerateMesh(MeshInfo meshInfo, bool calcBounds = false, bool calcNormals = false) {
		Mesh result = new Mesh();
		result.indexFormat = UnityEngine.Rendering.IndexFormat.UInt32;
		result.SetVertices(meshInfo.vertPositions);
		result.SetUVs(0, meshInfo.vertUvs);

		if (meshInfo.vertUvsUnique != null) {
			result.SetUVs(1, meshInfo.vertUvsUnique);
		}

		result.SetTriangles(meshInfo.triangles, 0);

		if (meshInfo.vertColors != null) {
			result.SetColors(meshInfo.vertColors);
		}

		if (calcBounds) {
			result.RecalculateBounds();
		}

		if (calcNormals) {
			result.RecalculateNormals();
		}

		return result;
	}

	public Mesh GenerateUiMesh(MeshInfo meshInfo) {
		int triCount = meshInfo.vertUvsUnique.Length / 3;

		Vector3[] verts = new Vector3[triCount * 3];
		Color32[] colors = new Color32[triCount * 3];
		int[] indices = new int[triCount * 6];

		Vector2 pixelScale = new Vector2(4096, -4096);
		Vector2 pixelOffset = new Vector2(0, 4096);
		Color32 color = new Color32(255, 255, 255, 255);

		for (int i = 0; i < triCount; ++i) {
			verts[i * 3 + 0] = meshInfo.vertUvsUnique[i * 3 + 0] * pixelScale + pixelOffset;
			verts[i * 3 + 1] = meshInfo.vertUvsUnique[i * 3 + 1] * pixelScale + pixelOffset;
			verts[i * 3 + 2] = meshInfo.vertUvsUnique[i * 3 + 2] * pixelScale + pixelOffset;

			colors[i * 3 + 0] = color;
			colors[i * 3 + 1] = color;
			colors[i * 3 + 2] = color;

			indices[i * 6 + 0] = i * 3 + 0;
			indices[i * 6 + 1] = i * 3 + 1;
			indices[i * 6 + 2] = i * 3 + 1;
			indices[i * 6 + 3] = i * 3 + 2;
			indices[i * 6 + 4] = i * 3 + 2;
			indices[i * 6 + 5] = i * 3 + 0;
		}

		Mesh result = new Mesh();
		result.indexFormat = UnityEngine.Rendering.IndexFormat.UInt32;	
		result.SetVertices(verts);
		result.SetColors(colors);
		result.SetIndices(indices, MeshTopology.Lines, 0);
		
		return result;
	}

	public Mesh GenerateUiMeshQuads(MeshInfo meshInfo) {
		int quadCount = meshInfo.vertUvsUnique.Length / 4;

		Vector3[] verts = new Vector3[quadCount * 4];
		Color32[] colors = new Color32[quadCount * 4];
		int[] indices = new int[quadCount * 8];

		Vector2 pixelScale = new Vector2(4096, -4096);
		Vector2 pixelOffset = new Vector2(0, 4096);
		Color32 color = new Color32(255, 255, 255, 255);

		for (int i = 0; i < quadCount; ++i) {
			verts[i * 4 + 0] = meshInfo.vertUvsUnique[i * 4 + 0] * pixelScale + pixelOffset;
			verts[i * 4 + 1] = meshInfo.vertUvsUnique[i * 4 + 1] * pixelScale + pixelOffset;
			verts[i * 4 + 2] = meshInfo.vertUvsUnique[i * 4 + 2] * pixelScale + pixelOffset;
			verts[i * 4 + 3] = meshInfo.vertUvsUnique[i * 4 + 3] * pixelScale + pixelOffset;

			colors[i * 4 + 0] = color;
			colors[i * 4 + 1] = color;
			colors[i * 4 + 2] = color;
			colors[i * 4 + 3] = color;

			indices[i * 8 + 0] = i * 4 + 0;
			indices[i * 8 + 1] = i * 4 + 1;
			indices[i * 8 + 2] = i * 4 + 1;
			indices[i * 8 + 3] = i * 4 + 2;
			indices[i * 8 + 4] = i * 4 + 2;
			indices[i * 8 + 5] = i * 4 + 3;
			indices[i * 8 + 6] = i * 4 + 3;
			indices[i * 8 + 7] = i * 4 + 0;
		}

		Mesh result = new Mesh();
		result.indexFormat = UnityEngine.Rendering.IndexFormat.UInt32;	
		result.SetVertices(verts);
		result.SetColors(colors);
		result.SetIndices(indices, MeshTopology.Lines, 0);
		
		return result;
	}

	public void GenerateDisplayMesh(MeshInfo meshInfo, Material material) {
		if (previewGob != null) {
			GameObject.Destroy(previewGob);
		}

		Mesh result = new Mesh();
		result.indexFormat = UnityEngine.Rendering.IndexFormat.UInt32;
		result.SetVertices(meshInfo.vertPositions);

		if (meshInfo.vertUvs != null) {
			result.SetUVs(0, meshInfo.vertUvs);
		}

		result.SetTriangles(meshInfo.triangles, 0);

		if (meshInfo.vertColors != null) {
			result.SetColors(meshInfo.vertColors);
		}

		result.RecalculateBounds();
		result.RecalculateNormals();

		previewGob = new GameObject();
		MeshRenderer renderer = previewGob.AddComponent<MeshRenderer>();
		renderer.shadowCastingMode = ShadowCastingMode.Off;
		renderer.material = material;

		if (texture != null) {
			renderer.material.SetTexture("_MainTex", texture);
		}

		previewGob.AddComponent<MeshFilter>().mesh = result;
	}

	public void _ShowCoveragePreviewMesh(Mesh mesh) {
		if (previewGob != null) {
			GameObject.Destroy(previewGob);
		}

		previewGob = new GameObject();
		MeshRenderer renderer = previewGob.AddComponent<MeshRenderer>();
		renderer.material = core.singleton.CoveragetMeshViewMat;
		renderer.material.SetTexture("_MainTex", coverageResultRt);

		previewGob.AddComponent<MeshFilter>().mesh = mesh;
	}

	public void GenerateDisplayWireMesh(MeshInfo meshInfo, Material material) {
		if (wireGob != null) {
			GameObject.Destroy(wireGob);
		}

		int triCount = meshInfo.triangles.Length / 3;
		int[] lines = new int[triCount * 6];

		for (int i = 0; i < triCount; ++i) {
			lines[i * 6 + 0] = meshInfo.triangles[i * 3 + 0];
			lines[i * 6 + 1] = meshInfo.triangles[i * 3 + 1];

			lines[i * 6 + 2] = meshInfo.triangles[i * 3 + 1];
			lines[i * 6 + 3] = meshInfo.triangles[i * 3 + 2];

			lines[i * 6 + 4] = meshInfo.triangles[i * 3 + 2];
			lines[i * 6 + 5] = meshInfo.triangles[i * 3 + 0];
		}

		Mesh result = new Mesh();
		result.indexFormat = UnityEngine.Rendering.IndexFormat.UInt32;
		result.SetVertices(meshInfo.vertPositions);
		result.SetIndices(lines, MeshTopology.Lines, 0);
		result.RecalculateBounds();
		
		wireGob = new GameObject();
		MeshRenderer renderer = wireGob.AddComponent<MeshRenderer>();
		renderer.material = material;

		wireGob.AddComponent<MeshFilter>().mesh = result;
	}

	public void GenerateQuadWireMesh(MeshInfo meshInfo, Material material) {
		if (wireGob != null) {
			GameObject.Destroy(wireGob);
		}

		int quadCount = meshInfo.triangles.Length / 6;
		int[] lines = new int[quadCount * 8];

		for (int i = 0; i < quadCount; ++i) {
			lines[i * 8 + 0] = meshInfo.triangles[i * 6 + 0];
			lines[i * 8 + 1] = meshInfo.triangles[i * 6 + 1];

			lines[i * 8 + 2] = meshInfo.triangles[i * 6 + 1];
			lines[i * 8 + 3] = meshInfo.triangles[i * 6 + 5];

			lines[i * 8 + 4] = meshInfo.triangles[i * 6 + 5];
			lines[i * 8 + 5] = meshInfo.triangles[i * 6 + 3];

			lines[i * 8 + 6] = meshInfo.triangles[i * 6 + 3];
			lines[i * 8 + 7] = meshInfo.triangles[i * 6 + 0];
		}

		Mesh result = new Mesh();
		result.indexFormat = UnityEngine.Rendering.IndexFormat.UInt32;
		result.SetVertices(meshInfo.vertPositions);
		result.SetIndices(lines, MeshTopology.Lines, 0);
		result.RecalculateBounds();
		
		wireGob = new GameObject();
		MeshRenderer renderer = wireGob.AddComponent<MeshRenderer>();
		renderer.material = material;

		wireGob.AddComponent<MeshFilter>().mesh = result;
	}

	private void _RenderVisibilityMeshView(Mesh triIdMesh, Matrix4x4 camViewMat, ComputeBuffer checklistBuffer) {
		CommandBuffer cmdBuffer = new CommandBuffer();
		cmdBuffer.name = "Render visbility";

		Matrix4x4 scaleMatrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, new Vector3(1, 1, -1));
		Matrix4x4 projMat = Matrix4x4.Ortho(-15, 15, -15, 15, 0.01f, 300.0f);
		
		Matrix4x4 rotMatX = Matrix4x4.Rotate(Quaternion.Euler(0, 0, 0));
		Matrix4x4 rotMatY = Matrix4x4.Rotate(Quaternion.Euler(0, 0, 0));
		Matrix4x4 offsetMat = Matrix4x4.Translate(new Vector3(0, -4.75f, 0));
		Matrix4x4 modelMat = offsetMat * rotMatX * rotMatY;
		
		cmdBuffer.SetRenderTarget(core.singleton.VisRt);
		cmdBuffer.SetViewport(new Rect(0, 0, 4096, 4096));
		cmdBuffer.ClearRenderTarget(true, true, new Color(0.5f, 0.5f, 0.5f, 1.0f), 1.0f);
		cmdBuffer.SetViewProjectionMatrices(scaleMatrix * camViewMat, projMat);
		cmdBuffer.DrawMesh(triIdMesh, modelMat, core.singleton.VisMat);
		Graphics.ExecuteCommandBuffer(cmdBuffer);

		int kernelIndex = core.singleton.VisCompute.FindKernel("CSMain");
		core.singleton.VisCompute.SetTexture(kernelIndex, "Source", core.singleton.VisRt);
		core.singleton.VisCompute.SetBuffer(kernelIndex, "Checklist", checklistBuffer);
		core.singleton.VisCompute.Dispatch(kernelIndex, 4096 / 8, 4096 / 8, 1);
	}

	private MeshInfo _GenerateVisibiltyMesh(MeshInfo meshInfo, Vector3[] views) {
		int sourceTriCount = meshInfo.triangles.Length / 3;
		int checklistSize = 512 * 512;

		if (sourceTriCount > checklistSize) {
			Debug.LogError("Source mesh has too many faces for mesh visibility check.");
			return null;
		}

		ProfileTimer t = new ProfileTimer();

		meshInfo.BuildUniqueTriangleIds();
		t.Stop("Calc unique tri Ids");

		t.Start();
		Mesh triIdMesh = GenerateMesh(meshInfo);
		t.Stop("Create unique tri mesh");

		t.Start();
		int[] visChecklist = new int[checklistSize];
		ComputeBuffer checklistBuffer = new ComputeBuffer(visChecklist.Length, 4);
		checklistBuffer.SetData(visChecklist);
		t.Stop("Prime compute buffers");

		t.Start();
		for (int i = 0; i < views.Length; ++i) {
			Vector3 dir = views[i];

			Matrix4x4 camViewMat = Matrix4x4.TRS(dir * 20.0f, Quaternion.LookRotation(-dir, Vector3.up), Vector3.one);
			camViewMat = camViewMat.inverse;

			_RenderVisibilityMeshView(triIdMesh, camViewMat, checklistBuffer);
		}

		// NOTE: Data sync forced here, GPU rendering will be completed at this point.
		checklistBuffer.GetData(visChecklist);
		t.Stop("Render views");

		int triCount = 0;

		t.Start();
		for (int i = 0; i < visChecklist.Length; ++i) {
			if (visChecklist[i] != 0) {
				++triCount;
			}
		}
		t.Stop("Count tris: " + sourceTriCount + " -> " + triCount);

		MeshInfo result = new MeshInfo();
		result.vertPositions = new Vector3[triCount * 3];
		result.vertUvs = new Vector2[triCount * 3];
		result.vertNormals = new Vector3[triCount * 3];
		result.triangles = new int[triCount * 3];

		int idx = 0;

		for (int i = 0; i < sourceTriCount; ++i) {
			if (visChecklist[i] == 0)
				continue;

			int v0 = meshInfo.triangles[i * 3 + 0];
			int v1 = meshInfo.triangles[i * 3 + 1];
			int v2 = meshInfo.triangles[i * 3 + 2];

			result.vertPositions[idx * 3 + 0] = meshInfo.vertPositions[v0];
			result.vertPositions[idx * 3 + 1] = meshInfo.vertPositions[v1];
			result.vertPositions[idx * 3 + 2] = meshInfo.vertPositions[v2];

			result.vertUvs[idx * 3 + 0] = meshInfo.vertUvs[v0];
			result.vertUvs[idx * 3 + 1] = meshInfo.vertUvs[v1];
			result.vertUvs[idx * 3 + 2] = meshInfo.vertUvs[v2];

			result.triangles[idx * 3 + 0] = idx * 3 + 0;
			result.triangles[idx * 3 + 1] = idx * 3 + 1;
			result.triangles[idx * 3 + 2] = idx * 3 + 2;

			++idx;
		}

		checklistBuffer.Release();

		return result;
	}

	public void GenerateDisplayViews(Vector3[] views) {
		for (int i = 0; i < views.Length; ++i) {
			GameObject viewMarker = GameObject.Instantiate(core.singleton.SampleLocatorPrefab, views[i] * 20.0f, Quaternion.LookRotation(-views[i], Vector3.up));
		}
	}

	public MeshInfo CreateUniqueUvsQuadMesh(MeshInfo meshInfo) {
		if (meshInfo.triangles.Length % 6 != 0) {
			Debug.LogError("Mesh contains non-quads");
			return null;
		}

		int quadCount = meshInfo.triangles.Length / 6;

		Debug.Log("Quad count: " + quadCount);

		float totalArea = 0.0f;

		for (int i = 0; i < quadCount; ++i) {
			Vector3 v0 = meshInfo.vertPositions[meshInfo.triangles[i * 6 + 0]];
			Vector3 v1 = meshInfo.vertPositions[meshInfo.triangles[i * 6 + 1]];
			Vector3 v2 = meshInfo.vertPositions[meshInfo.triangles[i * 6 + 5]];

			Vector3 l0 = v0 - v1;
			Vector3 l1 = v2 - v1;

			float area = l0.magnitude * l1.magnitude;

			totalArea += area;
		}

		totalArea /= quadCount;
		float squareLength = Mathf.Sqrt(totalArea);
		Debug.Log("Average area: " + totalArea + " length: " + (squareLength * 10000) + " um");

		// Create shared verts per quad.

		Vector3[] vertPositions = new Vector3[quadCount * 4];
		Vector2[] vertUvs = new Vector2[quadCount * 4];
		int[] triangles = new int[quadCount * 6];

		float currentX = 0.0f;
		float currentY = 0.0f;
		float quadSize = 3.0f / 4096.0f;
		float padding = 0.05f / 4096.0f;

		for (int i = 0; i < quadCount; ++i) {
			int vIdx0 = meshInfo.triangles[i * 6 + 0];
			int vIdx1 = meshInfo.triangles[i * 6 + 1];
			int vIdx2 = meshInfo.triangles[i * 6 + 5];
			int vIdx3 = meshInfo.triangles[i * 6 + 2];

			vertPositions[i * 4 + 0] = meshInfo.vertPositions[vIdx0];
			vertPositions[i * 4 + 1] = meshInfo.vertPositions[vIdx1];
			vertPositions[i * 4 + 2] = meshInfo.vertPositions[vIdx2];
			vertPositions[i * 4 + 3] = meshInfo.vertPositions[vIdx3];

			if (currentX + quadSize >= 1.0f) {
				currentX = 0.0f;
				currentY += quadSize;
			}

			vertUvs[i * 4 + 0] = new Vector2(currentX + padding, currentY + padding);
			vertUvs[i * 4 + 1] = new Vector2(currentX + padding, currentY + quadSize - padding);
			vertUvs[i * 4 + 2] = new Vector2(currentX + quadSize - padding, currentY + quadSize - padding);
			vertUvs[i * 4 + 3] = new Vector2(currentX + quadSize - padding, currentY + padding);

			currentX += quadSize;

			triangles[i * 6 + 0] = i * 4 + 0;
			triangles[i * 6 + 1] = i * 4 + 1;
			triangles[i * 6 + 2] = i * 4 + 2;
			triangles[i * 6 + 3] = i * 4 + 0;
			triangles[i * 6 + 4] = i * 4 + 2;
			triangles[i * 6 + 5] = i * 4 + 3;
		}

		MeshInfo result = new MeshInfo();
		result.vertPositions = vertPositions;
		// result.vertUvs = vertUvs;
		result.vertUvsUnique = vertUvs;
		result.triangles = triangles;

		return result;
	}

	private void _CreateUniqueUVs(MeshInfo meshInfo) {
		int triCount = meshInfo.vertPositions.Length / 3;

		// Scale: 50um is 1 pixel

		float textureSizePixels = 4096;
		float pixelSizeUm = 50;
		float physicalSizeUm = textureSizePixels * pixelSizeUm;

		float worldScaleUm = 10000;
		float triScale = worldScaleUm / physicalSizeUm;
		
		float padding = 0;
		// float triScale = 0.04f;
		// float triScale = 0.042f;
		// float triScale = 0.08f;

		Vector2 packOffset = new Vector2(padding, padding);
		
		float maxY = 0.0f;

		int processTriCount = triCount;

		TriangleRect[] triRects = new TriangleRect[processTriCount];

		ProfileTimer timer = new ProfileTimer();

		// Vector3[] debugMeshVerts = new Vector3[processTriCount * 6];
		// Color32[] debugeMeshColors = new Color32[processTriCount * 6];
		// int[] debugMeshIndices = new int[processTriCount * 6];

		for (int i = 0; i < processTriCount; ++i) {
			Vector3 v0 = meshInfo.vertPositions[meshInfo.triangles[i * 3 + 0]];
			Vector3 v1 = meshInfo.vertPositions[meshInfo.triangles[i * 3 + 1]];
			Vector3 v2 = meshInfo.vertPositions[meshInfo.triangles[i * 3 + 2]];

			// Get triangle basis
			Vector3 side = (v1 - v0).normalized;
			Vector3 side2 = (v0 - v2).normalized;
			Vector3 normal = Vector3.Cross(side2, side).normalized;
			Vector3 sidePerp = Vector3.Cross(side, normal).normalized;
			
			// Basis debug.
			// debugMeshVerts[i * 6 + 0] = v0;
			// debugeMeshColors[i * 6 + 0] = Color.red;
			// debugMeshIndices[i * 6 + 0] = i * 6 + 0;
			// debugMeshVerts[i * 6 + 1] = v0 + side * 0.05f;
			// debugeMeshColors[i * 6 + 1] = Color.red;
			// debugMeshIndices[i * 6 + 1] = i * 6 + 1;

			// debugMeshVerts[i * 6 + 2] = v0;
			// debugeMeshColors[i * 6 + 2] = Color.green;
			// debugMeshIndices[i * 6 + 2] = i * 6 + 2;
			// debugMeshVerts[i * 6 + 3] = v0 + normal * 0.05f;
			// debugeMeshColors[i * 6 + 3] = Color.green;
			// debugMeshIndices[i * 6 + 3] = i * 6 + 3;

			// debugMeshVerts[i * 6 + 4] = v0;
			// debugeMeshColors[i * 6 + 4] = Color.blue;
			// debugMeshIndices[i * 6 + 4] = i * 6 + 4;
			// debugMeshVerts[i * 6 + 5] = v0 + sidePerp * 0.05f;
			// debugeMeshColors[i * 6 + 5] = Color.blue;
			// debugMeshIndices[i * 6 + 5] = i * 6 + 5;

			// Project triangle flat
			Vector2 flatV0 = Vector2.zero;
			Vector2 flatV1 = new Vector2(Vector3.Dot((v1 - v0), side), Vector3.Dot((v1 - v0), sidePerp));
			Vector2 flatV2 = new Vector2(Vector3.Dot((v2 - v0), side), Vector3.Dot((v2 - v0), sidePerp));
			
			// Reprojection.
			Vector3 p0 = v0;
			Vector3 p1 = v0 + (side * flatV1.x) + (sidePerp * flatV1.y);
			Vector3 p2 = v0 + (side * flatV2.x) + (sidePerp * flatV2.y);
			// Debug.DrawLine(p0, p1, Color.red, 10000);
			// Debug.DrawLine(p1, p2, Color.green, 10000);
			// Debug.DrawLine(p2, p0, Color.blue, 10000);

			flatV1 *= triScale;
			flatV2 *= triScale;

			// Find bounds.
			Vector2 min = Vector2.Min(flatV0, Vector2.Min(flatV1, flatV2));
			Vector2 max = Vector2.Max(new Vector2(float.MinValue, float.MinValue), Vector2.Max(flatV1, flatV2));

			float width = max.x - min.x;

			TriangleRect tr = new TriangleRect();
			tr.id = i;
			tr.v0 = flatV0 - min;
			tr.v1 = flatV1 - min;
			tr.v2 = flatV2 - min;
			float packPadding = 1.0f / 4096.0f * 2.0f;
			tr.rect = new Rect(0, 0, max.x - min.x + packPadding, max.y - min.y + packPadding);
			triRects[i] = tr;

			if (packOffset.x + width > 1.0f) {
				packOffset.x = 0.0f;
				packOffset.y += maxY;
				maxY = 0.0f;
			}

			if (max.y > maxY) {
				maxY = max.y;
			}

			packOffset -= min;

			// uvs[sourceTris[i * 3 + 0]] = flatV0 + packOffset;
			// uvs[sourceTris[i * 3 + 1]] = flatV1 + packOffset;
			// uvs[sourceTris[i * 3 + 2]] = flatV2 + packOffset;

			packOffset.x += max.x;

			

			//packOffset = Vector2.zero;

			// packOffset.y += triSize + padding + triPad;

			// if (packOffset.y > 1.0f - triSize - triPad - padding) {
			// 	packOffset.x += triSize + padding + triPad;
			// 	packOffset.y = padding;
			// }
		}
		timer.Stop("Flatten");

		// Mesh debugMesh = new Mesh();
		// debugMesh.indexFormat = IndexFormat.UInt32;
		// debugMesh.SetVertices(debugMeshVerts);
		// debugMesh.SetColors(debugeMeshColors);
		// debugMesh.SetIndices(debugMeshIndices, MeshTopology.Lines, 0, true);

		// GameObject debugMeshGob = new GameObject();
		// debugMeshGob.AddComponent<MeshFilter>().mesh = debugMesh;
		// debugMeshGob.AddComponent<MeshRenderer>().material = core.singleton.BasicUnlitMaterial;
		
		//------------------------------------------------------------------------------------------------------------------------------
		// Pack rects.
		//------------------------------------------------------------------------------------------------------------------------------
		// NOTE: Packing algo: https://observablehq.com/@mourner/simple-rectangle-packing
		// Sort in height descending.
		timer.Start();
		Array.Sort<TriangleRect>(triRects, delegate(TriangleRect A, TriangleRect B) {
			return A.rect.height < B.rect.height ? 1 : -1;
		});
		timer.Stop("Sort");

		float totalRectArea = 0.0f;

		for (int i = 0; i < triRects.Length; ++i) {
			Rect r = triRects[i].rect;
			float area = r.width * r.height;
			totalRectArea += area;
		}

		if (totalRectArea > 1.0f) {
			Debug.LogError("React area is greater than 100%");
		}

		Debug.Log("Rect area: " + totalRectArea + " Tri area: " + (totalRectArea / 2.0f));

		List<Rect> spaces = new List<Rect>();

		spaces.Add(new Rect(0, 0, 1.0f, 1.0f));

		timer.Start();
		for (int i = 0; i < triRects.Length; ++i) {
			//Debug.Log("Tri: " + i + " " + spaces.Count);
			Rect box = triRects[i].rect;

			for (int j = spaces.Count - 1; j >= 0; --j) {
				//Debug.Log("Space: " + j + spaces.Count);
				Rect space = spaces[j];

				//Debug.Log("Prog: " + j);

				if (box.width > space.width || box.height > space.height) {
					continue;
				}

				triRects[i].rect.x = space.x;
				triRects[i].rect.y = space.y;

				if (box.width == space.width && box.height == space.height) {
					Rect last = spaces[spaces.Count - 1];

					//Debug.Log("Remove " + (spaces.Count - 1) + " : " + spaces.Count);
					spaces.RemoveAt(spaces.Count - 1);
					if (j < spaces.Count) spaces[j] = last;
				} else if (box.height == space.height) {
					space.x += box.width;
					space.width -= box.width;

					//if (j < spaces.Count) {
						spaces[j] = space;
					//}
				} else if (box.width == space.width) {
					space.y += box.height;
					space.height -= box.height;
					
					//if (j < spaces.Count) {
						spaces[j] = space;
					//}
				} else {
					spaces.Add(new Rect(space.x + box.width, space.y, space.width - box.width, box.height));
					space.y += box.height;
					space.height -= box.height;

					//if (j < spaces.Count) {
						spaces[j] = space;
					//}
				}
				break;
			}
		}
		timer.Stop("Pack");

		timer.Start();
		meshInfo.vertUvsUnique = new Vector2[triCount * 3];

		for (int i = 0; i < triRects.Length; ++i) {
			TriangleRect tr = triRects[i];
			meshInfo.vertUvsUnique[meshInfo.triangles[tr.id * 3 + 0]] = tr.v0 + tr.rect.min;
			meshInfo.vertUvsUnique[meshInfo.triangles[tr.id * 3 + 1]] = tr.v1 + tr.rect.min;
			meshInfo.vertUvsUnique[meshInfo.triangles[tr.id * 3 + 2]] = tr.v2 + tr.rect.min;
		}

		timer.Stop("UV copy");
		
		// Model.uv = uvs;
	}

	public void RenderUniqueUvs(Mesh mesh) {
		CommandBuffer cmdBuffer = new CommandBuffer();
		cmdBuffer.name = "Render tris";
		cmdBuffer.SetRenderTarget(core.singleton.UvTriRt);
		cmdBuffer.ClearRenderTarget(true, true, new Color(0.5f, 0.5f, 0.5f, 1.0f), 1.0f);
		cmdBuffer.DrawMesh(mesh, Matrix4x4.identity, core.singleton.UvTriMat, 0, -1);
		Graphics.ExecuteCommandBuffer(cmdBuffer);
	}

	// Creates a coverage map that uses non-machine aware view points.
	private void _GenerateCoverageMap(Mesh visibilityMesh) {
		coverageResultRt = new RenderTexture(4096, 4096, 0, RenderTextureFormat.ARGB32);
		coverageResultRt.filterMode = FilterMode.Point;
		coverageResultRt.enableRandomWrite = true;
		coverageResultRt.useMipMap = false;
		coverageResultRt.Create();

		// RenderTexture coverageBufferRt = new RenderTexture(4096, 4096, 24, RenderTextureFormat.ARGB32);

		ComputeShader coverageCompute = core.singleton.QuantaScoreCompute;

		int kernelIndex = coverageCompute.FindKernel("CSMain");
		// coverageCompute.SetTexture(kernelIndex, "Source", coverageBufferRt);
		coverageCompute.SetTexture(kernelIndex, "Source", core.singleton.QuantaBuffer);
		coverageCompute.SetTexture(kernelIndex, "Result", coverageResultRt);

		Vector3[] visibilityViews = core.PointsOnSphere(3000);

		for (int i = 0; i < visibilityViews.Length; ++i) {
			Vector3 dir = visibilityViews[i];

			Matrix4x4 camViewMat = Matrix4x4.TRS(dir * 20.0f, Quaternion.LookRotation(-dir, Vector3.up), Vector3.one);
			Vector3 camVisForward = camViewMat.MultiplyVector(Vector3.forward);
			camViewMat = camViewMat.inverse;

			CommandBuffer cmdBuffer = new CommandBuffer();
			cmdBuffer.name = "Render ortho depth";

			Matrix4x4 scaleMatrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, new Vector3(1, 1, -1));
			Matrix4x4 projMat = Matrix4x4.Ortho(-15, 15, -15, 15, 1.0f, 40.0f);
			Matrix4x4 realProj = GL.GetGPUProjectionMatrix(projMat, true);
			
			Matrix4x4 rotMatX = Matrix4x4.Rotate(Quaternion.Euler(0, 0, 0));
			Matrix4x4 rotMatY = Matrix4x4.Rotate(Quaternion.Euler(0, 0, 0));
			Matrix4x4 offsetMat = Matrix4x4.Translate(new Vector3(0, -4.75f, 0));
			Matrix4x4 modelMat = offsetMat * rotMatX * rotMatY;
			
			// Render depth.
			cmdBuffer.SetRenderTarget(core.singleton.OrthoDepthRt);
			cmdBuffer.SetViewport(new Rect(0, 0, 4096, 4096));
			cmdBuffer.ClearRenderTarget(true, true, new Color(0.0f, 0, 0, 1.0f), 1.0f);
			cmdBuffer.SetGlobalMatrix("vpMat", realProj * (scaleMatrix * camViewMat));
			cmdBuffer.DrawMesh(visibilityMesh, modelMat, core.singleton.OrthoDepthMat);
			// cmdBuffer.DrawRenderer(Blockers.GetComponent<MeshRenderer>(), core.singleton.OrthoDepthMat);
			Graphics.ExecuteCommandBuffer(cmdBuffer);
			cmdBuffer.Release();

			// Render quanta scores in UV space with normal angle and depth visibility.
			core.singleton.QuantaVisMat.SetTexture("_orthoDepthTex", core.singleton.OrthoDepthRt);
			CommandBuffer qCmdBuffer = new CommandBuffer();
			qCmdBuffer.name = "Render quanta buffer";
			qCmdBuffer.SetRenderTarget(core.singleton.QuantaBuffer);
			qCmdBuffer.ClearRenderTarget(true, true, new Color(1.0f, 0.0f, 0.1f, 0.0f), 1.0f);
			qCmdBuffer.SetGlobalMatrix("vpMat", realProj * (scaleMatrix * camViewMat));
			qCmdBuffer.SetGlobalMatrix("viewMat", scaleMatrix * camViewMat);
			qCmdBuffer.SetGlobalMatrix("modelMat", modelMat);
			// qCmdBuffer.SetGlobalVector("viewForward", -VisCam.forward);
			qCmdBuffer.SetGlobalVector("viewForward", -camVisForward);
			qCmdBuffer.DrawMesh(visibilityMesh, modelMat, core.singleton.QuantaVisMat, 0, -1);
			Graphics.ExecuteCommandBuffer(qCmdBuffer);
			qCmdBuffer.Release();

			// Accumulate quanta scores.
			coverageCompute.Dispatch(kernelIndex, 4096 / 8, 4096 / 8, 1);
			// coverageCompute.Dispatch(kernelIndex, 8192 / 8, 8192 / 8, 1);
		}

		// coverageBufferRt.Release();

		// CommandBuffer temp = new CommandBuffer();
		// temp.SetRenderTarget(coverageResultRt);
		// temp.ClearRenderTarget(true, true, new Color(1, 0, 0, 1), 1.0f);
		// Graphics.ExecuteCommandBuffer(temp);
	}

	public void Process(float scale, Vector3 translate) {
		ProfileTimer stageProfiler = new ProfileTimer();

		MeshInfo procMesh = initialMeshInfo.Clone();

		procMesh.Scale(scale);
		procMesh.Translate(translate);

		Vector3[] visibilityViews = core.PointsOnSphere(1000);

		stageProfiler.Start();
		procMesh = _GenerateVisibiltyMesh(procMesh, visibilityViews);
		stageProfiler.Stop("Total generate visibility mesh");

		// GenerateDisplayMesh(procMesh, core.singleton.VisMat); // For triId view.
		GenerateDisplayMesh(procMesh, core.singleton.BasicMaterial);
		// GenerateDisplayWireMesh(procMesh, core.singleton.BasicWireMaterial);
		// GenerateDisplayViews(visibilityViews);

		processedMeshInfo = procMesh;
	}

	public void ProcessB() {
		// Create unique tri UV space.
		// Create puffy mesh.
		// Calculate machine aware coverage quality.

		_CreateUniqueUVs(processedMeshInfo);
		Mesh uniqueUvMesh = GenerateMesh(processedMeshInfo, true, true);
		RenderUniqueUvs(uniqueUvMesh);
		_GenerateCoverageMap(uniqueUvMesh);

		_ShowCoveragePreviewMesh(uniqueUvMesh);

		// GameObject.Destroy(uniqueUvMesh);
	}

	public void ProcessC() {
		// Calculate machine positions for total coverage.

	}

	public void ShowImportedMesh() {
		GenerateDisplayMesh(initialMeshInfo, core.singleton.BasicMaterial);
	}

	public void ProcessQuadRemeshProjectionStrategy() {
		processedMeshInfo = CreateUniqueUvsQuadMesh(initialMeshInfo);
		// GenerateDisplayMesh(processedMeshInfo, core.singleton.BasicMaterial);
		Mesh uniqueUvMesh = GenerateMesh(processedMeshInfo, true, true);
		_GenerateCoverageMap(uniqueUvMesh);
		_ShowCoveragePreviewMesh(uniqueUvMesh);
	}
}
