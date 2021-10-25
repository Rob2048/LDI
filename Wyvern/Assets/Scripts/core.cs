using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Threading;
using UnityEngine;
using Unity.Jobs;
using Unity.Collections;
using UnityEngine.UI;
using UnityEngine.Rendering;

using Debug = UnityEngine.Debug;

public class core : MonoBehaviour {

	/*
	Coverage algo steps:

	- Build reasonably uniform triangle mesh.
		- Eliminates small bumps automatically.

	- Build coverage map.
		- Tris are mapped to pixels.
		- Each pixel is coverage element (Coxel).
		
	- Use coxels to build list of scanner positions.
		- Max 1 position per coxel. (Can have no positions if unreachable).
		- Ideally, check for coxel scores and decide if coverage is already sufficient.
		- Start search at optimal location, and move around until max angles are reached.
			- Each view check renders model, writes quality to coxel, appropriate coxels checked for quality, if above certain threshold then accept.
		- Figure out group score and write into coxel.
			- Group score determines how effective this scanner position has been in covering pixels.
			- This means a search will need to be done for every coxel.

	- Build raster list for each scanner position.
		- Start with highest score groups.
		- Use coxels coverage to determine if it needs coverage.
		- Render into coverage.

	- Determine visit order of scanner position list.
		- Travelling salesman problem.
		- Use time of slowest axis to determine time from position to position.


	angular frequency max
	------------------------

	fa = 400pixels * 3.14 * 300mm / (360 * 20)
	fa = 376800 / 7200
	fa max = 52.33 cycle/deg
	*/

	public RawImage uiOriginalImage;
	public RawImage uiCanvas;
	public RawImage uiPreviewImage;

	public Text uiTextPosX;
	public Text uiTextPosY;
	public Text uiTextPosZ;

	public GameObject SampleLocatorPrefab;

	public Material SurfelDebugMat;

	private JobManager _jobManager;

	private Texture2D _sourceImage;
	private Texture2D _canvas;
	private float[] _canvasRawData;
	private Color32[] _tempCols;
	private Color32[] _sourceRawData;
	
	private float _threshold = 0.5f;
	private float _brightness = 0.0f;
	
	private Diffuser _diffuser;

	public void uiBtnCapture() {
		_ScannerViewHit(true);
	}

	public void uiThresholdSliderChanged(float Value) {
		_threshold = Value;
		_CopyAndDither(_sourceImage, _canvas, _sourceImage.width, _sourceImage.height);
	}

	public void uiBrightnessSliderChanged(float Value) {
		_brightness = Value;
		_CopyAndDither(_sourceImage, _canvas, _sourceImage.width, _sourceImage.height);
	}

	public void uiBtnZero() {
		_jobManager.SetTaskZero();
	}

	public void uiBtnGotoZero() {
		_jobManager.SetTaskGotoZero();
	}

	public void uiBtnStart() {
		_jobManager.SetTaskRasterImage(_canvasRawData, _jobManager._controller._lastAxisPos[0], _jobManager._controller._lastAxisPos[1], _sourceImage.width, _sourceImage.height);
	}

	public void uiBtnStartLineTest() {
		// float[] data = new float[3];
		List<float> data = new List<float>();

		float div = 0.025f;

		for (int i = 0; i < 400; ++i) {
			data.Add(_jobManager._controller._lastAxisPos[0] + div * i);
		}

		//_jobManager.SetLineRaster(200, 200, 10, data.ToArray());
		_jobManager.SetTaskLineTest(_canvasRawData, _jobManager._controller._lastAxisPos[0], _jobManager._controller._lastAxisPos[1], _sourceImage.width, _sourceImage.height);
	}

	public void uiBtnMirrorLeft() {
		_jobManager.SetTaskSimpleMoveRelative(2, -10, 200);
	}

	public void uiBtnMirrorRight() {
		_jobManager.SetTaskSimpleMoveRelative(2, 10, 200);
	}

	public void uiBtnLeft() {
		_jobManager.SetTaskSimpleMoveRelative(0, -1.0f, 200);
	}

	public void uiBtnRight() {
		_jobManager.SetTaskSimpleMoveRelative(0, 1.0f, 200);
	}

	public void uiBtnUp() {
		_jobManager.SetTaskSimpleMoveRelative(1, 0.5f, 200);
	}

	public void uiBtnDown() {
		_jobManager.SetTaskSimpleMoveRelative(1, -0.5f, 200);
	}

	public void uiBtnLaserBurst() {
		_jobManager.CancelTask();
	}

	public void uiBtnSlowLaser() {
		_jobManager.SetTaskStartLaser(10000, 30, 950);
		// _jobManager.SetTaskStartLaser(10000, 5, 975);
		// _jobManager.SetTaskStartLaser(1, 60000, 0);
	}

	private ComputeBuffer _candidateCountBuffer;
	private ComputeBuffer candidateMetaBuffer;
	private float[] candidateMetaRaw = new float[7 * 1024 * 1024];

	private Texture2D gSourceImg;
	private Texture2D gPdDiffusionTex;

	void Start() {
		_candidateCountBuffer = new ComputeBuffer(1, 4);
		candidateMetaBuffer = new ComputeBuffer(7 * 1024 * 1024, 4);

		// gSourceImg = _LoadImage("content/gradient.png");
		// gSourceImg = _LoadImage("content/cmyk_test/c.png");
		//gSourceImg = _LoadImage("content/test_pattern.png");
		gSourceImg = _LoadImage("content/drag_rawr/k.png");
		// gSourceImg = _LoadImage("content/drag_2cm/b.png");
		//gSourceImg = _LoadImage("content/drag/b.png");
		//  gSourceImg = _LoadImage("content/chars/toon.png");
		//gSourceImg = _LoadImage("content/chars/disc.png");

		gPdDiffusionTex = new Texture2D(gSourceImg.width, gSourceImg.height, TextureFormat.RGB24, false, false);
		gPdDiffusionTex.filterMode = FilterMode.Point;

		_diffuser = new Diffuser(this);

		_ClearCoverageMap();

		// _RunDiffuser();
		_StartDiffuser();
		// _PrimeMesh();

		_jobManager = new JobManager();
		_jobManager.Init();
		
		// for (int i = 0; i < 6; ++i) {
		// 	float y = i * 35;
		// 	_diffuser.AddTexture(_LoadImage("content/drag_rawr/c.png"), new Vector3(0, y, 0));
		// 	_diffuser.AddTexture(_LoadImage("content/drag_rawr/m.png"), new Vector3(0, y, 15));
		// 	_diffuser.AddTexture(_LoadImage("content/drag_rawr/y.png"), new Vector3(0, y, 30));
		// 	_diffuser.AddTexture(_LoadImage("content/drag_rawr/k.png"), new Vector3(0, y, 45));
		// }
		// _diffuser.Process();
		// _diffuser.DrawDebug();
	}

	private float[,] _CreateGaussianKernel(int FilterSize, float StdDev) {
		double sigma = StdDev;
		double r, s = 2.0 * sigma * sigma;
		float sum = 0.0f;
		float[,] kernel = new float[FilterSize, FilterSize];
		int hSize = FilterSize / 2;

		for (int iX = -hSize; iX <= hSize; ++iX) {
			for (int iY = -hSize; iY <= hSize; ++iY) {
				r = Math.Sqrt(iX * iX + iY * iY);
				kernel[iX + hSize, iY + hSize] = (float)((Math.Exp(-(r * r) / s)) / (Math.PI * s));
				sum += kernel[iX + hSize, iY + hSize];
			} 
		}

		for (int i = 0; i < FilterSize; ++i) {
			for (int j = 0; j < FilterSize; ++j) {
				kernel[i, j] /= sum;
			}
		}

		return kernel;
	}

	private void _StartDiffuser() {
		_sourceImage = _LoadImage("content/test_pattern.png");
		//_sourceImage = _LoadImage("content/gradient.png");
		// _sourceImage = _LoadImage("content/colorchart_disc_grid_1_channel.png");
		//_sourceImage = _LoadImage("content/cmyk_test/y.png");

		//_sourceImage = _LoadImage("content/drag_rawr/k.png");

		//_sourceImage = _LoadImage("content/drag_2cm/b.png");
		//_sourceImage = _LoadImage("content/drag/b.png");
		//_sourceImage = _LoadImage("content/chars/toon.png");
		//_sourceImage = _LoadImage("content/chars/disc.png");

		uiOriginalImage.texture = _sourceImage;
		float scale = 0.2f;
		uiOriginalImage.rectTransform.sizeDelta = new Vector2(_sourceImage.width, _sourceImage.height) * scale;

		// Workspace image resolution.
		// 50um per point @ 50mm * 50mm
		// Canvas size: 1000 * 1000

		_canvas = new Texture2D(1000, 1000, TextureFormat.RGB24, false, false);
		_canvas.filterMode = FilterMode.Point;
		uiCanvas.texture = _canvas;
		uiCanvas.rectTransform.sizeDelta = new Vector2(1000, 1000);

		_canvasRawData = new float[1000 * 1000];
		_sourceRawData = _sourceImage.GetPixels32();
		_tempCols = new Color32[1000 * 1000];

		// _Histogram(_sourceRawData, _sourceImage.width * _sourceImage.height);
		
		_CopyAndDither(_sourceImage, _canvas, _sourceImage.width, _sourceImage.height);

		uiPreviewImage.rectTransform.sizeDelta = new Vector2(215, 215);
		uiPreviewImage.texture = _canvas;
	}

