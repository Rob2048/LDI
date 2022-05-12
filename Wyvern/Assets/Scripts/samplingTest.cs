using UnityEngine;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;

using Debug = UnityEngine.Debug;

public class SamplePoint {
	public Vector2 position;
	public float value;
	public float scale;
	public float radius;
}

public class SamplingTest {

	public Texture2D sourceTexture;
	public Color32[] sourcePixels;
	public float[] sourceIntensity;
	public int sourceWidth;
	public int sourceHeight;
	public List<SamplePoint> candidatesList;
	public List<SamplePoint> pointList;

	public SamplingTest(string ImagePath) {
		Debug.Log("Sampling texture " + ImagePath);
		
		// Load texture.
		sourceTexture = null;
		byte[] fileData = File.ReadAllBytes(ImagePath);
		sourceTexture = new Texture2D(2, 2, TextureFormat.RGB24, false, false);
		sourceTexture.filterMode = FilterMode.Point;
		sourceTexture.LoadImage(fileData);

		sourcePixels = sourceTexture.GetPixels32(0);
		sourceWidth = sourceTexture.width;
		sourceHeight = sourceTexture.height;
		int totalPixels = sourceWidth * sourceHeight;
		Debug.Log("Sampling: Source size " + sourceWidth + " x " + sourceHeight);

		// Convert image to greyscale.
		sourceIntensity = new float[totalPixels];

		for (int i = 0; i < totalPixels; ++i) {
			Color32 source = sourcePixels[i];
			float lum = source.r * 0.2126f + source.g * 0.7152f + source.b * 0.0722f;

			sourceIntensity[i] = core.GammaToLinear(lum / 255.0f);
			// sourceIntensity[i] = lum / 255.0f;
			sourcePixels[i].r = (byte)lum;
			sourcePixels[i].g = (byte)lum;
			sourcePixels[i].b = (byte)lum;
		}

		sourceTexture.SetPixels32(sourcePixels);
		sourceTexture.Apply(false, false);

		// NOTE: Size is 10x10 at scale of 1.
		GameObject sourceGob = GameObject.CreatePrimitive(PrimitiveType.Plane);
		sourceGob.name = "Sampling source";
		sourceGob.transform.localScale = new Vector3(-sourceWidth / 10.0f / 10.0f, 1.0f, -sourceHeight / 10.0f / 10.0f);
		sourceGob.transform.position = new Vector3((-sourceWidth / 20) * 3 - 8, 0, sourceHeight / 20 + 4);
		sourceGob.GetComponent<MeshRenderer>().material = core.singleton.BasicTextureUnlitMaterial;
		sourceGob.GetComponent<MeshRenderer>().material.SetTexture("_MainTex", sourceTexture);

		GameObject backingGob = GameObject.CreatePrimitive(PrimitiveType.Plane);
		backingGob.name = "Backing";
		backingGob.transform.localScale = new Vector3(-sourceWidth / 10.0f / 10.0f, 1.0f, -sourceHeight / 10.0f / 10.0f);
		backingGob.transform.position = new Vector3(-sourceWidth / 20 - 4, -0.01f, sourceHeight / 20 + 4);
		backingGob.GetComponent<MeshRenderer>().material = core.singleton.BasicUnlitMaterial;

		//----------------------------------------------------------------------------------------------------------------------------
		// Poisson disk sampler.
		// Generate sample pool.
		//----------------------------------------------------------------------------------------------------------------------------

		Stopwatch sw = new Stopwatch();
		sw.Start();

		float samplesPerPixel = 4.0f;
		float samplesScale = ((sourceWidth / 10.0f) / sourceWidth) / samplesPerPixel;
		float singlePixelScale = samplesScale * 4;

		int samplesWidth = (int)(sourceWidth * samplesPerPixel);
		int samplesHeight = (int)(sourceHeight * samplesPerPixel);

		candidatesList = new List<SamplePoint>();

		for (int iY = 0; iY < samplesHeight; ++iY) {
			for (int iX = 0; iX < samplesWidth; ++iX) {
				// Sample position.
				float sX = (float)iX / samplesWidth;
				float sY = (float)iY / samplesHeight;
				float value = GetSourceValue(sX, sY);

				if (value >= 0.999f) {
					continue;
				}

				// float area = 1.0f / (1.0f - value) * 3.14f;
				float area = 1.0f / (1.0f - value) * 3.14f;
				float radius = Mathf.Sqrt(area / Mathf.PI);
				radius = Mathf.Pow(radius, 3.14f);
				radius *= 1.5f;

				// (3.14) 1 = 0
				// (4.71) 1.22 = 0.25
				// (6.28) 1.414 = 0.5
				// inf = inf = 1.0

				// Multiply by 1.5 to keep samples apart.
				
				// float dMax = 40.0f;
				// float dMin = 1.5f;
				// float radius = (dMax - dMin) * (Mathf.Pow(value, (1.4f))) + dMin;
				// float radius = (dMax - dMin) * (value) + dMin;

				SamplePoint s = new SamplePoint();
				s.position = new Vector2(iX * samplesScale, iY * samplesScale);
				s.value = value;
				s.scale = singlePixelScale * 1.2f;
				s.radius = singlePixelScale * 0.5f * radius;
				// s.radius = singlePixelScale * 0.5f * 1.5f;
				candidatesList.Add(s);
			}
		}

		sw.Stop();
		Debug.Log("Sample pool time: " + sw.Elapsed.TotalMilliseconds + " ms");

		Debug.Log("Sample pool generated: " + candidatesList.Count);

		//----------------------------------------------------------------------------------------------------------------------------
		// Create random order for sample selection.
		//----------------------------------------------------------------------------------------------------------------------------
		int[] orderList = new int[candidatesList.Count];
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
		// Build spatial table.
		//----------------------------------------------------------------------------------------------------------------------------
		// Cell size is maximum diameter.
		// float gridCellSize = maxR * singlePixelScale;
		float gridCellSize = 5 * singlePixelScale;
		float gridPotentialWidth = samplesScale * samplesWidth;
		float gridPotentialHeight = samplesScale * samplesHeight;
		int gridCellsX = (int)Mathf.Ceil(gridPotentialWidth / gridCellSize);
		int gridCellsY = (int)Mathf.Ceil(gridPotentialHeight / gridCellSize);
		int gridTotalCells = gridCellsX * gridCellsY;

		Debug.Log("Grid size: " + gridCellsX + ", " + gridCellsY + "(" + gridCellSize + ") Total: " + gridTotalCells);

		List<int>[,] gridCells = new List<int>[gridCellsX, gridCellsY];

		for (int iY = 0; iY < gridCellsY; ++iY) {
			for (int iX = 0; iX < gridCellsX; ++iX) {
				gridCells[iX, iY] = new List<int>();
			}
		}

		//----------------------------------------------------------------------------------------------------------------------------
		// Add samples from pool to final result.
		//----------------------------------------------------------------------------------------------------------------------------
		pointList = new List<SamplePoint>();

		ProfileTimer pt = new ProfileTimer();

		// Insert a sample if there is room for it.
		for (int i = 0; i < orderList.Length; ++i) {
		// for (int i = 0; i < 10000; ++i) {
			// Debug.Log("Itr" + i);
			int candidateId = orderList[i];

			SamplePoint candidate = candidatesList[candidateId];
			bool found = false;

			//float halfGcs = gridCellSize * 0.5f;
			float halfGcs = candidate.radius;

			int gSx = (int)((candidate.position.x - halfGcs) / gridCellSize);
			int gEx = (int)((candidate.position.x + halfGcs) / gridCellSize);
			int gSy = (int)((candidate.position.y - halfGcs) / gridCellSize);
			int gEy = (int)((candidate.position.y + halfGcs) / gridCellSize);

			gSx = Mathf.Clamp(gSx, 0, gridCellsX - 1);
			gEx = Mathf.Clamp(gEx, 0, gridCellsX - 1);
			gSy = Mathf.Clamp(gSy, 0, gridCellsY - 1);
			gEy = Mathf.Clamp(gEy, 0, gridCellsY - 1);

			// Debug.Log(gSx + ", " + gEx + " " + gSy + ", " + gEy);

			for (int iY = gSy; iY <= gEy; ++iY) {
				for (int iX = gSx; iX <= gEx; ++iX) {
					for (int j = 0; j < gridCells[iX, iY].Count; ++j) {
						SamplePoint sample = candidatesList[gridCells[iX, iY][j]];

						float sqrDist = Vector2.SqrMagnitude(sample.position - candidate.position);
						if (sqrDist <= (candidate.radius * candidate.radius)) {
							found = true;
							break;
						}
					}

					if (found) {
						break;
					}
				}

				if (found) {
					break;
				}
			}

			// for (int j = 0; j < pointList.Count; ++j) {
			// 	SamplePoint sample = pointList[j];

			// 	float sqrDist = Vector2.SqrMagnitude(sample.position - candidate.position);
			// 	if (sqrDist <= (candidate.radius * candidate.radius)) {
			// 		found = true;
			// 		break;
			// 	}
			// }

			if (!found) {
				pointList.Add(candidate);

				int gX = (int)((candidate.position.x) / gridCellSize);
				int gY = (int)((candidate.position.y) / gridCellSize);

				// if (gX >= gridCellsX || gY >= gridCellsY) {
					// Debug.LogError("Grid cell out of bounds: " + gX + ", " + gY + " " + candidate.position.x + ", " + candidate.position.y);
					// continue;
				// }

				gridCells[gX, gY].Add(candidateId);
			}
		}

		pt.Stop("Generate poisson samples: " + pointList.Count);

		GeneratePointsMesh();
	}

