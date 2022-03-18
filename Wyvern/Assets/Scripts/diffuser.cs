using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Text;
using System.Diagnostics;
using System.Threading;
using UnityEngine;

using Debug = UnityEngine.Debug;

public struct SurfelComputeData {
	Vector3 position;
	Vector3 normal;
}

public static class DebugDrawTools {
	public static void DrawBounds(Bounds B, Vector3 Offset, Color C) {
		Vector3 c0 = Offset + new Vector3(B.min.x, B.min.y, B.min.z);
		Vector3 c1 = Offset + new Vector3(B.max.x, B.min.y, B.min.z);
		Vector3 c2 = Offset + new Vector3(B.max.x, B.min.y, B.max.z);
		Vector3 c3 = Offset + new Vector3(B.min.x, B.min.y, B.max.z);

		Vector3 c4 = Offset + new Vector3(B.min.x, B.max.y, B.min.z);
		Vector3 c5 = Offset + new Vector3(B.max.x, B.max.y, B.min.z);
		Vector3 c6 = Offset + new Vector3(B.max.x, B.max.y, B.max.z);
		Vector3 c7 = Offset + new Vector3(B.min.x, B.max.y, B.max.z);

		Debug.DrawLine(c0, c1, C, 10000.0f);
		Debug.DrawLine(c1, c2, C, 10000.0f);
		Debug.DrawLine(c2, c3, C, 10000.0f);
		Debug.DrawLine(c3, c0, C, 10000.0f);

		Debug.DrawLine(c4, c5, C, 10000.0f);
		Debug.DrawLine(c5, c6, C, 10000.0f);
		Debug.DrawLine(c6, c7, C, 10000.0f);
		Debug.DrawLine(c7, c4, C, 10000.0f);

		Debug.DrawLine(c0, c4, C, 10000.0f);
		Debug.DrawLine(c1, c5, C, 10000.0f);
		Debug.DrawLine(c2, c6, C, 10000.0f);
		Debug.DrawLine(c3, c7, C, 10000.0f);
	}
}

public class SpatialGridCell {
	public List<int> Ids;
}

public class SpatialGrid {
	public struct CellBoundsResult {
		public Vector3Int min;
		public Vector3Int max;
	}

	public float cellSize = 0.025f;
	
	public int cellCountX = 0;
	public int cellCountY = 0;
	public int cellCountZ = 0;
	public int totalCells = 0;

	public Vector3 offset;
	public Bounds bounds;

	public SpatialGridCell[] spatialGridCells;

	public SpatialGrid() {
	}

	public void Init(Bounds B) {
		bounds = B;
		// NOTE: This expansion makes sure no points are in the absolute border cells.
		bounds.Expand(cellSize * 2);
		Vector3 size = bounds.size;

		cellCountX = (int)Mathf.Ceil(size.x / cellSize);
		cellCountY = (int)Mathf.Ceil(size.y / cellSize);
		cellCountZ = (int)Mathf.Ceil(size.z / cellSize);

		Vector3 newSize = new Vector3(cellCountX * cellSize, cellCountY * cellSize, cellCountZ * cellSize);
		Vector3 diffSize = newSize - size;
		bounds.Expand(diffSize);

		offset = bounds.min;

		totalCells = cellCountX * cellCountY * cellCountZ;

		spatialGridCells = new SpatialGridCell[totalCells];

		for (int i = 0; i < totalCells; ++i) {
			spatialGridCells[i] = new SpatialGridCell();
			spatialGridCells[i].Ids = new List<int>();
		}
	}

	public void AddPoint(Vector3 Pos, int Id) {
		// Convert position into spatial cell position.
		int cX = (int)((Pos.x - offset.x) / cellSize);
		int cY = (int)((Pos.y - offset.y) / cellSize);
		int cZ = (int)((Pos.z - offset.z) / cellSize);

		spatialGridCells[cZ * cellCountY * cellCountX + cY * cellCountX + cX].Ids.Add(Id);
	}

	public SpatialGridCell GetCell(int X, int Y, int Z) {
		return spatialGridCells[Z * cellCountY * cellCountX + Y * cellCountX + X];
	}

	public CellBoundsResult GetCellBounds(Vector3 Pos) {
		int sX = (int)((Pos.x - offset.x - cellSize / 2.0f) / cellSize);
		int eX = sX + 1;
		// sX = Mathf.Clamp(sX, 0, cellCountX - 1);
		// eX = Mathf.Clamp(eX, 0, cellCountX - 1);

		int sY = (int)((Pos.y - offset.y - cellSize / 2.0f) / cellSize);
		int eY = sY + 1;
		// sY = Mathf.Clamp(sY, 0, cellCountY - 1);
		// eY = Mathf.Clamp(eY, 0, cellCountY - 1);

		int sZ = (int)((Pos.z - offset.z - cellSize / 2.0f) / cellSize);
		int eZ = sZ + 1;
		// sZ = Mathf.Clamp(sZ, 0, cellCountZ - 1);
		// eZ = Mathf.Clamp(eZ, 0, cellCountZ - 1);

		CellBoundsResult result = new CellBoundsResult();
		result.min = new Vector3Int(sX, sY, sZ);
		result.max = new Vector3Int(eX, eY, eZ);

		return result;
	}

