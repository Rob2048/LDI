using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Platform : MonoBehaviour {

	public Transform workEnvelope;
	public Transform axisA;
	public Transform axisC;

	public Vector3 targetNormal;

	private Vector3[] _dirTargets;
	private Vector3 _currentTarget;
	private int _targetId;
	private float _targetTimer;
	
	void Start() {
		_dirTargets = core.PointsOnSphere(2000);
	}

	void Update() {
		// return;

		_targetTimer += Time.deltaTime;

		if (_targetTimer > 1.0f) {
			_targetTimer -= 1.0f;
			_targetId = (_targetId + 1) % _dirTargets.Length;
		}
		// Vector3 tNormal = new Vector3(Mathf.Sin(Time.time * 0.5f), Mathf.Cos(Time.time * 0.5f), 0);
		// Vector3 tNormal =  Quaternion.Euler(targetNormal) * new Vector3(1, 0, 0);

		_currentTarget = Vector3.Slerp(_currentTarget, _dirTargets[_targetId], 10.0f * Time.deltaTime);
		Vector3 tNormal = _currentTarget;
		tNormal.Normalize();

		float aA = (float)Math.Atan2(tNormal.y, tNormal.x) * Mathf.Rad2Deg;
		axisC.localRotation = Quaternion.AngleAxis(-aA - 90, new Vector3(0, 0, 1));

		float aB = Mathf.Asin(tNormal.z) * Mathf.Rad2Deg;
		axisA.localRotation = Quaternion.AngleAxis(aB - 90, new Vector3(1, 0, 0));

		Vector3 figNormWorld = workEnvelope.TransformDirection(tNormal);
		Debug.DrawLine(workEnvelope.position, workEnvelope.position + figNormWorld, Color.magenta);
		
	}
}
