using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;
using SFB;

public class uiCore : MonoBehaviour {

	public static uiCore Singleton;

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

	public Material UILinesMat;

	public core Core;

	public Camera PrimaryCamera;

	private bool _showLaser = false;
	private VisualElement _modelCamViewContainer;
	private Image _primaryView;
	private RenderTexture _camRt;

	private VisualElement _testBenchPanel;
	private VisualElement _imageViewerPanel;
	private VisualElement _modelPanel;
	private VisualElement _platformPanel;

	void Start() {
		Singleton =  this;

		VisualElement root = GetComponent<UIDocument>().rootVisualElement;
		root.AddToClassList("app-root");

		VisualElement menuBar = new VisualElement();
		// panelToolbar.style.flexDirection = FlexDirection.Row;
		menuBar.AddToClassList("toolbar");
		root.Add(menuBar);

		
		
		//------------------------------------------------------------------------------------------------------------------------
		// Toolbar.
		//------------------------------------------------------------------------------------------------------------------------
		// uiPanel panelToolbar = new uiPanel("Wyvern");
		VisualElement panelToolbar = new VisualElement();
		// panelToolbar.style.flexDirection = FlexDirection.Row;
		panelToolbar.AddToClassList("toolbar");
		root.Add(panelToolbar);

		Button btn = new Button();
		btn.text = "Platform";
		btn.clicked += () => {
			if (_platformPanel.style.display == DisplayStyle.None) {
				_platformPanel.style.display = DisplayStyle.Flex;
			} else {
				_platformPanel.style.display = DisplayStyle.None;
			}

		};
		panelToolbar.Add(btn);

		VisualElement toolbarSep = new VisualElement();
		toolbarSep.AddToClassList("toolbar-separator");
		panelToolbar.Add(toolbarSep);

		btn = new Button();
		btn.text = "Images";
		btn.clicked += () => {
			if (_imageViewerPanel.style.display == DisplayStyle.None) {
				_imageViewerPanel.style.display = DisplayStyle.Flex;
			} else {
				_imageViewerPanel.style.display = DisplayStyle.None;
			}

		};
		panelToolbar.Add(btn);

		btn = new Button();
		btn.text = "Model";
		btn.clicked += () => {
			if (_modelPanel.style.display == DisplayStyle.None) {
				_modelPanel.style.display = DisplayStyle.Flex;
			} else {
				_modelPanel.style.display = DisplayStyle.None;
			}

		};
		panelToolbar.Add(btn);

		btn = new Button();
		btn.text = "Test bench";
		btn.clicked += () => {
			if (_testBenchPanel.style.display == DisplayStyle.None) {
				_testBenchPanel.style.display = DisplayStyle.Flex;
			} else {
				_testBenchPanel.style.display = DisplayStyle.None;
			}

		};
		panelToolbar.Add(btn);

		
		VisualElement toolbarExpander = new VisualElement();
		toolbarExpander.style.flexGrow = 1;
		panelToolbar.Add(toolbarExpander);

		//------------------------------------------------------------------------------------------------------------------------
		// Panel container.
		//------------------------------------------------------------------------------------------------------------------------
		VisualElement container = new VisualElement();
		container.style.flexDirection = FlexDirection.Row;
		container.style.flexGrow = 1;
		root.Add(container);

		//------------------------------------------------------------------------------------------------------------------------
		// Platform panel.
		//------------------------------------------------------------------------------------------------------------------------
		uiPanel motionPanel = new uiPanel("Platform");
		_platformPanel = motionPanel;
		motionPanel.style.width = 300;
		motionPanel.style.minWidth = 300;
		container.Add(motionPanel);

		uiPanelCategory connectionCat = new uiPanelCategory("Connection");
		motionPanel.Add(connectionCat);

		uiCore.createButton(connectionCat.Content, "Connect", () => {
		});

		uiPanelCategory positionCat = new uiPanelCategory("Position");
		motionPanel.Add(positionCat);

		VisualElement axisContainer = new VisualElement();
		axisContainer.style.flexDirection = FlexDirection.Row;
		axisContainer.style.justifyContent = Justify.SpaceBetween;
		positionCat.Add(axisContainer);

		Label axisPositionX = new Label("X: 0.00");
		axisPositionX.AddToClassList("motion-label");
		axisContainer.Add(axisPositionX);

		Label axisPositionY = new Label("Y: 0.00");
		axisPositionY.AddToClassList("motion-label");
		axisContainer.Add(axisPositionY);

		Label axisPositionZ = new Label("Z: 0.00");
		axisPositionZ.AddToClassList("motion-label");
		axisContainer.Add(axisPositionZ);

		Button findHomeBtn = new Button();
		findHomeBtn.text = "Find home";
		positionCat.Add(findHomeBtn);

		Button goHomeBtn = new Button();
		goHomeBtn.text = "Go home";
		positionCat.Add(goHomeBtn);

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

		Button moveNegY = new Button();
		moveNegY.AddToClassList("axis-button");
		moveNegY.text = "-Y";
		axisNegContainer.Add(moveNegY);

		Button moveNegZ = new Button();
		moveNegZ.AddToClassList("axis-button");
		moveNegZ.text = "-Z";
		axisNegContainer.Add(moveNegZ);

		VisualElement axisPosContainer = new VisualElement();
		axisPosContainer.style.flexDirection = FlexDirection.Row;
		axisPosContainer.style.justifyContent = Justify.SpaceBetween;
		positionCat.Add(axisPosContainer);

		Button movePosX = new Button();
		movePosX.AddToClassList("axis-button");
		movePosX.text = "+X";
		axisPosContainer.Add(movePosX);

		Button movePosY = new Button();
		movePosY.AddToClassList("axis-button");
		movePosY.text = "+Y";
		axisPosContainer.Add(movePosY);

		Button movePosZ = new Button();
		movePosZ.AddToClassList("axis-button");
		movePosZ.text = "+Z";
		axisPosContainer.Add(movePosZ);

		uiPanelCategory laserCat = new uiPanelCategory("Laser");
		motionPanel.Add(laserCat);
		
		Button showLaserBtn = new Button();
		showLaserBtn.text = "Start laser preview";
		showLaserBtn.clicked += () => {
			_showLaser = !_showLaser;

			if (_showLaser) {
				showLaserBtn.text = "Stop laser preview";
				showLaserBtn.AddToClassList("warning-bkg");
			} else {
				showLaserBtn.text = "Start laser preview";
				showLaserBtn.RemoveFromClassList("warning-bkg");
			}
		};

		laserCat.Add(showLaserBtn);
		
		
		//------------------------------------------------------------------------------------------------------------------------
		// Image viewer.
		//------------------------------------------------------------------------------------------------------------------------
		uiPanel panelRight = new uiPanel("Image viewer");
		_imageViewerPanel = panelRight;
		_imageViewerPanel.style.display = DisplayStyle.None;
		panelRight.style.flexGrow = 1;
		panelRight.Content.style.flexDirection = FlexDirection.Row;

		uiImageViewer imgView = new uiImageViewer("Galvo space 800 x 800 (color)", TextureMap);
		panelRight.Content.Add(imgView);
		panelRight.style.flexGrow = 1;

		VisualElement thumbnailContainer = new VisualElement();
		panelRight.Content.Add(thumbnailContainer);

		Image img = new Image();
		img.image = GalvoViewVis;
		img.style.width = 64;
		img.style.height = 64;
		uiCore.setBorderStyle(img, 1, new Color(0.1607843f, 0.1607843f, 0.1607843f));
		thumbnailContainer.Add(img);

		img = new Image();
		img.image = CandidatesVis;
		img.style.width = 64;
		img.style.height = 64;
		uiCore.setBorderStyle(img, 1, new Color(0.1607843f, 0.1607843f, 0.1607843f));
		thumbnailContainer.Add(img);

		img = new Image();
		img.image = TextureMap;
		img.style.width = 64;
		img.style.height = 64;
		uiCore.setBorderStyle(img, 1, new Color(0.1607843f, 0.1607843f, 0.1607843f));
		thumbnailContainer.Add(img);

		container.Add(panelRight);

		//------------------------------------------------------------------------------------------------------------------------
		// Model inspect panel.
		//------------------------------------------------------------------------------------------------------------------------
		uiPanel modelPanel = new uiPanel("Model");
		_modelPanel = modelPanel;
		modelPanel.style.flexGrow = 1;
		_modelPanel.style.display = DisplayStyle.None;
		// modelPanel.style.width = new StyleLength(200);

		_modelCamViewContainer = new VisualElement();
		_modelCamViewContainer.style.flexGrow = 1;
		// _modelCamViewContainer.AddToClassList("panel-border");
		// _modelCamViewContainer.style.backgroundColor = Color.green;
		_modelCamViewContainer.style.overflow = Overflow.Hidden;

		modelPanel.Content.Add(_modelCamViewContainer);

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

		_modelPanel = modelPanel;
		container.Add(modelPanel);

		//------------------------------------------------------------------------------------------------------------------------
		// Test bench panel.
		//------------------------------------------------------------------------------------------------------------------------
		{
			uiPanel testBenchPanel = new uiPanel("Test bench");
			_testBenchPanel= testBenchPanel;
			testBenchPanel.style.flexGrow = 1;
			container.Add(testBenchPanel);

			VisualElement divider = new VisualElement();
			divider.style.flexDirection = FlexDirection.Row;
			divider.style.flexGrow = 1;
			testBenchPanel.Content.Add(divider);

			VisualElement controlsPanel = new VisualElement();
			controlsPanel.style.width = 250;
			divider.Add(controlsPanel);

			uiImageViewer viewerPanel =  new uiImageViewer("2D diffusion image", null);
			divider.Add(viewerPanel);

			uiPanelCategory imageCat = new uiPanelCategory("Image");
			controlsPanel.Add(imageCat);

			uiCore.createButton(imageCat.Content, "Load image", () => {
				string[] paths = StandaloneFileBrowser.OpenFilePanel("Open File", "", "png", false);
				
				if (paths.Length == 1) {
					Texture2D diffImg = Core.Diffuse2dImage(paths[0]);
					viewerPanel.SetImage(diffImg);
				}
			});

			{
				DropdownField combo = new DropdownField("Display channel");
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
					Debug.Log("Combo change: " + combo.index);
				});

				imageCat.Content.Add(combo);
			}

			Toggle toggle = new Toggle("Show diffused");
			imageCat.Content.Add(toggle);

			uiPanelCategory jobCat = new uiPanelCategory("Job");
			controlsPanel.Add(jobCat);
			
			{
				DropdownField combo = new DropdownField("Channel");
				List<string> choices = new List<string>();
				choices.Add("C - Cyan");
				choices.Add("M - Magenta");
				choices.Add("Y - Yellow");
				choices.Add("K - Black");
				combo.choices = choices;
				combo.index = 0;
				
				jobCat.Content.Add(combo);
			}

			uiCore.createButton(jobCat.Content, "Start job", () => {
				Debug.Log("Start 2D job");
			});
		}
	}

	void Update() {
		Vector2 viewSize = _modelCamViewContainer.layout.size;

		if ((_camRt.width != viewSize.x || _camRt.height != viewSize.y) && (viewSize.x > 0 && viewSize.y > 0)) {
			_camRt.Release();
			_camRt.width = (int)viewSize.x;
			_camRt.height = (int)viewSize.y;
			_camRt.Create();

			_primaryView.style.width = viewSize.x;
			_primaryView.style.height = viewSize.y;
			PrimaryCamera.ResetAspect();
		}
	}
}