	public void GetCellBoundsList(Vector3 Pos, SpatialGridCell[] Cells) {
		int sX = (int)((Pos.x - offset.x - cellSize / 2.0f) / cellSize);
		int eX = sX + 1;
		
		int sY = (int)((Pos.y - offset.y - cellSize / 2.0f) / cellSize);
		int eY = sY + 1;
		
		int sZ = (int)((Pos.z - offset.z - cellSize / 2.0f) / cellSize);
		int eZ = sZ + 1;

		Cells[0] = spatialGridCells[sZ * cellCountY * cellCountX + sY * cellCountX + sX];
		Cells[1] = spatialGridCells[sZ * cellCountY * cellCountX + sY * cellCountX + eX];
		Cells[2] = spatialGridCells[sZ * cellCountY * cellCountX + eY * cellCountX + sX];
		Cells[3] = spatialGridCells[sZ * cellCountY * cellCountX + eY * cellCountX + eX];

		Cells[4] = spatialGridCells[eZ * cellCountY * cellCountX + sY * cellCountX + sX];
		Cells[5] = spatialGridCells[eZ * cellCountY * cellCountX + sY * cellCountX + eX];
		Cells[6] = spatialGridCells[eZ * cellCountY * cellCountX + eY * cellCountX + sX];
		Cells[7] = spatialGridCells[eZ * cellCountY * cellCountX + eY * cellCountX + eX];
	}

	public void GetCellIdsList(Vector3 Pos, int[] Cells) {
		int sX = (int)((Pos.x - offset.x - cellSize / 2.0f) / cellSize);
		int eX = sX + 1;
		
		int sY = (int)((Pos.y - offset.y - cellSize / 2.0f) / cellSize);
		int eY = sY + 1;
		
		int sZ = (int)((Pos.z - offset.z - cellSize / 2.0f) / cellSize);
		int eZ = sZ + 1;

		Cells[0] = sZ * cellCountY * cellCountX + sY * cellCountX + sX;
		Cells[1] = sZ * cellCountY * cellCountX + sY * cellCountX + eX;
		Cells[2] = sZ * cellCountY * cellCountX + eY * cellCountX + sX;
		Cells[3] = sZ * cellCountY * cellCountX + eY * cellCountX + eX;

		Cells[4] = eZ * cellCountY * cellCountX + sY * cellCountX + sX;
		Cells[5] = eZ * cellCountY * cellCountX + sY * cellCountX + eX;
		Cells[6] = eZ * cellCountY * cellCountX + eY * cellCountX + sX;
		Cells[7] = eZ * cellCountY * cellCountX + eY * cellCountX + eX;
	}

	public void DrawDebug() {
		DebugDrawTools.DrawBounds(bounds, Vector3.zero, Color.red);

		for (int iX = 0; iX < cellCountX; ++iX) {
			for (int iY = 0; iY < cellCountY; ++iY) {
				for (int iZ = 0; iZ < cellCountZ; ++iZ) {
					Bounds b = new Bounds(new Vector3(iX * cellSize + cellSize * 0.5f, iY * cellSize + cellSize * 0.5f, iZ * cellSize + cellSize * 0.5f) + offset, new Vector3(cellSize, cellSize, cellSize));
					DebugDrawTools.DrawBounds(b, Vector3.zero, Color.blue);

					SpatialGridCell c = GetCell(iX, iY, iZ);

					// Debug.Log(iX + " "  + iY + " " + iZ + ": " + c.Ids.Count);
				}
			}
		}
	}
}

public struct FastSurfel {
	public int id; // ID back to original surfel.
	public int cellId;
	public Vector3 pos;
	public float totalweight;
	public int neighbourStartIdx;
	public int neighbourCount;
	public Vector3 normal;
	public float scale;
	public float continuous;
	public int discrete;
	public float percep;
}

public struct FastSpatialCell {
	public int startIdx;
	public int count;
}

public struct FastNeighour {
	public int id;
	public float weight;
}

public struct SurfelChannel {
	public float continuous;
	public int discrete;
}

public class SurfelNeighbour {
	public Surfel surfel;
	public float weight;
}

public class Surfel {
	public Vector3 pos;
	public Vector3 normal;
	public float scale;
	
	// Per channel. (Could be in external array).
	//public SurfelChannel[] channels = new SurfelChannel[4];
	public float continuous;
	public int discrete;
	public float percep;

	// List of neighbours.
	// public int neighbourStartIdx;
	// public int neighbourCount;

	public List<SurfelNeighbour> neighbours;
	public float totalWeight;
	public float weight;
}

public class Diffuser {
	public List<Surfel> surfels;
	public SpatialGrid spatialGrid;
	public core unityInterface;

	public GameObject debugGob;

	// Fast surfels.
	FastSurfel[] fastSurfels;
	FastSpatialCell[] fastCells;
	FastNeighour[] fastNeighbours;

	public Diffuser(core UnityInterface) {
		unityInterface = UnityInterface;
		surfels = new List<Surfel>();
	}

	public void Init() {
		// Color32[] srcRaw = Source.GetPixels32();

		// float scale = 0.05f;
		// float spacing = scale;

		// for (int iY = 0; iY < Source.height; ++iY) {
		// 	for (int iX = 0; iX < Source.width; ++iX) {
		// 		int sIdx = (iY) * Source.width + (iX);
		// 		float lum = srcRaw[sIdx].r * 0.2126f + srcRaw[sIdx].g * 0.7152f + srcRaw[sIdx].b * 0.0722f;
		// 		lum /= 255.0f;

		// 		// if (iY < 400) {
		// 		// 	s.pos = new Vector3(iX * spacing, 0, iY * spacing);
		// 		// 	s.normal = Vector3.up;
		// 		// } else {
		// 		// 	s.pos = new Vector3(iX * spacing, (iY - 400) * spacing, 400 * spacing);
		// 		// 	s.normal = Vector3.back;
		// 		// }

		// 		{
		// 			Surfel s = new Surfel();
		// 			s.scale = scale;
		// 			s.continuous = lum;
		// 			surfels.Add(s);

		// 			float x = Mathf.Sin(((float)iX / Source.width) * Mathf.PI * 1);
		// 			float z = Mathf.Cos(((float)iX / Source.width) * Mathf.PI * 1);

		// 			float circumference = Source.width * spacing * 2.0f;
		// 			float r = circumference / Mathf.PI / 2.0f;

		// 			s.pos = new Vector3(x * r, iY * spacing, z * r);
		// 			s.normal = new Vector3(x, 0, z).normalized;
		// 		}

		// 		// {
		// 		// 	Surfel s = new Surfel();
		// 		// 	s.scale = scale;
		// 		// 	s.continuous = lum;
		// 		// 	surfels.Add(s);

		// 		// 	float x = Mathf.Sin(((float)iX / Source.width) * Mathf.PI * 1);
		// 		// 	float z = Mathf.Cos(((float)iX / Source.width) * Mathf.PI * 1);

		// 		// 	float circumference = Source.width * spacing * 2.0f;
		// 		// 	float r = circumference / Mathf.PI / 2.0f;

		// 		// 	s.pos = new Vector3(x * r, z * r + 10.0f, iY * spacing);
		// 		// 	s.normal = new Vector3(x, z, 0).normalized;
		// 		// }
		// 	}
		// }
	}

