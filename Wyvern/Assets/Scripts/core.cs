//------------------------------------------------------------------------------------------------------------------------------------------------------
// Attribution notes:
// https://github.com/gkngkc/UnityStandaloneFileBrowser
//------------------------------------------------------------------------------------------------------------------------------------------------------

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

using ImageMagick;

using Debug = UnityEngine.Debug;

public class core : MonoBehaviour {

	public static core singleton;

	public static AppContext appContext;

	public TextMesh bakeTextMesh;
	public Camera bakeTextCamera;

	public GameObject CameraController;

	public RawImage uiCanvas;
	
	public Text uiTextPosX;
	public Text uiTextPosY;
	public Text uiTextPosZ;
	public InputField uiTextMoveAmount;

	public GameObject SampleLocatorPrefab;

	public Material SurfelDebugMat;

	private JobManager _jobManager;
	
	// private Texture2D _canvas;
	// private float[] _canvasRawData;
	// private Color32[] _tempCols;
	
	private float _threshold = 0.5f;
	private float _brightness = 0.0f;
	
	private Diffuser _diffuser;

	private TextRenderer _textRenderer;

	public Material BasicMaterial;
	public Material BasicWireMaterial;
	public Material BasicUnlitMaterial;
	public Material SampledMeshMaterial;
	public Material UiLinesMateral;
	public Material CoveragetMeshViewMat;
	public Material QuadWireMaterial;
	public Material ProjectionMaterial;
	public Material CandidatesSoftMat;

	public Figure figure;

	public ComputeShader ViewSurfelCount;

	public void uiBtnCapture() {
		_ScannerViewHit(true);
	}

	public void uiThresholdSliderChanged(float Value) {
		_threshold = Value;
	}

	public void uiBrightnessSliderChanged(float Value) {
		_brightness = Value;
	}

	public void uiBtnZero() {
		_jobManager.StartTaskZero();
	}

	public void uiBtnGotoZero() {
		_jobManager.StartTaskGotoZero();
	}

	public void uiBtnStart() {
		float x = _jobManager.controller._lastAxisPos[0];
		float y = _jobManager.controller._lastAxisPos[1];
		float z = _jobManager.controller._lastAxisPos[2];
		float scaleMm = 0.5f;

		// Z at 0mm is 170mm above target.
		// Focus at 165.

		// 250us at 165mm

		List<JobTask> taskList = new List<JobTask>();

		// taskList.Add(new JobTaskSimpleMove(0, -25, 200));
		// taskList.Add(new JobTaskSimpleMove(1, 25, 200));

		// taskList.Add(new JobTaskGalvoMove(40, 40));

		// for (int iY = 0; iY < 40; ++iY) {
		// 	for (int iX = 0; iX < 40; ++iX) {
		// 		taskList.Add(new JobTaskGalvoMove(iX, iY));
		// 		taskList.Add(new JobTaskDelay(100));
		// 	}
		// }

		float imgX = -25;
		float imgY = 25;
		float imgZ = 0;
		
		taskList.Add(new JobTaskSimpleMove(0, imgX, 200));
		taskList.Add(new JobTaskSimpleMove(1, imgY, 200));
		taskList.Add(new JobTaskSimpleMove(2, imgZ, 400));
		// _jobManager.AppendTaskImageHalftoneGalvo(taskList, appContext.testBenchImage.data, appContext.testBenchImage.width, appContext.testBenchImage.height, 0.1f);
		_jobManager.AppendTaskImageGalvo(taskList, appContext.testBenchImage.data, appContext.testBenchImage.width, appContext.testBenchImage.height, 0.05f, 150);

		//-----------------------------------------------------------------
		// Test grid.
		//-----------------------------------------------------------------
		// float pointSpacing = 0.2f;
		// float blockSpacing = 1.0f;

		// // Initial position for 2cm target.
		// float blockOffsetX = -21; // 21
		// float blockOffsetY = 2;

		// int laserOnTimeUs;

		// taskList.Add(new JobTaskGalvoMove(20, 20));

		// for (int iZ = 0; iZ < 20; ++iZ) {
		// 	taskList.Add(new JobTaskSimpleMove(2, 70 + iZ, 400)); // 240 to 260
		// 	float oX = blockOffsetX + iZ * blockSpacing;

		// 	for (int iB = 0; iB < 20; ++iB) {
		// 		laserOnTimeUs = 150 + iB * 10; // 150 to 350
		// 		float oY = blockOffsetY + iB * blockSpacing;

		// 		for (int iY = 0; iY < 4; ++iY) {
		// 			taskList.Add(new JobTaskSimpleMove(1, oY + iY * pointSpacing, 400));

		// 			for (int iX = 0; iX < 4; ++iX) {
		// 				taskList.Add(new JobTaskSimpleMove(0, oX + iX * pointSpacing, 400));
		// 				taskList.Add(new JobTaskStartLaser(1, laserOnTimeUs, 10000));
		// 			}
		// 		}
		// 	}
		// }

		//-----------------------------------------------------------------
		// Test grid.
		//-----------------------------------------------------------------
		// Test ZDepth and burn time.
		// Burn time: 150 - 350
		// ZDepth: -10mm to +10mm

		// float pointSpacing = 0.2f;
		// float blockSpacing = 1.0f;

		// float blockOffsetX = x;
		// float blockOffsetY = y;

		// int laserOnTimeUs;

		// for (int iZ = 0; iZ < 20; ++iZ) {
		// 	taskList.Add(new JobTaskSimpleMove(2, -20 + iZ, 400));
		// 	blockOffsetX = x + iZ * blockSpacing;

		// 	for (int iB = 0; iB < 20; ++iB) {
		// 		laserOnTimeUs = 150 + iB * 10;
		// 		blockOffsetY = y + iB * blockSpacing;

		// 		for (int iY = 0; iY < 4; ++iY) {
		// 			taskList.Add(new JobTaskSimpleMove(1, blockOffsetY + iY * pointSpacing, 400));

		// 			for (int iX = 0; iX < 4; ++iX) {
		// 				taskList.Add(new JobTaskSimpleMove(0, blockOffsetX + iX * pointSpacing, 400));
		// 				taskList.Add(new JobTaskStartLaser(1, laserOnTimeUs, 10000));
		// 			}
		// 		}
		// 	}
		// }

		// Magenta on white primer: X2 Y3 for small but decent dots. Z: 12mm Burn: 180. (35um dots)
		// Magenta on white primer: X0 Y7 for bigger but decent dots. Z: 10mm Burn: 220. (60um dots)

		// for (int i = 1; i < 10; ++i) {
		// 	int burnTime = 300 + i * 25;

		// 	ImageData textTest = _textRenderer.GetTextImage(burnTime + " us", 70, this);
		
		// 	_jobManager.AppendTaskImageRaster(taskList, textTest.data, x, y, z, textTest.width, textTest.height, scaleMm, 350, 200, 200);
		// 	y += textTest.height * scaleMm;
		
		// 	_jobManager.AppendTaskImageRaster(taskList, _diffusedImg.data, x, y, z, _diffusedImg.width, _diffusedImg.height, scaleMm, burnTime, 2000, 400);
		// 	y += _diffusedImg.height * scaleMm;
		// }

		// for (int i = 1; i < 10; ++i) {
		// 	int burnTime = 150 + i * 20;

		// 	// ImageData textTest = _textRenderer.GetTextImage(burnTime + " us", 70, this);
		
		// 	// _jobManager.AppendTaskImageRaster(taskList, textTest.data, x, y, z, textTest.width, textTest.height, scaleMm, 350, 200, 200);
		// 	// y += textTest.height * scaleMm;
		
		// 	_jobManager.AppendTaskImageRaster(taskList, _diffusedImg.data, x, y, z, _diffusedImg.width, 60, scaleMm, burnTime, 2000, 400);
		// 	y += 60 * scaleMm;
		// }

		//-----------------------------------------------------------------
		// Burn image diffuse raster.
		//-----------------------------------------------------------------
		// float imgX = x;
		// float imgY = y;
		// float imgZ = -5; // 165mm.
		// _jobManager.AppendTaskImageRaster(taskList, appContext.testBenchImage.data, imgX, imgY, imgZ, appContext.testBenchImage.width, appContext.testBenchImage.height, 0.050f, 200, 4000, 400, 0);

		//-----------------------------------------------------------------
		// Burn image halftone raster.
		//-----------------------------------------------------------------
		// float imgX = x;
		// float imgY = y;
		// float imgZ = -5; // 165mm.
		// _jobManager.AppendTaskImageHalftoneRaster(taskList, appContext.testBenchImage.data, imgX, imgY, imgZ, appContext.testBenchImage.width, appContext.testBenchImage.height, 0.05f, 4000, 400, 0);

		//-----------------------------------------------------------------
		// Reset position to starting.
		//-----------------------------------------------------------------
		// taskList.Add(new JobTaskSimpleMove(0, x, 200));
		// taskList.Add(new JobTaskSimpleMove(1, y, 200));

		_jobManager.SetTaskList(taskList);
	}

