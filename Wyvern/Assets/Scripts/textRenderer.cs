using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

public class ImageData {
	public int width;
	public int height;
	public float[] data;
}

public class TextRenderer {
	private Font _font;

	public TextRenderer() {
		_font = Font.CreateDynamicFontFromOSFont("Arial", 32);
	}

	public ImageData GetTextImage(string Text, int Size, core Core) {
		Core.bakeTextMesh.fontSize = Size;
		Core.bakeTextMesh.text = Text;
		Bounds textBounds = Core.bakeTextMesh.GetComponent<MeshRenderer>().bounds;
		Core.bakeTextCamera.transform.localPosition = new Vector3(textBounds.size.x / 2, 10, -textBounds.size.z / 2);
		Core.bakeTextCamera.orthographicSize = textBounds.size.z / 2.0f;

		float scale = 9.0f;
		int texWidth = (int)(textBounds.size.x * scale);
		int texHeight = (int)(textBounds.size.z * scale);

		RenderTexture rt = RenderTexture.GetTemporary(texWidth, texHeight, 16);
		Core.bakeTextCamera.targetTexture = rt;
		Core.bakeTextCamera.Render();

		RenderTexture.active = rt;

		Texture2D texTemp = new Texture2D(texWidth, texHeight);
		texTemp.ReadPixels(new Rect(0, 0, texWidth, texHeight), 0, 0, false);
		RenderTexture.active = null;
		RenderTexture.ReleaseTemporary(rt);

		Color32[] tempRaw = texTemp.GetPixels32();

		// Threshold.
		ImageData result = new ImageData();
		result.width = texWidth;
		result.height = texHeight;
		result.data = new float[texWidth * texHeight];

		for (int iY = 0; iY < texHeight; ++iY) {
			for (int iX = 0; iX < texWidth; ++iX) {
				int dstIndx = (texHeight - 1 - iY) * texWidth + iX;
				int srcIndx = iY * texWidth + iX;

				if (tempRaw[srcIndx].r > 64) {
					result.data[dstIndx] = 0;
				} else {
					result.data[dstIndx] = 1;
				}

				byte v = (byte)(result.data[dstIndx] * 255);
				tempRaw[srcIndx].r = v;
				tempRaw[srcIndx].g = v;
				tempRaw[srcIndx].b = v;
				tempRaw[srcIndx].a = 255;
			}
		}

		// texTemp.SetPixels32(tempRaw);
		// texTemp.Apply();		
		// File.WriteAllBytes("rt.png", texTemp.EncodeToPNG());
		GameObject.Destroy(texTemp);

		return result;
	}
}
