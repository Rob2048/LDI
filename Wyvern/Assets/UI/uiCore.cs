using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;
using SFB;
using System.IO;

public class uiCore : MonoBehaviour {

	public static uiCore singleton;

	public static string EscapeString(string Str) {
		return Str.Replace("\\", "\\\\");
	}

	public static void SetPadding(VisualElement Element, float Padding) {
		Element.style.paddingBottom = Padding;
		Element.style.paddingLeft = Padding;
		Element.style.paddingRight = Padding;
		Element.style.paddingTop = Padding;
	}

	public static void setBorderStyle(VisualElement Element, float Width, Color Color) {
		Element.style.borderTopWidth = Width;
		Element.style.borderLeftWidth = Width;
		Element.style.borderRightWidth = Width;
		Element.style.borderBottomWidth = Width;
		Element.style.borderTopColor = Color;
		Element.style.borderLeftColor = Color;
		Element.style.borderRightColor = Color;
		Element.style.borderBottomColor = Color;
	}

	public static Button createButton(VisualElement Parent, string Text, System.Action Callback) {
		Button btn = new Button();
		btn.text = Text;
		btn.clicked += Callback;
		Parent.Add(btn);

		return btn;
	}

	public Texture GalvoViewVis;
	public Texture CandidatesVis;
	public Texture TextureMap; 
	public Texture PrimaryView; 
	public Texture AppBkg;

	public Camera PrimaryCamera;

	private bool _showLaser = false;
	private VisualElement _modelCamViewContainer;	
	private Image _primaryView;
	private RenderTexture _camRt;

	private VisualElement _testBenchPanel;
	private VisualElement _modelPanel;
	private VisualElement _scanPanel;
	private VisualElement _platformPanel;

	private Label _axisXposition;
	private Label _axisYposition;
	private Label _axisZposition;

	private int _mouseCapButton = -1;
	private Vector2 _mouseCapStart;
	private Vector3 _camData = new Vector3(0, 0, 10);
	private Vector3 _camPos;

	// TODO: Make sure this isn't only baked at startup.
	private float _dpiScale = 1.0f;

	void Awake() {
		singleton = this;
	}

	private void _UpdateTestBenchImage(uiImageViewer ImageViewer, int Index, bool Diffuse) {
		if (Index == 0) {
			if (core.appContext.testBenchImageSrgb != null) {
				ImageViewer.SetImage(core.appContext.testBenchImageSrgb);
			}
		} else if (Index > 0 && Index <= 5) {
			if (core.appContext.testBenchImageCmyk != null) {
				Texture2D viewImg;

				if (Diffuse) {
					viewImg = core.singleton.ImageDataToTexture(core.appContext.testBenchImageCmykDiffused[Index - 1]);
				} else {
					viewImg = core.singleton.ImageDataToTexture(core.appContext.testBenchImageCmyk[Index - 1]);
				}

				ImageViewer.SetImage(viewImg, true);
			}	
		}

		// if (core.appContext.testBenchImageCmyk != null) {
		// 	Texture2D diffImg = core.singleton.ImageDataToTexture(core.appContext.testBenchImageCmyk[combo.index]);
		// 	viewerPanel.SetImage(diffImg);
		// }

		// Texture2D diffImg = core.singleton.ImageDataToTexture(core.appContext.testBenchImageCmyk[0]);
		// viewerPanel.SetImage(diffImg);
	}