	public void startJob(int Index, bool Diffuse) {
		if (Index == 0) {
			Debug.LogError("Can't burn SRGB image");
			return;
		}

		if (Index > 0 && Index <= 5) {
			if (core.appContext.testBenchImageCmyk != null) {
				List<JobTask> taskList = new List<JobTask>();

				if (Diffuse) {
					ImageData imgData = core.appContext.testBenchImageCmykDiffused[Index - 1];
					_jobManager.AppendTaskImageGalvo(taskList, imgData.data, imgData.width, imgData.height, 0.05f, 180);
				} else {
					ImageData imgData = core.appContext.testBenchImageCmyk[Index - 1];
					_jobManager.AppendTaskImageHalftoneGalvo(taskList, imgData.data, imgData.width, imgData.height, 0.085f);
				}

				_jobManager.SetTaskList(taskList);
			}
		}
	}

	private float _GetMoveAmount() {
		return float.Parse(uiTextMoveAmount.text);
	}

	public void uiBtnMirrorLeft() {
		_jobManager.StartTaskSimpleMoveRelative(2, -_GetMoveAmount(), 200);
	}

	public void uiBtnMirrorRight() {
		_jobManager.StartTaskSimpleMoveRelative(2, _GetMoveAmount(), 200);
	}

	public void uiBtnLeft() {
		_jobManager.StartTaskSimpleMoveRelative(0, -_GetMoveAmount(), 200);
	}

	public void uiBtnRight() {
		_jobManager.StartTaskSimpleMoveRelative(0, _GetMoveAmount(), 200);
	}

	public void uiBtnUp() {
		_jobManager.StartTaskSimpleMoveRelative(1, _GetMoveAmount(), 200);
	}

	public void uiBtnDown() {
		_jobManager.StartTaskSimpleMoveRelative(1, -_GetMoveAmount(), 200);
	}

	public void uiBtnLaserBurst() {
		_jobManager.CancelTask();
	}

	public void uiBtnSlowLaser() {
		_jobManager.StartTaskModulateLaser(1000, 3);
	}

	public void uiBtnStopLaser() {
		_jobManager.StartTaskStopLaser();
	}

	private ComputeBuffer _candidateCountBuffer;
	private ComputeBuffer candidateMetaBuffer;
	private float[] candidateMetaRaw = new float[7 * 1024 * 1024];

	void Awake() {
		core.singleton = this;
		appContext = new AppContext();
	}

	private GameObject _sampledMeshGob;

	private byte[] _bitCount;

	void Start() {
		_textRenderer = new TextRenderer();
		ImageData textTest = _textRenderer.GetTextImage("100%", 32, this);
		
		_candidateCountBuffer = new ComputeBuffer(4000, 4);
		candidateMetaBuffer = new ComputeBuffer(7 * 1024 * 1024, 4);

		_surfelCandidateBuffer = new ComputeBuffer(1024 * 1024 * 2, 4);

		// gSourceImg = LoadImage("content/gradient.png");
		// gSourceImg = LoadImage("content/cmyk_test/c.png");
		//gSourceImg = LoadImage("content/test_pattern.png");
		// gSourceImg = LoadImage("content/drag_rawr/k.png");
		// gSourceImg = LoadImage("content/drag_2cm/b.png");
		//gSourceImg = LoadImage("content/drag/b.png");
		//  gSourceImg = LoadImage("content/chars/toon.png");
		//gSourceImg = LoadImage("content/chars/disc.png");
		// gSourceImg = LoadImage("content/char_render_sat_smaller.png");
		// _diffusedImg = _Dither(gSourceImg, gSourceImg.width, gSourceImg.height);

		_diffuser = new Diffuser(this);
		
		_ClearCoverageMap();
		// _PrimeMesh();

		_jobManager = new JobManager();
		appContext.jobManager = _jobManager;
		_jobManager.Init();


	}

