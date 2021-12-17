using System;
using System.IO;
using System.Collections.Generic;
using UnityEngine;

//------------------------------------------------------------------------------------------------------------------------------------------------------
// Attribution notes:
// (drankin2112) https://stackoverflow.com/questions/11241535/faster-parsing-of-numbers-on-net
//------------------------------------------------------------------------------------------------------------------------------------------------------

public class ObjImporter {

	public static int ParseInt(byte[] Bytes, ref int Index, int Length) {
		int part = 0;
		bool neg = false;

		while (Index < Length && (Bytes[Index] > '9' || Bytes[Index] < '0') && Bytes[Index] != '-') {
			Index++;
		}

		// sign
		if (Bytes[Index] == '-') {
			neg = true;
			Index++;
		}

		// integer part
		while (Index < Length && !(Bytes[Index] > '9' || Bytes[Index] < '0')) {
			part = part * 10 + (Bytes[Index++] - '0');
		}

		return neg ? (part * -1) : part;
	}

	public static float ParseFloat(byte[] Bytes, ref int Index, int Length) {
		int part = 0;
		bool neg = false;

		float ret;

		// find start
		while (Index < Length && (Bytes[Index] < '0' || Bytes[Index] > '9') && Bytes[Index] != '-' && Bytes[Index] != '.') {
			Index++;
		}

		// sign
		if (Bytes[Index] == '-') {
			neg = true;
			Index++;
		}

		// integer part
		while (Index < Length && !(Bytes[Index] > '9' || Bytes[Index] < '0')) {
			part = part * 10 + (Bytes[Index++] - '0');
		}

		ret = neg ? (float)(part * -1) : (float)part;

		// float part
		if (Index < Length && Bytes[Index] == '.') {
			Index++;
			double mul = 1;
			part = 0;

			while (Index < Length && !(Bytes[Index] > '9' || Bytes[Index] < '0')) {
				part = part * 10 + (Bytes[Index] - '0');
				mul *= 10;
				Index++;
			}

			if (neg) {
				ret -= (float)part / (float)mul;
			} else {
				ret += (float)part / (float)mul;
			}
		}

		// scientific part
		if (Index < Length && (Bytes[Index] == 'e' || Bytes[Index] == 'E')) {
			Index++;
			neg = (Bytes[Index] == '-'); Index++;
			part = 0;

			while (Index < Length && !(Bytes[Index] > '9' || Bytes[Index] < '0')) {
				part = part * 10 + (Bytes[Index++] - '0');
			}

			if (neg) {
				ret /= (float)Math.Pow(10d, (double)part);
			} else {
				ret *= (float)Math.Pow(10d, (double)part);
			}
		}

		// NOTE: Solves -0.0f issue.
		return ret == 0.0f ? 0.0f : ret;
	}

	public static MeshInfo Load(string FilePath) {
		Debug.Log("Importing OBJ from: " + FilePath);

		List<Vector3> sharedVertPositions = new List<Vector3>();
		List<Vector2> sharedVertUvs = new List<Vector2>();
		List<int> indexBuffer = new List<int>();
		int faceCount = 0;
		int vertCount = 0;

		List<Vector3> objVertPositions = new List<Vector3>();
		List<Vector2> objVertUvs = new List<Vector2>();

		ProfileTimer t = new ProfileTimer();
		t.Start();

		byte[] fileTemp = File.ReadAllBytes(FilePath);
		byte[] rawBytes = new byte[fileTemp.Length + 1];
		Buffer.BlockCopy(fileTemp, 0, rawBytes, 0, fileTemp.Length);
		rawBytes[fileTemp.Length] = (byte)'\0';

		int[] vertPosId = new int[3];
		int[] vertUvId = new int[3];

		int idx = 0;
		byte c = rawBytes[idx++];

		while (c != '\0') {
			if (c == 'v') {
				c = rawBytes[idx++];

				if (c == ' ') {
					Vector3 attr;
					attr.x = ParseFloat(rawBytes, ref idx, rawBytes.Length); idx++;
					attr.y = ParseFloat(rawBytes, ref idx, rawBytes.Length); idx++;
					attr.z = ParseFloat(rawBytes, ref idx, rawBytes.Length); idx++;
					c = rawBytes[idx++];

					// NOTE: Invert X axis due to OBJ being a right-handed coordinate system.
					attr.x = -attr.x;

					objVertPositions.Add(attr);
				} else if (c == 't') {
					Vector2 attr;
					attr.x = ParseFloat(rawBytes, ref idx, rawBytes.Length); idx++;
					attr.y = ParseFloat(rawBytes, ref idx, rawBytes.Length); idx++;
					c = rawBytes[idx++];

					objVertUvs.Add(attr);
				}
			} else if (c == 'f') {
				c = rawBytes[idx++];

				// NOTE: Reverse order of vertices due to OBJ being a right-handed coordinate system.

				// Vert 2
				vertPosId[2] = ParseInt(rawBytes, ref idx, rawBytes.Length); idx++;
				vertUvId[2] = ParseInt(rawBytes, ref idx, rawBytes.Length); idx++;
				int normalId = ParseInt(rawBytes, ref idx, rawBytes.Length);

				// Vert 1
				vertPosId[1] = ParseInt(rawBytes, ref idx, rawBytes.Length); idx++;
				vertUvId[1] = ParseInt(rawBytes, ref idx, rawBytes.Length); idx++;
				normalId = ParseInt(rawBytes, ref idx, rawBytes.Length);

				// Vert 0
				vertPosId[0] = ParseInt(rawBytes, ref idx, rawBytes.Length); idx++;
				vertUvId[0] = ParseInt(rawBytes, ref idx, rawBytes.Length); idx++;
				normalId = ParseInt(rawBytes, ref idx, rawBytes.Length);

				sharedVertPositions.Add(objVertPositions[vertPosId[0] - 1]);
				sharedVertPositions.Add(objVertPositions[vertPosId[1] - 1]);
				sharedVertPositions.Add(objVertPositions[vertPosId[2] - 1]);

				sharedVertUvs.Add(objVertUvs[vertUvId[0] - 1]);
				sharedVertUvs.Add(objVertUvs[vertUvId[1] - 1]);
				sharedVertUvs.Add(objVertUvs[vertUvId[2] - 1]);

				indexBuffer.Add(vertCount++);
				indexBuffer.Add(vertCount++);
				indexBuffer.Add(vertCount++);

				++faceCount;

				c = rawBytes[idx++];
				
			} else {
				while (c != '\n' && c != '\0') {
					c = rawBytes[idx++];
				}

				if (c == '\n') {
					c = rawBytes[idx++];
				}
			}
		}
		
		double totalTime = t.Stop();

		Debug.Log("OBJ import (" + totalTime + " ms) loaded verts: " + objVertPositions.Count + " verts: " + vertCount + " faces: " + faceCount);
		
		MeshInfo result = new MeshInfo();
		result.vertPositions = sharedVertPositions.ToArray();
		result.vertUvs = sharedVertUvs.ToArray();
		result.triangles = indexBuffer.ToArray();

		return result;
	}
}