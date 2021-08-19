using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using UnityEngine;
using UnityEngine.UI;

using Debug = UnityEngine.Debug;

public class Core : MonoBehaviour
{
	public pocketNc Pnc;

	public GameObject CamVisPrefab;
	public GameObject CubePlanePrefab;

	public Material PointBillboardMaterial;

	public GameObject CharucoCube;
	public GameObject CalibRefPrefab;
	public Camera CameraLeft;
	public Camera CameraRight;
	public ParticleSystem PointCloud;
	
	// UI.
	public RectTransform LocatorContainer;
	public GameObject LocatorPrefab;
	public Slider BrightnessSlider;
	public Slider ContrastSlider;
	public RawImage CameraLeftImage;
	public RawImage CameraRightImage;

	// Cameras
	public CamPeripheral[] cams = new CamPeripheral[2];

	// Shared persistent systems.
	public NativeCore nativeCore;
	public CameraServer camServer;

	private int _state = 0;
	private int _stateStep = 0;
	private List<CharucoBoard> _boards;
	private List<PoseEstimation> _poseEstimations;

	private Vector3[] _calibBoardPos = {
		new Vector3(-10, -5, -5),
		new Vector3(-5, -5, -5),
		new Vector3(5, -5, -5),
		new Vector3(10, -5, -5),

		new Vector3(-10, 0, 0),
		new Vector3(-5, 0, 0),
		new Vector3(5, 0, 0),
		new Vector3(10, 0, 0),

		new Vector3(-10, 5, 10),
		new Vector3(-5, 5, 10),
		new Vector3(5, 5, 10),
		new Vector3(10, 5, 10),
	};

	private ParticleSystem.Particle[] _pointCloudPoints;

	private List<CalibPoint> _smallCalibCube = new List<CalibPoint>();
	private Dictionary<int, CalibPoint> _smallCalibCubeIndex = new Dictionary<int, CalibPoint>();

	void Start() {
		// Small calibration cube.
		float sqrSize = 0.9f;
		float cubeSize = 4.0f;
		float cubeHalf = cubeSize * 0.5f;
		
		// Y+
		int boardId = 1;
		_smallCalibCube.Add(new CalibPoint(0, new Vector3(-sqrSize, cubeSize, sqrSize), boardId));
		_smallCalibCube.Add(new CalibPoint(1, new Vector3(-sqrSize, cubeSize, 0), boardId));
		_smallCalibCube.Add(new CalibPoint(2, new Vector3(-sqrSize, cubeSize, -sqrSize), boardId));

		_smallCalibCube.Add(new CalibPoint(3, new Vector3(0, cubeSize, sqrSize), boardId));
		_smallCalibCube.Add(new CalibPoint(4, new Vector3(0, cubeSize, 0), boardId));
		_smallCalibCube.Add(new CalibPoint(5, new Vector3(0, cubeSize, -sqrSize), boardId));

		_smallCalibCube.Add(new CalibPoint(6, new Vector3(sqrSize, cubeSize, sqrSize), boardId));
		_smallCalibCube.Add(new CalibPoint(7, new Vector3(sqrSize, cubeSize, 0), boardId));
		_smallCalibCube.Add(new CalibPoint(8, new Vector3(sqrSize, cubeSize, -sqrSize), boardId));

		// Z+
		boardId = 3;
		_smallCalibCube.Add(new CalibPoint(0, new Vector3(-sqrSize, sqrSize + cubeHalf, cubeHalf), boardId));
		_smallCalibCube.Add(new CalibPoint(1, new Vector3(0, sqrSize + cubeHalf, cubeHalf), boardId));
		_smallCalibCube.Add(new CalibPoint(2, new Vector3(sqrSize, sqrSize + cubeHalf, cubeHalf), boardId));

		_smallCalibCube.Add(new CalibPoint(3, new Vector3(-sqrSize, cubeHalf, cubeHalf), boardId));
		_smallCalibCube.Add(new CalibPoint(4, new Vector3(0, cubeHalf, cubeHalf), boardId));
		_smallCalibCube.Add(new CalibPoint(5, new Vector3(sqrSize, cubeHalf, cubeHalf), boardId));

		_smallCalibCube.Add(new CalibPoint(6, new Vector3(-sqrSize, -sqrSize + cubeHalf, cubeHalf), boardId));
		_smallCalibCube.Add(new CalibPoint(7, new Vector3(0, -sqrSize + cubeHalf, cubeHalf), boardId));
		_smallCalibCube.Add(new CalibPoint(8, new Vector3(sqrSize, -sqrSize + cubeHalf, cubeHalf), boardId));

		// X+
		boardId = 4;
		_smallCalibCube.Add(new CalibPoint(0, new Vector3(cubeHalf, -sqrSize + cubeHalf, -sqrSize), boardId));
		_smallCalibCube.Add(new CalibPoint(3, new Vector3(cubeHalf, cubeHalf, -sqrSize), boardId));
		_smallCalibCube.Add(new CalibPoint(6, new Vector3(cubeHalf, sqrSize + cubeHalf, -sqrSize), boardId));

		_smallCalibCube.Add(new CalibPoint(1, new Vector3(cubeHalf, -sqrSize + cubeHalf, 0), boardId));
		_smallCalibCube.Add(new CalibPoint(4, new Vector3(cubeHalf, cubeHalf, 0), boardId));
		_smallCalibCube.Add(new CalibPoint(7, new Vector3(cubeHalf, sqrSize + cubeHalf, 0), boardId));

		_smallCalibCube.Add(new CalibPoint(2, new Vector3(cubeHalf, -sqrSize + cubeHalf, sqrSize), boardId));
		_smallCalibCube.Add(new CalibPoint(5, new Vector3(cubeHalf, cubeHalf, sqrSize), boardId));
		_smallCalibCube.Add(new CalibPoint(8, new Vector3(cubeHalf, sqrSize + cubeHalf, sqrSize), boardId));

		// X-
		boardId = 0;
		_smallCalibCube.Add(new CalibPoint(0, new Vector3(-cubeHalf, -sqrSize + cubeHalf, -sqrSize), boardId));
		_smallCalibCube.Add(new CalibPoint(1, new Vector3(-cubeHalf, cubeHalf, -sqrSize), boardId));
		_smallCalibCube.Add(new CalibPoint(2, new Vector3(-cubeHalf, sqrSize + cubeHalf, -sqrSize), boardId));

		_smallCalibCube.Add(new CalibPoint(3, new Vector3(-cubeHalf, -sqrSize + cubeHalf, 0), boardId));
		_smallCalibCube.Add(new CalibPoint(4, new Vector3(-cubeHalf, cubeHalf, 0), boardId));
		_smallCalibCube.Add(new CalibPoint(5, new Vector3(-cubeHalf, sqrSize + cubeHalf, 0), boardId));

		_smallCalibCube.Add(new CalibPoint(6, new Vector3(-cubeHalf, -sqrSize + cubeHalf, sqrSize), boardId));
		_smallCalibCube.Add(new CalibPoint(7, new Vector3(-cubeHalf, cubeHalf, sqrSize), boardId));
		_smallCalibCube.Add(new CalibPoint(8, new Vector3(-cubeHalf, sqrSize + cubeHalf, sqrSize), boardId));

		// Z-
		boardId = 5;
		_smallCalibCube.Add(new CalibPoint(8, new Vector3(-sqrSize, sqrSize + cubeHalf, -cubeHalf), boardId));
		_smallCalibCube.Add(new CalibPoint(5, new Vector3(0, sqrSize + cubeHalf, -cubeHalf), boardId));
		_smallCalibCube.Add(new CalibPoint(2, new Vector3(sqrSize, sqrSize + cubeHalf, -cubeHalf), boardId));

		_smallCalibCube.Add(new CalibPoint(7, new Vector3(-sqrSize, cubeHalf, -cubeHalf), boardId));
		_smallCalibCube.Add(new CalibPoint(4, new Vector3(0, cubeHalf, -cubeHalf), boardId));
		_smallCalibCube.Add(new CalibPoint(1, new Vector3(sqrSize, cubeHalf, -cubeHalf), boardId));

		_smallCalibCube.Add(new CalibPoint(6, new Vector3(-sqrSize, -sqrSize + cubeHalf, -cubeHalf), boardId));
		_smallCalibCube.Add(new CalibPoint(3, new Vector3(0, -sqrSize + cubeHalf, -cubeHalf), boardId));
		_smallCalibCube.Add(new CalibPoint(0, new Vector3(sqrSize, -sqrSize + cubeHalf, -cubeHalf), boardId));

		for (int i = 0; i < _smallCalibCube.Count; ++i) {
			_smallCalibCubeIndex.Add(_smallCalibCube[i].fullId, _smallCalibCube[i]);
		}

		// GameObject cco = new GameObject("CalibCubeOutput");

		// for (int i = 0; i < _smallCalibCube.Count; ++i) {
		// 	CalibPoint c = _smallCalibCube[i];

		// 	GameObject gob = GameObject.CreatePrimitive(PrimitiveType.Sphere);
		// 	gob.transform.SetParent(cco.transform);
		// 	gob.transform.localPosition = c.pos;
		// 	gob.transform.localScale = new Vector3(0.1f, 0.1f, 0.1f);
		// 	gob.name = c.name;
		// }

		_camTexture = new Texture2D(1920, 1080, TextureFormat.RGB24, false);
		_camTexture.filterMode = FilterMode.Point;
		nativeCore = new NativeCore();
		nativeCore.Init();
		
		camServer = new CameraServer();
		camServer.Init();

		cams[0] = new VirtualCam(CameraLeft, 1920, 1080);
		// CameraLeftImage.texture = cams[0].frameTexture;

		CameraLeftImage.texture = _camTexture;

		cams[1] = new VirtualCam(CameraRight, 1920, 1080);
		CameraRightImage.texture = cams[1].frameTexture;
	}