	private void _RunDiffuser() {
		Texture2D sourceImg = gSourceImg;

		Debug.Log("Img: " + sourceImg.width + " " + sourceImg.height);

		DiffuserView1.GetComponent<MeshRenderer>().material.SetTexture("_MainTex", sourceImg);
		DiffuserView1.transform.localScale = new Vector3(sourceImg.width / 200.0f, 1.0f, sourceImg.height / 200.0f);

		// _canvasRawData = new float[1000 * 1000];
		Texture2D fsDiffusionTex = new Texture2D(sourceImg.width, sourceImg.height, TextureFormat.RGB24, false, false);
		fsDiffusionTex.filterMode = FilterMode.Point;

		_DiffuseFS(sourceImg, fsDiffusionTex);

		DiffuserView2.GetComponent<MeshRenderer>().material.SetTexture("_MainTex", fsDiffusionTex);
		DiffuserView2.transform.localScale = new Vector3(fsDiffusionTex.width / 200.0f, 1.0f, fsDiffusionTex.height / 200.0f);

		// Texture2D pdDiffusionTex = new Texture2D(sourceImg.width, sourceImg.height, TextureFormat.RGB24, false, false);
		// pdDiffusionTex.filterMode = FilterMode.Point;

		// _DiffusePD(sourceImg, pdDiffusionTex);

		// DiffuserView3.GetComponent<MeshRenderer>().material.SetTexture("_MainTex", pdDiffusionTex);
		// DiffuserView3.transform.localScale = new Vector3(pdDiffusionTex.width / 200.0f, 1.0f, pdDiffusionTex.height / 200.0f);
	}

	private void _CreatePerception(float[] Source, int Width, int Height, float[,] Filter, int FilterSize, float[] Dst) {
		int hfSize = FilterSize / 2;

		for (int iX = hfSize; iX < Width - hfSize; ++iX) {
			for (int iY = hfSize; iY < Height - hfSize; ++iY) {

				float sum = 0.0f;

				for (int fX = 0; fX < FilterSize; ++fX) {
					for (int fY = 0; fY < FilterSize; ++fY) {
						int dX = iX - hfSize + fX;
						int dY = iY - hfSize + fY;
						sum += Filter[fX, fY] * Source[dY * Width + dX];
					}
				}

				Dst[iY * Width + iX] = sum;
			}
		}
	}

	private float _GetMse(float[] Orig, float[] Source, int SrcWidth, int SrcHeight, float[,] Filter, int FilterSize, int X, int Y, int Width, int Height) {
		int hfSize = FilterSize / 2;
		float squareError = 0.0f;

		for (int iX = X; iX < X + Width; ++iX) {
			for (int iY = Y; iY < Y + Height; ++iY) {
				float sum = 0.0f;

				for (int fX = 0; fX < FilterSize; ++fX) {
					for (int fY = 0; fY < FilterSize; ++fY) {
						int dX = iX - hfSize + fX;
						int dY = iY - hfSize + fY;
						sum += Filter[fX, fY] * Source[dY * SrcWidth + dX];
					}
				}

				float error = sum - Orig[iY * SrcWidth + iX];
				squareError += error * error;
			}
		}

		squareError /= Width * Height;

		return squareError;
	}

	private void _DiffuseDBS(Texture2D Source, Texture2D Dest, float Factor, float Spread) {
		int totalPixels = Source.width * Source.height;

		Color32[] srcRaw = Source.GetPixels32();
		float[] srcLum = new float[totalPixels];

		// Convert image to lum values.
		for (int i = 0; i < totalPixels; ++i) {
			int dX = i % Source.width;
			int dY = i / Source.width;

			int sIdx = (dY) * Source.width + (dX);

			if (sIdx < Source.width * Source.height && dY < Source.height && dX < Source.width) {
				float lum = srcRaw[sIdx].r * 0.2126f + srcRaw[sIdx].g * 0.7152f + srcRaw[sIdx].b * 0.0722f;
				srcLum[i] = lum / 255.0f;

				// sRGB to linear.
				// srcLum[i] = Mathf.Pow(srcLum[i], 2.2f);
				// srcLum[i] = Mathf.Pow(srcLum[i], 1.4f);

			} else {
				srcLum[i] = 1.0f;
			}

			srcLum[i] = Mathf.Clamp01(srcLum[i]);
		}

		// Create search start point.
		float[] diffuseImg = new float[totalPixels];
		float[] diffuseImgBuffer = new float[totalPixels];

		// for (int i = 0; i < totalPixels; ++i) {
		// 	if (srcLum[i] > 0.5f) {
		// 		diffuseImg[i] = 0.0f;
		// 	} else {
		// 		diffuseImg[i] = 1.0f;
		// 	}
		// }

		// Create perception of diffuseimg.
		int filterSize = 3;
		float[] diffuseImgPercp = new float[totalPixels];
		float[,] filter = _CreateGaussianKernel(filterSize, 2.0f);

		int iters = 3;

		// Create a random order of numbers.
		int[] orderList = new int[(Source.height - filterSize - filterSize) * (Source.width - filterSize - filterSize)];
		int temp = 0;

		for (int iY = filterSize; iY < Source.height - filterSize; ++iY) {
			for (int iX = filterSize; iX < Source.width - filterSize; ++iX) {
				orderList[temp++] = iY * Source.width + iX;
			}
		}

		// Randomize pairs
		for (int j = 0; j < orderList.Length; ++j) {
			int targetSlot = UnityEngine.Random.Range(0, orderList.Length);

			int tmp = orderList[targetSlot];
			orderList[targetSlot] = orderList[j];
			orderList[j] = tmp;
		}

		// Debug.Log("Total cells: " + temp);

		Stopwatch sw = new Stopwatch();
		sw.Start();

		for (int i = 0; i < iters; ++i) {
			// Toggle each pixel

			float totalError = 0.0f;
			int toggleCount = 0;

			
			int batches = 1;
			int pointsPerBatch = (int)Mathf.Ceil(orderList.Length / batches);
			int listStart = 0;

			for (int bI = 0; bI < batches; ++bI) {
				int olS = listStart;
				int olE = olS + pointsPerBatch;

				olE = Mathf.Clamp(olE, 0, orderList.Length);

				// for (int iY = filterSize; iY < Source.height - filterSize; ++iY) {
				// 	for (int iX = filterSize; iX < Source.width - filterSize; ++iX) {			
				// for (int j = 0; j < orderList.Length; ++j) {
				for (int j = olS; j < olE; ++j) {
					int targetIdx = orderList[j];
					int iX = targetIdx % Source.width;
					int iY = targetIdx / Source.width;
					int idx = iY * Source.width + iX;

					// Get current MSE
					float currentError = _GetMse(srcLum, diffuseImg, Source.width, Source.height, filter, filterSize, iX - filterSize / 2, iY - filterSize / 2, filterSize, filterSize);

					float org = diffuseImg[idx];
					float newD = 0.0f;

					if (org == 0.0f) {
						newD = 1.0f;
					}

					diffuseImg[idx] = newD;

					float newError = _GetMse(srcLum, diffuseImg, Source.width, Source.height, filter, filterSize, iX - filterSize / 2, iY - filterSize / 2, filterSize, filterSize);

					totalError += newError;

					diffuseImg[idx] = org;

					if (newError >= currentError) {
						// Error worse or the same, so change back.
						newD = org;
					} else {
						++toggleCount;
					}

					//diffuseImgBuffer[idx] = newD;
					diffuseImg[idx] = newD;
				}
			// }

				listStart += pointsPerBatch;

				// Swap diffuse buffers.
				//Array.Copy(diffuseImgBuffer, diffuseImg, totalPixels);
				
			}

			Debug.Log("Iter: " + i + " Error: " + Mathf.Sqrt(totalError / orderList.Length) + " Toggles: " + toggleCount);

			if (toggleCount == 0) {
				break;
			}
		}

		sw.Stop();
		Debug.Log("DBS time: " + sw.Elapsed.TotalMilliseconds + " ms");

		float[,] percepFilter = _CreateGaussianKernel(3, 2.0f);
		_CreatePerception(diffuseImg, Source.width, Source.height, percepFilter, 3, diffuseImgPercp);

		// Apply final image.
		Color32[] _tempCols = new Color32[totalPixels];
		for (int i = 0; i < totalPixels; ++i) {
			// diffuseImgPercp[i] = Mathf.Pow(diffuseImgPercp[i], 1.0f / 2.2f);
			byte lumByte = (byte)(diffuseImg[i] * 255.0f);
			//byte lumByte = (byte)(diffuseImgPercp[i] * 255.0f);

			_tempCols[i].r = lumByte;
			_tempCols[i].g = lumByte;
			_tempCols[i].b = lumByte;
		}
		
		Dest.SetPixels32(_tempCols);
		Dest.Apply(false);

		byte[] _bytes = Dest.EncodeToPNG();
		System.IO.File.WriteAllBytes("dbs_img.png", _bytes);
	}