	private void _LoadModel() {
		
		//----------------------------------------------------------------------------------------------------------------------------
		// Load mesh resources.
		//----------------------------------------------------------------------------------------------------------------------------
		MeshInfo sourceMeshInfo = ObjImporter.Load("C:/Projects/LDI/Wyvern/Content/figure.obj");
		MeshInfo figureLoadedMeshInfo = PlyImporter.LoadQuadOnly("C:/Projects/LDI/Wyvern/Content/figure.ply");
		// MeshInfo figureLoadedMeshInfo = ObjImporter.Load("C:/Projects/LDI/Wyvern/Content/figure.obj");
		Texture2D figureLoadedTexture = LoadImage("C:/Projects/LDI/Wyvern/Content/None_Base_Color.png", true);

		// Temp resizing.
		sourceMeshInfo.Scale(0.5f);
		figureLoadedMeshInfo.Scale(0.5f);

		// Calculate normals for mesh.
		Mesh figureMeshNormals = appContext.figure.GenerateMesh(figureLoadedMeshInfo, false, true);
		figureLoadedMeshInfo.vertNormals = figureMeshNormals.normals;
		Destroy(figureMeshNormals);

		appContext.figure.SetMesh(figureLoadedMeshInfo);
		// appContext.figure.SetTexture(figureLoadedTexture);
		// appContext.figure.ShowImportedMesh();
		// appContext.figure.GenerateQuadWireMesh(figureLoadedMeshInfo, QuadWireMaterial);

		// Projection.
		// appContext.figure.GenerateDisplayMesh(appContext.figure.initialMeshInfo, ProjectionMaterial);

		// Textured source mesh.
		Mesh sourceMesh = appContext.figure.GenerateMesh(sourceMeshInfo, true, true);
		GameObject sourceMeshGob = new GameObject("source mesh");
		sourceMeshGob.AddComponent<MeshFilter>().mesh = sourceMesh;
		sourceMeshGob.layer = LayerMask.NameToLayer("VisLayer");
		MeshRenderer sourceMeshRenderer = sourceMeshGob.AddComponent<MeshRenderer>();
		sourceMeshRenderer.material = BasicMaterial;
		sourceMeshRenderer.material.SetTexture("_MainTex", figureLoadedTexture);

		//----------------------------------------------------------------------------------------------------------------------------
		// Project source mesh texture to retopo mesh verts.
		//----------------------------------------------------------------------------------------------------------------------------
		ProfileTimer pt = new ProfileTimer();
		MeshCollider sourceMeshCollider = sourceMeshGob.AddComponent<MeshCollider>();
		//sourceMeshCollider.cookingOptions = 
		sourceMeshCollider.sharedMesh = sourceMesh;
		pt.Stop("Created mesh collider");
		
		MeshInfo figureMeshInfo = appContext.figure.initialMeshInfo;
		int figureMeshVertCount = figureMeshInfo.vertPositions.Length;
		figureMeshInfo.vertColors = new Color32[figureMeshVertCount];

		int jobCount = figureMeshVertCount;

		var results = new NativeArray<RaycastHit>(jobCount, Allocator.TempJob);
		var commands = new NativeArray<RaycastCommand>(jobCount, Allocator.TempJob);

		LayerMask mask = LayerMask.GetMask("VisLayer");

		for (int i = 0; i < jobCount; ++i) {
			Vector3 dir = -figureLoadedMeshInfo.vertNormals[i];
			Vector3 origin = figureLoadedMeshInfo.vertPositions[i];
			origin = origin - dir * 0.02f;
			
			commands[i] = new RaycastCommand(origin, dir, 0.1f, mask);
		}

		pt.Start();
		JobHandle handle = RaycastCommand.ScheduleBatch(commands, results, 1, default(JobHandle));
		handle.Complete();
		pt.Stop("Raycasting (" + jobCount + " rays)");

		pt.Start();
		for (int i = 0; i < jobCount; ++i) {
			if (results[i].collider == null) {
				figureMeshInfo.vertColors[i] = new Color32(255, 0, 255, 255);
			} else {
				Vector2 uv = results[i].textureCoord;
				// NOTE: Color is sampled in sRGB space.
				figureMeshInfo.vertColors[i] = figureLoadedTexture.GetPixelBilinear(uv.x, uv.y);
			}
		}
		pt.Stop("Color sampling");

		results.Dispose();
		commands.Dispose();

		Mesh sampledMesh = appContext.figure.GenerateMesh(figureMeshInfo, true, true);
		GameObject sampledMeshGob = new GameObject("sampled mesh");
		sampledMeshGob.AddComponent<MeshFilter>().mesh = sampledMesh;
		MeshRenderer sampledMeshRenderer = sampledMeshGob.AddComponent<MeshRenderer>();
		sampledMeshRenderer.material = SampledMeshMaterial;
		// sampledMeshRenderer.material = ProjectionMaterial;

		sourceMeshGob.SetActive(false);
		_sampledMeshGob = sampledMeshGob;

		//----------------------------------------------------------------------------------------------------------------------------
		// Generate surfels.
		//----------------------------------------------------------------------------------------------------------------------------
		int surfelCount = figureMeshVertCount;
		_surfelCount = surfelCount;
		for (int i = 0; i < surfelCount; ++i) {
			Surfel s = new Surfel();
			s.scale = 0.005f;
			s.continuous = figureMeshInfo.vertColors[i].r / 255.0f;
			s.pos = figureMeshInfo.vertPositions[i];
			s.normal = figureMeshInfo.vertNormals[i];
			_diffuser.surfels.Add(s);
		}

		//----------------------------------------------------------------------------------------------------------------------------
		// Prepare surfel visibility test.
		//----------------------------------------------------------------------------------------------------------------------------
		float[] surfelData = new float[surfelCount * 6];
		_surfelDataBuffer = new ComputeBuffer(surfelCount, 4 * 6, ComputeBufferType.Structured);
		_resultsBuffer = new ComputeBuffer(surfelCount, 4 * 4, ComputeBufferType.Structured);
		_consumedSurfels = new ComputeBuffer(surfelCount, 4, ComputeBufferType.Structured);

		for (int i = 0; i < surfelCount; ++i) {
			surfelData[i * 6 + 0] = _diffuser.surfels[i].pos.x;
			surfelData[i * 6 + 1] = _diffuser.surfels[i].pos.y;
			surfelData[i * 6 + 2] = _diffuser.surfels[i].pos.z;
			surfelData[i * 6 + 3] = _diffuser.surfels[i].normal.x;
			surfelData[i * 6 + 4] = _diffuser.surfels[i].normal.y;
			surfelData[i * 6 + 5] = _diffuser.surfels[i].normal.z;
		}

		_surfelDataBuffer.SetData(surfelData);

		_surfelVisJobCount = Mathf.CeilToInt(surfelCount / 32.0f);
		Debug.Log("Surfel vis jobs: " + _surfelVisJobCount);

		// Vector4[] resultData = new Vector4[surfelCount];
		// resultsBuffer.GetData(resultData);

		// for (int i = 0; i < surfelCount; ++i) {
		// 	Debug.Log("Result: " + i + " " + resultData[i] + " " + _diffuser.surfels[i].normal);
		// }

		// float[] tempData = new float[surfelCount * 4];
		// for (int i = 0; i < surfelCount; ++i) {
		// 	tempData[i * 4 + 0] = _diffuser.surfels[i].normal.x;
		// 	tempData[i * 4 + 1] = _diffuser.surfels[i].normal.y;
		// 	tempData[i * 4 + 2] = _diffuser.surfels[i].normal.z;
		// 	tempData[i * 4 + 3] = 1.0f;
		// }

		// resultsBuffer.SetData(tempData);

		//----------------------------------------------------------------------------------------------------------------------------
		// Visualize surfels.
		//----------------------------------------------------------------------------------------------------------------------------
		_diffuser.Process();
		_surfelDebugMaterial = _diffuser.DrawDebug();
		// _diffuser.DrawDebugFastSurfels(null);

		//----------------------------------------------------------------------------------------------------------------------------
		// Bake surfel views.
		//----------------------------------------------------------------------------------------------------------------------------
		// _bitCount = new byte[256];
		// for (int i = 0; i < 256; ++i) {
		// 	int counter = 
		// 	((i >> 0) & 1) +
		// 	((i >> 1) & 1) +
		// 	((i >> 2) & 1) +
		// 	((i >> 3) & 1) +
		// 	((i >> 4) & 1) +
		// 	((i >> 5) & 1) +
		// 	((i >> 6) & 1) +
		// 	((i >> 7) & 1);

		// 	_bitCount[i] = (byte)counter;
		// }

		// Vector3[] visibilityViews = core.PointsOnSphere(2000);
		// SurfelViewsBake viewBake = BakeSurfelVisRough(visibilityViews);

		// for (int i = 0; i < 40; ++i) {
		// 	CountBakedSurfels(viewBake);
		// }

		// GPU count baked surfels.
		// _TestGPUCount();

		//----------------------------------------------------------------------------------------------------------------------------

		// for (int i = 0; i < 1; ++i) {
		// 	int viewId = CalcSurfelVisRough(visibilityViews);
		// 	Debug.Log("Found view: " + viewId);

		// 	if (viewId != -1) {
		// 		SetSurfelCoverageRough(visibilityViews[viewId], i + 1);
		// 	}
		// }

		// {

		// 	int threadCount = 12;
		// 	Thread[] threads = new Thread[threadCount];

		// 	// Speed test for buffer savings.
		// 	int totalSurfels = 1200000;
		// 	int maskBytes = Mathf.CeilToInt(totalSurfels / 8.0f);
		// 	byte[] stConsumed = new byte[maskBytes];

		// 	for (int i = 0; i < maskBytes; ++i) {
		// 		stConsumed[i] = (byte)(UnityEngine.Random.value * 255);
		// 	}

		// 	// NOTE: Set remainder bits to not visible.

		// 	int stViewCount = 2000;

		// 	ViewCache[] viewCaches = new ViewCache[stViewCount];

		// 	for (int i = 0; i < stViewCount; ++i) {
		// 		byte[] stViewVis = new byte[maskBytes];

		// 		for (int j = 0; j < maskBytes; ++j) {
		// 			if (UnityEngine.Random.value >= 0.6f) {
		// 				stViewVis[j] = (byte)(UnityEngine.Random.value * 255);
		// 			} else {
		// 				stViewVis[j] = 0;
		// 			}
		// 		}

		// 		viewCaches[i] = new ViewCache();
		// 		viewCaches[i].visibleSurfelMask = stViewVis;
		// 	}

		// 	ProfileTimer stPt = new ProfileTimer();

		// 	Queue<ViewCache> viewJobs = new Queue<ViewCache>();

		// 	for (int i = 0; i < viewCaches.Length; ++i) {
		// 		viewJobs.Enqueue(viewCaches[i]);
		// 	}

		// 	for (int t = 0; t < threadCount; ++t) {
		// 		threads[t] = new Thread(_ThreadCountSurfels);

		// 		object[] args = { stConsumed, t, viewJobs };
		// 		threads[t].Start(args);
		// 	}

		// 	for (int i = 0; i < threadCount; ++i) {
		// 		threads[i].Join();
		// 	}
			
		// 	stPt.Stop("ST Iter");

		// 	// for (int i = 0; i < viewCaches.Length; ++i) {
		// 	// 	Debug.Log("Count live: " + viewCaches[i].resultCount);
		// 	// }
		// }

		// appContext.figure.GenerateDisplayViews(visibilityViews);
	}

