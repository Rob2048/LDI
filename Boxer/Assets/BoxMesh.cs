using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class BoxMesh : MonoBehaviour {
	public RenderTexture depthMap;

	public GameObject TargetUpdateGob;

	private Matrix4x4 _currentTransMat;

	private Mesh _boxMesh;
	private Vector3[] _verts;
	private Vector3[] _norms;

	private Texture2D _readbackTexture;

	private int res;
	private int countX;
	private int countY;
	private float size;

	private float _lastUpdate;


	void Start() {
		_currentTransMat = Matrix4x4.identity;

		_readbackTexture = new Texture2D(depthMap.width, depthMap.height, UnityEngine.Experimental.Rendering.GraphicsFormat.R32_SFloat, UnityEngine.Experimental.Rendering.TextureCreationFlags.None);

		_boxMesh = new Mesh();
		_boxMesh.indexFormat = UnityEngine.Rendering.IndexFormat.UInt32;

		float boxSize = 12;
		res = 1024;
		countX = res;
		countY = res;
		size = boxSize / res;
		int totalCells = countX * countY;
		int totalTris = totalCells * 2;
		int totalVerts = (countX + 1) * (countY + 1);
		int totalIndices = totalTris * 3;

		_verts = new Vector3[totalVerts];
		_norms = new Vector3[totalVerts];
		int[] indices = new int[totalIndices];

		// Verts.
		for (int iY = 0; iY < countY + 1; ++iY) {
			for (int iX = 0; iX < countX + 1; ++iX) {
				_verts[iY * (countX + 1) + iX] = new Vector3(iX * -size, iY * size, 0);
			}
		}
		
		// Tris.
		for (int iY = 0; iY < countY; ++iY) {
			for (int iX = 0; iX < countX; ++iX) {
				int v0 = iY * (countX + 1) + iX;
				int v1 = iY * (countX + 1) + iX + 1;
				int v2 = (iY + 1) * (countX + 1) + iX + 1;
				int v3 = (iY + 1) * (countX + 1) + iX;

				int indId = (iY * countX + iX) * 6;

				indices[indId + 0] = v0;
				indices[indId + 1] = v1;
				indices[indId + 2] = v2;

				indices[indId + 3] = v2;
				indices[indId + 4] = v3;
				indices[indId + 5] = v0;
			}
		}

		_boxMesh.SetVertices(_verts);
		_boxMesh.SetNormals(_norms);
		_boxMesh.SetIndices(indices, MeshTopology.Triangles, 0);
		_boxMesh.RecalculateNormals();

		GetComponent<MeshFilter>().mesh = _boxMesh;
	}

	void Update() {

		_lastUpdate += Time.deltaTime;

		if (_lastUpdate > 1.0f) {

			// if (_currentTransMat != TargetUpdateGob.transform.localToWorldMatrix) {
				_currentTransMat = TargetUpdateGob.transform.localToWorldMatrix;
				_lastUpdate = 0.0f;

				RenderTexture currentActiveRT = RenderTexture.active;
				RenderTexture.active = depthMap;
				_readbackTexture.ReadPixels(new Rect(0, 0, _readbackTexture.width, _readbackTexture.height), 0, 0);
				RenderTexture.active = currentActiveRT;

				// Verts.
				for (int iY = 0; iY < countY + 1; ++iY) {
					for (int iX = 0; iX < countX + 1; ++iX) {
						Color depth = _readbackTexture.GetPixel((_readbackTexture.width / countX) * iX, (_readbackTexture.height / countY) * iY);
						_verts[iY * (countX + 1) + iX].z = depth.r * 20.0f;
					}
				}
				_boxMesh.SetVertices(_verts);
				_boxMesh.RecalculateNormals(UnityEngine.Rendering.MeshUpdateFlags.DontValidateIndices | UnityEngine.Rendering.MeshUpdateFlags.DontRecalculateBounds);
			// }
		}
	}
}