	private void _DiffusePD(Texture2D Source, Texture2D Dest, float Factor, float Spread) {
		int totalPixels = Source.width * Source.height;

		Color32[] srcRaw = Source.GetPixels32();
		float[] srcLum = new float[totalPixels];

		// Convert image to lum values.
		for (int i = 0; i < totalPixels; ++i) {
			int dX = i % Source.width;
			int dY = i / Source.width;

			int sIdx = (dY) * Source.width + (dX);

			if (sIdx < Source.width * Source.height && dY < Source.height && dX < Source.width) {
				float lum = srcRaw[sIdx].r * 0.2126f + srcRaw[sIdx].g * 0.7152f + srcRaw[sIdx].b * 0.0722f;
				srcLum[i] = lum / 255.0f;

				// sRGB to linear.
				//srcLum[i] = Mathf.Pow(srcLum[i], 2.2f);
				// Linear to sRGB.
				//srcLum[i] = Mathf.Pow(srcLum[i], 1.0f / 2.2f);
			} else {
				srcLum[i] = 1.0f;
			}

			// Apply brightness, contrast.
			float l = srcLum[i];
			//l = Mathf.Pow(l, _threshold) + _brightness;
			l = Mathf.Pow(l, TargetContrast) + TargetBrightness;
			srcLum[i] = Mathf.Clamp01(l);
		}

		// Dart throw.

		//float[] tempBuffer = new float[totalPixels];
		float[] darkBuffer = new float[totalPixels];
		float[] lightBuffer = new float[totalPixels];
		for (int i = 0; i < totalPixels; ++i) {
			//tempBuffer[i] = 0;
			darkBuffer[i] = 1;
			lightBuffer[i] = 1;
		}

		int darts = 1000000;
		// int darts = Source.width * Source.height;

		for (int i = 0; i < darts; ++i) {
			int dX = (int)UnityEngine.Random.Range(0, Source.width - 1);
			int dY = (int)UnityEngine.Random.Range(0, Source.height - 1);
			// int dX = i % Source.width;
			// int dY = i / Source.width;

			// float lum = 1.0f - srcLum[dY * Source.width + dX];
			float darkLum = 1.0f - srcLum[dY * Source.width + dX];
			float lightLum = srcLum[dY * Source.width + dX];

			float factor = Factor;
			float spread = Spread;
			
			// Dark lum
			if (darkBuffer[dY * Source.width + dX] != 0) {
				if (darkLum < 1.0f) {
					darkLum = Mathf.Pow(darkLum, factor) + 0.0f;
					float circleWidth = darkLum * spread;
					float circleHalfWidth = circleWidth / 2.0f;

					int sX = (int)(dX - circleHalfWidth);
					int sY = (int)(dY - circleHalfWidth);
					int eX = (int)Math.Ceiling(dX + circleHalfWidth);
					int eY = (int)Math.Ceiling(dY + circleHalfWidth);

					sX = Mathf.Clamp(sX, 0, Source.width - 1);
					sY = Mathf.Clamp(sY, 0, Source.height - 1);
					eX = Mathf.Clamp(eX, 0, Source.width - 1);
					eY = Mathf.Clamp(eY, 0, Source.height - 1);

					bool found = false;

					for (int iX = sX; iX <= eX; ++iX) {
						for (int iY = sY; iY <= eY; ++iY) {
							if (new Vector2(iX - dX, iY - dY).magnitude <= circleHalfWidth) {
								if (darkBuffer[iY * Source.width + iX] == 0) {
									found = true;
									break;
								}
							}
						}
						if (found) {
							break;
						}
					}

					if (found == false) {
						darkBuffer[dY * Source.width + dX] = 0;
					}
				}
			}

			// Light lum
			if (lightBuffer[dY * Source.width + dX] != 0) {
				if (lightLum < 1.0f) {
					lightLum = Mathf.Pow(lightLum, factor) + 0.0f;
					float circleWidth = lightLum * spread;
					float circleHalfWidth = circleWidth / 2.0f;

					int sX = (int)(dX - circleHalfWidth);
					int sY = (int)(dY - circleHalfWidth);
					int eX = (int)Math.Ceiling(dX + circleHalfWidth);
					int eY = (int)Math.Ceiling(dY + circleHalfWidth);

					sX = Mathf.Clamp(sX, 0, Source.width - 1);
					sY = Mathf.Clamp(sY, 0, Source.height - 1);
					eX = Mathf.Clamp(eX, 0, Source.width - 1);
					eY = Mathf.Clamp(eY, 0, Source.height - 1);

					bool found = false;

					for (int iX = sX; iX <= eX; ++iX) {
						for (int iY = sY; iY <= eY; ++iY) {
							if (new Vector2(iX - dX, iY - dY).magnitude <= circleHalfWidth) {
								if (lightBuffer[iY * Source.width + iX] == 0) {
									found = true;
									break;
								}
							}
						}
						if (found) {
							break;
						}
					}

					if (found == false) {
						lightBuffer[dY * Source.width + dX] = 0;
					}
				}
			}

			
		}

		// Apply final image.
		Color32[] _tempCols = new Color32[totalPixels];
		float[] resultImg = new float[totalPixels];
		float[] filterImg = new float[totalPixels];

		for (int i = 0; i < totalPixels; ++i) {
			float lum = srcLum[i];

			float outVal = 0.0f;

			if (lum < 0.5f) {
				outVal = 1.0f - Mathf.Clamp01(darkBuffer[i]);
			} else {
				outVal = Mathf.Clamp01(lightBuffer[i]);
			}

			resultImg[i] = outVal;
		}

		float[,] filter = _CreateGaussianKernel(5, 2.0f);

		// Apply filter;
		for (int iX = 3; iX < Source.width - 3; ++iX) {
			for (int iY = 3; iY < Source.height - 3; ++iY) {

				float sum = 0.0f;

				for (int fX = 0; fX < 5; ++fX) {
					for (int fY = 0; fY < 5; ++fY) {
						int dX = iX - 2 + fX;
						int dY = iY - 2 + fY;
						sum += filter[fX, fY] * resultImg[dY * Source.width + dX];
					}
				}

				filterImg[iY * Source.width + iX] = sum;
			}
		}

		for (int i = 0; i < totalPixels; ++i) {
			byte lumByte = (byte)(filterImg[i] * 255.0f);

			_tempCols[i].r = lumByte;
			_tempCols[i].g = lumByte;
			_tempCols[i].b = lumByte;

			// if (lum < 0.5f) {
			// 	_tempCols[i].r = 255;
			// 	outVal = 1.0f - Mathf.Clamp01(darkBuffer[i]);

			// 	if (outVal != 0.0f) {
			// 		_tempCols[i].g = 255;
			// 	} else {
			// 		_tempCols[i].g = 0;
			// 	}
			// } else {
			// 	_tempCols[i].r = 0;
			// 	outVal = Mathf.Clamp01(lightBuffer[i]);

			// 	if (outVal != 0.0f) {
			// 		_tempCols[i].b = 255;
			// 	} else {
			// 		_tempCols[i].b = 0;
			// 	}
			// }

			// byte lumByte = (byte)(outVal * 255.0f);

			// // _tempCols[i].r = lumByte;
			// //_tempCols[i].g = lumByte;
			// // _tempCols[i].b = lumByte;
		}
		
		Dest.SetPixels32(_tempCols);
		Dest.Apply(false);
	}

	// Floyd-Steinboi.
	private void _DiffuseFS(Texture2D Source, Texture2D Dest) {
		int totalPixels = Source.width * Source.height;

		Color32[] srcRaw = Source.GetPixels32();
		float[] srcLum = new float[totalPixels];

		// Convert image to lum values.
		for (int i = 0; i < totalPixels; ++i) {
			int dX = i % Source.width;
			int dY = i / Source.width;

			int sIdx = (dY) * Source.width + (dX);

			if (sIdx < Source.width * Source.height && dY < Source.height && dX < Source.width) {
				float lum = srcRaw[sIdx].r * 0.2126f + srcRaw[sIdx].g * 0.7152f + srcRaw[sIdx].b * 0.0722f;
				srcLum[i] = lum / 255.0f;

				// sRGB to linear.
				// srcLum[i] = Mathf.Pow(srcLum[i], 2.2f);
				// Linear to sRGB.
				// srcLum[i] = Mathf.Pow(srcLum[i], 1.0f / 2.2f);
			} else {
				srcLum[i] = 1.0f;
			}

			// Apply brightness, contrast.
			float l = srcLum[i];
			//l = Mathf.Pow(l, _threshold) + _brightness;
			srcLum[i] = Mathf.Clamp01(l);
		}
	
		// Floyd boi dithering.
		for (int i = 0; i < totalPixels; ++i) {
			int dX = i % Source.width;
			int dY = i / Source.width;

			float finalColor = _GetClosestLum(srcLum[i]);
			float error = srcLum[i] - finalColor;
			srcLum[i] = finalColor;

			if (dX < Source.width - 1) {
				srcLum[i + 1] += error * 7.0f / 16.0f;
			}

			if (dY < Source.height - 1) {
				if (dX < Source.width - 1) {
					srcLum[i + Source.width + 1] += error * 1.0f / 16.0f;
				}

				if (dX > 0) {
					srcLum[i + Source.width - 1] += error * 3.0f / 16.0f;
				}

				srcLum[i + Source.width] += error * 5.0f / 16.0f;
			}
		}

		float[] filterImg = new float[totalPixels];
		// Note: Must be an odd number.
		int filterSize = 5;
		int hFSize = filterSize / 2;
		float[,] filter = _CreateGaussianKernel(filterSize, 2.0f);

		// Apply filter;
		for (int iX = hFSize; iX < Source.width - hFSize; ++iX) {
			for (int iY = hFSize; iY < Source.height - hFSize; ++iY) {

				float sum = 0.0f;

				for (int fX = 0; fX < filterSize; ++fX) {
					for (int fY = 0; fY < filterSize; ++fY) {
						int dX = iX - hFSize + fX;
						int dY = iY - hFSize + fY;
						sum += filter[fX, fY] * srcLum[dY * Source.width + dX];
					}
				}

				filterImg[iY * Source.width + iX] = sum;
			}
		}

		// Apply final image.
		Color32[] _tempCols = new Color32[totalPixels];
		for (int i = 0; i < totalPixels; ++i) {
			// byte lumByte = (byte)(srcLum[i] * 255.0f);
			// filterImg[i] = Mathf.Pow(filterImg[i], 1.0f / 2.2f);
			byte lumByte = (byte)(filterImg[i] * 255.0f);

			_tempCols[i].r = lumByte;
			_tempCols[i].g = lumByte;
			_tempCols[i].b = lumByte;
		}

		Dest.SetPixels32(_tempCols);
		Dest.Apply(false);

		byte[] _bytes = Dest.EncodeToPNG();
		System.IO.File.WriteAllBytes("fs_img.png", _bytes);
	}