	void OnDestroy() {
		if (nativeCore != null) {
			nativeCore.Destroy();
		}
	}

	private Texture2D _camTexture;
	private Color32[] _camColors = new Color32[1920 * 1080];

	void Update() {
		camServer.Update(Time.deltaTime);

		byte[] camBuffer = null;

		if (camServer.GetLatestFrame(ref camBuffer)) {
			Stopwatch sw = new Stopwatch();
			sw.Start();
			float contrast = ContrastSlider.value;
			float brightness = BrightnessSlider.value;

			for (int i = 0; i < 1920 * 1080; ++i) {
				//float value = (camBuffer[i] * ContrastSlider.value + BrightnessSlider.value);
				byte f = (byte)(Mathf.Clamp(camBuffer[i] * contrast + brightness, 0, 255));
				// byte f = camBuffer[i];//-24.245 -24.15 = 0.095, -25.193, 0.948
				// f = (byte)(Mathf.Clamp(f, 0, 255));

				//  = new Color32(f, f, f, 255);
				_camColors[i].r = f;
				_camColors[i].g = f;
				_camColors[i].b = f;
				_camColors[i].a = 255;
			}
			sw.Stop();
			// Debug.Log("Time: " + sw.Elapsed.TotalMilliseconds + " ms");

			_camTexture.SetPixels32(_camColors);
			_camTexture.Apply(false);
		}
		
		if (_state == 0) {
			// Normal view process.
			//cams[0].Update(Time.deltaTime);
		} else if (_state == 1) {
			_RunCalibration();
		} else if (_state == 2) {
			_RunBundleAdjustSingleCamera();
		} else if (_state == 3) {
			_RunBundleAdjustStereoCamera();
		}
	}

	private void _RunCalibration() {
		// Calibration process.
		if (_stateStep < 24) {

			// Position everything for this update.
			//CharucoCube.transform.rotation = Quaternion.Euler(24, -52, -30);
			//CharucoCube.transform.position = new Vector3(_stateStep * 2 - 10, 0, 0);
			
			//CharucoCube.transform.rotation = Quaternion.Euler(45, 0, 0);
			CharucoCube.transform.rotation = Random.rotation;
			CharucoCube.transform.position = _calibBoardPos[_stateStep % 12];
			//Debug.Log(_stateStep + ": " + CharucoCube.transform.position);

			// Take images.
			cams[0].StoreLatestFrame();

			byte[] imgData = GetProcessReadyFrame(cams[0]);
			List<CharucoBoard> newBoards = nativeCore.FindCharuco(1920, 1080, imgData, -1);

			// Only add valid calibration boards.
			// Needs X amount of markers to be valid.
			//_boards.AddRange(newBoards);
			for (int i = 0; i < newBoards.Count; ++i) {
				if (newBoards[i].markers.Count > 5) {
					_boards.Add(newBoards[i]);
				}
			}
			
			//byte[] frameImg = cams[0].frameTexture.EncodeToPNG();
			//System.IO.File.WriteAllBytes("calib captures/cap_" + _stateStep + ".png", frameImg);

			_stateStep++;
		} else {
			Debug.Log("Completed capture cycle");
			_state = 0;

			Debug.Log("Calibrating with " + _boards.Count + " boards");
			cams[0].calibration = nativeCore.CalibrateCameraCharuco(1920, 1080, _boards);
		}
	}

	private void _RunBundleAdjustSingleCamera() {
		if (_stateStep < 24) {
			CharucoCube.transform.rotation = Random.rotation;
			CharucoCube.transform.position = _calibBoardPos[_stateStep % 12];
			
			cams[0].StoreLatestFrame();

			byte[] imgData = GetProcessReadyFrame(cams[0]);
			List<CharucoBoard> newBoards = nativeCore.FindCharuco(1920, 1080, imgData, -1);

			// Only add valid calibration boards.
			// Needs X amount of markers to be valid. (Why? -_-)
			for (int i = 0; i < newBoards.Count; ++i) {
				if (newBoards[i].markers.Count > 5) {
					PoseEstimation pose = new PoseEstimation();
					if (nativeCore.GetCharucoPose(cams[0].calibration, newBoards[i], ref pose)) {
						_poseEstimations.Add(pose);
						_boards.Add(newBoards[i]);
					}
				}
			}

			_stateStep++;
		} else {
			Debug.Log("Completed capture cycle");
			_state = 0;

			Debug.Log("Bundle adjusting with " + _poseEstimations.Count + " poses");

			int squareCount = 9;
			float squareSize = 0.9f;

			using (StreamWriter writer = new StreamWriter("ssba_in.txt")) {
				int pointCount = squareCount * squareCount;
				int viewCount = _poseEstimations.Count;
				int measurementCount = 0;

				for (int i = 0; i < _boards.Count; ++i) {
					measurementCount += _boards[i].markers.Count;
				}

				writer.WriteLine(pointCount + " " + viewCount + " " + measurementCount);

				// <fx> <skew> <cx> <fy> <cy> <k1> <k2> <p1> <p2>

				CalibratedCamera cc = cams[0].calibration;

				writer.WriteLine(cc.mat[0] + " " + cc.mat[1] + " " +  cc.mat[2] + " " + cc.mat[4] + " " + cc.mat[5] + " " + 
					cc.distCoeffs[0] + " " + cc.distCoeffs[1] + " " + cc.distCoeffs[2] + " " + cc.distCoeffs[3]);
				
				for (int iY = 0; iY < squareCount; ++iY) {
					for (int iX = 0; iX < squareCount; ++iX) {
						Vector3 pos = new Vector3((iX + 1) * squareSize, (iY + 1) * squareSize, 0);
						int idx = iX + iY * squareCount;

						writer.WriteLine(idx + " " + pos.x + " " + pos.y + " " + pos.z);
					}
				}

				for (int i = 0; i < _poseEstimations.Count; ++i) {
					PoseEstimation pose = _poseEstimations[i];
					Matrix4x4 camRt = Matrix4x4.TRS(pose.pos, pose.rotMat.rotation, Vector3.one);

					string output = i.ToString();

					Debug.Log(camRt);

					for (int j = 0; j < 12; ++j) {
						// NOTE: Index column major, output as row major.
						output += " " + camRt[(j % 4) * 4 + (j / 4)];
					}

					writer.WriteLine(output);
				}

				for (int i = 0; i < _boards.Count; ++i) {
					for (int j = 0; j < _boards[i].markers.Count; ++j) {
						CharucoMarker marker = _boards[i].markers[j];

						writer.WriteLine(i + " " + marker.id + " " + marker.pos.x + " " + marker.pos.y + " 1");
					}
				}
			}
		}
	}

	private List<int> _sharedMarkerIds = new List<int>();
	private List<Vector2> _sharedMarkerPos0 = new List<Vector2>();
	private List<Vector2> _sharedMarkerPos1 = new List<Vector2>();

	private List<Vector3> _totalPositions = new List<Vector3>();