	private void _TestGPUCount() {
		int viewCount = 2000;
		int surfelCount = 1200000;
		int maskIntCount = Mathf.CeilToInt(surfelCount / 32.0f);

		int[] workingSet = new int[maskIntCount];
		workingSet[0] = 255;
		ComputeBuffer workingSetBuffer = new ComputeBuffer(maskIntCount, 4, ComputeBufferType.Structured);		
		workingSetBuffer.SetData(workingSet);
		
		int[] sharedViewMasks = new int[maskIntCount * viewCount];
		sharedViewMasks[0] = 255;
		sharedViewMasks[maskIntCount * 3] = 255;
		ComputeBuffer sharedViewMasksBuffer = new ComputeBuffer(maskIntCount * viewCount, 4, ComputeBufferType.Structured);
		sharedViewMasksBuffer.SetData(sharedViewMasks);

		ComputeBuffer sharedViewResultsBuffer = new ComputeBuffer(viewCount, 4);
		int[] sharedViewResults = new int[viewCount];
		sharedViewResultsBuffer.SetData(sharedViewResults);

		int kernelIdx = ViewSurfelCount.FindKernel("CSMain");

		// int workJobSize = Mathf.CeilToInt(maskIntCount / 8.0f);
		int workJobSize = maskIntCount / 16;
		int totalExecutions = workJobSize * viewCount;
		int dimExecute = Mathf.CeilToInt(Mathf.Sqrt(totalExecutions) / 8.0f);

		Debug.Log("viewCount: " + viewCount);
		Debug.Log("maskIntCount: " + maskIntCount);
		Debug.Log("totalExecutions: " + totalExecutions);
		Debug.Log("dimExecute: " + dimExecute);

		ViewSurfelCount.SetBuffer(kernelIdx, "workingSet", workingSetBuffer);
		ViewSurfelCount.SetBuffer(kernelIdx, "sharedViewMasks", sharedViewMasksBuffer);
		ViewSurfelCount.SetBuffer(kernelIdx, "sharedViewResults", sharedViewResultsBuffer);
		ViewSurfelCount.SetInt("dispatchDim", dimExecute);
		ViewSurfelCount.SetInt("viewCount", viewCount);
		ViewSurfelCount.SetInt("workJobSize", workJobSize);
		ViewSurfelCount.SetInt("maskIntCount", maskIntCount);

		for (int i = 0; i < 10; ++i) {
			ProfileTimer pt = new ProfileTimer();
			ViewSurfelCount.Dispatch(kernelIdx, dimExecute, dimExecute, 1);
			sharedViewResultsBuffer.GetData(sharedViewResults);
			pt.Stop("GPU surfel view count test");
		}

		// for (int i = 0; i < viewCount; ++i) {
		// 	Debug.Log("View " + i + ": " + sharedViewResults[i]);
		// }

		workingSetBuffer.Release();
		sharedViewMasksBuffer.Release();
		sharedViewResultsBuffer.Release();
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

	public void SaveImageDataToPng(byte[] Data, int Width, int Height, string FileName) {
		int totalPixels = Width * Height;
		Color32[] tempCols = new Color32[totalPixels];

		Texture2D tex = new Texture2D(Width, Height);

		for (int i = 0; i < totalPixels; ++i) {
			float srgb = Mathf.Pow(Data[i] / 255.0f, 1.0f / 2.2f);
			byte v = (byte)(srgb * 255.0f);
			tempCols[i].r = v;
			tempCols[i].g = v;
			tempCols[i].b = v;
			tempCols[i].a = 255;
		}
		
		tex.SetPixels32(tempCols);
		tex.Apply(false);

		byte[] bytes = tex.EncodeToPNG();
		System.IO.File.WriteAllBytes(FileName, bytes);

		GameObject.Destroy(tex);
	}

	public void SaveImageDataToPng(ImageData Data, string FileName) {
		int totalPixels = Data.width * Data.height;
		Color32[] tempCols = new Color32[totalPixels];

		Texture2D tex = new Texture2D(Data.width, Data.height);

		for (int i = 0; i < totalPixels; ++i) {
			byte v = (byte)(Data.data[i] * 255);
			tempCols[i].r = v;
			tempCols[i].g = v;
			tempCols[i].b = v;
			tempCols[i].a = 255;
		}
		
		tex.SetPixels32(tempCols);
		tex.Apply(false);

		byte[] bytes = tex.EncodeToPNG();
		System.IO.File.WriteAllBytes(FileName, bytes);

		GameObject.Destroy(tex);
	}

	private void _PrintCMYK(MagickImage Img) {
		byte[] data = Img.GetPixels().ToArray();
		Debug.Log("CMYK: " + data[0] + " " + data[1] + " " + data[2] + " " + data[3]);
	}

	private void _PrintRGB(MagickImage Img) {
		byte[] data = Img.GetPixels().ToArray();
		Debug.Log("RGB: " + data[0] + " " + data[1] + " " + data[2]);
	}

	private void CMYKToSRGB(float C, float M, float Y, float K) {
		byte[] calcData = new byte[4];
		calcData[0] = (byte)(C * 255.0f);
		calcData[1] = (byte)(M * 255.0f);
		calcData[2] = (byte)(Y * 255.0f);
		calcData[3] = (byte)(K * 255.0f);

		MagickReadSettings readSettings = new MagickReadSettings();
		// readSettings.ColorType = ColorType.Bilevel
		readSettings.Width = 1;
		readSettings.Height = 1;
		readSettings.ColorSpace = ImageMagick.ColorSpace.CMYK;
		readSettings.Format = MagickFormat.Cmyk;
		
		MagickImage calcImg = new MagickImage(calcData, readSettings);
		
		_PrintCMYK(calcImg);
		// byte[] iccProfile = System.IO.File.ReadAllBytes("USWebUncoated.icc");
		// bool result = calcImg.TransformColorSpace(new ColorProfile(iccProfile), ColorProfile.SRGB, ColorTransformMode.HighRes);
		bool result = calcImg.TransformColorSpace(ColorProfile.USWebCoatedSWOP, ColorProfile.SRGB, ColorTransformMode.HighRes);
		// Debug.Log("TEST: " + result + " " + calcImg.ChannelCount);
		_PrintRGB(calcImg);

		calcImg.Write("cmykToSrgb.png");
	}

	private int _CountDots(ImageData Img) {
		int result = 0;

		for (int j = 0; j < Img.width * Img.height; ++j) {
				if (Img.data[j] == 0) {
					++result;
				}
			}

		return result;
	}

	private byte[][] _SRGBFileToCMYKChannels(string Path, out int Width, out int Height) {
		byte[][] channels = new byte[4][];
		Width = 0;
		Height = 0;

		using (MagickImage image = new MagickImage(Path)) {
			// byte[] iccProfile = System.IO.File.ReadAllBytes("SWOP2006_Coated3_GCR_bas.icc");
			// byte[] iccProfile = System.IO.File.ReadAllBytes("USWebCoatedSWOP.icc");
			// byte[] iccProfile = System.IO.File.ReadAllBytes("WebCoatedSWOP2006Grade5.icc");
			// byte[] iccProfile = System.IO.File.ReadAllBytes("RSWOP.icm");
			// byte[] iccProfile = System.IO.File.ReadAllBytes("USWebUncoated.icc");
			// bool result = image.TransformColorSpace(ColorProfile.SRGB, new ColorProfile(iccProfile), ColorTransformMode.HighRes);
			bool result = image.TransformColorSpace(ColorProfile.SRGB, ColorProfile.USWebCoatedSWOP, ColorTransformMode.HighRes);
			Debug.Log("Channels: " + image.ChannelCount + " " + result);

			IPixelCollection<byte> pixels = image.GetPixels();

			byte[] rawPixels = pixels.ToArray();
			int channelByteCount = image.Width * image.Height;

			Width = image.Width;
			Height = image.Height;

			// byte[][] channels = new byte[4][];
			
			for (int i = 0; i < 4; ++i) {
				channels[i] = new byte[channelByteCount];
			}

			// byte[] imgC = new byte[channelByteCount];
			// byte[] imgM = new byte[channelByteCount];
			// byte[] imgY = new byte[channelByteCount];
			// byte[] imgK = new byte[channelByteCount];

			// resultImg.width = image.Width;
			// resultImg.height = image.Height;
			// resultImg.data = new float[channelByteCount];

			for (int iY = 0; iY < image.Height; ++iY) {
				for (int iX = 0; iX < image.Width; ++iX) {
					int idx = iY * image.Width + iX;
					// int dIdx = (image.Height - 1 - iY) * image.Width + iX;
					int dIdx = (iY) * image.Width + iX;

					float c = rawPixels[idx * 4 + 0] / 255.0f;
					float m = rawPixels[idx * 4 + 1] / 255.0f;
					float y = rawPixels[idx * 4 + 2] / 255.0f;
					float k = rawPixels[idx * 4 + 3] / 255.0f;

					// Convert value to sRGB for viewing purposes.
					// c = Mathf.Pow(c, 1.0f / 2.2f);
					// m = Mathf.Pow(m, 1.0f / 2.2f);
					// y = Mathf.Pow(y, 1.0f / 2.2f);
					// k = Mathf.Pow(k, 1.0f / 2.2f);

					channels[0][dIdx] = (byte)((1.0f - c) * 255);
					channels[1][dIdx] = (byte)((1.0f - m) * 255);
					channels[2][dIdx] = (byte)((1.0f - y) * 255);
					channels[3][dIdx] = (byte)((1.0f - k) * 255);

					// resultImg.data[dIdx] = channels[Channel][dIdx] / 255.0f;
				}
			}
			
			// SaveImageDataToPng(imgC, image.Width, image.Height, "0.png");
			// SaveImageDataToPng(imgM, image.Width, image.Height, "1.png");
			// SaveImageDataToPng(imgY, image.Width, image.Height, "2.png");
			// SaveImageDataToPng(imgK, image.Width, image.Height, "3.png");
		}

		return channels;
	}

	public Texture2D ImageDataToTexture(ImageData Data) {
		Texture2D result = new Texture2D(Data.width, Data.height, TextureFormat.RGB24, false, false);
		result.filterMode = FilterMode.Point;
		Color32[] tempCols = new Color32[Data.data.Length];

		for (int i = 0; i < Data.data.Length; ++i) {
			byte val = (byte)(Data.data[i] * 255.0f);
			tempCols[i].r = val;
			tempCols[i].g = val;
			tempCols[i].b = val;
		}

		result.SetPixels32(tempCols);
		result.Apply(false);

		return result;
	}

	public ImageData[] Load2dImageCmykMulti(string FilePath) {
		int cmykWidth = 0;
		int cmykHeight = 0;
		byte[][] cmykChannels = _SRGBFileToCMYKChannels(FilePath, out cmykWidth, out cmykHeight);
		ImageData[] channels = new ImageData[4];
		
		for (int i = 0; i < 4; ++i) {
			channels[i] = new ImageData();
			channels[i].width = cmykWidth;
			channels[i].height = cmykHeight;
			channels[i].data = new float[cmykWidth * cmykHeight];

			for (int j = 0; j < cmykWidth * cmykHeight; ++j) {
				channels[i].data[j] = cmykChannels[i][j] / 255.0f;
			}
		}

		return channels;
	}

	public ImageData Load2dImageCMYK(string FilePath, int CmykId) {
		int cmykWidth = 0;
		int cmykHeight = 0;
		byte[][] cmykChannels = _SRGBFileToCMYKChannels(FilePath, out cmykWidth, out cmykHeight);
		ImageData[] channels = new ImageData[4];
		
		for (int i = 0; i < 4; ++i) {
			channels[i] = new ImageData();
			channels[i].width = cmykWidth;
			channels[i].height = cmykHeight;
			channels[i].data = new float[cmykWidth * cmykHeight];

			for (int j = 0; j < cmykWidth * cmykHeight; ++j) {
				channels[i].data[j] = cmykChannels[i][j] / 255.0f;
			}
		}

		return channels[CmykId];
	}

	public ImageData Diffuse2dImageCMYK(string FilePath, int CmykId) {
		int cmykWidth = 0;
		int cmykHeight = 0;
		// byte[][] cmykChannels = _SRGBFileToCMYKChannels("content/galvoViewTest.png", out cmykWidth, out cmykHeight);
		byte[][] cmykChannels = _SRGBFileToCMYKChannels(FilePath, out cmykWidth, out cmykHeight);
		
		ImageData[] channels = new ImageData[4];
		int totalBurnDots = 0;

		for (int i = 0; i < 4; ++i) {
			ImageData channel = new ImageData();
			channel.width = cmykWidth;
			channel.height = cmykHeight;
			channel.data = new float[channel.width * channel.height];

			for (int j = 0; j < channel.width * channel.height; ++j) {
				channel.data[j] = cmykChannels[i][j] / 255.0f;
			}

			channels[i] = _Dither(channel);

			int dots = _CountDots(channels[i]);
			Debug.Log("Dots: " + dots + "/" + channels[i].width * channels[i].height);

			totalBurnDots += dots;
		}

		Debug.Log("Total dots: " + totalBurnDots + "/" + ((cmykWidth * cmykHeight) * 4));

		// _diffusedImg = channels[3];

		return channels[CmykId];
	}

	private void _DiffusionPerceptualTest() {
		Texture2D sourceImg = null;// core.appContext.testBenchImage;

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
				// float lum = srcRaw[sIdx].r * 0.2126f + srcRaw[sIdx].g * 0.7152f + srcRaw[sIdx].b * 0.0722f;
				float lum = srcRaw[sIdx].r;
				srcLum[i] = lum / 255.0f;

				// sRGB to linear.
				srcLum[i] = GammaToLinear(srcLum[i]);
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
			byte lumByte = (byte)(srcLum[i] * 255.0f);
			// filterImg[i] = Mathf.Pow(filterImg[i], 1.0f / 2.2f);
			// filterImg[i] = LinearToGamma(filterImg[i]);
			// byte lumByte = (byte)(filterImg[i] * 255.0f);

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

	// Vector3 GammaToLinear(Vector3 In) {
	// 	Vector3 linearRGBLo = In / 12.92f;
	// 	Vector3 linearRGBHi = pow(max(abs((In + 0.055) / 1.055), 1.192092896e-07), float3(2.4, 2.4, 2.4));

	// 	return Out = float3(In <= 0.04045) ? linearRGBLo : linearRGBHi;
	// }

	// void LinearToGamma(float3 In, out float3 Out) {
	// 	float3 sRGBLo = In * 12.92;
	// 	float3 sRGBHi = (pow(max(abs(In), 1.192092896e-07), float3(1.0 / 2.4, 1.0 / 2.4, 1.0 / 2.4)) * 1.055) - 0.055;
	// 	Out = float3(In <= 0.0031308) ? sRGBLo : sRGBHi;
	// }

	float GammaToLinear(float In) {
		float linearRGBLo = In / 12.92f;
		float linearRGBHi = Mathf.Pow(Mathf.Max(Mathf.Abs((In + 0.055f) / 1.055f), 1.192092896e-07f), 2.4f);

		return (In <= 0.04045) ? linearRGBLo : linearRGBHi;
	}

	float LinearToGamma(float In) {
		float sRGBLo = In * 12.92f;
		float sRGBHi = (Mathf.Pow(Mathf.Max(Mathf.Abs(In), 1.192092896e-07f), 1.0f / 2.4f) * 1.055f) - 0.055f;
		return (In <= 0.0031308f) ? sRGBLo : sRGBHi;
	}

	public ImageData _Dither(ImageData Data) {
		ImageData result = new ImageData();
		result.width = Data.width;
		result.height = Data.height;
		result.data = new float[Data.width * Data.height];

		// Get image.
		for (int iY = 0; iY < Data.height; ++iY) {
			for (int iX = 0; iX < Data.width; ++iX) {
				int idx = iY * Data.width + iX;
				
				// result.data[idx] = 1.0f - Data.data[idx];
				result.data[idx] = Data.data[idx];

				// sRGB to linear.
				// result.data[idx] = Mathf.Pow(result.data[idx], 2.2f);
				// Linear to sRGB.
				// result.data[idx] = Mathf.Pow(result.data[idx], 1.0f / 2.2f);
			}
		}
		
		// Floyd boi dithering.
		for (int iY = 0; iY < Data.height; ++iY) {
			for (int iX = 0; iX < Data.width; ++iX) {
				int sIdx = iY * Data.width + iX;
				
				float finalColor = _GetClosestLum(result.data[sIdx]);

				float error = result.data[sIdx] - finalColor;

				result.data[sIdx] = finalColor;

				if (iX < Data.width - 1) {
					result.data[sIdx + 1] += error * 7.0f / 16.0f;
				}

				if (iY < Data.height - 1) {
					if (iX < Data.width - 1) {
						result.data[sIdx + Data.width + 1] += error * 1.0f / 16.0f;
					}

					if (iX > 0) {
						result.data[sIdx + Data.width - 1] += error * 3.0f / 16.0f;
					}

					result.data[sIdx + Data.width] += error * 5.0f / 16.0f;
				}
			}
		}

		return result;
	}

	private ImageData _Dither(Texture2D Source, int SourceWidth, int SourceHeight) {
		Color32[] sourceRawData = Source.GetPixels32();
		ImageData result = new ImageData();
		result.width = SourceWidth;
		result.height = SourceHeight;
		result.data = new float[SourceWidth * SourceHeight];

		// Get image.
		for (int iY = 0; iY < SourceHeight; ++iY) {
			for (int iX = 0; iX < SourceWidth; ++iX) {
				int sIdx = (SourceHeight - 1 - iY) * SourceWidth + iX;
				int idx = iY * SourceWidth + iX;
				
				float lum = sourceRawData[sIdx].r * 0.2126f + sourceRawData[sIdx].g * 0.7152f + sourceRawData[sIdx].b * 0.0722f;
				result.data[idx] = lum / 255.0f;

				// sRGB to linear.
				result.data[idx] = Mathf.Pow(result.data[idx], 2.2f);
				// Linear to sRGB.
				// result.data[idx] = Mathf.Pow(result.data[idx], 1.0f / 2.2f);
				
				// Apply brightness, contrast.
				// float l = result.data[idx];
				// l = Mathf.Pow(l, _threshold) + _brightness;

				// result.data[idx] = 1.0f - Mathf.Clamp01(l);
				// result.data[idx] = Mathf.Clamp01(l);
			}
		}
		
		// Floyd boi dithering.
		for (int iY = 0; iY < SourceHeight; ++iY) {
			for (int iX = 0; iX < SourceWidth; ++iX) {
				int sIdx = iY * SourceWidth + iX;
				
				float finalColor = _GetClosestLum(result.data[sIdx]);

				float error = result.data[sIdx] - finalColor;

				result.data[sIdx] = finalColor;

				if (iX < SourceWidth - 1) {
					result.data[sIdx + 1] += error * 7.0f / 16.0f;
				}

				if (iY < SourceHeight - 1) {
					if (iX < SourceWidth - 1) {
						result.data[sIdx + SourceWidth + 1] += error * 1.0f / 16.0f;
					}

					if (iX > 0) {
						result.data[sIdx + SourceWidth - 1] += error * 3.0f / 16.0f;
					}

					result.data[sIdx + SourceWidth] += error * 5.0f / 16.0f;
				}
			}
		}

		return result;
	}

	// private void _CopyAndDither(Texture2D Source, Texture2D Dest, int SourceWidth, int SourceHeight) {
	// 	Color32[] sourceRawData = Source.GetPixels32();

	// 	// Get image.
	// 	for (int i = 0; i < 1000 * 1000; ++i) {
	// 		int dX = i % 1000;
	// 		int dY = i / 1000;

	// 		// int sIdx = (dY + 500) * SourceWidth + (dX + 500);
	// 		int sIdx = (dY) * SourceWidth + (dX);

	// 		if (sIdx < SourceWidth * SourceHeight && dY < SourceHeight && dX < SourceWidth) {
	// 			float lum = sourceRawData[sIdx].r * 0.2126f + sourceRawData[sIdx].g * 0.7152f + sourceRawData[sIdx].b * 0.0722f;
	// 			_canvasRawData[i] = lum / 255.0f;

	// 			// sRGB to linear.
	// 			_canvasRawData[i] = Mathf.Pow(_canvasRawData[i], 2.2f);
	// 			// Linear to sRGB.
	// 			// _canvasRawData[i] = Mathf.Pow(_canvasRawData[i], 1.0f / 2.2f);
	// 		} else {
	// 			_canvasRawData[i] = 1.0f;
	// 		}

	// 		// Apply brightness, contrast.
	// 		float l = _canvasRawData[i];
	// 		l = Mathf.Pow(l, _threshold) + _brightness;

	// 		// _canvasRawData[i] = 1.0f - Mathf.Clamp01(l);
	// 		_canvasRawData[i] = Mathf.Clamp01(l);
	// 	}
		
	// 	// Floyd boi dithering.
	// 	for (int i = 0; i < 1000 * 1000; ++i) {
	// 		int dX = i % 1000;
	// 		int dY = i / 1000;

	// 		float finalColor = _GetClosestLum(_canvasRawData[i]);

	// 		float error = _canvasRawData[i] - finalColor;

	// 		_canvasRawData[i] = finalColor;

	// 		if (dX < 1000 - 1) {
	// 			_canvasRawData[i + 1] += error * 7.0f / 16.0f;
	// 		}

	// 		if (dY < 1000 - 1) {
	// 			if (dX < 1000 - 1) {
	// 				_canvasRawData[i + 1000 + 1] += error * 1.0f / 16.0f;
	// 			}

	// 			if (dX > 0) {
	// 				_canvasRawData[i + 1000 - 1] += error * 3.0f / 16.0f;
	// 			}

	// 			_canvasRawData[i + 1000] += error * 5.0f / 16.0f;
	// 		}
	// 	}

	// 	// Apply final image.
	// 	for (int i = 0; i < 1000 * 1000; ++i) {
	// 		_canvasRawData[i] = Mathf.Clamp01(_canvasRawData[i]);
	// 		byte lumByte = (byte)(_canvasRawData[i] * 255.0f);

	// 		_tempCols[i].r = lumByte;
	// 		_tempCols[i].g = lumByte;
	// 		_tempCols[i].b = lumByte;
	// 	}
		
	// 	Dest.SetPixels32(_tempCols);
	// 	Dest.Apply(false);
	// }
	
	public static Texture2D LoadImage(string path, bool filtered = false) {
		Texture2D tex = null;
		byte[] fileData;

		if (File.Exists(path)) {
			fileData = File.ReadAllBytes(path);
			tex = new Texture2D(2, 2, TextureFormat.RGB24, false, false);

			if (!filtered) {
				tex.filterMode = FilterMode.Point;
			}

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

	private int _surfelCount;
	private int _surfelVisJobCount;
	private int _surfelKernelIndex;
	private ComputeBuffer _surfelDataBuffer;
	private ComputeBuffer _resultsBuffer;
	private ComputeBuffer _consumedSurfels;
	private Material _surfelDebugMaterial;
	private ComputeBuffer _surfelCandidateBuffer;

	public void CaptureSurfels() {
		Debug.Log("Capture surfel view");
		int[] candidateCount = new int[1];
		candidateCount[0] = 69;

		_candidateCountBuffer.SetData(candidateCount);

		// Projector view.
		Matrix4x4 scaleMatrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, new Vector3(1, 1, -1));
		Matrix4x4 projMat = Matrix4x4.Perspective(14.25f, 1.0f, 1.0f, 40.0f);
		Matrix4x4 realProj = GL.GetGPUProjectionMatrix(projMat, true);
		Matrix4x4 projVP = realProj * (scaleMatrix * RayTester.transform.worldToLocalMatrix);

		GameObject previewModel = _sampledMeshGob;

		CommandBuffer cmdBuffer = new CommandBuffer();
		cmdBuffer.name = "Render scanner view";

		Mesh figureMesh = previewModel.GetComponent<MeshFilter>().mesh;
		Matrix4x4 figureMatrix = previewModel.transform.localToWorldMatrix;

		// Draw depth.
		cmdBuffer.SetRenderTarget(ScannerViewDepthRt);
		cmdBuffer.SetViewport(new Rect(0, 0, 1024, 1024));
		cmdBuffer.ClearRenderTarget(true, true, new Color(0.0f, 0.0f, 1.0f, 1.0f), 1.0f);
		BasicShaderMat.SetMatrix("vpMat", projVP);
		cmdBuffer.DrawMesh(figureMesh, figureMatrix, BasicShaderMat);

		Graphics.ExecuteCommandBuffer(cmdBuffer);
		cmdBuffer.Release();

		// NOTE: Force updates to all shader params to support hot reloading.
		int kernelIdx = SurfelVisCompute.FindKernel("GetCandidates");
		SurfelVisCompute.SetInt("surfelCount", _surfelCount);
		SurfelVisCompute.SetBuffer(kernelIdx, "candidateCounter", _candidateCountBuffer);
		SurfelVisCompute.SetBuffer(kernelIdx, "candidateMeta", _surfelCandidateBuffer);
		SurfelVisCompute.SetTexture(kernelIdx, "depthBuffer", ScannerViewDepthRt);
		SurfelVisCompute.SetBuffer(kernelIdx, "consumed", _consumedSurfels);
		SurfelVisCompute.SetBuffer(kernelIdx, "surfels", _surfelDataBuffer);
		SurfelVisCompute.SetBuffer(kernelIdx, "results", _resultsBuffer);
		SurfelVisCompute.SetMatrix("modelMat", figureMatrix);
		SurfelVisCompute.SetMatrix("projVP", projVP);
		SurfelVisCompute.SetVector("projWorldPos", RayTester.transform.position);
		SurfelVisCompute.Dispatch(kernelIdx, _surfelVisJobCount, 1, 1);
		
		_candidateCountBuffer.GetData(candidateCount);

		Debug.Log("Candidate count: " + candidateCount[0]);
	}

	public void SetSurfelCoverageRough(Vector3 visView, int coverageValue) {
		GameObject previewModel = _sampledMeshGob;
		Mesh figureMesh = previewModel.GetComponent<MeshFilter>().mesh;
		Matrix4x4 figureMatrix = previewModel.transform.localToWorldMatrix;

		int kernelIdx = SurfelVisCompute.FindKernel("CoverageSetNoFocalDepth");
		SurfelVisCompute.SetInt("surfelCount", _surfelCount);
		
		SurfelVisCompute.SetTexture(kernelIdx, "depthBuffer", ScannerViewDepthRt);
		SurfelVisCompute.SetBuffer(kernelIdx, "consumed", _consumedSurfels);
		SurfelVisCompute.SetBuffer(kernelIdx, "surfels", _surfelDataBuffer);
		SurfelVisCompute.SetBuffer(kernelIdx, "candidateCounter", _candidateCountBuffer);
		SurfelVisCompute.SetBuffer(kernelIdx, "results", _resultsBuffer);
		SurfelVisCompute.SetInt("coverageValue", coverageValue);
		SurfelVisCompute.SetMatrix("modelMat", figureMatrix);

		// Projector view.
		Matrix4x4 scaleMatrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, new Vector3(1, 1, -1));
		Matrix4x4 projMat = Matrix4x4.Perspective(14.25f, 1.0f, 1.0f, 100.0f);
		Matrix4x4 realProj = GL.GetGPUProjectionMatrix(projMat, true);

		CommandBuffer cmdBuffer = new CommandBuffer();
		cmdBuffer.name = "Render scanner view";

		Vector3 dir = visView;

		Vector3 camPos = dir * 40.0f;
		Matrix4x4 camViewMat = Matrix4x4.TRS(camPos, Quaternion.LookRotation(-dir, Vector3.up), Vector3.one);
		camViewMat = camViewMat.inverse;
		Matrix4x4 projVP = realProj * (scaleMatrix * camViewMat);
		
		cmdBuffer.Clear();

		// Draw depth.
		cmdBuffer.SetRenderTarget(ScannerViewDepthRt);
		cmdBuffer.SetViewport(new Rect(0, 0, 1024, 1024));
		cmdBuffer.ClearRenderTarget(true, true, new Color(0.0f, 0.0f, 1.0f, 1.0f), 1.0f);
		BasicShaderMat.SetMatrix("vpMat", projVP);
		cmdBuffer.DrawMesh(figureMesh, figureMatrix, BasicShaderMat);

		Graphics.ExecuteCommandBuffer(cmdBuffer);
		
		SurfelVisCompute.SetMatrix("projVP", projVP);
		SurfelVisCompute.SetVector("projWorldPos", camPos);
		SurfelVisCompute.Dispatch(kernelIdx, _surfelVisJobCount, 1, 1);

		cmdBuffer.Release();
	}

	public int CalcSurfelVisRough(Vector3[] visibilityViews) {
		GameObject previewModel = _sampledMeshGob;
		Mesh figureMesh = previewModel.GetComponent<MeshFilter>().mesh;
		Matrix4x4 figureMatrix = previewModel.transform.localToWorldMatrix;

		int kernelIdx = SurfelVisCompute.FindKernel("CoverageNoFocalDepth");
		SurfelVisCompute.SetInt("surfelCount", _surfelCount);
		SurfelVisCompute.SetTexture(kernelIdx, "depthBuffer", ScannerViewDepthRt);
		SurfelVisCompute.SetBuffer(kernelIdx, "consumed", _consumedSurfels);
		SurfelVisCompute.SetBuffer(kernelIdx, "surfels", _surfelDataBuffer);
		SurfelVisCompute.SetBuffer(kernelIdx, "candidateCounter", _candidateCountBuffer);
		SurfelVisCompute.SetBuffer(kernelIdx, "results", _resultsBuffer);
		SurfelVisCompute.SetMatrix("modelMat", figureMatrix);

		// Projector view.
		Matrix4x4 scaleMatrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, new Vector3(1, 1, -1));
		Matrix4x4 projMat = Matrix4x4.Perspective(14.25f, 1.0f, 1.0f, 100.0f);
		Matrix4x4 realProj = GL.GetGPUProjectionMatrix(projMat, true);

		CommandBuffer cmdBuffer = new CommandBuffer();
		cmdBuffer.name = "Render scanner view";

		ProfileTimer pt = new ProfileTimer();

		int[] countBuffer = new int[visibilityViews.Length];
		_candidateCountBuffer.SetData(countBuffer);

		for (int i = 0; i < visibilityViews.Length; ++i) {
			// countBuffer[0] = 0;
			// _candidateCountBuffer.SetData(countBuffer);

			Vector3 dir = visibilityViews[i];

			Vector3 camPos = dir * 40.0f;
			Matrix4x4 camViewMat = Matrix4x4.TRS(camPos, Quaternion.LookRotation(-dir, Vector3.up), Vector3.one);
			camViewMat = camViewMat.inverse;
			Matrix4x4 projVP = realProj * (scaleMatrix * camViewMat);
			
			cmdBuffer.Clear();

			// Draw depth.
			cmdBuffer.SetRenderTarget(ScannerViewDepthRt);
			cmdBuffer.SetViewport(new Rect(0, 0, 1024, 1024));
			cmdBuffer.ClearRenderTarget(true, true, new Color(0.0f, 0.0f, 1.0f, 1.0f), 1.0f);
			BasicShaderMat.SetMatrix("vpMat", projVP);
			cmdBuffer.DrawMesh(figureMesh, figureMatrix, BasicShaderMat);

			Graphics.ExecuteCommandBuffer(cmdBuffer);
			
			SurfelVisCompute.SetInt("coverageValue", i);
			SurfelVisCompute.SetMatrix("projVP", projVP);
			SurfelVisCompute.SetVector("projWorldPos", camPos);
			SurfelVisCompute.Dispatch(kernelIdx, _surfelVisJobCount, 1, 1);
		}

		_candidateCountBuffer.GetData(countBuffer);

		int highestCount = 0;
		int highestId = -1;

		for (int i = 0; i < visibilityViews.Length; ++i) {
			if (countBuffer[i] > highestCount) {
				highestCount = countBuffer[i];
				highestId = i;
			}
		}

		Debug.Log("Highest: " + highestId + " " + highestCount);

		cmdBuffer.Release();

		pt.Stop("Calc surfel vis rough");

		return highestId;
	}

	public void CountBakedSurfels(SurfelViewsBake bakedViews) {
		int threadCount = 12;
		Thread[] threads = new Thread[threadCount];

		ProfileTimer stPt = new ProfileTimer();

		Queue<ViewCache> viewJobs = new Queue<ViewCache>();

		for (int i = 0; i < bakedViews.views.Length; ++i) {
			viewJobs.Enqueue(bakedViews.views[i]);
		}

		for (int t = 0; t < threadCount; ++t) {
			threads[t] = new Thread(_ThreadCountSurfels);

			object[] args = { bakedViews.workingSet, viewJobs };
			threads[t].Start(args);
		}

		for (int i = 0; i < threadCount; ++i) {
			threads[i].Join();
		}
		
		int highest = 0;
		int highestId = -1;

		for (int i = 0; i < bakedViews.views.Length; ++i) {
			ViewCache v = bakedViews.views[i];
			// Debug.Log("Count live: " + bakedViews.views[i].resultCount);

			if (v.resultCount > highest) {
				highest = v.resultCount;
				highestId = i;
			}
		}

		stPt.Stop("Count baked views");
		Debug.Log("Highest: " + highestId + " " + highest);
	}

	private void _ThreadCountSurfels(object Data) {
		byte[] workingSet = (byte[])((object[])Data)[0];
		Queue<ViewCache> viewJobs = (Queue<ViewCache>)((object[])Data)[1];

		while (true) {
			ViewCache nextJob = null;

			lock (viewJobs) {
				if (viewJobs.Count == 0) {
					break;
				}

				nextJob = viewJobs.Dequeue();
			}

			int countLive = 0;
			for (int i = 0; i < nextJob.visibleSurfelMask.Length; ++i) {
				int bA = workingSet[i];
				int bB = nextJob.visibleSurfelMask[i];
				int bC = bA & bB;

				countLive += _bitCount[bC];
			}

			nextJob.resultCount = countLive;
		}
	}
	

	public SurfelViewsBake BakeSurfelVisRough(Vector3[] visibilityViews) {
		SurfelViewsBake result = new SurfelViewsBake();
		result.surfelCount = _surfelCount;
		int byteMaskCount = Mathf.CeilToInt(_surfelCount / 8.0f);
		result.workingSet = new byte[byteMaskCount];
		result.views = new ViewCache[visibilityViews.Length];

		// Set all surfels as part of working set.
		for (int i = 0; i < byteMaskCount; ++i) {
			result.workingSet[i] = 255;
		}

		// Clear remaining bits.
		int remainingBits = _surfelCount % 8;
		int finalByte = 255 >> remainingBits;
		result.workingSet[byteMaskCount - 1] = (byte)finalByte;
		Debug.Log(finalByte);

		GameObject previewModel = _sampledMeshGob;
		Mesh figureMesh = previewModel.GetComponent<MeshFilter>().mesh;
		Matrix4x4 figureMatrix = previewModel.transform.localToWorldMatrix;

		int kernelIdx = SurfelVisCompute.FindKernel("CoverageNoFocalDepthBits");
		SurfelVisCompute.SetInt("surfelCount", _surfelCount);
		SurfelVisCompute.SetTexture(kernelIdx, "depthBuffer", ScannerViewDepthRt);
		SurfelVisCompute.SetBuffer(kernelIdx, "consumed", _consumedSurfels);
		SurfelVisCompute.SetBuffer(kernelIdx, "surfels", _surfelDataBuffer);
		SurfelVisCompute.SetBuffer(kernelIdx, "candidateCounter", _candidateCountBuffer);
		SurfelVisCompute.SetBuffer(kernelIdx, "results", _resultsBuffer);
		SurfelVisCompute.SetMatrix("modelMat", figureMatrix);

		// Projector view.
		Matrix4x4 scaleMatrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, new Vector3(1, 1, -1));
		Matrix4x4 projMat = Matrix4x4.Perspective(14.25f, 1.0f, 1.0f, 100.0f);
		Matrix4x4 realProj = GL.GetGPUProjectionMatrix(projMat, true);

		CommandBuffer cmdBuffer = new CommandBuffer();
		cmdBuffer.name = "Render scanner view";

		ProfileTimer pt = new ProfileTimer();

		int intByteMaskCount = Mathf.CeilToInt(_surfelCount / 32.0f);
		ComputeBuffer maskBuffer = new ComputeBuffer(intByteMaskCount, 4, ComputeBufferType.Structured);
		SurfelVisCompute.SetBuffer(kernelIdx, "maskBuffer", maskBuffer);

		Debug.Log("SurfelCount: " + _surfelCount);
		Debug.Log("byteMaskCount: " + byteMaskCount);
		Debug.Log("intMaskCount: " + intByteMaskCount);

		for (int i = 0; i < visibilityViews.Length; ++i) {
			int[] mask = new int[intByteMaskCount];
			maskBuffer.SetData(mask);

			Vector3 dir = visibilityViews[i];

			Vector3 camPos = dir * 40.0f;
			Matrix4x4 camViewMat = Matrix4x4.TRS(camPos, Quaternion.LookRotation(-dir, Vector3.up), Vector3.one);
			camViewMat = camViewMat.inverse;
			Matrix4x4 projVP = realProj * (scaleMatrix * camViewMat);
			
			cmdBuffer.Clear();

			// Draw depth.
			cmdBuffer.SetRenderTarget(ScannerViewDepthRt);
			cmdBuffer.SetViewport(new Rect(0, 0, 1024, 1024));
			cmdBuffer.ClearRenderTarget(true, true, new Color(0.0f, 0.0f, 1.0f, 1.0f), 1.0f);
			BasicShaderMat.SetMatrix("vpMat", projVP);
			cmdBuffer.DrawMesh(figureMesh, figureMatrix, BasicShaderMat);

			Graphics.ExecuteCommandBuffer(cmdBuffer);
			
			SurfelVisCompute.SetInt("coverageValue", i);
			SurfelVisCompute.SetMatrix("projVP", projVP);
			SurfelVisCompute.SetVector("projWorldPos", camPos);
			SurfelVisCompute.Dispatch(kernelIdx, _surfelVisJobCount, 1, 1);

			maskBuffer.GetData(mask);

			ViewCache vc = new ViewCache();
			vc.visibleSurfelMask = new byte[byteMaskCount];
			Buffer.BlockCopy(mask, 0, vc.visibleSurfelMask, 0, byteMaskCount);

			result.views[i] = vc;
		}

		cmdBuffer.Release();
		maskBuffer.Release();

		pt.Stop("Bake surfel vis rough");

		return result;
	}