	private void _DiffuseDFS(Texture2D Source, Texture2D Dest) {
		int totalPixels = Source.width * Source.height;

		Color32[] srcRaw = Source.GetPixels32();
		float[] srcLum = new float[totalPixels];
		float[] diffusedLum = new float[totalPixels];
		float[] errorLum = new float[totalPixels];

		// Convert image to lum values.
		for (int i = 0; i < totalPixels; ++i) {
			int dX = i % Source.width;
			int dY = i / Source.width;

			int sIdx = (dY) * Source.width + (dX);

			if (sIdx < Source.width * Source.height && dY < Source.height && dX < Source.width) {
				float lum = srcRaw[sIdx].r * 0.2126f + srcRaw[sIdx].g * 0.7152f + srcRaw[sIdx].b * 0.0722f;
				srcLum[i] = lum / 255.0f;

				// sRGB to linear.
				srcLum[i] = Mathf.Pow(srcLum[i], 2.2f);
				// Linear to sRGB.
				// srcLum[i] = Mathf.Pow(srcLum[i], 1.0f / 2.2f);
			} else {
				srcLum[i] = 1.0f;
			}

			// Apply brightness, contrast.
			float l = srcLum[i];
			//l = Mathf.Pow(l, _threshold) + _brightness;
			srcLum[i] = Mathf.Clamp01(l);
		}

		//Array.Copy(srcLum, errorLum, totalPixels);

		// int sampleCount = 100000;

		int iters = 10;

		for (int j = 0; j < iters; ++j) {
			for (int i = 0; i < totalPixels; ++i) {
				srcLum[i] += errorLum[i];
				errorLum[i] = 0;
			}

			for (int i = 0; i < totalPixels; ++i) {
				// int dX = (int)UnityEngine.Random.Range(0, Source.width - 1);
				// int dY = (int)UnityEngine.Random.Range(0, Source.height - 1);
				int dX = i % Source.width;
				int dY = i / Source.width;
				int dIdx = dY + Source.width * dX;

				// Distribute error to surrounding pixels.

				int sX = dX - 1;
				int eX = dX + 1;
				int sY = dY - 1;
				int eY = dY + 1;

				sX = Mathf.Clamp(sX, 0, Source.width - 1);
				sY = Mathf.Clamp(sY, 0, Source.height - 1);
				eX = Mathf.Clamp(eX, 0, Source.width - 1);
				eY = Mathf.Clamp(eY, 0, Source.height - 1);

				//float finalColor = _GetClosestLum(srcLum[i]);

				float finalColor = 0.0f;

				if (srcLum[dIdx] >= 0.5) finalColor = 1.0f;
				
				diffusedLum[dIdx] = finalColor;
				float error = srcLum[dIdx] - finalColor;

				for (int iX = sX; iX <= eX; ++iX) {
					for (int iY = sY; iY <= eY; ++iY) {
						int idx  = iY * Source.width + iX;
						errorLum[idx] += error * 1.0f / 16.0f;
					}
				}
			}
		}
	
		// Floyd boi dithering.
		// for (int i = 0; i < totalPixels; ++i) {
		// 	int dX = i % Source.width;
		// 	int dY = i / Source.width;

		// 	float finalColor = _GetClosestLum(srcLum[i]);
		// 	float error = srcLum[i] - finalColor;
		// 	srcLum[i] = finalColor;

		// 	if (dX < Source.width - 1) {
		// 		srcLum[i + 1] += error * 7.0f / 16.0f;
		// 	}

		// 	if (dY < Source.height - 1) {
		// 		if (dX < Source.width - 1) {
		// 			srcLum[i + Source.width + 1] += error * 1.0f / 16.0f;
		// 		}

		// 		if (dX > 0) {
		// 			srcLum[i + Source.width - 1] += error * 3.0f / 16.0f;
		// 		}

		// 		srcLum[i + Source.width] += error * 5.0f / 16.0f;
		// 	}
		// }

		// Apply final image.
		Color32[] _tempCols = new Color32[totalPixels];

		for (int i = 0; i < totalPixels; ++i) {
			diffusedLum[i] = Mathf.Clamp01(diffusedLum[i]);
			byte lumByte = (byte)(diffusedLum[i] * 255.0f);

			_tempCols[i].r = lumByte;
			_tempCols[i].g = lumByte;
			_tempCols[i].b = lumByte;
		}
		
		Dest.SetPixels32(_tempCols);
		Dest.Apply(false);
	}

	private void _Histogram(Color32[] Data, int Size) {
		Debug.Log("Hisogram: " + Size);

		Dictionary<int, int> colors = new Dictionary<int, int>();

		for (int i = 0; i < Size; ++i) {
			int hash = Data[i].r << 24 | Data[i].g << 8 | Data[i].b;
			
			try {
				colors[hash]++;
			} catch (Exception Ex) {
				colors[hash] = 1;
			}
		}

		Debug.Log("Hisogram done: " + colors.Count);
	}

	private float _GetClosestLum(float Value) {
		int levels = 2;

		return (float)((int)(Value * (levels - 1)) / (float)(levels - 1));
	}

	private void _CMYKColor() {
		// Calculate CMYK palette options (layers, color combines).
		// Convert CMYK palette into LAB.
		// Convert sRGB source image into LAB.
		// Dither in LAB against palette.
		// Convert LAB to sRGB for display.
		// Convert LAB to CMYK for print.
	}

	private void _CopyAndDither(Texture2D Source, Texture2D Dest, int SourceWidth, int SourceHeight) {
		// Get image.
		for (int i = 0; i < 1000 * 1000; ++i) {
			int dX = i % 1000;
			int dY = i / 1000;

			// int sIdx = (dY + 500) * SourceWidth + (dX + 500);
			int sIdx = (dY) * SourceWidth + (dX);

			if (sIdx < SourceWidth * SourceHeight && dY < SourceHeight && dX < SourceWidth) {
				float lum = _sourceRawData[sIdx].r * 0.2126f + _sourceRawData[sIdx].g * 0.7152f + _sourceRawData[sIdx].b * 0.0722f;
				_canvasRawData[i] = lum / 255.0f;

				// sRGB to linear.
				_canvasRawData[i] = Mathf.Pow(_canvasRawData[i], 2.2f);
				// Linear to sRGB.
				// _canvasRawData[i] = Mathf.Pow(_canvasRawData[i], 1.0f / 2.2f);
			} else {
				_canvasRawData[i] = 1.0f;
			}

			// Apply brightness, contrast.
			float l = _canvasRawData[i];
			l = Mathf.Pow(l, _threshold) + _brightness;

			//_canvasRawData[i] = 1.0f - Mathf.Clamp01(l);
			_canvasRawData[i] = Mathf.Clamp01(l);
		}

		// Probability based dithering.
		// for (int i = 0; i < 1000 * 1000; ++i) {
		// 	int dX = i % 1000;
		// 	int dY = i / 1000;

		// 	float r = UnityEngine.Random.value;

		// 	if (r > _canvasRawData[i])
		// 		_canvasRawData[i] = 0;
		// 	else 
		// 		_canvasRawData[i] = 1;
		// }

		// Floyd boi dithering.
		for (int i = 0; i < 1000 * 1000; ++i) {
			int dX = i % 1000;
			int dY = i / 1000;

			float finalColor = _GetClosestLum(_canvasRawData[i]);

			float error = _canvasRawData[i] - finalColor;

			_canvasRawData[i] = finalColor;

			if (dX < 1000 - 1) {
				_canvasRawData[i + 1] += error * 7.0f / 16.0f;
			}

			if (dY < 1000 - 1) {
				if (dX < 1000 - 1) {
					_canvasRawData[i + 1000 + 1] += error * 1.0f / 16.0f;
				}

				if (dX > 0) {
					_canvasRawData[i + 1000 - 1] += error * 3.0f / 16.0f;
				}

				_canvasRawData[i + 1000] += error * 5.0f / 16.0f;
			}
		}

		// Apply final image.
		for (int i = 0; i < 1000 * 1000; ++i) {
			_canvasRawData[i] = Mathf.Clamp01(_canvasRawData[i]);
			byte lumByte = (byte)(_canvasRawData[i] * 255.0f);

			_tempCols[i].r = lumByte;
			_tempCols[i].g = lumByte;
			_tempCols[i].b = lumByte;
		}
		
		Dest.SetPixels32(_tempCols);
		Dest.Apply(false);
	}
	
	private Texture2D _LoadImage(string Path) {
		Texture2D tex = null;
		byte[] fileData;

		if (File.Exists(Path)) {
			fileData = File.ReadAllBytes(Path);
			tex = new Texture2D(2, 2, TextureFormat.RGB24, false, false);
			tex.filterMode = FilterMode.Point;
			tex.LoadImage(fileData);
		}

		return tex;
	}

	public float TargetFactor = 6.0f;
	public float TargetSpread = 12.0f;
	public float TargetContrast = 1.0f;
	public float TargetBrightness = 0.0f;

	private float _actualFactor = 0.0f;
	private float _actualSpread = 0.0f;
	private float _actualContrast = 1.0f;
	private float _actualBrightness = 0.0f;

	// 2.3, 10