	private void _RunBundleAdjustStereoCamera() {
		if (_stateStep < _totalPositions.Count) {
			//CharucoCube.transform.rotation = Random.rotation;
			//CharucoCube.transform.position = _calibBoardPos[_stateStep % 12];

			//CharucoCube.transform.position = Vector3.zero;
			//CharucoCube.transform.rotation = Quaternion.AngleAxis(90.0f / 24.0f * _stateStep - 45.0f, Vector3.up);

			//CharucoCube.transform.position = new Vector3(-10.0f + (_stateStep / 24.0f) * 20.0f, 0.0f, 0.0f);
			//CharucoCube.transform.rotation = Quaternion.identity;
			CharucoCube.transform.position = _totalPositions[_stateStep];
			CharucoCube.transform.rotation = Quaternion.Euler(-45, 35, 0);
			
			cams[0].StoreLatestFrame();
			cams[1].StoreLatestFrame();

			byte[] imgData = GetProcessReadyFrame(cams[0]);
			List<CharucoBoard> newBoardsCam0 = nativeCore.FindCharuco(1920, 1080, imgData, -1);

			imgData = GetProcessReadyFrame(cams[1]);
			List<CharucoBoard> newBoardsCam1 = nativeCore.FindCharuco(1920, 1080, imgData, -1);

			int frameId = _stateStep;

			// Find markers that are visible by both views.
			for (int i = 0; i < newBoardsCam0.Count; ++i) {
				CharucoBoard b0 = newBoardsCam0[i];

				for (int j = 0; j < newBoardsCam1.Count; ++j) {
					CharucoBoard b1 = newBoardsCam1[j];

					if (b0.id == b1.id) {

						for (int iM = 0; iM < b0.markers.Count; ++iM) {
							CharucoMarker m0 = b0.markers[iM];

							for (int jM = 0; jM < b1.markers.Count; ++jM) {
								CharucoMarker m1 = b1.markers[jM];

								if (m0.id == m1.id) {
									// Found a shared marker.
									_sharedMarkerIds.Add(frameId * 1000 + b0.id * 100 + m0.id);
									_sharedMarkerPos0.Add(m0.pos);
									_sharedMarkerPos1.Add(m1.pos);
								}
							}
						}
					}
				}
			}

			_stateStep++;
		} else {
			Debug.Log("Completed capture cycle");
			_state = 0;

			Debug.Log("Shared maker count: " + _sharedMarkerIds.Count);

			// Find relative positions of each camera.
			CharucoCube.transform.rotation = Quaternion.identity;
			CharucoCube.transform.position = Vector3.zero;

			// NOTE: Only pose match on charuco board 0.
			{
				cams[0].StoreLatestFrame();
				byte[] imgData = GetProcessReadyFrame(cams[0]);
				List<CharucoBoard> newBoards = nativeCore.FindCharuco(1920, 1080, imgData, 100);

				for (int i = 0; i < newBoards.Count; ++i) {
					if (newBoards[i].id == 0) {
						PoseEstimation pose = new PoseEstimation();
						if (nativeCore.GetCharucoPose(cams[0].calibration, newBoards[i], ref pose)) {
							Debug.Log("Found pose cam0");
							cams[0].calibration.r = pose.rotMat;
							cams[0].calibration.t = pose.pos;
							cams[0].calibration.CalculateMats();
							
							Matrix4x4 poseMatInv = cams[0].calibration.rt.inverse;
							GameObject camTest = GameObject.CreatePrimitive(PrimitiveType.Cube);
							camTest.transform.position = new Vector3(poseMatInv.m03, poseMatInv.m13, poseMatInv.m23);
							camTest.transform.rotation = poseMatInv.rotation;
						}
					}
				}
			}

			{
				cams[1].StoreLatestFrame();
				byte[] imgData = GetProcessReadyFrame(cams[1]);
				List<CharucoBoard> newBoards = nativeCore.FindCharuco(1920, 1080, imgData, 101);

				for (int i = 0; i < newBoards.Count; ++i) {
					if (newBoards[i].id == 0) {
						PoseEstimation pose = new PoseEstimation();
						if (nativeCore.GetCharucoPose(cams[1].calibration, newBoards[i], ref pose)) {
							Debug.Log("Found pose cam1");
							cams[1].calibration.r = pose.rotMat;
							cams[1].calibration.t = pose.pos;
							cams[1].calibration.CalculateMats();
							
							Matrix4x4 poseMatInv = cams[1].calibration.rt.inverse;
							GameObject camTest = GameObject.CreatePrimitive(PrimitiveType.Cube);
							camTest.transform.position = new Vector3(poseMatInv.m03, poseMatInv.m13, poseMatInv.m23);
							camTest.transform.rotation = poseMatInv.rotation;
						}
					}
				}
			}

			// Triangulate shared markers.
			// TODO: There is no distortion correction here. Triangulation must happen in non-distorted space.

			Debug.Log("Cam proj0: " + cams[0].calibration.p);
			Debug.Log("Cam proj1: " + cams[1].calibration.p);
			
			Debug.Log("Triangulating...");
			List<Vector3> triangulatedPoints = new List<Vector3>();
			nativeCore.TriangulatePoints(cams[0].calibration.p, cams[1].calibration.p, _sharedMarkerPos0, _sharedMarkerPos1, ref triangulatedPoints);

			if (triangulatedPoints.Count != _sharedMarkerIds.Count) {
				Debug.LogError("Point counts don't match");
			}

			// TODO: Find planarity of boards, and error of marker positions.
			// Show the overall error in point cloud colors.
			// Plane fitting?

			_pointCloudPoints = new ParticleSystem.Particle[triangulatedPoints.Count];
		
			for (int i = 0; i < triangulatedPoints.Count; ++i) {
				//GameObject gob = GameObject.Instantiate(PointCloudPointPrefab);
				//gob.transform.position = triangulatedPoints[i];

				int pointIdx = _sharedMarkerIds[i];

				int frameId = pointIdx / 1000;
				int boardId = (pointIdx - (frameId * 1000)) / 100;
				int markerId =  (pointIdx - (frameId * 1000) - (boardId * 100));

				Random.InitState(boardId + frameId * 10);
				Color col = Random.ColorHSV(0, 1, 1, 1);
				
				_pointCloudPoints[i].position = triangulatedPoints[i];
				_pointCloudPoints[i].startSize = 0.02f;
				_pointCloudPoints[i].startColor = col;
				_pointCloudPoints[i].startLifetime = 1000.0f;
				_pointCloudPoints[i].remainingLifetime = 1000.0f;
			}

			PointCloud.SetParticles(_pointCloudPoints, triangulatedPoints.Count, 0);
			
			using (StreamWriter writer = new StreamWriter("ssba_var.txt")) {
				int pointCount = _sharedMarkerIds.Count;
				
				writer.WriteLine(pointCount + " 2 " + (pointCount * 2));

				// <fx> <skew> <cx> <fy> <cy> <k1> <k2> <p1> <p2>

				for (int i = 0; i < 2; ++i) {
					CalibratedCamera cc = cams[i].calibration;

					writer.WriteLine(cc.mat[0] + " " + cc.mat[1] + " " +  cc.mat[2] + " " + cc.mat[4] + " " + cc.mat[5] + " " + 
						cc.distCoeffs[0] + " " + cc.distCoeffs[1] + " " + cc.distCoeffs[2] + " " + cc.distCoeffs[3]);
				}
				
				for (int i = 0; i < _sharedMarkerIds.Count; ++i) {
					Vector3 p = triangulatedPoints[i];
					writer.WriteLine(_sharedMarkerIds[i] + " " + p.x + " " + p.y + " " + p.z);
				}
				
				for (int i = 0; i < 2; ++i) {
					CalibratedCamera cc = cams[i].calibration;

					string output = i.ToString();

					for (int j = 0; j < 12; ++j) {
						// NOTE: Index column major, output as row major.
						output += " " + cc.rt[(j % 4) * 4 + (j / 4)];
					}

					writer.WriteLine(output);
				}

				for (int i = 0; i < _sharedMarkerIds.Count; ++i) {
					Vector2 p = _sharedMarkerPos0[i];
					writer.WriteLine("0 " + _sharedMarkerIds[i] + " " + p.x + " " + p.y + " 1");
				}

				for (int i = 0; i < _sharedMarkerIds.Count; ++i) {
					Vector2 p = _sharedMarkerPos1[i];
					writer.WriteLine("1 " + _sharedMarkerIds[i] + " " + p.x + " " + p.y + " 1");
				}
			}

			// https://en.wikipedia.org/wiki/Point_set_registration
			// What global scale do we end up with after bundle adjust?
			// OpenCV estimateAffine3D() Computes transform between 2 3d points.
			// Spring based multi point view error minimization.
			// bundle adjust, rough align each frame's dataset.
			// Open3D
			// PCL - point cloud library
			// https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.47.3503&rep=rep1&type=pdf
			// https://www.isprs.org/proceedings/xxxiv/part3/papers/paper158.pdf
			// https://github.com/abreheret/icp-opencv/blob/master/src/icp-opencv/icp.c
		}
	}