	public void AddTexture(Texture2D Source, Vector3 Offset) {
		Color32[] srcRaw = Source.GetPixels32();

		float scale = 0.05f;
		float spacing = scale;

		for (int iY = 0; iY < Source.height; ++iY) {
			for (int iX = 0; iX < Source.width; ++iX) {
				int sIdx = (iY) * Source.width + (iX);
				float lum = srcRaw[sIdx].r * 0.2126f + srcRaw[sIdx].g * 0.7152f + srcRaw[sIdx].b * 0.0722f;
				lum /= 255.0f;

				{
					Surfel s = new Surfel();
					s.scale = scale;
					s.continuous = lum;
					surfels.Add(s);

					float x = Mathf.Sin(((float)iX / Source.width) * Mathf.PI * 1);
					float z = -Mathf.Cos(((float)iX / Source.width) * Mathf.PI * 1);

					float circumference = Source.width * spacing * 2.0f;
					float r = circumference / Mathf.PI / 2.0f;

					s.pos = new Vector3(x * r, iY * spacing, z * r) + Offset;
					s.normal = new Vector3(x, 0, z).normalized;
				}
			}
		}
	}
	
	public void DrawDebugFastSurfels(ComputeBuffer surfelDataBuffer) {
		if (debugGob != null) {
			GameObject.Destroy(debugGob.GetComponent<MeshFilter>().mesh);
			GameObject.Destroy(debugGob);
		}

		debugGob = new GameObject("diffuserDebug");
		MeshFilter mf = debugGob.AddComponent<MeshFilter>();
		MeshRenderer mr = debugGob.AddComponent<MeshRenderer>();
		mr.material = unityInterface.SurfelDebugMat;

		int surfelCount = fastSurfels.Length;

		float[] intensityBuffer = new float[surfelCount];

		Vector3[] verts = new Vector3[surfelCount * 4];
		Vector2[] uvs = new Vector2[surfelCount * 4];
		Color32[] cols = new Color32[surfelCount * 4];
		int[] inds = new int[surfelCount * 6];

		if (surfelCount > 255 * 255 * 255) {
			Debug.LogError("Surfel count exceeds ID bytes");
		}

		for (int i = 0; i < surfelCount; ++i) {
			FastSurfel s = fastSurfels[i];
			float hscale = s.scale * 0.5f;

			// Calculate surfel plane.
			Vector3 normal = s.normal;
			Vector3 tempVec;

			if (normal != Vector3.up && normal != Vector3.down) {
				tempVec = Vector3.Cross(normal, Vector3.up).normalized;
			} else {
				tempVec = Vector3.Cross(normal, Vector3.left).normalized;
			}
			
			Vector3 tangent = Vector3.Cross(normal, tempVec).normalized;
			Vector3 bitangent = Vector3.Cross(tangent, normal).normalized;
			
			Vector3 v0 = tangent * -hscale + bitangent * -hscale;
			Vector3 v1 = tangent * -hscale + bitangent * hscale;
			Vector3 v2 = tangent * hscale + bitangent * hscale;
			Vector3 v3 = tangent * hscale + bitangent * -hscale;

			Vector3 basePos = s.pos + normal * 0.0015f;

			verts[i * 4 + 0] = basePos + v0;
			verts[i * 4 + 1] = basePos + v1;
			verts[i * 4 + 2] = basePos + v2;
			verts[i * 4 + 3] = basePos + v3;

			uvs[i * 4 + 0] = new Vector2(0, 0);
			uvs[i * 4 + 1] = new Vector2(0, 1);
			uvs[i * 4 + 2] = new Vector2(1, 1);
			uvs[i * 4 + 3] = new Vector2(1, 0);

			byte v = (byte)(Mathf.Clamp01(s.discrete) * 255.0f);
			// byte v = (byte)(Mathf.Clamp01(s.continuous) * 255.0f);

			// intensityBuffer[i] = s.discrete;

			// Color32 c = new Color32((byte)(i & 0xFF), (byte)((i >> 8) & 0xFF), (byte)((i >> 16) & 0xFF), 255);
			Color32 c = new Color32(v, v, v, 255);

			// byte r = (byte)(i % 255);
			// byte b = (byte)((i / (255)) % 255);
			// Color32 c = new Color32(r, b, 0, 255);
			
			// UnityEngine.Random.InitState(s.cellId);
			// Color32 c = UnityEngine.Random.ColorHSV(0, 1, 1.0f, 1.0f);

			cols[i * 4 + 0] = c;
			cols[i * 4 + 1] = c;
			cols[i * 4 + 2] = c;
			cols[i * 4 + 3] = c;

			inds[i * 6 + 0] = i * 4 + 0;
			inds[i * 6 + 1] = i * 4 + 1;
			inds[i * 6 + 2] = i * 4 + 2;

			inds[i * 6 + 3] = i * 4 + 0;
			inds[i * 6 + 4] = i * 4 + 2;
			inds[i * 6 + 5] = i * 4 + 3;

			// Debug.DrawLine(s.pos, s.pos + normal * 0.1f, Color.red, float.MaxValue);
			// Debug.DrawLine(s.pos, s.pos + tangent * 0.1f, Color.green, float.MaxValue);
			// Debug.DrawLine(s.pos, s.pos + bitangent * 0.1f, Color.blue, float.MaxValue);
		}

		// ComputeBuffer surfelIntensityBuffer = new ComputeBuffer(surfelCount, 4, ComputeBufferType.Structured);
		// surfelIntensityBuffer.SetData(intensityBuffer);

		Mesh mesh = new Mesh();
		mesh.indexFormat = UnityEngine.Rendering.IndexFormat.UInt32;
		mesh.vertices = verts;
		mesh.uv = uvs;
		mesh.colors32 = cols;
		mesh.triangles = inds;
		
		mf.mesh = mesh;

		mr.material.SetBuffer("surfelData", surfelDataBuffer);
	}

