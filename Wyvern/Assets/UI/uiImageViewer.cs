using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

public class uiModelTest : ImmediateModeElement {

	public Mesh mesh;
	public Vector3 position;
	public float scale;

	public uiModelTest() {
		// mesh = new Mesh();

		// Vector3[] verts = new Vector3[3];
		// Color32[] colors = new Color32[3];
		// int[] indices = new int[6];

		// verts[0] = new Vector3(0, 0, 0);
		// verts[1] = new Vector3(100, 0, 0);
		// verts[2] = new Vector3(100, 100, 0);

		// colors[0] = new Color32(255, 255, 255, 128);
		// colors[1] = new Color32(255, 255, 255, 128);
		// colors[2] = new Color32(255, 255, 255, 128);

		// indices[0] = 0;
		// indices[1] = 1;
		// indices[2] = 1;
		// indices[3] = 2;
		// indices[4] = 2;
		// indices[5] = 0;

		// mesh.SetVertices(verts);
		// mesh.SetColors(colors);
		// mesh.SetIndices(indices, MeshTopology.Lines, 0);

		position = Vector3.zero;
		scale = 1.0f;
	}

	protected override void ImmediateRepaint() {
		if (mesh == null) {
			return;
		}

		Material mat = core.singleton.UiLinesMateral;
		Shader.SetGlobalColor("_Color", new Color(0.2f, 0.5f, 1.0f, 1));
		Shader.SetGlobalVector("_ClipRect", new Vector4(worldBound.xMin, worldBound.yMin, worldBound.xMax, worldBound.yMax));
		Shader.SetGlobalVector("ScreenParams", new Vector4(Screen.width, Screen.height, 0, 0));
		Shader.SetGlobalVector("Transform", new Vector4(position.x, position.y, scale, 0));
		mat.SetPass(0);

		Graphics.DrawMeshNow(mesh, Matrix4x4.identity);
	}
}

public class uiImageViewer : VisualElement {

	private Image img;
	public uiModelTest modelTest;
	private string _title;
	private Vector2 mouseCapStart;
	private Vector2 imgPos = new Vector2(0, 0);
	private	float imgZoom = 1.0f;
	private Texture _img;
	private Label titleLabel;
	
	public uiImageViewer(string Title, Texture Image) {

		this.style.overflow = Overflow.Hidden;
		// this.style.backgroundColor = new Color(0.2431373f, 0.2196078f, 0.282353f);
		uiCore.setBorderStyle(this, 1, new Color(0.1607843f, 0.1607843f, 0.1607843f));
		uiCore.SetPadding(this, 0);

		this.style.flexGrow = 1;
		
		_title = Title;
		titleLabel = new Label(Title);
		titleLabel.style.backgroundColor = new Color(0, 0, 0, 0.5921569f);
		titleLabel.style.marginBottom = 0;
		titleLabel.style.marginTop = 0;
		titleLabel.style.marginLeft = 0;
		titleLabel.style.marginRight = 0;
		titleLabel.style.fontSize = 10;
		titleLabel.style.color = new Color(0.945098f, 0.945098f, 0.945098f);
		titleLabel.style.position = Position.Absolute;

		// uiGridBackground grid = new uiGridBackground();
		// grid.style.position = Position.Absolute;
		// grid.style.width = 200;
		// grid.style.height = 200;
		// this.Add(grid);

		_img = Image;

		img = new Image();
		img.image = _img;
		img.style.position = Position.Absolute;
		img.scaleMode = ScaleMode.StretchToFill;
		img.uv = new Rect(0, 0.0f, 1, -1.0f);
		this.Add(img);

		modelTest = new uiModelTest();
		modelTest.style.position = Position.Absolute;
		modelTest.style.width = Length.Percent(100);
		modelTest.style.height = Length.Percent(100);
		// _modelTest.style.backgroundColor = new Color(0.5f, 0,0, 0.5f);
		this.Add(modelTest);

		_UpdateImageLayout();

		VisualElement titleBar = new VisualElement();
		titleBar.Add(titleLabel);

		this.Add(titleBar);

		// this.focusable = true;
		
		RegisterCallback<KeyDownEvent>(OnKeyDown);
		
		RegisterCallback<FocusInEvent>(OnFocusIn);
		RegisterCallback<FocusOutEvent>(OnFocusOut);
		
		RegisterCallback<MouseDownEvent>(OnMouseDown);
		RegisterCallback<MouseUpEvent>(OnMouseUp);
		RegisterCallback<MouseMoveEvent>(OnMouseMove);
		RegisterCallback<WheelEvent>(OnWheelEvent);
	}