	// Get raw frame input, convert to grayscale single channel output as bytes.
	private byte[] _processReadyTemp = new byte[1920 * 1080];
	public byte[] GetProcessReadyFrame(CamPeripheral Cam) {
		Color32[] frameColors = Cam.frameTexture.GetPixels32();

		for (int i = 0; i < frameColors.Length; ++i) {
			_processReadyTemp[i] = frameColors[i].r;
		}

		return _processReadyTemp;
	}

	public byte[] GetProcessReadyFrame(PhysicalCam Cam) {
		byte[] camBuffer = Cam._frameBuffer;

		float contrast = ContrastSlider.value;
		float brightness = BrightnessSlider.value;

		for (int i = 0; i < 1920 * 1080; ++i) {
			//float value = (camBuffer[i] * ContrastSlider.value + BrightnessSlider.value);
			byte f = (byte)(Mathf.Clamp(camBuffer[i] * contrast + brightness, 0, 255));
			// byte f = camBuffer[i];//-24.245 -24.15 = 0.095, -25.193, 0.948
			// f = (byte)(Mathf.Clamp(f, 0, 255));

			//  = new Color32(f, f, f, 255);
			_processReadyTemp[i] = f;
		}

		return _processReadyTemp;
	}

	public void OnGenericCapture() {
		// byte[] imgData = GetProcessReadyFrame(cams[0]);

		// List<Blob> blobs = nativeCore.GenericAnalysis(1920, 1080, imgData);

		// // Build markers
		// foreach(RectTransform child in LocatorContainer) {
		// 	Destroy(child.gameObject);
		// }

		// for (int i = 0; i < blobs.Count; ++i) {
		// 	GameObject locator = GameObject.Instantiate(LocatorPrefab, LocatorContainer);
		// 	locator.GetComponent<RectTransform>().localPosition = new Vector3(blobs[i].pos.x, blobs[i].pos.y, 0);
		// 	locator.GetComponent<RectTransform>().sizeDelta = new Vector2(blobs[i].size, blobs[i].size);

		// 	// int col = i / 4;
		// 	// int row = i % 11;
		// 	// Color clr = new Color(0.1f, 1.0f, 0.2f, 0.5f);
		// 	locator.GetComponent<Image>().color = blobs[i].color;
		// 	locator.transform.GetChild(0).GetComponent<Text>().text = blobs[i].text;
		// }
	}

	public void SaveMainCamImage(string FileName, bool DataFile, string DataFileName, MachineFrame Frame) {
		Debug.Log("Save image: " + FileName);

		float contrast = ContrastSlider.value;
		float brightness = BrightnessSlider.value;

		byte[] camBuffer = null;

		camServer.GetLatestFrame(ref camBuffer);

		for (int i = 0; i < 1920 * 1080; ++i) {
			camBuffer[i] = (byte)(Mathf.Clamp(camBuffer[i] * contrast + brightness, 0, 255));

			_camColors[i].r = camBuffer[i];
			_camColors[i].g = camBuffer[i];
			_camColors[i].b = camBuffer[i];
			_camColors[i].a = 255;
		}

		if (!DataFile) {
			_camTexture.SetPixels32(_camColors);
			_camTexture.Apply(false);

			byte[] frameImg = _camTexture.EncodeToPNG();
			System.IO.File.WriteAllBytes(FileName, frameImg);
		}

		if (DataFile) {
			BinaryWriter br = new BinaryWriter(new MemoryStream(1920 * 1080 + 5 * 4));
			br.Write(Frame.x);
			br.Write(Frame.y);
			br.Write(Frame.z);
			br.Write(Frame.a);
			br.Write(Frame.b);
			br.Write(camBuffer, 0, 1920 * 1080);
			System.IO.File.WriteAllBytes(DataFileName, ((MemoryStream)br.BaseStream).GetBuffer());
		}
	}

	public void OnCalibrate() {
		// Pnc.MoveToPosition(60, 10, 0, 45, 60);
		// Pnc.QueueCommand(new DelayCmd(this, 300));
		// Pnc.QueueCommand(new SaveCameraFrameCmd(this, "calib seq/step1.png"));
		// Pnc.MoveToPosition(45, 0, 0, 0, 0);
		// Pnc.QueueCommand(new DelayCmd(this, 300));
		// Pnc.QueueCommand(new SaveCameraFrameCmd(this, "calib seq/step2.png"));

		int calibId = 200;

		// Pose14 = XYZ all zero B-70

		// float[] bAngles = { -70, 20, 110, 200 };
		// float[] bAngles = { 10 };

		// for (int iB = 0; iB < bAngles.Length; ++iB) {
		// 	float b = bAngles[iB];

		// 	for (int iY = 0; iY < 4; ++iY) {
		// 		for (int i = 0; i < 8; ++i) {
		// 			Pnc.MoveToPosition(60 - i * 10, iY * 10 - 10, 0, 0, b);
		// 			Pnc.QueueCommand(new DelayCmd(this, 300));
		// 			Pnc.QueueCommand(new SaveCameraFrameCmd(this, "calib seq/" + calibId++ + ".png"));
		// 		}
		// 	}
		// }

		// Z axis
		// for (int iB = 0; iB < bAngles.Length; ++iB) {
		// 	float b = bAngles[iB];

		// 	for (int i = 0; i < 4; ++i) {
		// 		Pnc.MoveToPosition(55, 0, i * 20 - 60, 0, b);
		// 		Pnc.QueueCommand(new DelayCmd(this, 300));
		// 		Pnc.QueueCommand(new SaveCameraFrameCmd(this, "calib seq/" + calibId++ + ".png"));
		// 	}
		// }

		// float[] bAngles = { -70, 20, 110, 200 };

		// for (int iB = 0; iB < bAngles.Length; ++iB) {
		// 	float b = bAngles[iB];

		// 	for (int i = 0; i < 6; ++i) {
		// 		Pnc.MoveToPosition(55, 0, 0, i * 15 + 15, b);
		// 		Pnc.QueueCommand(new DelayCmd(this, 300));
		// 		Pnc.QueueCommand(new SaveCameraFrameCmd(this, "calib seq/" + calibId++ + ".png"));
		// 	}
		// }

		// Laser plane z axis.
		for (int i = 0; i < 31; ++i) {
			Pnc.MoveToPosition(45, 0, i * 2 - 60, 0, 20);
			Pnc.QueueCommand(new DelayCmd(this, 300));
			Pnc.QueueCommand(new SaveCameraFrameCmd(this, "calib seq/" + calibId++ + ".png"));
		}
	}

	public void OnCaptureScan() {
		// X: 60 to 20

		int seqId = 0;
		// for (int i = 0; i < 400; ++i) {
		// 	MachineFrame frame = new MachineFrame(60 - i * 0.1f, 0, 0, 0, 180);
		// 	Pnc.MoveToPosition(frame.x, frame.y, frame.z, frame.a, frame.b);
		// 	Pnc.QueueCommand(new DelayCmd(this, 250));
		// 	Pnc.QueueCommand(new SaveCameraFrameCmd(this, "scan seq/" + seqId + ".png", true, "scan seq/" + seqId + ".dat", frame));
		// 	seqId++;
		// }

		for (int iB = 0; iB < 360; iB += 20) {
			for (int i = 0; i < 400; ++i) {
				MachineFrame frame = new MachineFrame(60 - i * 0.1f, 0, 0, 0, iB);
				Pnc.MoveToPosition(frame.x, frame.y, frame.z, frame.a, frame.b);
				Pnc.QueueCommand(new DelayCmd(this, 250));
				Pnc.QueueCommand(new SaveCameraFrameCmd(this, "scan seq/" + seqId + ".png", true, "scan seq/" + seqId + ".dat", frame));
				seqId++;
			}
		}
	}