	public Material DrawDebug() {
		if (debugGob != null) {
			GameObject.Destroy(debugGob.GetComponent<MeshFilter>().mesh);
			GameObject.Destroy(debugGob);
		}

		debugGob = new GameObject("diffuserDebug");
		MeshFilter mf = debugGob.AddComponent<MeshFilter>();
		MeshRenderer mr = debugGob.AddComponent<MeshRenderer>();

		mr.material = unityInterface.SurfelDebugMat;

		int surfelCount = surfels.Count;

		Vector3[] verts = new Vector3[surfelCount * 4];
		Vector2[] uvs = new Vector2[surfelCount * 4];
		Color32[] cols = new Color32[surfelCount * 4];
		int[] inds = new int[surfelCount * 6];

		if (surfelCount > 255 * 255 * 255) {
			Debug.LogError("Surfel count exceeds ID bytes");
		}

		for (int i = 0; i < surfelCount; ++i) {
			Surfel s = surfels[i];
			float hscale = s.scale * 0.5f;
			// float hscale = s.scale * 2.0f;

			// Calculate surfel plane.
			Vector3 normal = s.normal;
			Vector3 tempVec;

			if (normal != Vector3.up && normal != Vector3.down) {
				tempVec = Vector3.Cross(normal, Vector3.up).normalized;
			} else {
				tempVec = Vector3.Cross(normal, Vector3.left).normalized;
			}
			
			Vector3 tangent = Vector3.Cross(normal, tempVec).normalized;
			Vector3 bitangent = Vector3.Cross(tangent, normal).normalized;
			
			Vector3 v0 = tangent * -hscale + bitangent * -hscale;
			Vector3 v1 = tangent * -hscale + bitangent * hscale;
			Vector3 v2 = tangent * hscale + bitangent * hscale;
			Vector3 v3 = tangent * hscale + bitangent * -hscale;

			Vector3 basePos = s.pos + normal * 0.0015f;
			// Vector3 basePos = s.pos + normal * 20.0f;

			verts[i * 4 + 0] = basePos + v0;
			verts[i * 4 + 1] = basePos + v1;
			verts[i * 4 + 2] = basePos + v2;
			verts[i * 4 + 3] = basePos + v3;

			uvs[i * 4 + 0] = new Vector2(0, 0);
			uvs[i * 4 + 1] = new Vector2(0, 1);
			uvs[i * 4 + 2] = new Vector2(1, 1);
			uvs[i * 4 + 3] = new Vector2(1, 0);

			byte v = (byte)(Mathf.Clamp01(s.discrete) * 255.0f);
			// byte v = (byte)(Mathf.Clamp01(s.continuous) * 255.0f);

			Color32 c = new Color32(v, v, v, 255);
			
			// Color32 c = new Color32((byte)(i & 0xFF), (byte)((i >> 8) & 0xFF), (byte)((i >> 16) & 0xFF), 255);

			cols[i * 4 + 0] = c;
			cols[i * 4 + 1] = c;
			cols[i * 4 + 2] = c;
			cols[i * 4 + 3] = c;

			inds[i * 6 + 0] = i * 4 + 0;
			inds[i * 6 + 1] = i * 4 + 1;
			inds[i * 6 + 2] = i * 4 + 2;

			inds[i * 6 + 3] = i * 4 + 0;
			inds[i * 6 + 4] = i * 4 + 2;
			inds[i * 6 + 5] = i * 4 + 3;

			// Debug.DrawLine(s.pos, s.pos + normal * 0.1f, Color.red, float.MaxValue);
			// Debug.DrawLine(s.pos, s.pos + tangent * 0.1f, Color.green, float.MaxValue);
			// Debug.DrawLine(s.pos, s.pos + bitangent * 0.1f, Color.blue, float.MaxValue);
		}

		Mesh mesh = new Mesh();
		mesh.indexFormat = UnityEngine.Rendering.IndexFormat.UInt32;
		mesh.vertices = verts;
		mesh.uv = uvs;
		mesh.colors32 = cols;
		mesh.triangles = inds;
		
		mf.mesh = mesh;

		return mr.material;
	}

	public float GetSurfelPercep(Surfel S) {
		float sum = 0.0f;

		for (int i = 0; i < S.neighbours.Count; ++i) {
			sum += S.neighbours[i].weight * S.neighbours[i].surfel.discrete;
		}

		sum += S.weight * S.discrete;

		return sum;
	}

