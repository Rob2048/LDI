using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class laserAssembly : MonoBehaviour
{
	public GameObject laserSource;
	public GameObject mirrorX;
	public GameObject mirrorY;
	public GameObject viewPoint;

	public float laserFocus = 20.0f;
	public float laserSourceSize = 0.5f;
	public float galvoMaxAngle = 7.125f;
	public float scaleX = 1.0f;
	public float scaleY = 1.0f;
	public float laserVirtualProjLength = 1.0f;

	private Color laserBounceA = new Color(0.1f, 0, 0);
	private Color laserBounceB = new Color(0.5f, 0, 0);
	private Color laserBounceC = new Color(1.0f, 0, 0);

	void Start() {

	}

	void Update() {
		// SetGalvos(Mathf.Sin(Time.time * Mathf.PI * 2) * galvoMaxAngle, Mathf.Cos(Time.time * Mathf.PI * 2) * galvoMaxAngle);

		float sqrIntTime = (Time.time * 0.25f) % 4.0f;

		if (sqrIntTime <= 1.0f) {
			float t = sqrIntTime;
			SetGalvos(Mathf.Lerp(-galvoMaxAngle, +galvoMaxAngle, t), -galvoMaxAngle);
		} else if (sqrIntTime <= 2.0f) {
			float t = sqrIntTime - 1.0f;
			SetGalvos(galvoMaxAngle, Mathf.Lerp(-galvoMaxAngle, +galvoMaxAngle, t));
		} else if (sqrIntTime <= 3.0f) {
			float t = sqrIntTime - 2.0f;
			SetGalvos(Mathf.Lerp(+galvoMaxAngle, -galvoMaxAngle, t), +galvoMaxAngle);
		} else if (sqrIntTime <= 4.0f) {
			float t = sqrIntTime - 3.0f;
			SetGalvos(-galvoMaxAngle, Mathf.Lerp(+galvoMaxAngle, -galvoMaxAngle, t));
		}
		
		int ringSegs = 32;
		float ringDiv = (Mathf.PI * 2) / ringSegs;
		float halfSourceSize = laserSourceSize * 0.5f;
		Transform ls = laserSource.transform;

		for (int i = 0; i < ringSegs; ++i) {
			float c = i * ringDiv;

			float x = Mathf.Sin(c) * halfSourceSize;
			float y = Mathf.Cos(c) * halfSourceSize;

			Vector3 v0 = new Vector3(x, y, 0);
			Vector3 v1 = new Vector3(0, 0, laserFocus);
			Vector3 vDir = (v1 - v0).normalized;

			Vector3 sourcePoint = ls.TransformPoint(x, y, 0);
			Vector3 sourceDir = ls.TransformDirection(vDir);

			Ray finalRay;
			_Project(sourcePoint, sourceDir, true, out finalRay, laserBounceB);
		}

		Ray centerRay;
		_Project(ls.position, ls.forward, true, out centerRay, Color.green);

		Debug.DrawLine(centerRay.origin, centerRay.origin - centerRay.direction * laserVirtualProjLength, Color.yellow);

		
		DrawViewpointFrustum();
	}

	public void DrawViewpointFrustum() {
		float cornerPos = 2.5f;
		Vector3 vp = viewPoint.transform.position;
		Vector3 v0 = viewPoint.transform.TransformPoint(-cornerPos, -cornerPos, 20);
		Vector3 v1 = viewPoint.transform.TransformPoint(cornerPos, -cornerPos, 20);
		Vector3 v2 = viewPoint.transform.TransformPoint(cornerPos, cornerPos, 20);
		Vector3 v3 = viewPoint.transform.TransformPoint(-cornerPos, cornerPos, 20);
		
		Debug.DrawLine(vp, v0, Color.white, 0);
		Debug.DrawLine(vp, v1, Color.white, 0);
		Debug.DrawLine(vp, v2, Color.white, 0);
		Debug.DrawLine(vp, v3, Color.white, 0);
	}

	public void SetGalvos(float AngleX, float AngleY) {
		AngleX = Mathf.Clamp(AngleX, -galvoMaxAngle, galvoMaxAngle);
		AngleY = Mathf.Clamp(AngleY, -galvoMaxAngle, galvoMaxAngle);

		AngleX *= scaleX;
		AngleY *= scaleY;

		mirrorX.transform.rotation = Quaternion.Euler(0, -90, -45 - AngleX);
		mirrorY.transform.rotation = Quaternion.Euler(0, 0, 45 - AngleY);
	}

	private void _Project(Vector3 Pos, Vector3 Dir, bool Draw, out Ray FinalRay, Color DrawCol) {
		Vector3 sourcePos = Pos;
		Vector3 dir = Dir;

		Plane pX = new Plane(mirrorX.transform.right, mirrorX.transform.position);
		Plane pY = new Plane(mirrorY.transform.right, mirrorY.transform.position);
		Ray r = new Ray(sourcePos, dir);

		FinalRay = r;
		float dist = 0.0f;

		float t = 0.0f;
		if (pX.Raycast(r, out t)) {
			Vector3 hitPos = sourcePos + dir * t;
			Vector3 refDir = Vector3.Reflect(r.direction, pX.normal);
			dist += Vector3.Magnitude(hitPos - sourcePos);

			if (Draw) {
				Debug.DrawLine(sourcePos, hitPos, DrawCol, 0.0f);
			}
			
			r = new Ray(hitPos, refDir);
			FinalRay = r;

			if (pY.Raycast(r, out t)) {
				Vector3 hitPos2 = hitPos + refDir * t;
				Vector3 refDir2 = Vector3.Reflect(r.direction, pY.normal);
				dist += Vector3.Magnitude(hitPos2 - hitPos);

				r = new Ray(hitPos2, refDir2);
				FinalRay = r;

				if (Draw) {
					Debug.DrawLine(hitPos, hitPos2, DrawCol, 0.0f);
					Debug.DrawLine(hitPos2, hitPos2 + refDir2 * (laserFocus - dist), DrawCol, 0.0f);
				}
			}
		}
	}
}
