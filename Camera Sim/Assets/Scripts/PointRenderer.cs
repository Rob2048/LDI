using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using UnityEngine;

using Debug = UnityEngine.Debug;

public class PointRenderer {
	public GameObject gob;
	public Mesh mesh;
	
	public void Init(string Name, Material BillboardMaterial, Vector3 Position) {
		gob = new GameObject(Name);
		gob.transform.position = Position;

		MeshRenderer meshRenderer = gob.AddComponent<MeshRenderer>();
		MeshFilter meshFilter = gob.AddComponent<MeshFilter>();

		meshRenderer.material = BillboardMaterial;
	}

	public void Generate(List<Vector3> Points, List<Color32> Colors) {
		int pointCount = Points.Count;
		Vector3[] verts = new Vector3[pointCount * 4];
		Color32[] cols = new Color32[pointCount * 4];
		Vector2[] uvs = new Vector2[pointCount * 4];
		int[] tris = new int[pointCount * 2 * 3];

		Color32 white = new Color32(255, 255, 255, 255);

		for (int i = 0; i < pointCount; ++i) {
			tris[i * 2 * 3 + 0] = i * 4 + 0;
			tris[i * 2 * 3 + 1] = i * 4 + 1;
			tris[i * 2 * 3 + 2] = i * 4 + 3;

			tris[i * 2 * 3 + 3] = i * 4 + 1;
			tris[i * 2 * 3 + 4] = i * 4 + 2;
			tris[i * 2 * 3 + 5] = i * 4 + 3;

			verts[i * 4 + 0] = Points[i];
			verts[i * 4 + 1] = Points[i];
			verts[i * 4 + 2] = Points[i];
			verts[i * 4 + 3] = Points[i];

			uvs[i * 4 + 0] = new Vector2(-1, -1);
			uvs[i * 4 + 1] = new Vector2(1, -1);
			uvs[i * 4 + 2] = new Vector2(1, 1);
			uvs[i * 4 + 3] = new Vector2(-1, 1);

			cols[i * 4 + 0] = Colors[i];
			cols[i * 4 + 1] = Colors[i];
			cols[i * 4 + 2] = Colors[i];
			cols[i * 4 + 3] = Colors[i];
		}

		mesh = new Mesh();
		mesh.indexFormat = UnityEngine.Rendering.IndexFormat.UInt32;
		mesh.SetVertices(verts);
		mesh.SetUVs(0, uvs);
		mesh.SetColors(cols);
		mesh.SetTriangles(tris, 0);

		gob.GetComponent<MeshFilter>().mesh = mesh;
	}

	public void Destroy() {
		Object.Destroy(gob);

		if (mesh != null) {
			Object.Destroy(mesh);
		}
	}
}