	public float GetSurfelMse(Surfel S) {
		float squareError = 0.0f;
		
		float error = GetSurfelPercep(S) - S.continuous;
		squareError += error * error;

		for (int i = 0; i < S.neighbours.Count; ++i) {
			Surfel n = S.neighbours[i].surfel;
			error = GetSurfelPercep(n) - n.continuous;
			squareError += error * error;
		}

		int count = S.neighbours.Count + 1;
		float mse = squareError / count;

		return mse;
	}

	public float GetFastSurfelPercep(int Id) {
		float sum = 0.0f;

		FastSurfel s = fastSurfels[Id];
		int nS = s.neighbourStartIdx;
		int nE = nS + s.neighbourCount;

		for (int i = nS; i < nE; ++i) {
			FastNeighour n = fastNeighbours[i];
			sum += n.weight * fastSurfels[n.id].discrete;
		}

		return sum;
	}

	public float GetFastSurfelMse(int Id) {
		float squareError = 0.0f;

		FastSurfel s = fastSurfels[Id];
		int nS = s.neighbourStartIdx;
		int nE = nS + s.neighbourCount;
		
		for (int i = nS; i < nE; ++i) {
			float error = GetFastSurfelPercep(fastNeighbours[i].id) - fastSurfels[fastNeighbours[i].id].continuous;
			squareError += error * error;
		}

		int count = s.neighbourCount;
		float mse = squareError / count;

		return mse;
	}

	public float GetFasterSurfelMse(int Id) {
		float squareError = 0.0f;

		FastSurfel s = fastSurfels[Id];
		int nS = s.neighbourStartIdx;
		int nE = nS + s.neighbourCount;
		
		for (int i = nS; i < nE; ++i) {
			int neighId = fastNeighbours[i].id;
			float error = fastSurfels[neighId].percep - fastSurfels[neighId].continuous;
			squareError += error * error;
		}

		int count = s.neighbourCount;
		float mse = squareError / count;

		return mse;
	}

	public float GetFasterSurfelMseMod(int Id, float Value) {
		float squareError = 0.0f;

		FastSurfel s = fastSurfels[Id];
		int nS = s.neighbourStartIdx;
		int nE = nS + s.neighbourCount;
		
		for (int i = nS; i < nE; ++i) {
			int neighId = fastNeighbours[i].id;
			float percep = fastSurfels[neighId].percep;
			percep += (fastNeighbours[i].weight / fastSurfels[neighId].totalweight) * Value;

			float error = percep - fastSurfels[neighId].continuous;
			squareError += error * error;
		}

		int count = s.neighbourCount;
		float mse = squareError / count;

		return mse;
	}

	public void ApplyFasterSurfelMod(int Id, float Value) {
		FastSurfel s = fastSurfels[Id];
		int nS = s.neighbourStartIdx;
		int nE = nS + s.neighbourCount;
		
		for (int i = nS; i < nE; ++i) {
			int neighId = fastNeighbours[i].id;
			fastSurfels[neighId].percep += (fastNeighbours[i].weight / fastSurfels[neighId].totalweight) * Value;
		}
	}