	public void OnBundleAdjustMono() {
		// Load all files in calibration folder.
		string[] files = Directory.GetFiles("calib seq/");
		
		Texture2D tex = new Texture2D(1920, 1080, UnityEngine.Experimental.Rendering.DefaultFormat.LDR, UnityEngine.Experimental.Rendering.TextureCreationFlags.None);
		byte[] imgData = new byte[1920 * 1080];

		// CalibratedCamera cc = camServer._cams[0].calibration;
		CalibratedCamera cc = cams[0].calibration;

		GameObject parentGob = new GameObject();
		parentGob.name = "Bundle prep";

		List<PoseEstimation> poses = new List<PoseEstimation>();
		List<List<Vector2>> viewPointLists = new List<List<Vector2>>();
		List<List<int>> viewPointIdsLists = new List<List<int>>();
		List<int> viewPointImgIds = new List<int>();

		int measurementCount = 0;
		
		for (int i = 0; i < files.Length; ++i) {
			// Debug.Log("File: " + files[i]);
			// if (files[i] != "calib seq/calib_18.png") {
			// 	continue;
			// }

			string fileName = "calib seq/" + i + ".png";
			Debug.Log("File: " + fileName);
			
			byte[] fileBytes = System.IO.File.ReadAllBytes(fileName);
			tex.LoadImage(fileBytes, false);
			Color32[] fileColors = tex.GetPixels32();
			for (int j = 0; j < 1920 * 1080; ++j) {
				imgData[j] = fileColors[j].r;
			}

			List<CharucoBoard> newBoards = nativeCore.FindCharuco(1920, 1080, imgData, i);

			List<Vector2> imagePoints = new List<Vector2>();
			List<int> imagePointIds = new List<int>();
			List<Vector3> objectPoints = new List<Vector3>();
			PoseEstimation pose = new PoseEstimation();

			for (int iB = 0; iB < newBoards.Count; ++iB) {
				CharucoBoard b = newBoards[iB];

				// if (b.id != 3) {
				// 	continue;
				// }

				for (int iM = 0; iM < b.markers.Count; ++iM) {
					CharucoMarker m = b.markers[iM];

					int fullId = b.id * 10 + m.id;

					imagePoints.Add(m.pos);
					Vector3 op = _smallCalibCubeIndex[fullId].pos;
					objectPoints.Add(new Vector3(op.x, op.y, op.z));
					imagePointIds.Add(fullId);

					// Debug.Log(iB + " " + m.id + " " + fullId + ": " + op.x + ", " + op.y + ", " + op.z);
				}
			}
			
			if (imagePoints.Count >= 4) {
				Debug.Log("Get pose...");
				if (nativeCore.GetGeneralPose(cc, imagePoints, objectPoints, ref pose)) {
					CalibratedCamera camCal = new CalibratedCamera();
					camCal.r = pose.rotMat;
					camCal.t = pose.pos;
					camCal.CalculateMats();
							
					Matrix4x4 poseMatInv = camCal.rt.inverse;
					pose.worldMat = poseMatInv;

					// NOTE: Invert for cam pos.
					GameObject camGob = GameObject.CreatePrimitive(PrimitiveType.Cube);
					camGob.name = "Pose_" + i;
					camGob.transform.SetParent(parentGob.transform);
					camGob.transform.position = new Vector3(poseMatInv.m03, poseMatInv.m13, poseMatInv.m23);
					camGob.transform.rotation = poseMatInv.rotation;
					// camGob.transform.localPosition = pose.pos;
					// camGob.transform.localRotation = pose.rotMat.rotation;

					// Matrix4x4 worldMat = camGob.transform.worldToLocalMatrix;
					// camGob.transform.FromMatrix(worldMat);

					poses.Add(pose);
					viewPointImgIds.Add(i);
					viewPointLists.Add(imagePoints);
					viewPointIdsLists.Add(imagePointIds);
					measurementCount += imagePoints.Count;
				}
			}
		}

		using (StreamWriter writer = new StreamWriter("ssba_new.txt")) {
			int pointCount = _smallCalibCube.Count;
			int viewCount = poses.Count;
			
			// Header.
			writer.WriteLine(pointCount + " " + viewCount + " " + measurementCount);

			// Camera intrinsics.
			writer.WriteLine(cc.mat[0] + " " + cc.mat[1] + " " +  cc.mat[2] + " " + cc.mat[4] + " " + cc.mat[5] + " " + 
				cc.distCoeffs[0] + " " + cc.distCoeffs[1] + " " + cc.distCoeffs[2] + " " + cc.distCoeffs[3]);

			// 3D points.
			for (int i = 0; i < _smallCalibCube.Count; ++i) {
				CalibPoint c = _smallCalibCube[i];
				writer.WriteLine(c.fullId + " " + c.pos.x + " " + c.pos.y + " " + c.pos.z);
			}

			// View poses.
			for (int i = 0; i < poses.Count; ++i) {
				int viewPointImgId = viewPointImgIds[i];
				PoseEstimation pose = poses[i];
				Matrix4x4 camRt = Matrix4x4.TRS(pose.pos, pose.rotMat.rotation, Vector3.one);
				// Matrix4x4 camRt = pose.worldMat;

				string output = viewPointImgId.ToString();
				// Debug.Log(camRt);

				for (int j = 0; j < 12; ++j) {
					// NOTE: Index column major, output as row major.
					output += " " + camRt[(j % 4) * 4 + (j / 4)];
				}

				writer.WriteLine(output);
			}

			// 2D measurements.
			for (int i = 0; i < viewPointLists.Count; ++i) {
				List<Vector2> viewPoints = viewPointLists[i];
				List<int> viewPointIds = viewPointIdsLists[i];
				int viewPointImgId = viewPointImgIds[i];

				for (int j = 0; j < viewPoints.Count; ++j) {
					Vector2 viewPoint = viewPoints[j];
					int viewPointId = viewPointIds[j];
					writer.WriteLine(viewPointImgId + " " + viewPointId + " " + viewPoint.x + " " + viewPoint.y + " 1");
				}
			}
		}

		// NOTE: Sim bundle adjust.
		// Debug.Log("Start bundle adjust process");
		// _state = 2;
		// _stateStep = 0;
		// _poseEstimations = new List<PoseEstimation>();
		// _boards = new List<CharucoBoard>();
	}

	public string[] ReadLineSplit(StreamReader Reader) {
		return Reader.ReadLine().Split(' ');
	}