	void Update() {
		// CameraController.transform.localRotation = Quaternion.Euler(0, Time.time * 10.0f, 0);

		Vector3 laserPos = LaserLocator.transform.position;
		LaserCoverageMat.SetVector("laserPos", new Vector4(laserPos.x, laserPos.y, laserPos.z, 0.0f));

		if (_jobManager != null && _jobManager.controller != null) {
			uiTextPosX.text = "X: " + _jobManager.controller._lastAxisPos[0];
			uiTextPosY.text = "Y: " + _jobManager.controller._lastAxisPos[1];
			uiTextPosZ.text = "Z: " + _jobManager.controller._lastAxisPos[2];
		}

		// NOTE: Only if we have primed mesh.
		// _ScannerViewHit(false);
		// _SurfelTestUpdate();
		// _ProjectionTestUpdate();
	}

	private void _SurfelTestUpdate() {
		// Projector view.
		Matrix4x4 scaleMatrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, new Vector3(1, 1, -1));
		Matrix4x4 projMat = Matrix4x4.Perspective(14.25f, 1.0f, 1.0f, 40.0f);
		Matrix4x4 realProj = GL.GetGPUProjectionMatrix(projMat, true);
		Matrix4x4 projVP = realProj * (scaleMatrix * RayTester.transform.worldToLocalMatrix);

