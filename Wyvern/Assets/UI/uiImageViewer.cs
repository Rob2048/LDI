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

		img = new Image();
		img.image = Image;
		img.style.position = Position.Absolute;
		img.style.top = 0;
		img.style.left = 0;
		img.style.width = Image.width * imgZoom;
		img.style.height = Image.height * imgZoom;

		this.Add(img);
		_img = Image;

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


	private void OnWheelEvent(WheelEvent evt) {
		Vector2 canvasLocalStart = (evt.localMousePosition - imgPos) / imgZoom;

		imgZoom -= evt.delta.y * imgZoom * 0.05f;
		imgZoom = Mathf.Clamp(imgZoom, 0.01f, 100.0f);

		imgPos = evt.localMousePosition - canvasLocalStart * imgZoom;

		img.style.left = imgPos.x;
		img.style.top = imgPos.y;
		img.style.width = _img.width * imgZoom;
		img.style.height = _img.height * imgZoom;

		titleLabel.text = _title + " (" + imgZoom * 100 + " %)";
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

		Label titleLabel = new Label(Title);
		titleLabel.style.backgroundColor = new StyleColor(new Color(0.1607843f, 0.1607843f, 0.1607843f));
		titleLabel.style.marginBottom = 0;
		titleLabel.style.marginTop = 0;
		titleLabel.style.marginLeft = 0;
		titleLabel.style.marginRight = 0;
		titleLabel.style.paddingLeft = 5;
		titleLabel.style.paddingRight = 2;
		titleLabel.style.paddingTop = 4;
		titleLabel.style.paddingBottom = 4;
		titleLabel.style.fontSize = 10;
		titleLabel.style.color = new StyleColor(new Color(0.945098f, 0.945098f, 0.945098f));

		this.Add(titleLabel);

		Content = new VisualElement();
		Content.style.flexGrow = 1;
		uiCore.SetPadding(Content, 5);
		// Content.style.flexGrow = 1;

		this.Add(Content);
	}
}