	public void OnReadRefined() {
		List<CalibPoint> cubePoints = new List<CalibPoint>();

		GameObject cco = new GameObject("CalibCubeRefined");

		CalibratedCamera camCal = new CalibratedCamera();
		List<Matrix4x4> poses = new List<Matrix4x4>();
		// Matrix4x4 pose200Mat = Matrix4x4.identity;

		List<Vector3> lineFitZ = new List<Vector3>();
		List<Vector3> lineFitX = new List<Vector3>();
		List<Vector3> lineFitY = new List<Vector3>();

		List<Vector3> bAxisFitPoints = new List<Vector3>();

		Vector3 camAlmostZeroPos = Vector3.zero;
		Matrix4x4 camAlmostZeroMat = Matrix4x4.identity;

		Stopwatch sw = new Stopwatch();

		//----------------------------------------------------------------------------------------------------------
		// Primary refinement.
		//----------------------------------------------------------------------------------------------------------
		sw.Start();
		
		using (StreamReader reader = new StreamReader("refined.txt")) {
			string[] newK = ReadLineSplit(reader);

			camCal.mat[0] = float.Parse(newK[0]);
			camCal.mat[1] = float.Parse(newK[1]);
			camCal.mat[2] = float.Parse(newK[2]);
			camCal.mat[4] = float.Parse(newK[3]);
			camCal.mat[5] = float.Parse(newK[4]);

			camCal.distCoeffs[0] = float.Parse(newK[5]);
			camCal.distCoeffs[1] = float.Parse(newK[6]);
			camCal.distCoeffs[2] = float.Parse(newK[7]);
			camCal.distCoeffs[3] = float.Parse(newK[8]);

			string[] header = ReadLineSplit(reader);
			int objectPointCount = int.Parse(header[0]);
			int viewCount = int.Parse(header[1]);

			Debug.Log("Object point count: " + objectPointCount + " view count: " + viewCount);

			for (int i = 0; i < objectPointCount; ++i) {
				string[] obLine = ReadLineSplit(reader);

				float x = float.Parse(obLine[1]);
				float y = float.Parse(obLine[2]);
				float z = float.Parse(obLine[3]);

				GameObject gob = GameObject.CreatePrimitive(PrimitiveType.Sphere);
				gob.transform.SetParent(cco.transform);
				gob.transform.localPosition = new Vector3(x, y, z);
				gob.transform.localScale = new Vector3(0.1f, 0.1f, 0.1f);
				gob.name = obLine[0];

				cubePoints.Add(new CalibPoint(int.Parse(obLine[0]), new Vector3(x, y, z)));
			}

			GameObject parentGob = new GameObject();
			parentGob.name = "Bundle prep";

			for (int i = 0; i < viewCount; ++i) {
				string[] viewMatStr = ReadLineSplit(reader);
				Matrix4x4 viewMat = Matrix4x4.identity;

				for (int j = 0; j < 12; ++j) {
					// NOTE: Index column major, input is row major.
					viewMat[(j % 4) * 4 + (j / 4)] = float.Parse(viewMatStr[j + 1]);
				}

				int viewId = int.Parse(viewMatStr[0]);

				poses.Add(viewMat);

				Matrix4x4 poseMatInv = viewMat.inverse;
				
				// NOTE: Invert for cam pos.
				Vector3 camPos = new Vector3(poseMatInv.m03, poseMatInv.m13, poseMatInv.m23);

				GameObject camGob = GameObject.Instantiate(CamVisPrefab);
				camGob.name = "Pose_" + viewId;
				camGob.transform.SetParent(parentGob.transform);
				camGob.transform.position = camPos;
				camGob.transform.rotation = poseMatInv.rotation;

				if (viewId >= 200 && viewId <= 230) {
					lineFitZ.Add(camPos);
				}
				
				if (viewId >= 40 && viewId <= 47) {
					lineFitX.Add(camPos);
				}
				
				if (viewId == 38 || viewId == 46 || viewId == 54 || viewId == 62) {
					lineFitY.Add(camPos);
				}

				if (viewId == 14 || viewId == 46 || viewId == 78 || viewId == 110) {
					bAxisFitPoints.Add(camPos);
				}

				if (viewId == 46) {
					camAlmostZeroPos = camPos;
					camAlmostZeroMat = poseMatInv;
				}
			}
		}

		int[] cubeFaceIds = { 0, 1, 3, 4, 5 };

		Vector3 basisY = Vector3.zero;
		Vector3 basisX = Vector3.zero;
		Vector3 basisZ = Vector3.zero;

		Vector3 boardZPos = Vector3.zero;
		Vector3 boardZNeg = Vector3.zero;

		// Generare cube planes.
		for (int i = 0; i < cubeFaceIds.Length; ++i) {
			int boardId = cubeFaceIds[i];

			List<CalibPoint> boardPoints = new List<CalibPoint>();

			for (int j = 0; j < cubePoints.Count; ++j) {
				CalibPoint c = cubePoints[j];

				if (c.boardId == boardId) {
					boardPoints.Add(c);
				}
			}

			// Find planes
			List<Vector3> points = new List<Vector3>();
			for (int j = 0; j < boardPoints.Count; ++j) {
				points.Add(boardPoints[j].pos);
			}

			Vector3 origin;
			Vector3 normal;

			FitPlane(points, out origin, out normal);

			// Correct normals.
			if (boardId == 0) {
				if (normal.x > 0) {
					normal = -normal;
				}
			} else if (boardId == 1) {
				if (normal.y < 0) {
					normal = -normal;
				}
				basisY = normal;
			} else if (boardId == 3) {
				if (normal.z < 0) {
					normal = -normal;
				}
				basisZ = normal;
				boardZPos = origin;
			} else if (boardId == 4) {
				if (normal.x < 0) {
					normal = -normal;
				}
				basisX = normal;
			} else if (boardId == 5) {
				if (normal.z > 0) {
					normal = -normal;
				}
				boardZNeg = origin;
			}

			GameObject planeGob = GameObject.Instantiate(CubePlanePrefab);
			planeGob.transform.SetParent(cco.transform);
			planeGob.name = "Plane_" + boardId;
			planeGob.transform.rotation = Quaternion.LookRotation(normal, Vector3.forward);
			planeGob.transform.position = origin;
			Debug.Log(normal.x + " " + normal.y + " " + normal.z);
		}

		float scaleValue = (Vector3.Magnitude(boardZNeg - boardZPos)) / 4.0f;
		Debug.Log("Scale: " + scaleValue);

		List<Vector3> boardCorners = new List<Vector3>();
		float dist = 2.0f * scaleValue;
		boardCorners.Add(boardZPos + basisX * dist + basisY * dist);
		boardCorners.Add(boardZPos - basisX * dist + basisY * dist);
		boardCorners.Add(boardZPos - basisX * dist - basisY * dist);
		boardCorners.Add(boardZPos + basisX * dist - basisY * dist);
		

		for (int i = 0; i < boardCorners.Count; ++i) {
			GameObject gob = GameObject.CreatePrimitive(PrimitiveType.Cube);
			gob.transform.SetParent(cco.transform);
			gob.transform.localScale = new Vector3(0.1f, 0.1f, 0.1f);
			gob.transform.position = boardCorners[i];
		}

		sw.Stop();
		Debug.Log("Primary refine: " + sw.Elapsed.TotalMilliseconds + " ms");

		//----------------------------------------------------------------------------------------------------------
		// Calibrate laser plane to camera.
		//----------------------------------------------------------------------------------------------------------
		sw.Restart();

		// Get scanlines from img 200 to 230.
		Texture2D tex = new Texture2D(1920, 1080, UnityEngine.Experimental.Rendering.DefaultFormat.LDR, UnityEngine.Experimental.Rendering.TextureCreationFlags.None);
		byte[] imgData = new byte[1920 * 1080];

		Plane p = new Plane(basisZ, boardZPos);

		List<Vector3> planePoints = new List<Vector3>();

		for (int iP = 200; iP <= 230; ++iP) {
			byte[] fileBytes = System.IO.File.ReadAllBytes("calib laser plane/" + iP + ".png");

			tex.LoadImage(fileBytes, false);
			Color32[] fileColors = tex.GetPixels32();

			for (int j = 0; j < 1920 * 1080; ++j) {
				imgData[j] = fileColors[j].r;
			}
			
			List<Vector2> scanPoints = nativeCore.FindScanline(1920, 1080, imgData, 69, boardCorners, camCal, poses[iP]);
			Vector4 camOrigin = new Vector4(0, 0, 0, 1);
			Matrix4x4 camPose = poses[iP].inverse;
			camOrigin = camPose * camOrigin;

			for (int i = 0; i < scanPoints.Count; ++i) {
				Vector2 scanPoint = scanPoints[i];
				Vector4 scanPointPos = new Vector4(scanPoint.x, -scanPoint.y, 1, 1);
				scanPointPos = camPose * scanPointPos;

				Vector3 pA = new Vector3(camOrigin.x, camOrigin.y, camOrigin.z);
				Vector3 pB = new Vector3(scanPointPos.x,scanPointPos.y, scanPointPos.z);
				Vector3 pDir = (pB - pA).normalized;

				Ray r = new Ray(pA, pDir);

				// Debug.DrawLine(pA, pA + pDir * 1000.0f, Color.red, 100000.0f);
				// Debug.DrawRay(pA, pDir, Color.red, 10000.0f);

				float t;
				if (p.Raycast(r, out t)) {
					// Get point in camera space.
					Vector3 workspacePos = r.origin + r.direction * t;
					Vector3 camSpacePos = poses[iP] * new Vector4(workspacePos.x, workspacePos.y, workspacePos.z, 1);

					planePoints.Add(camSpacePos);

					// GameObject scanPointGob = GameObject.CreatePrimitive(PrimitiveType.Sphere);
					// scanPointGob.transform.localScale = new Vector3(0.02f, 0.02f, 0.02f);
					// scanPointGob.transform.position = camSpacePos;
				}
			}
		}

		Vector3 scanPlaneOrigin = Vector3.zero;
		Vector3 scanPlaneNormal = Vector3.zero;

		FitPlane(planePoints, out scanPlaneOrigin, out scanPlaneNormal);

		// Make sure normal is facing towards camera.
		if (scanPlaneNormal.x > 0) {
			scanPlaneNormal = -scanPlaneNormal;
		}

		// GameObject scanPlaneGob = GameObject.Instantiate(CubePlanePrefab);
		// scanPlaneGob.name = "ScanPlane";
		// scanPlaneGob.transform.rotation = Quaternion.LookRotation(scanPlaneNormal, Vector3.forward);
		// scanPlaneGob.transform.position = scanPlaneOrigin;

		sw.Stop();
		Debug.Log("Calibrate scan plane: " + sw.Elapsed.TotalMilliseconds + " ms");

		//----------------------------------------------------------------------------------------------------------
		// Find machine movement basis.
		//----------------------------------------------------------------------------------------------------------
		sw.Restart();

		Ray lineRayX = nativeCore.FitLine(lineFitX);
		Debug.DrawLine(lineRayX.origin - lineRayX.direction * 10, lineRayX.origin + lineRayX.direction * 10, Color.red, 10000.0f);

		Ray lineRayY = nativeCore.FitLine(lineFitY);
		Debug.DrawLine(lineRayY.origin - lineRayY.direction * 10, lineRayY.origin + lineRayY.direction * 10, Color.green, 10000.0f);

		Ray lineRayZ = nativeCore.FitLine(lineFitZ);
		Debug.DrawLine(lineRayZ.origin - lineRayZ.direction * 10, lineRayZ.origin + lineRayZ.direction * 10, Color.blue, 10000.0f);

		Vector3 bAxisPlaneOrigin;
		Vector3 bAxisPlaneNormal;
		FitPlane(bAxisFitPoints, out bAxisPlaneOrigin, out bAxisPlaneNormal);

		if (bAxisPlaneNormal.y < 0) {
			bAxisPlaneNormal = -bAxisPlaneNormal;
		}

		// GameObject bAxisPlaneGob = GameObject.Instantiate(CubePlanePrefab);
		// bAxisPlaneGob.name = "BAxisPlane";
		// bAxisPlaneGob.transform.rotation = Quaternion.LookRotation(bAxisPlaneNormal, Vector3.forward);
		// bAxisPlaneGob.transform.position = bAxisPlaneOrigin;

		{
			// Find original 0, 0, 0, 0, 0 position
			// Take 0, 0, 0, 0, 20 and rotate -20 on B.
			Vector3 targetPos = new Vector3(-6, 0, 0) * scaleValue;
			float targetB = 0;

			Matrix4x4 camOriginZero = Matrix4x4.TRS(camAlmostZeroPos - bAxisPlaneOrigin, camAlmostZeroMat.rotation, Vector3.one);
			Matrix4x4 camOriginZeroNoRot = Matrix4x4.TRS(camAlmostZeroPos - bAxisPlaneOrigin, Quaternion.identity, Vector3.one);
			
			camOriginZero = Matrix4x4.TRS(Vector3.zero, Quaternion.AngleAxis(-20 + targetB, bAxisPlaneNormal), Vector3.one) * camOriginZero;
			camOriginZeroNoRot = Matrix4x4.TRS(Vector3.zero, Quaternion.AngleAxis(-20 + targetB, bAxisPlaneNormal), Vector3.one) * camOriginZeroNoRot;
			
			Vector3 camOriginZeroPos = camOriginZero.ExtractPosition() + bAxisPlaneOrigin;

			// scanPlaneGob = GameObject.CreatePrimitive(PrimitiveType.Sphere);
			// scanPlaneGob.name = "AlmostZeroPosCam";
			// scanPlaneGob.transform.position = camOriginZeroPos;

			Ray rX = new Ray(camOriginZeroPos, camOriginZeroNoRot * lineRayX.direction);
			Ray rY = new Ray(camOriginZeroPos, camOriginZeroNoRot * lineRayY.direction);
			Ray rZ = new Ray(camOriginZeroPos, camOriginZeroNoRot * lineRayZ.direction);
			
			Debug.DrawLine(rX.origin - rX.direction * 10, rX.origin + rX.direction * 10, Color.red, 10000.0f);
			Debug.DrawLine(rY.origin - rY.direction * 10, rY.origin + rY.direction * 10, Color.green, 10000.0f);
			Debug.DrawLine(rZ.origin - rZ.direction * 10, rZ.origin + rZ.direction * 10, Color.blue, 10000.0f);

			Vector3 finalPos = camOriginZeroPos + rX.direction * targetPos.x + rY.direction * targetPos.y + rZ.direction * targetPos.z;
			Matrix4x4 finalMat = Matrix4x4.TRS(finalPos, camOriginZero.rotation, Vector3.one);

			GameObject finalCamGob = GameObject.Instantiate(CamVisPrefab);
			finalCamGob.name = "FinalCamPos";
			finalCamGob.transform.position = finalPos;
			finalCamGob.transform.rotation = camOriginZero.rotation;
		}

		sw.Stop();
		Debug.Log("Machine basis: " + sw.Elapsed.TotalMilliseconds + " ms");

		//----------------------------------------------------------------------------------------------------------
		// Reconstruct scan data.
		//----------------------------------------------------------------------------------------------------------
		// Clear boundry points.
		boardCorners.Clear();

		// Bounds inBounds = new Bounds(Vector3.zero, new Vector3(6, 12, 6));
		Bounds inBounds = new Bounds(new Vector3(-0.81f, 2.079f, -0.333f), new Vector3(4.95f, 6.337f, 5.098f));

		List<Vector3> pointCloudPoints = new List<Vector3>();
		List<Color32> pointCloudColors = new List<Color32>();

		string[] scanFiles = Directory.GetFiles("scan seq/");

		for (int iP = 0; iP < scanFiles.Length; ++iP) {
			sw.Restart();

			byte[] fileBytes = System.IO.File.ReadAllBytes(scanFiles[iP]);

			// tex.LoadImage(fileBytes, false);
			// Color32[] fileColors = tex.GetPixels32();

			// sw.Stop();
			// Debug.Log("Load " + iP + ": " + sw.Elapsed.TotalMilliseconds + " ms");
			// sw.Restart();

			// for (int j = 0; j < 1920 * 1080; ++j) {
			// 	imgData[j] = fileColors[j].r;
			// }

			// sw.Stop();
			// Debug.Log("Copy " + iP + ": " + sw.Elapsed.TotalMilliseconds + " ms");

			//Pnc.MoveToPosition(60, 0, 0, 0, 0);

			//---

			BinaryReader br = new BinaryReader(new MemoryStream(fileBytes));

			MachineFrame frame = new MachineFrame();
			frame.x = br.ReadSingle();
			frame.y = br.ReadSingle();
			frame.z = br.ReadSingle();
			frame.a = br.ReadSingle();
			frame.b = br.ReadSingle();

			System.Array.Copy(fileBytes, 5 * 4, imgData, 0, 1920 * 1080);
			
			Vector3 targetPos = new Vector3(frame.x / -10.0f, frame.y / -10.0f, frame.z / -10.0f) * scaleValue;

			Matrix4x4 camOriginZero = Matrix4x4.TRS(camAlmostZeroPos - bAxisPlaneOrigin, camAlmostZeroMat.rotation, Vector3.one);
			Matrix4x4 camOriginZeroNoRot = Matrix4x4.TRS(camAlmostZeroPos - bAxisPlaneOrigin, Quaternion.identity, Vector3.one);
			camOriginZero = Matrix4x4.TRS(Vector3.zero, Quaternion.AngleAxis(-20 + frame.b, bAxisPlaneNormal), Vector3.one) * camOriginZero;
			camOriginZeroNoRot = Matrix4x4.TRS(Vector3.zero, Quaternion.AngleAxis(-20 + frame.b, bAxisPlaneNormal), Vector3.one) * camOriginZeroNoRot;
			Vector3 camOriginZeroPos = camOriginZero.ExtractPosition() + bAxisPlaneOrigin;

			Ray rX = new Ray(camOriginZeroPos, camOriginZeroNoRot * lineRayX.direction);
			Ray rY = new Ray(camOriginZeroPos, camOriginZeroNoRot * lineRayY.direction);
			Ray rZ = new Ray(camOriginZeroPos, camOriginZeroNoRot * lineRayZ.direction);
			
			Vector3 finalPos = camOriginZeroPos + rX.direction * targetPos.x + rY.direction * targetPos.y + rZ.direction * targetPos.z;
			Matrix4x4 finalMat = Matrix4x4.TRS(finalPos, camOriginZero.rotation, Vector3.one);

			//---

			int outImgId = -1;

			// if (scanFiles[iP] == "scan seq/170.dat") {
			// 	outImgId = 170;
			// }
			
			List<Vector2> scanPoints = nativeCore.FindScanline(1920, 1080, imgData, outImgId, boardCorners, camCal, finalMat);
			// Debug.Log("Scan points: " + iP + " " + scanPoints.Count);
			Vector4 camOrigin = new Vector4(finalPos.x, finalPos.y, finalPos.z, 1);
			Matrix4x4 camPose = finalMat;
			camOrigin = camPose * camOrigin;

			Vector3 scanPlaneOriginRelativeToCamera = camPose * new Vector4(scanPlaneOrigin.x, scanPlaneOrigin.y, scanPlaneOrigin.z, 1);
			Vector3 scanPlaneNormalRelativeToCamera = camPose * new Vector4(scanPlaneNormal.x, scanPlaneNormal.y, scanPlaneNormal.z, 0);

			Plane scanPlaneRelativeToCamera = new Plane(scanPlaneNormalRelativeToCamera, scanPlaneOriginRelativeToCamera);

			// scanPlaneGob = GameObject.Instantiate(CubePlanePrefab);
			// scanPlaneGob.name = "ScanPlane" + iP;
			// scanPlaneGob.transform.rotation = Quaternion.LookRotation(scanPlaneNormalRelativeToCamera, Vector3.forward);
			// scanPlaneGob.transform.position = scanPlaneOriginRelativeToCamera;

			for (int i = 0; i < scanPoints.Count; ++i) {
				Vector2 scanPoint = scanPoints[i];
				Vector4 scanPointPos = new Vector4(scanPoint.x, -scanPoint.y, 1, 1);
				scanPointPos = camPose * scanPointPos;

				Vector3 pA = new Vector3(finalPos.x, finalPos.y, finalPos.z);
				Vector3 pB = new Vector3(scanPointPos.x,scanPointPos.y, scanPointPos.z);
				Vector3 pDir = (pB - pA).normalized;

				Ray r = new Ray(pA, pDir);

				// Debug.DrawLine(pA, pA + pDir * 1000.0f, Color.red, 100000.0f);
				// Debug.DrawRay(pA, pDir, Color.red, 10000.0f);

				float t;
				if (scanPlaneRelativeToCamera.Raycast(r, out t)) {
					// Get point in camera space.
					Vector3 workspacePos = r.origin + r.direction * t;

					if (inBounds.Contains(workspacePos)) {
						// GameObject scanPointGob = GameObject.CreatePrimitive(PrimitiveType.Sphere);
						// scanPointGob.transform.localScale = new Vector3(0.01f, 0.01f, 0.01f);
						// scanPointGob.transform.position = workspacePos;
						pointCloudPoints.Add(workspacePos);

						Vector3 normPos = (workspacePos - inBounds.min);
						float repeat = 1.0f / 1.0f;
						normPos.x /= inBounds.size.x * repeat;
						normPos.y /= inBounds.size.y * repeat;
						normPos.z /= inBounds.size.z * repeat;

						Color32 c = new Color32();
						c.a = 255;
						c.r = (byte)(normPos.x * 255);
						c.g = (byte)(normPos.y * 255);
						c.b = (byte)(normPos.z * 255);
						
						pointCloudColors.Add(c);
					}
				}
			}
		}

		PointRenderer pointCloud = new PointRenderer();
		pointCloud.Init("Scan points", PointBillboardMaterial, Vector3.zero);
		pointCloud.Generate(pointCloudPoints, pointCloudColors);

		CreatePly("scanPoints.ply", pointCloudPoints);
	}