	public void Process() {
		Stopwatch sw = new Stopwatch();
		
		_FindNeighbours();

		//----------------------------------------------------------------------------------------------------------------------------
		// Create random order for DBS.
		//----------------------------------------------------------------------------------------------------------------------------
		int[] orderList = new int[surfels.Count];
		for (int i = 0; i < orderList.Length; ++i) {
			orderList[i] = i;
		}

		// Randomize pairs
		for (int j = 0; j < orderList.Length; ++j) {
			int targetSlot = UnityEngine.Random.Range(0, orderList.Length);

			int tmp = orderList[targetSlot];
			orderList[targetSlot] = orderList[j];
			orderList[j] = tmp;
		}

		//----------------------------------------------------------------------------------------------------------------------------
		// Prepare surfels.
		//----------------------------------------------------------------------------------------------------------------------------
		// Reset surfels.
		for (int i = 0; i < fastSurfels.Length; ++i) {
			fastSurfels[i].discrete = 0;
			fastSurfels[i].percep = 0.0f;
		}

		// Threshold continous set and store in discrete set.
		// for (int i = 0; i < fastSurfels.Length; ++i) {
		// 	if (fastSurfels[i].continuous < 0.5f) {
		// 		fastSurfels[i].discrete = 0;
		// 	} else {
		// 		fastSurfels[i].discrete = 1;
		// 	}
		// }

		//----------------------------------------------------------------------------------------------------------------------------
		// Run DBS.
		//----------------------------------------------------------------------------------------------------------------------------
		sw.Restart();

		int threadCount = 12;
		Thread[] threads = new Thread[threadCount];

		int totalJobs = surfels.Count;
		int jobSize = totalJobs / threadCount;
		int jobsRemaining = totalJobs - jobSize * threadCount;

		int iters = 5;

		for (int i = 0; i < iters; ++i) {
			float totalError = 0.0f;
			int toggleCount = 0;

			//----------------------------------------------------------------------------------------------------------------------------
			// Non parallel.
			//----------------------------------------------------------------------------------------------------------------------------
			// for (int j = 0; j < orderList.Length; ++j) {
			// 	int surfelId = orderList[j];
				
			// 	float currentError = GetFasterSurfelMse(surfelId);

			// 	int initialD = fastSurfels[surfelId].discrete;
				
			// 	if (initialD == 0) {
			// 		float newError = GetFasterSurfelMseMod(surfelId, 1.0f);

			// 		if (newError < currentError) {
			// 			++toggleCount;
			// 			totalError += newError;
			// 			ApplyFasterSurfelMod(surfelId, 1.0f);
			// 			fastSurfels[surfelId].discrete = 1;
			// 		} else {
			// 			totalError += currentError;
			// 		}
			// 	} else {
			// 		float newError = GetFasterSurfelMseMod(surfelId, -1.0f);

			// 		if (newError < currentError) {
			// 			++toggleCount;
			// 			totalError += newError;
			// 			ApplyFasterSurfelMod(surfelId, -1.0f);
			// 			fastSurfels[surfelId].discrete = 0;
			// 		} else {
			// 			totalError += currentError;
			// 		}
			// 	}
			// }

			// Debug.Log("Iter: " + i + " Error: " + Mathf.Sqrt(totalError / orderList.Length) + " Toggles: " + toggleCount);

			//----------------------------------------------------------------------------------------------------------------------------
			// Batch based.
			//----------------------------------------------------------------------------------------------------------------------------
			int batches = 8;
			int pointsPerBatch = (int)Mathf.Ceil(orderList.Length / batches);
			int listStart = 0;

			for (int bI = 0; bI < batches; ++bI) {
				int olS = listStart;
				listStart += pointsPerBatch;
				int olE = olS + pointsPerBatch;
				olE = Mathf.Clamp(olE, 0, orderList.Length);

				// Update perceptual image before each sub iteration.
				for (int t = 0; t < threadCount; ++t) {
					threads[t] = new Thread(_ThreadCalcPercep);

					int startId = t * jobSize;
					int endId = (t + 1) * jobSize;
					
					if (t == threadCount - 1) {
						endId += jobsRemaining;
					}

					object[] args = { startId, endId };
					threads[t].Start(args);
				}
				for (int t = 0; t < threadCount; ++t) {
					threads[t].Join();
				}

				// for (int j = olS; j < olE; ++j) {
				// 	int surfelId = orderList[j];
					
				// 	float currentError = GetFasterSurfelMse(surfelId);

				// 	int initialD = fastSurfels[surfelId].discrete;
					
				// 	if (initialD == 0) {
				// 		float newError = GetFasterSurfelMseMod(surfelId, 1.0f);

				// 		if (newError < currentError) {
				// 			++toggleCount;
				// 			totalError += newError;
				// 			// ApplyFasterSurfelMod(surfelId, 1.0f);
				// 			fastSurfels[surfelId].discrete = 1;
				// 		} else {
				// 			totalError += currentError;
				// 		}
				// 	} else {
				// 		float newError = GetFasterSurfelMseMod(surfelId, -1.0f);

				// 		if (newError < currentError) {
				// 			++toggleCount;
				// 			totalError += newError;
				// 			// ApplyFasterSurfelMod(surfelId, -1.0f);
				// 			fastSurfels[surfelId].discrete = 0;
				// 		} else {
				// 			totalError += currentError;
				// 		}
				// 	}
				// }

				int totalSurfelJobs = olE - olS;
				int surfelJobSize = totalSurfelJobs / threadCount;
				int surfelJobsRemaining = totalSurfelJobs - surfelJobSize * threadCount;

				for (int t = 0; t < threadCount; ++t) {
					threads[t] = new Thread(_ThreadCalcSurfelFlip);

					int startId = olS + t * surfelJobSize;
					int endId = olS + (t + 1) * surfelJobSize;
					
					if (t == threadCount - 1) {
						endId += surfelJobsRemaining;
					}

					object[] args = { startId, endId, orderList };
					threads[t].Start(args);
				}

				for (int t = 0; t < threadCount; ++t) {
					threads[t].Join();
				}
			}

			Debug.Log("Iter: " + i + " Error: " + Mathf.Sqrt(totalError / orderList.Length) + " Toggles: " + toggleCount);

			// if (toggleCount == 0) {
			// 	break;
			// }
		}

		sw.Stop();
		Debug.Log("DBS time: " + sw.Elapsed.TotalMilliseconds + " ms");

		//----------------------------------------------------------------------------------------------------------------------------
		// Create final perceptual image of discrete set.
		//----------------------------------------------------------------------------------------------------------------------------
		for (int i = 0; i < threadCount; ++i) {
			threads[i] = new Thread(_ThreadCalcPercep);

			int startId = i * jobSize;
			int endId = (i + 1) * jobSize;
			
			if (i == threadCount - 1) {
				endId += jobsRemaining;
			}

			object[] args = { startId, endId };
			threads[i].Start(args);
		}

		for (int i = 0; i < threadCount; ++i) {
			threads[i].Join();
		}

		// sw.Restart();
		// for (int i = 0; i < fastSurfels.Length; ++i) {
		// 	FastSurfel s = fastSurfels[i];

		// 	float sum = 0.0f;

		// 	int nS = s.neighbourStartIdx;
		// 	int nE = nS + s.neighbourCount;

		// 	for (int j = nS; j < nE; ++j) {
		// 		FastNeighour n = fastNeighbours[j];

		// 		sum += fastSurfels[n.id].discrete * n.weight;
		// 	}

		// 	fastSurfels[i].percep = sum / fastSurfels[i].totalweight;

		// 	if (sum > 1.001f) {
		// 		Debug.LogError("Percep over 1.0 " + sum);
		// 	}
		// }

		// sw.Stop();
		// Debug.Log("Create perceptual image - time: " + sw.Elapsed.TotalMilliseconds + " ms");
	}

