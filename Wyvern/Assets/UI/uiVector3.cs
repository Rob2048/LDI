using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

public class uiVector3 : VisualElement {

	private TextField _fieldX;
	private TextField _fieldY;
	private TextField _fieldZ;

	public uiVector3(Vector3 data) {
		VisualElement container = new VisualElement();
		this.Add(container);

		container.style.flexDirection = FlexDirection.Row;
		container.style.flexGrow = 1;

		Label fieldXLabel = new Label("X");
		fieldXLabel.AddToClassList("vector-label");
		fieldXLabel.AddToClassList("bkg-x");
		container.Add(fieldXLabel);
		_fieldX = new TextField();
		_fieldX.value = data.x.ToString();
		_fieldX.style.flexGrow = 1;
		container.Add(_fieldX);

		Label fieldYLabel = new Label("Y");
		fieldYLabel.AddToClassList("vector-label");
		fieldYLabel.AddToClassList("bkg-y");
		container.Add(fieldYLabel);
		_fieldY = new TextField();
		_fieldY.value = data.y.ToString();
		_fieldY.style.flexGrow = 1;
		container.Add(_fieldY);

		Label fieldZLabel = new Label("Z");
		fieldZLabel.AddToClassList("vector-label");
		fieldZLabel.AddToClassList("bkg-z");
		container.Add(fieldZLabel);
		_fieldZ = new TextField();
		_fieldZ.value = data.z.ToString();
		_fieldZ.style.flexGrow = 1;
		container.Add(_fieldZ);
	}

	public Vector3 GetVector3() {
		Vector3 result = new Vector3();

		if (!float.TryParse(_fieldX.value, out result.x)) {
				result.x = 0.0f;
				_fieldX.value = result.x.ToString();
		}

		if (!float.TryParse(_fieldY.value, out result.y)) {
				result.y = 0.0f;
				_fieldY.value = result.y.ToString();
		}

		if (!float.TryParse(_fieldZ.value, out result.z)) {
				result.z = 0.0f;
				_fieldZ.value = result.z.ToString();
		}

		return result;
	}
}