		GameObject previewModel = _sampledMeshGob;

		CommandBuffer cmdBuffer = new CommandBuffer();
		cmdBuffer.name = "Render scanner view";

		Mesh figureMesh = previewModel.GetComponent<MeshFilter>().mesh;
		Matrix4x4 figureMatrix = previewModel.transform.localToWorldMatrix;

		// Draw depth.
		cmdBuffer.SetRenderTarget(ScannerViewDepthRt);
		cmdBuffer.SetViewport(new Rect(0, 0, 1024, 1024));
		cmdBuffer.ClearRenderTarget(true, true, new Color(0.0f, 0.0f, 1.0f, 1.0f), 1.0f);
		BasicShaderMat.SetMatrix("vpMat", projVP);
		cmdBuffer.DrawMesh(figureMesh, figureMatrix, BasicShaderMat);

		Graphics.ExecuteCommandBuffer(cmdBuffer);
		cmdBuffer.Release();

		// NOTE: Force updates to all shader params to support hot reloading.
		_surfelKernelIndex = SurfelVisCompute.FindKernel("CSMain");
		SurfelVisCompute.SetInt("surfelCount", _surfelCount);
		SurfelVisCompute.SetTexture(_surfelKernelIndex, "depthBuffer", ScannerViewDepthRt);
		SurfelVisCompute.SetBuffer(_surfelKernelIndex, "consumed", _consumedSurfels);
		SurfelVisCompute.SetBuffer(_surfelKernelIndex, "surfels", _surfelDataBuffer);
		SurfelVisCompute.SetBuffer(_surfelKernelIndex, "results", _resultsBuffer);
		SurfelVisCompute.SetMatrix("modelMat", figureMatrix);
		SurfelVisCompute.SetMatrix("projVP", projVP);
		SurfelVisCompute.SetVector("projWorldPos", RayTester.transform.position);
		SurfelVisCompute.Dispatch(_surfelKernelIndex, _surfelVisJobCount, 1, 1);
		