	private void _FindNeighbours() {
		Stopwatch sw = new Stopwatch();

		//----------------------------------------------------------------------------------------------------------------------------
		// Create spatial grid of points.
		//----------------------------------------------------------------------------------------------------------------------------
		sw.Start();
		
		spatialGrid = new SpatialGrid();

		Vector3 boundsMin = new Vector3(float.MaxValue, float.MaxValue, float.MaxValue);
		Vector3 boundsMax = new Vector3(float.MinValue, float.MinValue, float.MinValue);

		for (int i = 0; i < surfels.Count; ++i) {
			boundsMin = Vector3.Min(boundsMin, surfels[i].pos);
			boundsMax = Vector3.Max(boundsMax, surfels[i].pos);
		}

		Bounds spatialGridBounds = new Bounds((boundsMax - boundsMin) * 0.5f + boundsMin, boundsMax - boundsMin);
		spatialGrid.Init(spatialGridBounds);
		
		for (int i = 0; i < surfels.Count; ++i) {
			Vector3 p = surfels[i].pos;

			spatialGrid.AddPoint(p, i);
		}

		sw.Stop();
		Debug.Log("Create spatial grid - time: " + sw.Elapsed.TotalMilliseconds + " ms cells: " + spatialGrid.totalCells);

		// spatialGrid.DrawDebug();

		//----------------------------------------------------------------------------------------------------------------------------
		// Fast implementation.
		//----------------------------------------------------------------------------------------------------------------------------
		sw.Restart();

		int threadCount = 24;
		Thread[] threads = new Thread[threadCount];

		int totalJobs = surfels.Count;
		int jobSize = totalJobs / threadCount;
		int jobsRemaining = totalJobs - jobSize * threadCount;

		fastSurfels = new FastSurfel[surfels.Count];
		int fastSurfelIdx = 0;
		fastCells = new FastSpatialCell[spatialGrid.totalCells];
		fastNeighbours = new FastNeighour[surfels.Count * 16];
		
		for (int i = 0; i < spatialGrid.totalCells; ++i) {
			SpatialGridCell c = spatialGrid.spatialGridCells[i];

			fastCells[i].startIdx = fastSurfelIdx;
			fastCells[i].count = c.Ids.Count;

			for (int j = 0; j < c.Ids.Count; ++j) {
				Surfel s = surfels[c.Ids[j]];
				fastSurfels[fastSurfelIdx].id = c.Ids[j];
				fastSurfels[fastSurfelIdx].cellId = i;
				fastSurfels[fastSurfelIdx].totalweight = 0.0f;
				fastSurfels[fastSurfelIdx].pos = s.pos;
				fastSurfels[fastSurfelIdx].normal = s.normal;
				fastSurfels[fastSurfelIdx].continuous = s.continuous;
				fastSurfels[fastSurfelIdx].scale = s.scale;
				++fastSurfelIdx;
			}
		}

		sw.Stop();
		Debug.Log("Prepare fast neighbour search - time: " + sw.Elapsed.TotalMilliseconds + " ms");

		sw.Restart();

		for (int i = 0; i < threadCount; ++i) {
			threads[i] = new Thread(_ThreadFindNeighboursFast);

			int startId = i * jobSize;
			int endId = (i + 1) * jobSize;
			
			if (i == threadCount - 1) {
				endId += jobsRemaining;
			}

			object[] args = { startId, endId, fastSurfels, fastCells, fastNeighbours };
			threads[i].Start(args);
		}

		for (int i = 0; i < threadCount; ++i) {
			threads[i].Join();
		}

		sw.Stop();
		Debug.Log("Fast neighbour search - time: " + sw.Elapsed.TotalMilliseconds + " ms fast surfels: " + fastSurfels.Length);

		float avgNeighs = 0;

		// Normalize weights.
		for (int i = 0; i < fastSurfels.Length; ++i) {
			avgNeighs += fastSurfels[i].neighbourCount;

			// int nS = fastSurfels[i].neighbourStartIdx;
			// int nE = nS + fastSurfels[i].neighbourCount;
			// float totalWeight = fastSurfels[i].totalweight;

			// for (int j = nS; j < nE; ++j) {
			// 	fastNeighbours[j].weight /= totalWeight;
			// }
		}

		avgNeighs /= fastSurfels.Length;
		Debug.Log("Average neighs: " + avgNeighs);

		//----------------------------------------------------------------------------------------------------------------------------
		// Slow implementation.
		//----------------------------------------------------------------------------------------------------------------------------
		// sw.Restart();
		// for (int i = 0; i < surfels.Count; ++i) {
		// 	surfels[i].neighbours = new List<SurfelNeighbour>(8);
		// }
		
		// for (int i = 0; i < threadCount; ++i) {
		// 	threads[i] = new Thread(_ThreadFindNeighbours);

		// 	int startId = i * jobSize;
		// 	int endId = (i + 1) * jobSize;
			
		// 	if (i == threadCount - 1) {
		// 		endId += jobsRemaining;
		// 	}

		// 	int[] args = { startId, endId };
		// 	threads[i].Start(args);
		// }

		// for (int i = 0; i < threadCount; ++i) {
		// 	threads[i].Join();
		// }

		// sw.Stop();
		// Debug.Log("Surfel neighbour gen - total: " + surfels.Count + " time: " + sw.Elapsed.TotalMilliseconds + " ms");
	}

	private void _ThreadCalcSurfelFlip(object Data) {
		int start = (int)((object[])Data)[0];
		int end = (int)((object[])Data)[1];
		int[] orderList = (int[])((object[])Data)[2];

		// Debug.Log(start + " " + end + " " + (end - start));
		
		for (int j = start; j < end; ++j) {
			int surfelId = orderList[j];
			
			float currentError = GetFasterSurfelMse(surfelId);

			int initialD = fastSurfels[surfelId].discrete;
			
			if (initialD == 0) {
				float newError = GetFasterSurfelMseMod(surfelId, 1.0f);

				if (newError < currentError) {
					// ++toggleCount;
					// totalError += newError;
					fastSurfels[surfelId].discrete = 1;
				} else {
					// totalError += currentError;
				}
			} else {
				float newError = GetFasterSurfelMseMod(surfelId, -1.0f);

				if (newError < currentError) {
					// ++toggleCount;
					// totalError += newError;
					fastSurfels[surfelId].discrete = 0;
				} else {
					// totalError += currentError;
				}
			}
		}
	}

