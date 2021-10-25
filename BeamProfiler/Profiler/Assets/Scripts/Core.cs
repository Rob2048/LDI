using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Threading;
using UnityEngine.UI;
using System.Diagnostics;

using Debug = UnityEngine.Debug;

public class Core : MonoBehaviour {

	public RawImage CamImage;
	public Slider BrightnessSlider;
	public Slider ContrastSlider;
	public RectTransform CentroidLocator;	
	public Text CentroidLocatorText;
	public RectTransform CentroidBounds;

	private Server _server = new Server();

	private Byte[] _frameData = new Byte[Server.FrameWidth * Server.FrameHeight];
	private int[] _frameCounter = new int[Server.FrameWidth * Server.FrameHeight];
	private int _monotonicCounter = 0;
	private int _frameWidth = Server.FrameWidth;
	private int _frameHeight = Server.FrameHeight;

	private Texture2D _camTexture;
	private Color32[] _camColors = new Color32[Server.FrameWidth * Server.FrameHeight];

	void Start() {
		_server.Init();
		// Start network thread.
		
		_camTexture = new Texture2D(Server.FrameWidth, Server.FrameHeight, TextureFormat.RGB24, false);
		_camTexture.filterMode = FilterMode.Point;
		CamImage.texture = _camTexture;
	}

	private List<int> FloodFillThreshold(int X, int Y, byte Threshold, int ProcessedTag) {
		List<int> result = new List<int>();

		int idx = Y * _frameWidth + X;

		if (_frameData[idx] < Threshold) {
			return result;
		}

		Stack<Vector2Int> pixels = new Stack<Vector2Int>();
			
		pixels.Push(new Vector2Int(X, Y));
		while (pixels.Count != 0) {
			Vector2Int temp = pixels.Pop();
			int y1 = temp.y;

			while (y1 >= 0 && _frameCounter[y1 * _frameWidth + temp.x] != ProcessedTag && _frameData[y1 * _frameWidth + temp.x] >= Threshold) {
				y1--;
			}
			y1++;

			bool spanLeft = false;
			bool spanRight = false;

			while (y1 < _frameHeight && _frameCounter[y1 * _frameWidth + temp.x] != ProcessedTag && _frameData[y1 * _frameWidth + temp.x] >= Threshold)
			{
				// bmp.SetPixel(temp.X, y1, replacementColor);
				result.Add(y1 * _frameWidth + temp.x);
				_frameCounter[y1 * _frameWidth + temp.x] = ProcessedTag;

				if (!spanLeft && temp.x > 0 && _frameCounter[y1 * _frameWidth + temp.x - 1] != ProcessedTag && _frameData[y1 * _frameWidth + temp.x - 1] >= Threshold) {
					pixels.Push(new Vector2Int(temp.x- 1, y1));
					spanLeft = true;
				} else if(spanLeft && temp.x - 1 == 0 && _frameCounter[y1 * _frameWidth + temp.x - 1] != ProcessedTag && _frameData[y1 * _frameWidth + temp.x - 1] < Threshold) {
					spanLeft = false;
				}
				
				if (!spanRight && temp.x < _frameWidth - 1 && _frameCounter[y1 * _frameWidth + temp.x + 1] != ProcessedTag && _frameData[y1 * _frameWidth + temp.x + 1] >= Threshold) {
					pixels.Push(new Vector2Int(temp.x + 1, y1));
					spanRight = true;
				} else if (spanRight && temp.x < _frameWidth - 1 && _frameCounter[y1 * _frameWidth + temp.x + 1] != ProcessedTag && _frameData[y1 * _frameWidth + temp.x + 1] < Threshold) {
					spanRight = false;
				} 
				y1++;
			}

		}

		return result;
	}

	private Color32 GetIntensityColor(byte I) {
		Color32 result = new Color32();

		Color32 cA = new Color32(255, 0, 255, 255);
		Color32 cB = new Color32(0, 255, 0, 255);
		Color32 cC = new Color32(0, 0, 255, 255);
		Color32 cD = new Color32(0, 0, 0, 255);

		if (I >= 255) {
			result.r = 255;
			result.g = 0;
			result.b = 0;
			result.a = 255;
		} else if (I >= 170) {
			result = Color32.Lerp(cB, cA, (float)(I - 170) / 85.0f);
		} else if (I >= 85) {
			result = Color32.Lerp(cC, cB, (float)(I - 85) / 85.0f);
		} else {
			result = Color32.Lerp(cD, cC, (float)I / 85.0f);
		}

		return result;
	}