	void Start() {
		VisualElement root = GetComponent<UIDocument>().rootVisualElement;
		root.AddToClassList("app-root");

		_dpiScale = Screen.dpi / GetComponent<UIDocument>().panelSettings.referenceDpi;
		Debug.Log("DPI scale: " + _dpiScale);

		VisualElement rootBkg = new VisualElement();
		rootBkg.AddToClassList("root-bkg");
		root.Add(rootBkg);

		Image bkgImg = new Image();
		bkgImg.image = AppBkg;
		rootBkg.Add(bkgImg);

		VisualElement app = new VisualElement();
		app.style.flexGrow = 1;
		root.Add(app);

		VisualElement menuBar = new VisualElement();
		// panelToolbar.style.flexDirection = FlexDirection.Row;
		menuBar.AddToClassList("menu-bar");
		app.Add(menuBar);

		Label tLabel = new Label("File");
		menuBar.Add(tLabel);

		tLabel = new Label("Help");
		menuBar.Add(tLabel);

		uiImageViewer viewerPanel =  new uiImageViewer("2D diffusion image", null);
		
		//------------------------------------------------------------------------------------------------------------------------
		// Toolbar.
		//------------------------------------------------------------------------------------------------------------------------
		// uiPanel panelToolbar = new uiPanel("Wyvern");
		VisualElement panelToolbar = new VisualElement();
		// panelToolbar.style.flexDirection = FlexDirection.Row;
		panelToolbar.AddToClassList("toolbar");
		app.Add(panelToolbar);

		Button wsBtnPlatform = new Button();
		Button wsBtnTestBench = new Button();
		Button wsBtnModel = new Button();

		wsBtnPlatform = new Button();
		wsBtnPlatform.text = "Platform";
		wsBtnPlatform.clicked += () => {
			wsBtnModel.RemoveFromClassList("selected");
			wsBtnTestBench.RemoveFromClassList("selected");
			wsBtnPlatform.AddToClassList("selected");
			_modelPanel.style.display = DisplayStyle.None;
			_testBenchPanel.style.display = DisplayStyle.None;
			_platformPanel.style.display = DisplayStyle.Flex;
		};
		panelToolbar.Add(wsBtnPlatform);

		uiCore.createButton(panelToolbar, "Calibration", null);

		wsBtnTestBench.text = "Test bench";
		wsBtnTestBench.clicked += () => {
			wsBtnModel.RemoveFromClassList("selected");
			wsBtnPlatform.RemoveFromClassList("selected");
			wsBtnTestBench.AddToClassList("selected");
			_modelPanel.style.display = DisplayStyle.None;
			_testBenchPanel.style.display = DisplayStyle.Flex;
			_platformPanel.style.display = DisplayStyle.Flex;
		};
		panelToolbar.Add(wsBtnTestBench);

		{
			VisualElement toolbarSep = new VisualElement();
			toolbarSep.AddToClassList("toolbar-separator");
			panelToolbar.Add(toolbarSep);
		}

		
		wsBtnModel.text = "Model";
		wsBtnModel.clicked += () => {
			wsBtnModel.AddToClassList("selected");
			wsBtnTestBench.RemoveFromClassList("selected");
			wsBtnPlatform.RemoveFromClassList("selected");
			_testBenchPanel.style.display = DisplayStyle.None;
			_platformPanel.style.display = DisplayStyle.None;
			_modelPanel.style.display = DisplayStyle.Flex;
		};
		panelToolbar.Add(wsBtnModel);

		uiCore.createButton(panelToolbar, "Scan", null);
		uiCore.createButton(panelToolbar, "Path", null);
		uiCore.createButton(panelToolbar, "Run", null);
		
		VisualElement toolbarExpander = new VisualElement();
		toolbarExpander.style.flexGrow = 1;
		panelToolbar.Add(toolbarExpander);

		//------------------------------------------------------------------------------------------------------------------------
		// Panel container.
		//------------------------------------------------------------------------------------------------------------------------
		VisualElement container = new VisualElement();
		container.style.flexDirection = FlexDirection.Row;
		container.style.flexGrow = 1;
		app.Add(container);

		//------------------------------------------------------------------------------------------------------------------------
		// Platform panel.
		//-----------------------------------------------------------------------------------------------------------------------
		{
			uiPanel motionPanel = new uiPanel("Platform");
			_platformPanel = motionPanel;
			motionPanel.style.display = DisplayStyle.None;
			motionPanel.style.width = 300;
			motionPanel.style.minWidth = 300;
			container.Add(motionPanel);

			// uiPanelCategory connectionCat = new uiPanelCategory("Connection");
			// motionPanel.Add(connectionCat);

			// uiCore.createButton(connectionCat.Content, "Connect", () => {
			// });

			uiPanelCategory positionCat = new uiPanelCategory("Position");
			motionPanel.Add(positionCat);

			VisualElement axisContainer = new VisualElement();
			axisContainer.style.flexDirection = FlexDirection.Row;
			axisContainer.style.justifyContent = Justify.SpaceBetween;
			positionCat.Add(axisContainer);

			_axisXposition = new Label("X: 0.00");
			_axisXposition.AddToClassList("motion-label-x");
			axisContainer.Add(_axisXposition);

			_axisYposition = new Label("Y: 0.00");
			_axisYposition.AddToClassList("motion-label-y");
			axisContainer.Add(_axisYposition);

			_axisZposition = new Label("Z: 0.00");
			_axisZposition.AddToClassList("motion-label-z");
			axisContainer.Add(_axisZposition);

			Button findHomeBtn = new Button();
			findHomeBtn.text = "Find home";
			positionCat.Add(findHomeBtn);
			findHomeBtn.clicked += () => {
				core.appContext.jobManager.StartTaskZero();
			};

			Button goHomeBtn = new Button();
			goHomeBtn.text = "Go home";
			positionCat.Add(goHomeBtn);
			goHomeBtn.clicked += () => {
				core.appContext.jobManager.StartTaskGotoZero();
			};


			TextField distText = new TextField("Distance");
			distText.value = "1";
			positionCat.Add(distText);

			VisualElement axisNegContainer = new VisualElement();
			axisNegContainer.style.flexDirection = FlexDirection.Row;
			axisNegContainer.style.justifyContent = Justify.SpaceBetween;
			positionCat.Add(axisNegContainer);

			Button moveNegX = new Button();
			moveNegX.AddToClassList("axis-button");
			moveNegX.text = "-X";
			axisNegContainer.Add(moveNegX);
			moveNegX.clicked += () => {
				float dist = 0.0f;
				float.TryParse(distText.text, out dist);
				core.appContext.jobManager.StartTaskSimpleMoveRelative(0, -dist, 300);
			};

			Button moveNegY = new Button();
			moveNegY.AddToClassList("axis-button");
			moveNegY.text = "-Y";
			axisNegContainer.Add(moveNegY);
			moveNegY.clicked += () => {
				float dist = 0.0f;
				float.TryParse(distText.text, out dist);
				core.appContext.jobManager.StartTaskSimpleMoveRelative(1, -dist, 300);
			};

			Button moveNegZ = new Button();
			moveNegZ.AddToClassList("axis-button");
			moveNegZ.text = "-Z";
			axisNegContainer.Add(moveNegZ);
			moveNegZ.clicked += () => {
				float dist = 0.0f;
				float.TryParse(distText.text, out dist);
				core.appContext.jobManager.StartTaskSimpleMoveRelative(2, -dist, 300);
			};

			VisualElement axisPosContainer = new VisualElement();
			axisPosContainer.style.flexDirection = FlexDirection.Row;
			axisPosContainer.style.justifyContent = Justify.SpaceBetween;
			positionCat.Add(axisPosContainer);

			Button movePosX = new Button();
			movePosX.AddToClassList("axis-button");
			movePosX.text = "+X";
			axisPosContainer.Add(movePosX);
			movePosX.clicked += () => {
				float dist = 0.0f;
				float.TryParse(distText.text, out dist);
				core.appContext.jobManager.StartTaskSimpleMoveRelative(0, dist, 300);
			};

			Button movePosY = new Button();
			movePosY.AddToClassList("axis-button");
			movePosY.text = "+Y";
			axisPosContainer.Add(movePosY);
			movePosY.clicked += () => {
				float dist = 0.0f;
				float.TryParse(distText.text, out dist);
				core.appContext.jobManager.StartTaskSimpleMoveRelative(1, dist, 300);
			};

			Button movePosZ = new Button();
			movePosZ.AddToClassList("axis-button");
			movePosZ.text = "+Z";
			axisPosContainer.Add(movePosZ);
			movePosZ.clicked += () => {
				float dist = 0.0f;
				float.TryParse(distText.text, out dist);
				core.appContext.jobManager.StartTaskSimpleMoveRelative(2, dist, 300);
			};

			uiPanelCategory laserCat = new uiPanelCategory("Laser");
			motionPanel.Add(laserCat);
			
			createButton(laserCat.Content, "Start laser preview", () => {
				core.appContext.jobManager.StartTaskModulateLaser(1000, 3);
			});

			createButton(laserCat.Content, "Stop laser preview", () => {
				core.appContext.jobManager.StartTaskStopLaser();
			});

			createButton(laserCat.Content, "Start galvo preview", () => {
				core.appContext.jobManager.StartTaskGalvoPreview();
			});
			
			uiPanelCategory jobCat = new uiPanelCategory("Job");
			motionPanel.Add(jobCat);
			
			createButton(jobCat.Content, "Start job", () => {
				core.singleton.uiBtnStart();
			});

			createButton(jobCat.Content, "Stop job", () => {
				core.appContext.jobManager.CancelTask();
			});
		}
		
		//------------------------------------------------------------------------------------------------------------------------
		// Model inspect panel.
		//------------------------------------------------------------------------------------------------------------------------
		{
			uiPanel modelPanel = new uiPanel("Model viewer");
			_modelPanel = modelPanel;
			modelPanel.style.flexGrow = 1;
			_modelPanel.style.display = DisplayStyle.None;
			// modelPanel.style.width = new StyleLength(200);
			container.Add(modelPanel);

			Label titleLabel = new Label("Textures");
			titleLabel.AddToClassList("panel-title-label-inactive");
			((uiPanel)_modelPanel)._titleContainer.Add(titleLabel);
			
			VisualElement divider = new VisualElement();
			divider.style.flexDirection = FlexDirection.Row;
			divider.style.flexGrow = 1;
			modelPanel.Add(divider);

			VisualElement controlsPanel = new VisualElement();
			controlsPanel.style.width = 300;
			controlsPanel.AddToClassList("border-right");
			divider.Add(controlsPanel);
			
			_modelCamViewContainer = new VisualElement();
			_modelCamViewContainer.style.flexGrow = 1;
			// _modelCamViewContainer.AddToClassList("panel-border");
			// _modelCamViewContainer.style.backgroundColor = Color.green;
			_modelCamViewContainer.style.overflow = Overflow.Hidden;
			
			divider.Add(_modelCamViewContainer);

			_camRt = new RenderTexture(1280, 720, 24);
			_camRt.filterMode = FilterMode.Point;
			PrimaryCamera.targetTexture = _camRt;

			_primaryView = new Image();
			_primaryView.image = _camRt;
			_primaryView.style.flexGrow = 1;
			_primaryView.style.position = Position.Absolute;
			_primaryView.style.width = _camRt.width;
			_primaryView.style.height = _camRt.height;

			_modelCamViewContainer.Add(_primaryView);

			_modelCamViewContainer.RegisterCallback<MouseDownEvent>(OnMouseDown);
			_modelCamViewContainer.RegisterCallback<MouseUpEvent>(OnMouseUp);
			_modelCamViewContainer.RegisterCallback<MouseMoveEvent>(OnMouseMove);
			_modelCamViewContainer.RegisterCallback<WheelEvent>(OnWheelEvent);

			uiPanelCategory sourceCat = new uiPanelCategory("Source model");
			controlsPanel.Add(sourceCat);

			Label statsLabel = new Label("(none)");
			statsLabel.AddToClassList("stats-label");

			Label modelSourcePath = new Label("(none)");
			// Label modelSourcePath = new Label("Triangles");
			// Label modelSourcePath = new Label("Verts");

			uiCore.createButton(sourceCat.Content, "Load model", () => {
				string[] paths = StandaloneFileBrowser.OpenFilePanel("Open File", "", "obj", false);
				
				if (paths.Length == 1) {
					Debug.Log("Model path: " + paths[0]);
					string fileName = Path.GetFileName(paths[0]);
					modelSourcePath.text = EscapeString(fileName);

					MeshInfo meshInfo = ObjImporter.Load(paths[0]);
					meshInfo.Scale(5.0f / 11.0f);

					ModelPipeline modelPipeline = new ModelPipeline();
					modelPipeline.Start(meshInfo);


					// core.appContext.figure.SetMesh(meshInfo);
					// core.appContext.figure.ShowImportedMesh();
					// statsLabel.text = "Vert count: " + meshInfo.vertUvs.Length;
				}
			});
			
			// modelSourcePath.SetEnabled(false);
			sourceCat.Content.Add(modelSourcePath);

			Label textureSourcePath = new Label("(none)");

			uiCore.createButton(sourceCat.Content, "Load texture", () => {
				string[] paths = StandaloneFileBrowser.OpenFilePanel("Open File", "", "png", false);
				
				if (paths.Length == 1) {
					Debug.Log("Texture path: " + paths[0]);
					string fileName = Path.GetFileName(paths[0]);
					textureSourcePath.text = EscapeString(fileName);

					Texture2D texture = core.LoadImage(paths[0]);

					core.appContext.figure.SetTexture(texture);
					core.appContext.figure.ShowImportedMesh();
				}
			});

			uiPanelCategory statsCat = new uiPanelCategory("Statistics");
			controlsPanel.Add(statsCat);
			statsCat.Add(statsLabel);

			sourceCat.Content.Add(textureSourcePath);

			uiPanelCategory propCat = new uiPanelCategory("Stage A");
			controlsPanel.Add(propCat);

			TextField scaleInput = new TextField("Scale");
			scaleInput.value = "1.0";
			propCat.Add(scaleInput);

			uiVector3 translation = new uiVector3(Vector3.zero);
			propCat.Add(translation);

			uiCore.createButton(propCat.Content, "Process stage A", () => {
				float scale = 1.0f;
				if (!float.TryParse(scaleInput.value, out scale)) {
					scale = 1.0f;
					scaleInput.value = scale.ToString();
				}

				Vector3 pos = translation.GetVector3();
				
				// core.appContext.figure.Process(scale, pos);
				core.appContext.figure.ProcessQuadRemeshProjectionStrategy();
				viewerPanel.SetImage(core.appContext.figure.coverageResultRt);
				viewerPanel.modelTest.mesh = core.appContext.figure.GenerateUiMeshQuads(core.appContext.figure.processedMeshInfo);
			});

			uiPanelCategory stageBCat = new uiPanelCategory("Stage B");
			controlsPanel.Add(stageBCat);

			uiCore.createButton(stageBCat.Content, "Process stage B", () => {
				core.appContext.figure.ProcessB();
				// viewerPanel.SetImage(core.singleton.UvTriRt);
				viewerPanel.SetImage(core.appContext.figure.coverageResultRt);
				// viewerPanel.SetImage(core.singleton.OrthoDepthRt);
				viewerPanel.modelTest.mesh = core.appContext.figure.GenerateUiMesh(core.appContext.figure.processedMeshInfo);
			});

			uiPanelCategory stageCCat = new uiPanelCategory("Stage C");
			controlsPanel.Add(stageCCat);

			uiCore.createButton(stageCCat.Content, "Process stage C", () => {
				// core.appContext.figure.ProcessC();
				core.singleton.CaptureSurfels();
			});
		}

		//------------------------------------------------------------------------------------------------------------------------
		// Test bench panel.
		//------------------------------------------------------------------------------------------------------------------------
		{
			uiPanel testBenchPanel = new uiPanel("2D Image diffusion");
			_testBenchPanel = testBenchPanel;
			testBenchPanel.style.flexGrow = 1;
			testBenchPanel.style.display = DisplayStyle.None;
			container.Add(testBenchPanel);

			VisualElement divider = new VisualElement();
			divider.style.flexDirection = FlexDirection.Row;
			divider.style.flexGrow = 1;
			testBenchPanel.Content.Add(divider);

			VisualElement controlsPanel = new VisualElement();
			controlsPanel.style.width = 250;
			divider.Add(controlsPanel);

			divider.Add(viewerPanel);

			uiPanelCategory imageCat = new uiPanelCategory("Image");
			controlsPanel.Add(imageCat);

			Toggle diffuseToggle = new Toggle("Show diffused");
			DropdownField _showDisplayChannel = new DropdownField("Display channel");

			diffuseToggle.RegisterCallback<ChangeEvent<bool>>((evt) => {
					_UpdateTestBenchImage(viewerPanel, _showDisplayChannel.index, diffuseToggle.value);
				});


			uiCore.createButton(imageCat.Content, "Load image", () => {
				string[] paths = StandaloneFileBrowser.OpenFilePanel("Open File", "", "png", false);
				
				if (paths.Length == 1) {
					core.appContext.testBenchImageSrgb = core.LoadImage(paths[0]);
					core.appContext.testBenchImageCmyk = core.singleton.Load2dImageCmykMulti(paths[0]);

					ImageData[] channels = new ImageData[4];
					for (int i = 0; i < 4; ++i) {
						channels[i] = core.singleton._Dither(core.appContext.testBenchImageCmyk[i]);
					}

					core.appContext.testBenchImageCmykDiffused = channels;

					_UpdateTestBenchImage(viewerPanel, _showDisplayChannel.index, diffuseToggle.value);
				}
			});

			{
				DropdownField combo = _showDisplayChannel;
				List<string> choices = new List<string>();
				choices.Add("sRGB");
				choices.Add("C - Cyan");
				choices.Add("M - Magenta");
				choices.Add("Y - Yellow");
				choices.Add("K - Black");
				combo.choices = choices;
				combo.index = 0;
				// combo.RegisterValueChangedCallback<string>(
				combo.RegisterCallback<ChangeEvent<string>>((evt) => {
					_UpdateTestBenchImage(viewerPanel, combo.index, diffuseToggle.value);
				});

				imageCat.Content.Add(combo);
			}
			
			imageCat.Content.Add(diffuseToggle);

			uiPanelCategory jobCat = new uiPanelCategory("Job");
			controlsPanel.Add(jobCat);
			
			// {
			// 	DropdownField combo = new DropdownField("Channel");
			// 	List<string> choices = new List<string>();
			// 	choices.Add("C - Cyan");
			// 	choices.Add("M - Magenta");
			// 	choices.Add("Y - Yellow");
			// 	choices.Add("K - Black");
			// 	combo.choices = choices;
			// 	combo.index = 0;
				
			// 	jobCat.Content.Add(combo);
			// }

			uiCore.createButton(jobCat.Content, "Start job", () => {
				core.singleton.startJob(_showDisplayChannel.index, diffuseToggle.value);
			});
		}

		// Force startup panel: Test bench
		// {
		// 	wsBtnModel.RemoveFromClassList("selected");
		// 	wsBtnPlatform.RemoveFromClassList("selected");
		// 	wsBtnTestBench.AddToClassList("selected");
		// 	_modelPanel.style.display = DisplayStyle.None;
		// 	_testBenchPanel.style.display = DisplayStyle.Flex;
		// 	_platformPanel.style.display = DisplayStyle.Flex;
		// }
		// Force startup panel: Model
		{
			wsBtnModel.AddToClassList("selected");
			wsBtnTestBench.RemoveFromClassList("selected");
			wsBtnPlatform.RemoveFromClassList("selected");
			_testBenchPanel.style.display = DisplayStyle.None;
			_platformPanel.style.display = DisplayStyle.None;
			_modelPanel.style.display = DisplayStyle.Flex;
		}
	}