		_surfelDebugMaterial.SetBuffer("surfelData", _resultsBuffer);
	}

	private void _ProjectionTestUpdate() {
		// GameObject previewModel = appContext.figure.previewGob;
		GameObject previewModel = _sampledMeshGob;

		CommandBuffer cmdBuffer = new CommandBuffer();
		cmdBuffer.name = "Render scanner view";

		// Get galvo view.
		Matrix4x4 scaleMatrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, new Vector3(1, 1, -1));
		Matrix4x4 projMat = Matrix4x4.Perspective(14.25f, 1.0f, 1.0f, 40.0f);
		Matrix4x4 realProj = GL.GetGPUProjectionMatrix(projMat, true);

		Mesh figureMesh = previewModel.GetComponent<MeshFilter>().mesh;
		Matrix4x4 figureMatrix = previewModel.transform.localToWorldMatrix;

		// Draw depth.
		cmdBuffer.SetRenderTarget(ScannerViewDepthRt);
		cmdBuffer.SetViewport(new Rect(0, 0, 1024, 1024));
		cmdBuffer.ClearRenderTarget(true, true, new Color(0.0f, 0.0f, 1.0f, 1.0f), 1.0f);
		BasicShaderMat.SetMatrix("vpMat", realProj * (scaleMatrix * RayTester.transform.worldToLocalMatrix));
		cmdBuffer.DrawMesh(figureMesh, figureMatrix, BasicShaderMat);

		// Draw pixel candidate buffer.
		cmdBuffer.SetRenderTarget(ScannerViewRt);
		cmdBuffer.SetViewport(new Rect(0, 0, 1024, 1024));
		cmdBuffer.ClearRenderTarget(true, true, new Color(1.0f, 0.0f, 1.0f, 1.0f), 1.0f);
		CandidatesSoftMat.SetMatrix("vpMat", realProj * (scaleMatrix * RayTester.transform.worldToLocalMatrix));
		CandidatesSoftMat.SetVector("camWorldPos", RayTester.transform.position);
		CandidatesSoftMat.SetTexture("depthPreTex", ScannerViewDepthRt);
		cmdBuffer.DrawMesh(figureMesh, figureMatrix, CandidatesSoftMat);

		// Execute commands.
		Graphics.ExecuteCommandBuffer(cmdBuffer);
		cmdBuffer.Release();
		
		// Update figure projection test material.
		Material ptMaterial = previewModel.GetComponent<MeshRenderer>().material;
		ptMaterial.SetTexture("depthPreTex", ScannerViewDepthRt);
		ptMaterial.SetTexture("candidateTex", ScannerViewRt);
		ptMaterial.SetMatrix("projVpMat", realProj * (scaleMatrix * RayTester.transform.worldToLocalMatrix));
		ptMaterial.SetMatrix("projPInvMat", realProj.inverse);
		ptMaterial.SetVector("camWorldPos", RayTester.transform.position);
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
			
		// Matrix4x4 rotMatX = Matrix4x4.Rotate(Quaternion.Euler(-90, 0, 0));
		Matrix4x4 rotMatX = Matrix4x4.Rotate(Quaternion.Euler(0, 0, 0));
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
		// cmdBuffer.DrawMesh(_puffyMesh, modelMat, PuffyMat);

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
			_diffuser.DrawDebugFastSurfels(null);

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
	public ComputeShader SurfelVisCompute;
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

	public Shader QuantaScoreVis;

	public RenderTexture PrimaryViewTexture;

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

		// Vector3[] searchLocations = PointsOnSphere(200);
		// for (int i = 0; i < searchLocations.Length; ++i) {
		// 	GameObject.Instantiate(SampleLocatorPrefab, searchLocations[i] * 20.0f, Quaternion.LookRotation(-searchLocations[i], Vector3.up));
		// }

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
		_sourceUV0 = SourceMesh.uv;
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
		Vector2[] uvs = new Vector2[triCount * 3];
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

			uvs[idx * 3 + 0] = _sourceUV0[v0];
			uvs[idx * 3 + 1] = _sourceUV0[v1];
			uvs[idx * 3 + 2] = _sourceUV0[v2];

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
		visMesh.uv2 = uvs;
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

	public static Vector3[] PointsOnSphere(int n)
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
		rend.material = new Material(QuantaScoreVis);
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
		// float triScale = 0.042f;
		float triScale = 0.08f;

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