	// Get intensity value from source texture at x,y, 0.0 to 1.0.
	public float GetSourceValue(float X, float Y) {
		int pX = (int)(X * sourceWidth);
		int pY = (int)(Y * sourceHeight);
		int idx = pY * sourceWidth + pX;

		return sourceIntensity[idx];
	}

	public void GeneratePointsMesh() {
		GameObject pointsGob = new GameObject();
		pointsGob.name = "Sampling points";
		// pointsGob.transform.position = new Vector3(4, 0, 4);
		pointsGob.transform.position = new Vector3(-44, 0.01f, 4);
		Mesh pointsMesh = new Mesh();
		pointsGob.AddComponent<MeshFilter>().mesh = pointsMesh;
		pointsGob.AddComponent<MeshRenderer>().material = core.singleton.SurfelDebugMat;
		
		int pointCount = pointList.Count;

		Vector3[] verts = new Vector3[pointCount * 4];
		Vector2[] uvs = new Vector2[pointCount * 4];
		Color32[] cols = new Color32[pointCount * 4];
		int[] inds = new int[pointCount * 6];

		for (int i = 0; i < pointCount; ++i) {
			SamplePoint s = pointList[i];
			Vector3 basePos = new Vector3(s.position.x, 0, s.position.y);

			// float halfScale = s.radius;
			float halfScale = s.scale * 0.5f;

			verts[i * 4 + 0] = basePos + new Vector3(halfScale, 0, -halfScale);
			verts[i * 4 + 1] = basePos + new Vector3(-halfScale, 0, -halfScale);
			verts[i * 4 + 2] = basePos + new Vector3(-halfScale, 0, halfScale);
			verts[i * 4 + 3] = basePos + new Vector3(halfScale, 0, halfScale);

			uvs[i * 4 + 0] = new Vector2(0, 0);
			uvs[i * 4 + 1] = new Vector2(0, 1);
			uvs[i * 4 + 2] = new Vector2(1, 1);
			uvs[i * 4 + 3] = new Vector2(1, 0);

			// byte lum = (byte)(s.value * 255);
			// Color32 c = new Color32(lum, lum, lum, 1);
			Color32 c = new Color32(0, 0, 0, 1);

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
		}

		pointsMesh.indexFormat = UnityEngine.Rendering.IndexFormat.UInt32;
		pointsMesh.vertices = verts;
		pointsMesh.uv = uvs;
		pointsMesh.colors32 = cols;
		pointsMesh.triangles = inds;
	}
}