	void Update() {
		Vector2 viewSize = _modelCamViewContainer.layout.size * _dpiScale;

		if ((_camRt.width != viewSize.x || _camRt.height != viewSize.y) && (viewSize.x > 0 && viewSize.y > 0)) {
			_camRt.Release();
			_camRt.width = (int)viewSize.x;
			_camRt.height = (int)viewSize.y;
			_camRt.Create();

			Vector2 layoutSize = _modelCamViewContainer.layout.size;

			_primaryView.style.width = layoutSize.x;
			_primaryView.style.height = layoutSize.y;
			PrimaryCamera.ResetAspect();
		}

		float currentZ = PrimaryCamera.transform.localPosition.z;
		float targetZ = -_camData.z;

		float error = (targetZ - currentZ);
		float result = error * 16.0f * Time.deltaTime + currentZ;

		if (Mathf.Abs(error) <= 0.01f) {
			result = targetZ;
		}

		PrimaryCamera.transform.localPosition = new Vector3(0, 0, result);

		_axisXposition.text = "X: " + core.appContext.jobManager.controller._lastAxisPos[0];
		_axisYposition.text = "Y: " + core.appContext.jobManager.controller._lastAxisPos[1];
		_axisZposition.text = "Z: " + core.appContext.jobManager.controller._lastAxisPos[2];
	}