	public void CreatePly(string Filename, List<Vector3> Points) {
		// Create PLY file.
 		using (BinaryWriter writer = new BinaryWriter(File.Open(Filename, FileMode.Create))) {
			writer.Write(Encoding.UTF8.GetBytes("ply\n"));
			writer.Write(Encoding.UTF8.GetBytes("format binary_little_endian 1.0\n"));
			writer.Write(Encoding.UTF8.GetBytes("element vertex " + Points.Count + "\n"));
			writer.Write(Encoding.UTF8.GetBytes("property float x\n"));
			writer.Write(Encoding.UTF8.GetBytes("property float y\n"));
			writer.Write(Encoding.UTF8.GetBytes("property float z\n"));
			writer.Write(Encoding.UTF8.GetBytes("end_header\n"));

			for (int i = 0; i < Points.Count; ++i) {
				writer.Write(-Points[i].x);
				writer.Write(Points[i].y);
				writer.Write(Points[i].z);
			}
		}
	}

	public void OnBundleAdjustStereo() {
		Debug.Log("Start bundle adjust process");
		_state = 3;
		_stateStep = 0;
		_poseEstimations = new List<PoseEstimation>();
		_boards = new List<CharucoBoard>();
		_sharedMarkerIds = new List<int>();
		_sharedMarkerPos0 = new List<Vector2>();
		_sharedMarkerPos1 = new List<Vector2>();

		_totalPositions = new List<Vector3>();
		_totalPositions.Add(new Vector3(0, 0, 0));

		for (int iY = 0; iY < 4; ++iY) {
			for (int iX = 0; iX < 4; ++iX) {
				for (int iZ = 0; iZ < 10; ++iZ) {
					float x = -10.0f + iX * (20.0f / 3.0f);
					float y = -10.0f + iY * (20.0f / 3.0f);
					float z = -10.0f + iZ * 2.0f;
					_totalPositions.Add(new Vector3(x, y, z));
				}
			}
		}
	}