	void Update() {
		Vector3 laserPos = LaserLocator.transform.position;
		LaserCoverageMat.SetVector("laserPos", new Vector4(laserPos.x, laserPos.y, laserPos.z, 0.0f));

		if (_jobManager != null && _jobManager._controller != null) {
			uiTextPosX.text = "X: " + _jobManager._controller._lastAxisPos[0];
			uiTextPosY.text = "Y: " + _jobManager._controller._lastAxisPos[1];
			uiTextPosZ.text = "Z: " + _jobManager._controller._lastAxisPos[2];
		}

		// NOTE: Only if we have primed mesh.
		//_ScannerViewHit(false);

		// if (_actualFactor != TargetFactor || _actualSpread != TargetSpread | _actualContrast != TargetContrast || _actualBrightness != TargetBrightness) {
		// 	_actualFactor = TargetFactor;
		// 	_actualSpread = TargetSpread;
		// 	_actualContrast = TargetContrast;
		// 	_actualBrightness = TargetBrightness;

		// 	//_DiffusePD(gSourceImg, gPdDiffusionTex, _actualFactor, _actualSpread);
		// 	_DiffuseDBS(gSourceImg, gPdDiffusionTex, _actualFactor, _actualSpread);
		// 	DiffuserView3.GetComponent<MeshRenderer>().material.SetTexture("_MainTex", gPdDiffusionTex);
		// 	DiffuserView3.transform.localScale = new Vector3(gPdDiffusionTex.width / 200.0f, 1.0f, gPdDiffusionTex.height / 200.0f);
		// }
	}

	public void RenderFromScannerView(bool Capture) {
		Stopwatch sw = new Stopwatch();
		sw.Start();

		CommandBuffer cmdBuffer = new CommandBuffer();
		cmdBuffer.name = "Render scanner view";

		Matrix4x4 scaleMatrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, new Vector3(1, 1, -1));
		// Matrix4x4 projMat = Matrix4x4.Ortho(-15, 15, -15, 15, 1.0f, 40.0f);
		Matrix4x4 projMat = Matrix4x4.Perspective(14.25f, 1.0f, 1.0f, 40.0f);
		Matrix4x4 realProj = GL.GetGPUProjectionMatrix(projMat, true);
			
		Matrix4x4 rotMatX = Matrix4x4.Rotate(Quaternion.Euler(-90, 0, 0));
		Matrix4x4 rotMatY = Matrix4x4.Rotate(Quaternion.Euler(0, 0, 0));
		Matrix4x4 offsetMat = Matrix4x4.Translate(new Vector3(0, -4.75f, 0));
		Matrix4x4 modelMat = offsetMat * rotMatX * rotMatY;

		// Depth vis.
		cmdBuffer.SetRenderTarget(ScannerViewDepthRt);
		cmdBuffer.SetViewport(new Rect(0, 0, 1024, 1024));
		cmdBuffer.ClearRenderTarget(true, true, new Color(0.0f, 0.0f, 1.0f, 1.0f), 1.0f);
		BasicShaderMat.SetMatrix("vpMat", realProj * (scaleMatrix * VisCam.worldToLocalMatrix));
		cmdBuffer.DrawMesh(visMesh, modelMat, BasicShaderMat);

		PuffyMat.SetMatrix("vpMat", realProj * (scaleMatrix * VisCam.worldToLocalMatrix));
		PuffyMat.SetMatrix("vMat", (scaleMatrix * VisCam.worldToLocalMatrix));
		PuffyMat.SetMatrix("pMat", realProj);
		PuffyMat.SetVector("camWorldPos", VisCam.position);
		cmdBuffer.DrawMesh(_puffyMesh, modelMat, PuffyMat);

		// Pixel candidates.
		int[] candidateCountDataRaw = new int[1];
		candidateCountDataRaw[0] = 0;
		_candidateCountBuffer.SetData(candidateCountDataRaw);

		cmdBuffer.SetRenderTarget(ScannerViewRt);
		cmdBuffer.SetViewport(new Rect(0, 0, 1024, 1024));
		cmdBuffer.ClearRenderTarget(true, true, new Color(0.0f, 1.0f, 0.0f, 1.0f), 1.0f);
		CandidatesMat.SetMatrix("vpMat", realProj * (scaleMatrix * VisCam.worldToLocalMatrix));
		CandidatesMat.SetVector("camWorldPos", VisCam.position);
		// CandidatesMat.SetBuffer("candidateCounter", _candidateCountBuffer);
		CandidatesMat.SetTexture("depthPreTex", ScannerViewDepthRt);
		CandidatesMat.SetTexture("coverageTex", CoverageRt);
		cmdBuffer.DrawMesh(visMesh, modelMat, CandidatesMat);

		// PuffyMat.SetMatrix("vpMat", realProj * (scaleMatrix * VisCam.worldToLocalMatrix));
		// PuffyMat.SetMatrix("vMat", (scaleMatrix * VisCam.worldToLocalMatrix));
		// PuffyMat.SetMatrix("pMat", realProj);
		// PuffyMat.SetVector("camWorldPos", VisCam.position);
		// cmdBuffer.DrawMesh(_puffyMesh, modelMat, PuffyMat);

		Graphics.SetRandomWriteTarget(1, _candidateCountBuffer);

		// X, Y, Z, nX, nY, nZ, C
		Graphics.SetRandomWriteTarget(2, candidateMetaBuffer);
		
		Graphics.ExecuteCommandBuffer(cmdBuffer);
		
		Graphics.ClearRandomWriteTargets();
		cmdBuffer.Release();

		sw.Stop();
		// Debug.Log(candidateCountDataRaw[0] + " time: " + sw.Elapsed.TotalMilliseconds);