	private Centroid GetCentroid(List<int> Candidates) {
		Centroid result = new Centroid();

		double posX = 0;
		double posY = 0;
		double totalValue = 0;

		// Get total value of pixels.
		for (int i = 0; i < Candidates.Count; ++i) {
			totalValue += (float)_frameData[Candidates[i]];
		}

		result.min = new Vector2(float.MaxValue, float.MaxValue);
		result.max = new Vector2(float.MinValue, float.MinValue);

		// Apply normalized weights.
		for (int i = 0; i < Candidates.Count; ++i) {
			int idx = Candidates[i];
			float x = (float)(idx % _frameWidth) + 0.5f;
			float y = (float)(idx / _frameWidth) + 0.5f;

			float weight = (float)((double)_frameData[idx] / totalValue);

			posX += x * weight;
			posY += y * weight;

			result.min = Vector2.Min(result.min, new Vector2(x, y));
			result.max = Vector2.Max(result.max, new Vector2(x, y));
		}

		result.position.x = (float)posX;
		result.position.y = (float)posY;

		result.size = result.max - result.min;

		// Debug.Log("Total value: " + totalValue + " Total pixels: " + Candidates.Count + " Pos: " + result.position.x + ", " + result.position.y);

		return result;
	}

	private Vector2 _centroidAvg = new Vector2(0, 0);

	void Update() {

		// Check for new camera frame.

		bool newFrame = false;

		if (Monitor.TryEnter(_server.frameDataLock)) {
			if (_server.frameDataNew) {
				_server.frameDataNew = false;
				newFrame = true;
				Array.Copy(_server.frameData, _frameData, _frameData.Length);
			}

			Monitor.Exit(_server.frameDataLock);
		}

		if (newFrame) {
			// Debug.Log("Got new frame on main thread. " + Time.time);

			float contrast = ContrastSlider.value;
			float brightness = BrightnessSlider.value;

			Color32 cA = new Color32(255, 0, 255, 255);
			Color32 cB = new Color32(0, 255, 0, 255);
			Color32 cC = new Color32(0, 0, 255, 255);
			Color32 cD = new Color32(0, 0, 0, 255);

			Stopwatch sw = new Stopwatch();
			sw.Start();

			// Only process inner portion of frame.

			++_monotonicCounter;

			for (int iY = 50; iY < _frameHeight - 100; ++iY) {
				for (int iX = 50; iX < _frameWidth - 100; ++ iX) {
					int i = iY * _frameWidth + iX;
					// byte f = (byte)(Mathf.Clamp(_frameData[i] * contrast + brightness, 0, 255));
					byte f = _frameData[i];

					_camColors[i].a = 255;

					// // If pixel has already been processed, then forget it.
					// if (_frameCounter[i] == _monotonicCounter) {
					// 	continue;
					// }

					// // Threshold
					// if (f > 250) {
					// 	List<int> filledPixels = FloodFillThreshold(iX, iY, 80, _monotonicCounter);

					// 	for (int j = 0; j < filledPixels.Count; ++j) {
					// 		_camColors[filledPixels[j]] = GetIntensityColor(_frameData[filledPixels[j]]);
					// 	}

					// 	Centroid centroid = GetCentroid(filledPixels);

					// 	_centroidAvg = _centroidAvg * 0.5f  + centroid.position * 0.5f;

					// 	Vector2 realPos = _centroidAvg * 2.24f;

					// 	CentroidLocator.anchoredPosition = _centroidAvg;
					// 	CentroidLocatorText.text = realPos.x.ToString("0.0") + ", " + realPos.y.ToString("0.0");
					// 	CentroidBounds.anchoredPosition = centroid.min + centroid.size / 2.0f;
					// 	CentroidBounds.sizeDelta = centroid.size;
					// } else {
					// 	_camColors[i].r = 0;
					// 	_camColors[i].g = 0;
					// 	_camColors[i].b = 0;
					// }

					// Color map.
					if (f >= 255) {
						_camColors[i].r = 255;
						_camColors[i].g = 255;
						_camColors[i].b = 255;
						_camColors[i].a = 255;
					} else if (f >= 170) {
						_camColors[i] = Color32.Lerp(cB, cA, (float)(f - 170) / 85.0f);
					} else if (f >= 85) {
						_camColors[i] = Color32.Lerp(cC, cB, (float)(f - 85) / 85.0f);
					} else {
						_camColors[i] = Color32.Lerp(cD, cC, (float)f / 85.0f);
					}

				}
			}

			sw.Start();
			Debug.Log("Frame process time: " + sw.Elapsed.TotalMilliseconds + " ms");

			_camTexture.SetPixels32(_camColors);
			_camTexture.Apply(false);
		}
	}

	void OnDestroy() {
		_server.Destroy();
	}
}
