using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

public class uiGridBackground : ImmediateModeElement {

	public uiGridBackground() {

	}

	protected override void ImmediateRepaint() {

		// UnityEditor.HandleUtility.ApplyWireMaterial
		Material mat = uiCore.Singleton.UILinesMat;
		// mat.SetFloat("_HandleZTest", (float)zTest);
		mat.SetPass(0);

		GL.Begin(GL.QUADS);
		GL.Color(new Color(1, 0, 0, 1));
		GL.Vertex(new Vector3(0, 0));
		GL.Vertex(new Vector3(600, 0));
		GL.Vertex(new Vector3(600, 20));
		GL.Vertex(new Vector3(0, 20));
		GL.End();

		
	}
}

public class uiImageViewer : VisualElement {

	private Image img;
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
		this.Add(img);

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

	public void SetImage(Texture Image) {
		_img = Image;
		img.image = _img;
		_UpdateImageLayout();
	}

	private void _UpdateImageLayout() {
		if (_img == null) {
			return;
		}

		img.style.left = imgPos.x;
		img.style.top = imgPos.y;
		img.style.width = _img.width * imgZoom;
		img.style.height = _img.height * imgZoom;
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

			img.style.left = imgPos.x;
			img.style.top = imgPos.y;
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

	public uiPanel(string Title) {
		this.style.backgroundColor = new StyleColor(new Color(0.2196078f, 0.2196078f, 0.2196078f));

		uiCore.SetPadding(this, 0);

		this.style.borderTopWidth = 1;
		this.style.borderLeftWidth = 1;
		this.style.borderRightWidth = 1;
		this.style.borderBottomWidth = 1;
		Color borderColor = new Color(0.1607843f, 0.1607843f, 0.1607843f);
		this.style.borderTopColor = borderColor;
		this.style.borderLeftColor = borderColor;
		this.style.borderRightColor = borderColor;
		this.style.borderBottomColor = borderColor;

		VisualElement titleContainer = new VisualElement();
		titleContainer.AddToClassList("panel-title-container");
		((VisualElement)this).Add(titleContainer);

		Label titleLabel = new Label(Title);
		titleLabel.AddToClassList("panel-title-label");
		// titleLabel.style.backgroundColor = new StyleColor(new Color(0.1607843f, 0.1607843f, 0.1607843f));
		// titleLabel.style.marginBottom = 0;
		// titleLabel.style.marginTop = 0;
		// titleLabel.style.marginLeft = 0;
		// titleLabel.style.marginRight = 0;
		// titleLabel.style.paddingLeft = 5;
		// titleLabel.style.paddingRight = 2;
		// titleLabel.style.paddingTop = 4;
		// titleLabel.style.paddingBottom = 4;
		// titleLabel.style.fontSize = 10;
		// titleLabel.style.color = new StyleColor(new Color(0.945098f, 0.945098f, 0.945098f));
		titleContainer.Add(titleLabel);

		Content = new VisualElement();
		Content.style.flexGrow = 1;
		// uiCore.SetPadding(Content, 5);
		// Content.style.flexGrow = 1;

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