	private void OnWheelEvent(WheelEvent evt) {
		_camData.z += evt.delta.y * _camData.z * 0.05f;
		_camData.z = Mathf.Clamp(_camData.z, 0.01f, 50.0f);
		// imgZoom -= evt.delta.y * imgZoom * 0.05f;
	}

	private void OnMouseDown(MouseDownEvent evt) {
		if (_mouseCapButton != -1) {
			return;
		}

		_mouseCapButton = evt.button;
		MouseCaptureController.CaptureMouse(_modelCamViewContainer);
		_mouseCapStart = evt.localMousePosition;
		_camPos = core.singleton.CameraController.transform.position;
	}

	private void OnMouseUp(MouseUpEvent evt) {
		if (evt.button != _mouseCapButton) {
			return;
		}

		if (_mouseCapButton == 0) {
			Vector2 moveDelta = evt.localMousePosition - _mouseCapStart;
			moveDelta *= 0.4f;
		
			_camData.x += moveDelta.x;
			_camData.y -= moveDelta.y;
		}

		_mouseCapButton = -1;
		MouseCaptureController.ReleaseMouse(_modelCamViewContainer);
	}	

	private void OnMouseMove(MouseMoveEvent evt) {
		if (MouseCaptureController.HasMouseCapture(_modelCamViewContainer)) {
			Vector2 moveDelta = evt.localMousePosition - _mouseCapStart;

			if (_mouseCapButton == 0) {
				moveDelta *= 0.4f;

				Quaternion camRotX = Quaternion.AngleAxis(_camData.x + moveDelta.x, Vector3.up);
				Quaternion camRotY = Quaternion.AngleAxis(_camData.y - moveDelta.y, Vector3.left);
				
				core.singleton.CameraController.transform.rotation = camRotX * camRotY;
			} else if (_mouseCapButton == 1) {
				moveDelta *= 0.002f * _camData.z;

				Vector3 worldMove = core.singleton.CameraController.transform.TransformVector(new Vector3(-moveDelta.x, moveDelta.y, 0));

				core.singleton.CameraController.transform.position = _camPos + worldMove;
			}
		}
	}
}