	public void OnFindPose() {
		//cams[0].StoreLatestFrame();

		byte[] imgData = GetProcessReadyFrame(camServer._cams[0]);
		List<CharucoBoard> newBoards = nativeCore.FindCharuco(1920, 1080, imgData, 99);

		for (int i = 0; i < newBoards.Count; ++i) {
			if (newBoards[i].markers.Count > 0) {
				PoseEstimation pose = new PoseEstimation();
				if (nativeCore.GetCharucoPose(camServer._cams[0].calibration, newBoards[i], ref pose)) {
					Debug.Log("Pose found");

					
					// Pose is in camera space, so append pose to camera object.
					GameObject poseGob = GameObject.Instantiate(CalibRefPrefab, CameraLeft.transform);
					//poseGob.transform.SetPositionAndRotation(pose.pos, pose.rotMat.rotation);
					poseGob.transform.localPosition = pose.pos;
					poseGob.transform.localRotation = pose.rotMat.rotation;

					// Matrix4x4 poseMat = Matrix4x4.TRS(pose.pos, pose.rotMat.rotation, Vector3.one);
					// Matrix4x4 poseMatInv = poseMat.inverse;
					// GameObject camTest = GameObject.CreatePrimitive(PrimitiveType.Cube);
					// camTest.transform.position = new Vector3(poseMatInv.m03, poseMatInv.m13, poseMatInv.m23);
					// camTest.transform.rotation = poseMatInv.rotation;

					// Debug.Log(poseMat);
					// Debug.Log(poseMatInv);
					// Debug.Log(camTest.transform.localToWorldMatrix);
				}
			}
		}
	}

	public void OnFindScanline() {
		Texture2D tex = new Texture2D(1920, 1080, UnityEngine.Experimental.Rendering.DefaultFormat.LDR, UnityEngine.Experimental.Rendering.TextureCreationFlags.None);
		byte[] imgData = new byte[1920 * 1080];
		byte[] fileBytes = System.IO.File.ReadAllBytes("calib seq/200.png");

		tex.LoadImage(fileBytes, false);
		Color32[] fileColors = tex.GetPixels32();

		for (int j = 0; j < 1920 * 1080; ++j) {
			imgData[j] = fileColors[j].r;
		}

		// nativeCore.FindScanline(1920, 1080, imgData, 69);
	}

	public void FitPlane(List<Vector3> Points, out Vector3 Origin, out Vector3 Normal) {
		// https://www.ilikebigbits.com/2017_09_25_plane_from_points_2.html
		float pointCount = Points.Count;

		Vector3 sum = Vector3.zero;

		for (int i = 0; i < pointCount; ++i) {
			sum += Points[i];
		}

		Vector3 centroid = sum * (1.0f / pointCount);

		float xx = 0.0f;
		float xy = 0.0f;
		float xz = 0.0f;
		float yy = 0.0f;
		float yz = 0.0f;
		float zz = 0.0f;

		for (int i = 0; i < pointCount; ++i) {
			Vector3 r = Points[i] - centroid;
			xx += r.x * r.x;
			xy += r.x * r.y;
			xz += r.x * r.z;
			yy += r.y * r.y;
			yz += r.y * r.z;
			zz += r.z * r.z;
		}

		xx /= pointCount;
		xy /= pointCount;
		xz /= pointCount;
		yy /= pointCount;
		yz /= pointCount;
		zz /= pointCount;

		Vector3 weightedDir = Vector3.zero;

		{
			float detX = yy * zz - yz * yz;
			Vector3 axisDir = new Vector3(
				detX,
				xz * yz - xy * zz,
				xy * yz - xz * yy
			);
			
			float weight = detX * detX;
			
			if (Vector3.Dot(weightedDir, axisDir) < 0.0f) {
				weight = -weight;
			}

			weightedDir += axisDir * weight;
		}

		{
			float detY = xx * zz - xz * xz;
			Vector3 axisDir = new Vector3(
				xz * yz - xy * zz,
				detY,
				xy * xz - yz * xx
			);
			
			float weight = detY * detY;
			
			if (Vector3.Dot(weightedDir, axisDir) < 0.0f) {
				weight = -weight;
			}

			weightedDir += axisDir * weight;
		}

		{
			float detZ = xx * yy - xy * xy;
			Vector3 axisDir = new Vector3(
				xy * yz - xz * yy,
				xy * xz - yz * xx,
				detZ
			);
			
			float weight = detZ * detZ;
			
			if (Vector3.Dot(weightedDir, axisDir) < 0.0f) {
				weight = -weight;
			}

			weightedDir += axisDir * weight;
		}

		Vector3 normal = weightedDir.normalized;

		Origin = centroid;
		Normal = normal;

		if (normal.magnitude == 0.0f) {
			Origin = Vector3.zero;
			Normal = Vector3.up;
		}

		// Debug.Log(Normal);
	}
}
