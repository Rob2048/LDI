using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Diagnostics;
using UnityEngine.Rendering;

using Debug = UnityEngine.Debug;

public class MeshInfo {
	public Vector3[] vertPositions;
	public Vector3[] vertNormals;
	public Vector2[] vertUvs;
	public Color32[] vertColors;
	public int[] triangles;

	public MeshInfo Clone() {
		MeshInfo result = new MeshInfo();

		if (vertPositions != null) result.vertPositions = (Vector3[])vertPositions.Clone();
		if (vertUvs != null) result.vertUvs = (Vector2[])vertUvs.Clone();
		if (vertColors != null) result.vertColors = (Color32[])vertColors.Clone();
		if (vertNormals != null) result.vertNormals = (Vector3[])vertNormals.Clone();
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

	public void GenerateDisplayMesh(MeshInfo meshInfo, Material material) {
		if (previewGob != null) {
			GameObject.Destroy(previewGob);
		}

		Mesh result = new Mesh();
		result.indexFormat = UnityEngine.Rendering.IndexFormat.UInt32;
		result.SetVertices(meshInfo.vertPositions);
		result.SetUVs(0, meshInfo.vertUvs);
		result.SetTriangles(meshInfo.triangles, 0);

		if (meshInfo.vertColors != null) {
			result.SetColors(meshInfo.vertColors);
		}

		result.RecalculateBounds();
		result.RecalculateNormals();

		previewGob = new GameObject();
		MeshRenderer renderer = previewGob.AddComponent<MeshRenderer>();
		renderer.material = material;

		if (texture != null) {
			renderer.material.SetTexture("_MainTex", texture);
		}

		previewGob.AddComponent<MeshFilter>().mesh = result;
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

		cmdBuffer.Release();
	}

	private MeshInfo _GenerateVisibiltyMesh(MeshInfo meshInfo, Vector3[] views) {
		int sourceTriCount = meshInfo.triangles.Length / 3;
		int checklistSize = 512 * 512;

		if (sourceTriCount > checklistSize) {
			Debug.LogError("Source mesh has too many faces for mesh visibility check.");
			return null;
		}

		meshInfo.BuildUniqueTriangleIds();

		Mesh triIdMesh = GenerateMesh(meshInfo);

		int[] visChecklist = new int[checklistSize];
		ComputeBuffer checklistBuffer = new ComputeBuffer(visChecklist.Length, 4);
		checklistBuffer.SetData(visChecklist);

		for (int i = 0; i < views.Length; ++i) {
			Vector3 dir = views[i];

			Matrix4x4 camViewMat = Matrix4x4.TRS(dir * 20.0f, Quaternion.LookRotation(-dir, Vector3.up), Vector3.one);
			camViewMat = camViewMat.inverse;

			_RenderVisibilityMeshView(triIdMesh, camViewMat, checklistBuffer);
		}

		checklistBuffer.GetData(visChecklist);

		int triCount = 0;

		for (int i = 0; i < visChecklist.Length; ++i) {
			if (visChecklist[i] != 0) {
				++triCount;
			}
		}

		Debug.Log(triCount + "/" + sourceTriCount);

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

	public void GenerateDisplayPoints(Vector3[] views) {
		for (int i = 0; i < views.Length; ++i) {
			GameObject viewMarker = GameObject.Instantiate(core.singleton.SampleLocatorPrefab, views[i] * 20.0f, Quaternion.LookRotation(-views[i], Vector3.up));
		}
	}

	public void Process(float scale, Vector3 translate) {
		ProfileTimer segmentProfiler = new ProfileTimer();

		MeshInfo procMesh = initialMeshInfo.Clone();

		procMesh.Scale(scale);
		procMesh.Translate(translate);

		Vector3[] visibilityViews = core.PointsOnSphere(1000);

		segmentProfiler.Start();
		procMesh = _GenerateVisibiltyMesh(procMesh, visibilityViews);
		segmentProfiler.Stop("Total generate visibility mesh");

		// GenerateDisplayMesh(procMesh, core.singleton.VisMat); // For triId view.
		GenerateDisplayMesh(procMesh, core.singleton.BasicMaterial);
		GenerateDisplayWireMesh(procMesh, core.singleton.BasicWireMaterial);
		GenerateDisplayPoints(visibilityViews);

		processedMeshInfo = procMesh;
	}

	public void ShowImportedMesh() {
		GenerateDisplayMesh(initialMeshInfo, core.singleton.BasicMaterial);
	}
}