		if (Capture) {
			_candidateCountBuffer.GetData(candidateCountDataRaw);
			candidateMetaBuffer.GetData(candidateMetaRaw);

			//----------------------------------------------------------------------------------------------------------------------------
			// Create and process surfels.
			//----------------------------------------------------------------------------------------------------------------------------
			for (int i = 0; i < candidateCountDataRaw[0]; ++i) {
				int idx = i * 7;
				//Debug.Log(candidateMetaRaw[idx + 0] + " " + candidateMetaRaw[idx + 1] + " " + candidateMetaRaw[idx + 2] + " - " + candidateMetaRaw[idx + 6]);

				Surfel s = new Surfel();
				s.scale = 0.005f;
				s.continuous = candidateMetaRaw[idx + 6];
				s.pos = new Vector3(candidateMetaRaw[idx + 0], candidateMetaRaw[idx + 1], candidateMetaRaw[idx + 2]);
				s.normal = new Vector3(candidateMetaRaw[idx + 3], candidateMetaRaw[idx + 4], candidateMetaRaw[idx + 5]);
				_diffuser.surfels.Add(s);
			}

			_diffuser.Process();
			_diffuser.DrawDebug();

			//----------------------------------------------------------------------------------------------------------------------------
			// Update coverage map.
			//----------------------------------------------------------------------------------------------------------------------------
			sw.Restart();

			cmdBuffer = new CommandBuffer();
			cmdBuffer.name = "Coverage update";

			// Pixel candidates.
			cmdBuffer.SetRenderTarget(CoverageRt);
			cmdBuffer.SetViewport(new Rect(0, 0, 4096, 4096));
			// cmdBuffer.ClearRenderTarget(true, true, new Color(0.5f, 0.0f, 0.5f, 1.0f), 1.0f);
			CoverageCandidatesMat.SetMatrix("vpMat", realProj * (scaleMatrix * VisCam.worldToLocalMatrix));
			CoverageCandidatesMat.SetVector("camWorldPos", VisCam.position);
			CoverageCandidatesMat.SetTexture("depthPreTex", ScannerViewDepthRt);
			cmdBuffer.DrawMesh(visMesh, modelMat, CoverageCandidatesMat);

			Graphics.ExecuteCommandBuffer(cmdBuffer);
			
			cmdBuffer.Release();

			sw.Stop();
			Debug.Log("Coverage update - time: " + sw.Elapsed.TotalMilliseconds);
		}
	}

	private void _ClearCoverageMap() {
		CommandBuffer cmdBuffer = new CommandBuffer();
		cmdBuffer.name = "Coverage reset.";

		// Pixel candidates.
		cmdBuffer.SetRenderTarget(CoverageRt);
		cmdBuffer.SetViewport(new Rect(0, 0, 4096, 4096));
		cmdBuffer.ClearRenderTarget(true, true, new Color(0.5f, 0.0f, 0.5f, 1.0f), 1.0f);
		Graphics.ExecuteCommandBuffer(cmdBuffer);
		
		cmdBuffer.Release();
	}

	private void _ScannerViewHit(bool Capture) {
		Stopwatch sw = new Stopwatch();
		Vector3 origin = RayTester.transform.position;
		Vector3 dir = RayTester.transform.forward;
		Ray r = new Ray(origin, dir);
		RaycastHit hit;

		if (_visMeshCollider.Raycast(r, out hit, 1000)) {
			Vector3 idealPos = hit.point + hit.normal * 20.0f;
			Vector3 scannerPos = hit.point + r.direction * -20.0f;

			Debug.DrawLine(hit.point, idealPos, Color.magenta, 0);
			Debug.DrawLine(r.origin, hit.point, Color.yellow, 0);
			Debug.DrawLine(hit.point, scannerPos, Color.blue, 0);

			// Render from scanner perspective.
			VisCam.transform.rotation = Quaternion.LookRotation(dir, Vector3.up);
			// VisCam.transform.position = scannerPos;
			VisCam.transform.position = origin;
			RenderFromScannerView(Capture);
		}

		// int jobCount = 100000; // 3ms

		// var results = new NativeArray<RaycastHit>(jobCount, Allocator.TempJob);
		// var commands = new NativeArray<RaycastCommand>(jobCount, Allocator.TempJob);

		// LayerMask mask = LayerMask.GetMask("VisLayer");

		// for (int i = 0; i < jobCount; ++i) {
		// 	commands[i] = new RaycastCommand(origin, dir, 1000.0f, mask);
		// }

		// sw.Start();
		// JobHandle handle = RaycastCommand.ScheduleBatch(commands, results, 1, default(JobHandle));
		// handle.Complete();
		// sw.Stop();
		// Debug.Log("Hit test time: " + sw.Elapsed.TotalMilliseconds + " ms");

		// if (results[0].collider != null) {
		// 	Debug.DrawLine(origin, results[0].point, Color.green, 0);
		// }

		// results.Dispose();
		// commands.Dispose();
	}

	void OnDestroy() {
		_candidateCountBuffer.Dispose();
		candidateMetaBuffer.Release();

		if (_jobManager != null) {
			_jobManager.Destroy();
		}
	}

	//-----------------------------------------------------------------------------------------------------------
	// Mesh priming.
	//-----------------------------------------------------------------------------------------------------------
	public Mesh SourceMesh;
	public ComputeShader VisCompute;
	public ComputeShader QuantaScoreCompute;
	public RenderTexture VisRt;
	public RenderTexture ScannerViewRt;
	public RenderTexture ScannerViewDepthRt;
	public RenderTexture CoverageRt;
	public Material CoverageCandidatesMat;
	public Transform VisCam;
	public Material VisMat;
	public RenderTexture UvRt;
	public Material UVMat;
	public RenderTexture UvTriRt;
	public Material UvTriMat;
	public RenderTexture OrthoDepthRt;
	public Material OrthoDepthMat;
	public Material CandidatesMat;
	public Material PuffyMat;
	public RenderTexture QuantaBuffer;
	public RenderTexture QuantaScores;
	public Material QuantaVisMat;
	public GameObject Blockers;
	public GameObject LaserLocator;
	public Material LaserCoverageMat;
	public GameObject DebugQuantaScoreView;

	public GameObject RayTester;
	public GameObject PuffyGob;

	public GameObject DiffuserView1;
	public GameObject DiffuserView2;
	public GameObject DiffuserView3;

	public Material BasicShaderMat;

	private Mesh visMesh;
	// private Mesh mesh;
	private Mesh _triIdMesh;
	private Mesh _puffyMesh;
	private ComputeBuffer checklistBuffer;
	private	int[] _visChecklist;
	private int[] _sourceTris;
	private Vector3[] _sourceVerts;
	private Vector3[] _sourceNormals;
	private Vector2[] _sourceUV0;
	private int _sourceTriCount;
	private int _currentTargetFace = -1;
	private int _visRot = 0;
	private Vector3[] _pointsOnSphere;

	private MeshCollider _visMeshCollider;

	private void _PrimeMesh() {
		// Debug.Log(GL.GetGPUProjectionMatrix Camera.main.worldToCameraMatrix);

		_pointsOnSphere = PointsOnSphere(1000);
		for (int i = 0; i < _pointsOnSphere.Length; ++i) {
			//GameObject.Instantiate(SampleLocatorPrefab, _pointsOnSphere[i] * 20.0f, Quaternion.LookRotation(-_pointsOnSphere[i], Vector3.up));
		}

		Stopwatch s = new Stopwatch();
		s.Start();
		LoadMesh();
		CreateTriIdMesh();
		CreateVisibilityMesh();
		s.Stop();
		double t = s.Elapsed.TotalMilliseconds;
		Debug.Log("Visibility culling: " + t + " ms");

		s.Restart();
		CreateUniqueUVs(visMesh);
		s.Stop();
		t = s.Elapsed.TotalMilliseconds;
		Debug.Log("Create unique UVs: " + t + " ms");

		s.Restart();
		CreatePuffyMesh();
		s.Stop();
		t = s.Elapsed.TotalMilliseconds;
		Debug.Log("Create puffy mesh: " + t + " ms");

		s.Restart();
		RenderUVs(visMesh);
		RenderTris(visMesh);
		MeasureArea(visMesh);
		s.Stop();
		t = s.Elapsed.TotalMilliseconds;
		Debug.Log("Render UVs, measure area: " + t + " ms");

		s.Restart();
		ComputeQuantaScores();
		s.Stop();
		t = s.Elapsed.TotalMilliseconds;
		Debug.Log("Compute quanta scores: " + t + " ms");
	}
	
	void LoadMesh() {
		// TODO: Load mesh from file.
		_sourceTris = SourceMesh.GetTriangles(0);
		_sourceVerts = SourceMesh.vertices;
		_sourceNormals = SourceMesh.normals;
		_sourceTriCount = _sourceTris.Length / 3;
	}

	public void CreateTriIdMesh() {
		Vector3[] verts = new Vector3[_sourceTriCount * 3];
		Color32[] colors = new Color32[_sourceTriCount * 3];
		int[] tris = new int[_sourceTriCount * 3];
		
		for (int i = 0; i < _sourceTriCount; ++i) {
			int v0 = _sourceTris[i * 3 + 0];
			int v1 = _sourceTris[i * 3 + 1];
			int v2 = _sourceTris[i * 3 + 2];

			verts[i * 3 + 0] = _sourceVerts[v0];
			verts[i * 3 + 1] = _sourceVerts[v1];
			verts[i * 3 + 2] = _sourceVerts[v2];

			tris[i * 3 + 0] = i * 3 + 0;
			tris[i * 3 + 1] = i * 3 + 1;
			tris[i * 3 + 2] = i * 3 + 2;

			Color32 col = new Color32((byte)(i & 0xFF), (byte)((i >> 8) & 0xFF), (byte)((i >> 16) & 0xFF), 255);
			colors[i * 3 + 0] = col;
			colors[i * 3 + 1] = col;
			colors[i * 3 + 2] = col;
		}

		_triIdMesh = new Mesh();
		_triIdMesh.indexFormat = IndexFormat.UInt32;
		_triIdMesh.vertices = verts;
		_triIdMesh.triangles = tris;
		_triIdMesh.colors32 = colors;
		_triIdMesh.RecalculateBounds();
	}

	public void RenderVisibility() {
		// Bounds b = _triIdMesh.bounds;
		// Matrix4x4 meshMat = new Matrix4x4();
		// Matrix4x4 offset = Matrix4x4.Translate(-b.center);	

		// float xRot = (_visRot / 72) * 5.0f;
		// float yRot = (_visRot % 72) * 5.0f;
		// Matrix4x4 rotMatX = Matrix4x4.Rotate(Quaternion.Euler(0 - xRot, 0, 0));
		// Matrix4x4 rotMatY = Matrix4x4.Rotate(Quaternion.Euler(0, 0, yRot));
		// meshMat = rotMatX * rotMatY * offset;
		
		// Vector3[] bp = new Vector3[8];
		// bp[0] = b.center + new Vector3(b.extents.x, b.extents.y, b.extents.z);
		// bp[1] = b.center + new Vector3(-b.extents.x, b.extents.y, b.extents.z);
		// bp[2] = b.center + new Vector3(-b.extents.x, -b.extents.y, b.extents.z);
		// bp[3] = b.center + new Vector3(b.extents.x, -b.extents.y, b.extents.z);
		// bp[4] = b.center + new Vector3(b.extents.x, b.extents.y, -b.extents.z);
		// bp[5] = b.center + new Vector3(-b.extents.x, b.extents.y, -b.extents.z);
		// bp[6] = b.center + new Vector3(-b.extents.x, -b.extents.y, -b.extents.z);
		// bp[7] = b.center + new Vector3(b.extents.x, -b.extents.y, -b.extents.z);

		// Vector2 min = new Vector2(1000, 1000);
		// Vector2 max = new Vector2(-1000, -1000);

		// for (int i = 0; i < bp.Length; ++i) {
		// 	Vector4 worldPos = meshMat * new Vector4(bp[i].x, bp[i].y, bp[i].z, 1);
		// 	min.x = Mathf.Min(worldPos.x, min.x);
		// 	min.y = Mathf.Min(worldPos.y, min.y);
		// 	max.x = Mathf.Max(worldPos.x, max.x);
		// 	max.y = Mathf.Max(worldPos.y, max.y);
		// }

		CommandBuffer cmdBuffer = new CommandBuffer();
		cmdBuffer.name = "Render visbility";

		Matrix4x4 scaleMatrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, new Vector3(1, 1, -1));
		// Matrix4x4 projMat = Matrix4x4.Ortho(min.x, max.x, max.y, min.y, 0.01f, 300.0f);
		Matrix4x4 projMat = Matrix4x4.Ortho(-15, 15, -15, 15, 0.01f, 300.0f);
		
		Matrix4x4 rotMatX = Matrix4x4.Rotate(Quaternion.Euler(-90, 0, 0));
		Matrix4x4 rotMatY = Matrix4x4.Rotate(Quaternion.Euler(0, 0, 0));
		Matrix4x4 offsetMat = Matrix4x4.Translate(new Vector3(0, -4.75f, 0));
		Matrix4x4 modelMat = offsetMat * rotMatX * rotMatY;
		// Matrix4x4 modelMat = Matrix4x4.identity;
		
		cmdBuffer.SetRenderTarget(VisRt);
		cmdBuffer.SetViewport(new Rect(0, 0, 4096, 4096));
		cmdBuffer.ClearRenderTarget(true, true, new Color(0.5f, 0.5f, 0.5f, 1.0f), 1.0f);
		cmdBuffer.SetViewProjectionMatrices(scaleMatrix * VisCam.worldToLocalMatrix, projMat);
		cmdBuffer.DrawMesh(_triIdMesh, modelMat, VisMat);
		Graphics.ExecuteCommandBuffer(cmdBuffer);

		int kernelIndex = VisCompute.FindKernel("CSMain");
		VisCompute.SetTexture(kernelIndex, "Source", VisRt);
		VisCompute.SetBuffer(kernelIndex, "Checklist", checklistBuffer);
		VisCompute.Dispatch(kernelIndex, 4096 / 8, 4096 / 8, 1);

		cmdBuffer.Release();
	}

	public void CreateVisibilityMesh() {
		_visChecklist = new int[512 * 512];
		checklistBuffer = new ComputeBuffer(_visChecklist.Length, 4);
		checklistBuffer.SetData(_visChecklist);

		for (int i = 0; i < _pointsOnSphere.Length; ++i) {
			Vector3 dir = _pointsOnSphere[i];

			VisCam.transform.rotation = Quaternion.LookRotation(-dir, Vector3.up);
			VisCam.transform.position = dir * 20.0f;
			RenderVisibility();
		}
		
		checklistBuffer.GetData(_visChecklist);

		int triCount = 0;

		for (int i = 0; i < _visChecklist.Length; ++i) {
			if (_visChecklist[i] != 0) {
				++triCount;
			}
		}

		Debug.Log(triCount + "/" + _sourceTriCount);

		Vector3[] verts = new Vector3[triCount * 3];
		// Color32[] colors = new Color32[triCount * 3];
		Vector3[] normals = new Vector3[triCount * 3];
		int[] tris = new int[triCount * 3];
		int idx = 0;
		
		for (int i = 0; i < _sourceTriCount; ++i) {
			if (_visChecklist[i] == 0)
				continue;

			int v0 = _sourceTris[i * 3 + 0];
			int v1 = _sourceTris[i * 3 + 1];
			int v2 = _sourceTris[i * 3 + 2];

			verts[idx * 3 + 0] = _sourceVerts[v0];
			verts[idx * 3 + 1] = _sourceVerts[v1];
			verts[idx * 3 + 2] = _sourceVerts[v2];

			normals[idx * 3 + 0] = _sourceNormals[v0];
			normals[idx * 3 + 1] = _sourceNormals[v1];
			normals[idx * 3 + 2] = _sourceNormals[v2];

			tris[idx * 3 + 0] = idx * 3 + 0;
			tris[idx * 3 + 1] = idx * 3 + 1;
			tris[idx * 3 + 2] = idx * 3 + 2;

			++idx;

			// Color32 col = new Color32((byte)(i & 0xFF), (byte)((i >> 8) & 0xFF), (byte)((i >> 16) & 0xFF), 255);
			// colors[i * 3 + 0] = col;
			// colors[i * 3 + 1] = col;
			// colors[i * 3 + 2] = col;
		}

		if (visMesh != null)
			GameObject.Destroy(visMesh);

		visMesh = new Mesh();
		visMesh.indexFormat = IndexFormat.UInt32;
		visMesh.vertices = verts;
		visMesh.triangles = tris;
		visMesh.normals = normals;
		visMesh.name = "Vis mesh";
		// _triIdMesh.colors32 = colors;

		visMesh.RecalculateNormals();

		GetComponent<MeshFilter>().mesh = visMesh;
		GetComponent<MeshRenderer>().receiveShadows = false;
		GetComponent<MeshRenderer>().shadowCastingMode = ShadowCastingMode.Off;

		Debug.Log("Done computing visibility.");

		_visMeshCollider = gameObject.AddComponent<MeshCollider>();
		_visMeshCollider.sharedMesh = visMesh;
		
		checklistBuffer.Release();
	}

	public void CreatePuffyMesh() {
		// Merge identical verts.
		Vector3[] expandedVerts = visMesh.vertices;
		int[] tris = visMesh.triangles;
		List<Vector3> collapsedVerts = new List<Vector3>();
		Dictionary<Vector3, int> vertMap = new Dictionary<Vector3, int>();
		
		for (int i = 0; i < tris.Length; ++i) {
			Vector3 v = expandedVerts[tris[i]];

			int collapsedId = 0;
			
			if (vertMap.ContainsKey(v)) {
				collapsedId = vertMap[v];
			} else {
				collapsedId = collapsedVerts.Count;
				vertMap[v] = collapsedId;
				collapsedVerts.Add(v);
			}

			tris[i] = collapsedId;
		}

		if (_puffyMesh != null) {
			GameObject.Destroy(_puffyMesh);
		}

		_puffyMesh = new Mesh();
		_puffyMesh.SetVertices(collapsedVerts);
		_puffyMesh.SetTriangles(tris, 0);
		_puffyMesh.RecalculateNormals();

		PuffyGob.GetComponent<MeshFilter>().mesh = _puffyMesh;

		Debug.Log("Collapsed verts: " + collapsedVerts.Count);
	}

	public static Quaternion ExtractRotation(Matrix4x4 Matrix)
	{
		Vector3 forward;
		forward.x = Matrix.m02;
		forward.y = Matrix.m12;
		forward.z = Matrix.m22;
 
		Vector3 upwards;
		upwards.x = Matrix.m01;
		upwards.y = Matrix.m11;
		upwards.z = Matrix.m21;
 
		return Quaternion.LookRotation(forward, upwards);
	}
 
	public static Vector3 ExtractPosition(Matrix4x4 Matrix)
	{
		Vector3 position;
		position.x = Matrix.m03;
		position.y = Matrix.m13;
		position.z = Matrix.m23;
		return position;
	}

	public static Vector3 ExtractScale(Matrix4x4 Matrix)
	{
		Vector3 scale;
		scale.x = new Vector4(Matrix.m00, Matrix.m10, Matrix.m20, Matrix.m30).magnitude;
		scale.y = new Vector4(Matrix.m01, Matrix.m11, Matrix.m21, Matrix.m31).magnitude;
		scale.z = new Vector4(Matrix.m02, Matrix.m12, Matrix.m22, Matrix.m32).magnitude;
		return scale;
	}

	Vector3[] PointsOnSphere(int n)
	{
		List<Vector3> upts = new List<Vector3>();
		float inc = Mathf.PI * (3 - Mathf.Sqrt(5));
		float off = 2.0f / n;
		float x = 0;
		float y = 0;
		float z = 0;
		float r = 0;
		float phi = 0;
	   
		for (var k = 0; k < n; k++){
			y = k * off - 1 + (off /2);
			r = Mathf.Sqrt(1 - y * y);
			phi = k * inc;
			x = Mathf.Cos(phi) * r;
			z = Mathf.Sin(phi) * r;

			Vector3 point = new Vector3(x, y, z);
			upts.Add(point.normalized);

		}
		Vector3[] pts = upts.ToArray();
		return pts;
	}

	public void ComputeQuantaScores() {
		QuantaScores = new RenderTexture(4096, 4096, 24, RenderTextureFormat.ARGB32);
		QuantaScores.filterMode = FilterMode.Point;
		QuantaScores.enableRandomWrite = true;
		QuantaScores.useMipMap = false;
		QuantaScores.Create();

		int kernelIndex = QuantaScoreCompute.FindKernel("CSMain");
		QuantaScoreCompute.SetTexture(kernelIndex, "Source", QuantaBuffer);
		QuantaScoreCompute.SetTexture(kernelIndex, "Result", QuantaScores);

		for (int i = 0; i < _pointsOnSphere.Length; ++i) {
		// for (int i = 0; i < 1; ++i) {
			// i = 0;
			Vector3 dir = _pointsOnSphere[i];
			VisCam.transform.rotation = Quaternion.LookRotation(-dir, Vector3.up);
			VisCam.transform.position = dir * 20.0f;

			CommandBuffer cmdBuffer = new CommandBuffer();
			cmdBuffer.name = "Render ortho depth";

			Matrix4x4 scaleMatrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, new Vector3(1, 1, -1));
			Matrix4x4 projMat = Matrix4x4.Ortho(-15, 15, -15, 15, 1.0f, 40.0f);
			Matrix4x4 realProj = GL.GetGPUProjectionMatrix(projMat, true);
			
			Matrix4x4 rotMatX = Matrix4x4.Rotate(Quaternion.Euler(-90, 0, 0));
			Matrix4x4 rotMatY = Matrix4x4.Rotate(Quaternion.Euler(0, 0, 0));
			Matrix4x4 offsetMat = Matrix4x4.Translate(new Vector3(0, -4.75f, 0));
			Matrix4x4 modelMat = offsetMat * rotMatX * rotMatY;
			// Matrix4x4 modelMat = Matrix4x4.identity;
			
			// Render depth.
			cmdBuffer.SetRenderTarget(OrthoDepthRt);
			cmdBuffer.SetViewport(new Rect(0, 0, 4096, 4096));
			cmdBuffer.ClearRenderTarget(true, true, new Color(0.0f, 0, 0, 1.0f), 1.0f);
			cmdBuffer.SetGlobalMatrix("vpMat", realProj * (scaleMatrix * VisCam.worldToLocalMatrix));
			// cmdBuffer.SetGlobalMatrix("modelMat", modelMat);
			cmdBuffer.DrawMesh(visMesh, modelMat, OrthoDepthMat);
			cmdBuffer.DrawRenderer(Blockers.GetComponent<MeshRenderer>(), OrthoDepthMat);
			Graphics.ExecuteCommandBuffer(cmdBuffer);
			cmdBuffer.Release();
			
			// Render quanta scores in UV space with normal angle and depth visibility.
			QuantaVisMat.SetTexture("_orthoDepthTex", OrthoDepthRt);
			CommandBuffer qCmdBuffer = new CommandBuffer();
			qCmdBuffer.name = "Render quanta buffer";
			qCmdBuffer.SetRenderTarget(QuantaBuffer);
			qCmdBuffer.ClearRenderTarget(true, true, new Color(1.0f, 0.0f, 0.1f, 0.0f), 1.0f);
			qCmdBuffer.SetGlobalMatrix("vpMat", realProj * (scaleMatrix * VisCam.worldToLocalMatrix));
			qCmdBuffer.SetGlobalMatrix("viewMat", scaleMatrix * VisCam.worldToLocalMatrix);
			qCmdBuffer.SetGlobalMatrix("modelMat", modelMat);
			qCmdBuffer.SetGlobalVector("viewForward", -VisCam.forward);
			qCmdBuffer.DrawMesh(visMesh, modelMat, QuantaVisMat, 0, -1);
			Graphics.ExecuteCommandBuffer(qCmdBuffer);
			qCmdBuffer.Release();

			// Accumulate quanta scores.
			QuantaScoreCompute.Dispatch(kernelIndex, 4096 / 8, 4096 / 8, 1);
		}

		Debug.Log("Done quanta view scores");

		Renderer rend = GetComponent<Renderer>();
		rend.material = new Material(Shader.Find("Misc/quantaScoreVis"));
		rend.material.mainTexture = QuantaScores;

		DebugQuantaScoreView.GetComponent<Renderer>().material = rend.material;

		//rend.material = LaserCoverageMat;
	}

	public void CreateUniqueUVsHomogeneous(Mesh Model) {
		Vector3[] sourceVerts = Model.vertices;
		Vector3[] sourceNormals = Model.normals;
		Vector2[] uvs = new Vector2[sourceVerts.Length];
		int triCount = sourceVerts.Length / 3;
		
		float triSize = 0.0025f;
		float padding = 0.00025f;
		float triPad = 0.00025f;

		Vector2 packOffset = new Vector2(padding, padding);
		
		for (int i = 0; i < triCount; ++i) {
			if (i % 2 == 0) {
				uvs[i * 3 + 0] = packOffset + new Vector2(triPad + 0, 0);
				uvs[i * 3 + 1] = packOffset + new Vector2(triPad + triSize, 0);
				uvs[i * 3 + 2] = packOffset + new Vector2(triPad + triSize, triSize);
			} else {
				uvs[i * 3 + 0] = packOffset + new Vector2(0, triPad + 0);
				uvs[i * 3 + 1] = packOffset + new Vector2(0, triPad + triSize);
				uvs[i * 3 + 2] = packOffset + new Vector2(triSize, triPad + triSize);

				packOffset.y += triSize + padding + triPad;

				if (packOffset.y > 1.0f - triSize - triPad - padding) {
					packOffset.x += triSize + padding + triPad;
					packOffset.y = padding;
				}
			}
		}

		// NOTE: This writes directly over the imported asset?
		Model.uv = uvs;
	}

	public void CreateUniqueUVs(Mesh Model) {
		Vector3[] sourceVerts = Model.vertices;
		int[] sourceTris = Model.triangles;
		Vector3[] sourceNormals = Model.normals;
		Vector2[] uvs = new Vector2[sourceVerts.Length];
		int triCount = sourceVerts.Length / 3;
		
		float triSize = 0.0025f;
		float padding = 0.00025f;
		float triPad = 0.00025f;
		// float triScale = 0.025f;
		float triScale = 0.042f;

		// Vector2 packOffset = new Vector2(padding, padding);
		Vector2 packOffset = new Vector2(0.0f, 0.0f);

		float maxY = 0.0f;

		int processTriCount = triCount;

		TriangleRect[] triRects = new TriangleRect[processTriCount];

		Stopwatch sw = new Stopwatch();

		sw.Start();
		for (int i = 0; i < processTriCount; ++i) {
			Vector3 v0 = sourceVerts[sourceTris[i * 3 + 0]];
			Vector3 v1 = sourceVerts[sourceTris[i * 3 + 1]];
			Vector3 v2 = sourceVerts[sourceTris[i * 3 + 2]];

			//Vector3 n1 = _sourceNormals[_sourceTris[i * 3 + 0]].normalized;

			// Get triangle basis
			Vector3 side = (v1 - v0).normalized;
			Vector3 side2 = (v0 - v2).normalized;
			//Vector3 normal = n1;
			Vector3 normal = Vector3.Cross(side, side2).normalized;
			Vector3 sidePerp = Vector3.Cross(side, normal).normalized;

			// Original tri.
			// Debug.DrawLine(v0, v1, Color.white, 10000);
			// Debug.DrawLine(v1, v2, Color.white, 10000);
			// Debug.DrawLine(v2, v0, Color.white, 10000);

			// Basis.
			// Debug.DrawLine(v0, v0 + side * 0.1f, Color.red, 10000);
			// Debug.DrawLine(v0, v0 + normal * 0.1f, Color.green, 10000);
			// Debug.DrawLine(v0, v0 + sidePerp * 0.1f, Color.blue, 10000);

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
			tr.rect = new Rect(0, 0, max.x - min.x, max.y - min.y);
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
		sw.Stop();
		Debug.Log("Flatten - time: " + sw.Elapsed.TotalMilliseconds + " ms");

		// NOTE: Packing algo: https://observablehq.com/@mourner/simple-rectangle-packing
		// Sort in height descending.
		sw.Restart();
		Array.Sort<TriangleRect>(triRects, delegate(TriangleRect A, TriangleRect B) {
			return A.rect.height < B.rect.height ? 1 : -1;
		});
		sw.Stop();
		Debug.Log("Sort - time: " + sw.Elapsed.TotalMilliseconds + " ms");

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

		//------------------------------------------------------------------------------------------------------------------------------
		// Pack rects.
		//------------------------------------------------------------------------------------------------------------------------------
		//float startWidth = 1.0f;

		List<Rect> spaces = new List<Rect>();

		spaces.Add(new Rect(0, 0, 1.0f, 1.0f));

		sw.Restart();
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
		sw.Stop();
		Debug.Log("Pack - time: " + sw.Elapsed.TotalMilliseconds + " ms");

		for (int i = 0; i < triRects.Length; ++i) {
			TriangleRect tr = triRects[i];
			uvs[sourceTris[tr.id * 3 + 0]] = tr.v0 + tr.rect.min;
			uvs[sourceTris[tr.id * 3 + 1]] = tr.v1 + tr.rect.min;
			uvs[sourceTris[tr.id * 3 + 2]] = tr.v2 + tr.rect.min;
		}
		
		// NOTE: This writes directly over the imported asset?
		Model.uv = uvs;
	}

	public void MeasureArea(Mesh Model) {
		Vector3[] sourceVerts = Model.vertices;
		int triCount = sourceVerts.Length / 3;

		float totalArea = 0.0f;

		for (int i = 0; i < triCount; ++i) {
			Vector3 v0 = sourceVerts[i * 3 + 0];
			Vector3 v1 = sourceVerts[i * 3 + 1];
			Vector3 v2 = sourceVerts[i * 3 + 2];

		// 	Vector3 side = (v1 - v0).normalized;
		// }

		// {
			// Vector3 v0 = new Vector3(1, 1, 3);
			// Vector3 v1 = new Vector3(12, 3, 10);
			// Vector3 v2 = new Vector3(15, 7, 10);

			Vector3 sideA = v1 - v0;
			Vector3 sideB = v2 - v0;
			
			float t = Vector3.Dot(sideB, sideA.normalized) / sideA.magnitude;
			
			Vector3 heightPoint = (sideA * t);
			// Debug.Log(heightPoint);
			float height = (v2 - heightPoint - v0).magnitude;
			float length = sideA.magnitude;
			float area = (length * height) * 0.5f;

			totalArea += Mathf.Abs(area);

			// Debug.Log("T: " + t + " height: " + height + " length: " + length + " area: " + area);
		}

		double dotArea = 0.005f * 0.005f;
		double dotCount = totalArea / dotArea;

		Debug.Log("Area (" + triCount + " triangles): " + totalArea + " cm2 dots: " + dotCount);
	}

	public void RenderUVs(Mesh Model) {
		Vector3[] sourceVerts = Model.vertices;
		Vector3[] sourceNormals = Model.normals;
		Vector2[] sourceUVs = Model.uv;
		int triCount = sourceVerts.Length / 3;

		// Create wireframe mesh
		int[] lineTris = new int[triCount * 3 * 2];

		for (int i = 0; i < triCount; ++i) {
			int v0 = i * 3 + 0;
			int v1 = i * 3 + 1;
			int v2 = i * 3 + 2;

			lineTris[i * 6 + 0] = v0;
			lineTris[i * 6 + 1] = v1;
			lineTris[i * 6 + 2] = v1;
			lineTris[i * 6 + 3] = v2;
			lineTris[i * 6 + 4] = v2;
			lineTris[i * 6 + 5] = v0;
		}

		Mesh wfMesh = new Mesh();
		wfMesh.indexFormat = IndexFormat.UInt32;
		wfMesh.vertices = sourceVerts;
		wfMesh.uv = sourceUVs;
		wfMesh.SetIndices(lineTris, MeshTopology.Lines, 0);
		
		CommandBuffer cmdBuffer = new CommandBuffer();
		cmdBuffer.name = "Render UVs";
		cmdBuffer.SetRenderTarget(UvRt);
		cmdBuffer.ClearRenderTarget(true, true, new Color(0.5f, 0.5f, 0.5f, 1.0f), 1.0f);
		cmdBuffer.DrawMesh(wfMesh, Matrix4x4.identity, UVMat, 0, -1);
		Graphics.ExecuteCommandBuffer(cmdBuffer);

		// // byte[] _bytes = UvRt. .EncodeToPNG();
		// System.IO.File.WriteAllBytes(_fullPath, _bytes);
		// Debug.Log(_bytes.Length/1024  + "Kb was saved as: " + _fullPath);
	}

	public void RenderTris(Mesh Model) {
		CommandBuffer cmdBuffer = new CommandBuffer();
		cmdBuffer.name = "Render tris";
		cmdBuffer.SetRenderTarget(UvTriRt);
		cmdBuffer.ClearRenderTarget(true, true, new Color(0.5f, 0.5f, 0.5f, 1.0f), 1.0f);
		cmdBuffer.DrawMesh(Model, Matrix4x4.identity, UvTriMat, 0, -1);
		Graphics.ExecuteCommandBuffer(cmdBuffer);

		// // byte[] _bytes = UvRt. .EncodeToPNG();
		// System.IO.File.WriteAllBytes(_fullPath, _bytes);
		// Debug.Log(_bytes.Length/1024  + "Kb was saved as: " + _fullPath);
	}
}
