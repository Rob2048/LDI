using System;
using System.IO;
using System.Collections.Generic;
using UnityEngine;
using System.Diagnostics;

using Debug = UnityEngine.Debug;

public class ObjImporter {

	public static MeshInfo Load(string FilePath) {
		Debug.Log("Importing OBJ from: " + FilePath);

		List<Vector3> sharedVertPositions = new List<Vector3>();
		List<Vector2> sharedVertUvs = new List<Vector2>();
		List<int> indexBuffer = new List<int>();
		int faceCount = 0;
		int vertCount = 0;

		List<Vector3> objVertPositions = new List<Vector3>();
		List<Vector2> objVertUvs = new List<Vector2>();

		Stopwatch sw = new Stopwatch();
		sw.Start();

		string[] lines = File.ReadAllLines(FilePath);

		for (int i = 0; i < lines.Length; ++i) {
			string line = lines[i];

			if (line.StartsWith("v ")) {
				Vector3 vert;
				string[] attrs = line.Split(' ');
				vert.x = -float.Parse(attrs[1]);
				vert.y = float.Parse(attrs[2]);
				vert.z = float.Parse(attrs[3]);

				objVertPositions.Add(vert);

			} else if (line.StartsWith("vt ")) {
				Vector2 uv;
				string[] attrs = line.Split(' ');
				uv.x = float.Parse(attrs[1]);
				uv.y = float.Parse(attrs[2]);

				objVertUvs.Add(uv);

			} else if (line.StartsWith("f ")) {
				string[] attrs = line.Split(' ');

				// NOTE: Reverse order of vertices due to OBJ being a right-handed coordinate system. (Also negate vert X position).
				for (int j = 2; j >= 0; --j) {
					string[] ids = attrs[j + 1].Split('/');

					int vertPosId = int.Parse(ids[0]) - 1;
					int vertUvId = int.Parse(ids[1]) - 1;

					sharedVertPositions.Add(objVertPositions[vertPosId]);
					sharedVertUvs.Add(objVertUvs[vertUvId]);
					
					indexBuffer.Add(vertCount++);
				}

				++faceCount;
			}
		}
		
		sw.Stop();

		Debug.Log("Read lines: " + sw.Elapsed.TotalMilliseconds);
		Debug.Log("OBJ vert position count: " + objVertPositions.Count);
		Debug.Log("OBJ vert uv count: " + objVertUvs.Count);
		Debug.Log("Shared vert count: " + sharedVertPositions.Count);
		Debug.Log("Face count: " + faceCount);

		MeshInfo result = new MeshInfo();
		result.vertPositions = sharedVertPositions.ToArray();
		result.vertUvs = sharedVertUvs.ToArray();
		result.triangles = indexBuffer.ToArray();

		return result;
	}
}