	public void SetImage(Texture Image, bool FlipY = false) {
		_img = Image;
		img.image = _img;
		
		if (FlipY) {
			img.uv = new Rect(0, 0.0f, 1, -1.0f);
		} else {
			img.uv = new Rect(0, 0.0f, 1, 1.0f);
		}

		_UpdateImageLayout();
	}

	private void _UpdateImageLayout() {
		if (_img != null) {
			img.style.left = imgPos.x;
			img.style.top = imgPos.y;
			img.style.width = _img.width * imgZoom;
			img.style.height = _img.height * imgZoom;
		}

		modelTest.position = new Vector3(imgPos.x, imgPos.y, 0);
		modelTest.scale = imgZoom;
	}

	private void OnWheelEvent(WheelEvent evt) {
		Vector2 canvasLocalStart = (evt.localMousePosition - imgPos) / imgZoom;

		imgZoom -= evt.delta.y * imgZoom * 0.05f;
		imgZoom = Mathf.Clamp(imgZoom, 0.01f, 100.0f);

		imgPos = evt.localMousePosition - canvasLocalStart * imgZoom;

		_UpdateImageLayout();
	}

	private void OnMouseDown(MouseDownEvent evt) {
		MouseCaptureController.CaptureMouse(this);
		mouseCapStart = evt.localMousePosition - imgPos;
	}

	private void OnMouseUp(MouseUpEvent evt) {
		MouseCaptureController.ReleaseMouse(this);
	}	

	private void OnMouseMove(MouseMoveEvent evt) {
		if (MouseCaptureController.HasMouseCapture(this)) {
			Vector2 moveDelta = evt.localMousePosition - mouseCapStart;
			imgPos = moveDelta;

			_UpdateImageLayout();
			// img.style.left = imgPos.x;
			// img.style.top = imgPos.y;
		}
	}

	private void OnFocusIn(FocusInEvent evt) {
		uiCore.setBorderStyle(this, 1, new Color(0.7882353f, 0.5529412f, 1));
	}

	private void OnFocusOut(FocusOutEvent evt) {
		uiCore.setBorderStyle(this, 1, new Color(0.1607843f, 0.1607843f, 0.1607843f));
	}

	private void OnKeyDown(KeyDownEvent evt) {
		// Debug.Log("Key: " + evt.keyCode);

		// evt.StopPropagation();
	}
}

public class uiPanel : VisualElement {

	public VisualElement Content;
	public VisualElement _titleContainer;

	public uiPanel(string Title) {
		this.style.backgroundColor = new StyleColor(new Color(0.2196078f, 0.2196078f, 0.2196078f));

		uiCore.SetPadding(this, 0);

		_titleContainer = new VisualElement();
		_titleContainer.AddToClassList("panel-title-container");
		((VisualElement)this).Add(_titleContainer);

		Label titleLabel = new Label(Title);
		titleLabel.AddToClassList("panel-title-label");
		_titleContainer.Add(titleLabel);

		Content = new VisualElement();
		Content.AddToClassList("panel-content");
		((VisualElement)this).Add(Content);
	}

	public new void Add(VisualElement Element) {
		Content.Add(Element);
	}
}

public class uiPanelCategory : VisualElement {

	public VisualElement Content;
	
	public uiPanelCategory(string Title) {
		this.AddToClassList("panel-category");

		Label titleLabel = new Label(Title);
		titleLabel.AddToClassList("panel-category-title");
		((VisualElement)this).Add(titleLabel);

		Content = new VisualElement();
		Content.AddToClassList("panel-category-content");
		((VisualElement)this).Add(Content);
	}

	public new void Add(VisualElement Element) {
		Content.Add(Element);
	}
}