	private void _ThreadCalcPercep(object Data) {
		int start = (int)((object[])Data)[0];
		int end = (int)((object[])Data)[1];
		
		for (int i = start; i < end; ++i) {
			FastSurfel s = fastSurfels[i];

			float sum = 0.0f;

			int nS = s.neighbourStartIdx;
			int nE = nS + s.neighbourCount;

			for (int j = nS; j < nE; ++j) {
				FastNeighour n = fastNeighbours[j];

				sum += fastSurfels[n.id].discrete * n.weight;
			}

			fastSurfels[i].percep = sum / fastSurfels[i].totalweight;
		}
	}

	private void _ThreadFindNeighboursFast(object Data) {
		int start = (int)((object[])Data)[0];
		int end = (int)((object[])Data)[1];
		FastSurfel[] fastSurfels = (FastSurfel[])((object[])Data)[2];
		FastSpatialCell[] fastCells = (FastSpatialCell[])((object[])Data)[3];
		FastNeighour[] fastNeighbours = (FastNeighour[])((object[])Data)[4];

		double stdDev = 2.0f;
		double sigma = 2.0 * stdDev * stdDev;

		int[] cellList = new int[8];

		for (int i = start; i < end; ++i) {
			FastSurfel s0 = fastSurfels[i];

			fastSurfels[i].neighbourCount = 0;
			fastSurfels[i].neighbourStartIdx = i * 16;
			int neighbourIdx = i * 16;

			spatialGrid.GetCellIdsList(fastSurfels[i].pos, cellList);

			for (int j = 0; j < 8; ++j) {
				FastSpatialCell c = fastCells[cellList[j]];
				for (int n = c.startIdx; n < c.startIdx + c.count; ++n) {
					if (fastSurfels[i].neighbourCount == 16) {
						break;
					}

					// ++totalSearches;
					
					FastSurfel s1 = fastSurfels[n];

					float sqrD = Vector3.SqrMagnitude(s1.pos - s0.pos);

					// float checkD = 0.08f; // Just within distance to include 9 neighbours (0.707).
					float checkD = 0.008f; // Just within distance to include 9 neighbours (0.707).
					if (sqrD <= checkD * checkD) {
						float weight = (float)((Math.Exp(-sqrD / sigma)) / (Math.PI * sigma));

						fastNeighbours[neighbourIdx].id = n;
						fastNeighbours[neighbourIdx].weight = weight;
						fastSurfels[i].totalweight += weight;

						++fastSurfels[i].neighbourCount;
						++neighbourIdx;
					}
				}
			}
		}
	}

	private void _ThreadFindNeighbours(object Data) {
		int start = ((int[])Data)[0];
		int end = ((int[])Data)[1];

		double stdDev = 2.0f;
		double sigma = 2.0 * stdDev * stdDev;

		// Debug.Log(start + " - " + end);

		float avgChecks = 0;
		int totalCount = 0;

		SpatialGridCell[] cellList = new SpatialGridCell[8];

		for (int i = start; i < end; ++i) {
			Surfel a = surfels[i];

			// SpatialGrid.CellBoundsResult cellBounds = spatialGrid.GetCellBounds(a.pos);
			spatialGrid.GetCellBoundsList(a.pos, cellList);

			int checks = 0;

			// for (int iZ = cellBounds.min.z; iZ <= cellBounds.max.z; ++iZ) {
			// 	for (int iY = cellBounds.min.y; iY <= cellBounds.max.y; ++iY) {
			// 		for (int iX = cellBounds.min.x; iX <= cellBounds.max.x; ++iX) {
			
			for (int c = 0; c < 8; ++c) {
				SpatialGridCell cell = cellList[c];

				checks += cell.Ids.Count;

				for (int j = 0; j < cell.Ids.Count; ++j) {
					Surfel b = surfels[cell.Ids[j]];

					if (a == b) {
						continue;
					}

					float sqrD = Vector3.SqrMagnitude(b.pos - a.pos);

					float checkD = 0.08f; // Just within distance to include 9 neighbours (0.707).
					if (sqrD <= checkD * checkD) {
						float weight = (float)((Math.Exp(-sqrD / sigma)) / (Math.PI * sigma));
						
						SurfelNeighbour n = new SurfelNeighbour();
						n.surfel = b;
						n.weight = weight;
						a.neighbours.Add(n);
						a.totalWeight += weight;
					}
				}
			}

			avgChecks += checks;
			++totalCount;

			// for (int j = 0; j < surfels.Count; ++j) {
			// 	if (j == i) {
			// 		continue;
			// 	}

			// 	Surfel b = surfels[j];

			// 	float sqrD = Vector3.SqrMagnitude(b.pos - a.pos);

			// 	float checkD = 0.08f; // Just within distance to include 9 neighbours (0.707).
			// 	if (sqrD <= checkD * checkD) {
			// 		float weight = (float)((Math.Exp(-sqrD / sigma)) / (Math.PI * sigma));
					
			// 		SurfelNeighbour n = new SurfelNeighbour();
			// 		n.surfel = b;
			// 		n.weight = weight;
			// 		a.neighbours.Add(n);
			// 		a.totalWeight += weight;
			// 	}
			// }
		}

		avgChecks /= totalCount;
		Debug.Log("start: " + start + " end: " + end + " total: " + totalCount + " checks: " + avgChecks);
	}
}
