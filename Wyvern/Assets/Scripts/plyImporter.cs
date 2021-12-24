using System;
using System.IO;
using System.Text;
using System.Collections.Generic;
using UnityEngine;

public class PlyImporter {


	public static string ReadAsciiLine(BinaryReader reader) {
		List<byte> strBytes = new List<byte>();

		while (reader.BaseStream.Position != reader.BaseStream.Length) {
			byte b = reader.ReadByte();

			if (b == '\n') {
				break;
			}

			strBytes.Add(b);
		}

		return Encoding.UTF8.GetString(strBytes.ToArray());
	}

	public static MeshInfo LoadQuadOnly(string FilePath) {
		Debug.Log("Importing PLY from: " + FilePath);

		MeshInfo result = new MeshInfo();
		
		using (BinaryReader r = new BinaryReader(File.Open(FilePath, FileMode.Open))) {

			ProfileTimer t = new ProfileTimer();
			t.Start();

			int faceCount = 0;
			int vertCount = 0;

			while (true) {
				string line = ReadAsciiLine(r);
				
				if (line == "end_header") {
					break;
				} else if (line.StartsWith("element vertex")) {
					string[] attrs = line.Split(' ');
					vertCount = int.Parse(attrs[2]);
				} else if (line.StartsWith("element face")) {
					string[] attrs = line.Split(' ');
					faceCount = int.Parse(attrs[2]);
				}
			}

			Vector3[] vertPositions = new Vector3[vertCount];

			for (int i = 0; i < vertCount; ++i) {
				float x = r.ReadSingle();
				float y = r.ReadSingle();
				float z = r.ReadSingle();
				vertPositions[i] = new Vector3(-x, z, -y);
			}

			// int[] indices = new int[faceCount * 4];
			int[] indices = new int[faceCount * 6];

			for (int i = 0; i < faceCount; ++i) {
				byte faceVertCount = r.ReadByte();

				if (faceVertCount != 4) {
					throw new Exception("PLY should only contain quads");
				}

				// indices[i * 4 + 0] = r.ReadInt32();
				// indices[i * 4 + 1] = r.ReadInt32();
				// indices[i * 4 + 2] = r.ReadInt32();
				// indices[i * 4 + 3] = r.ReadInt32();

				int v3 = r.ReadInt32();
				int v2 = r.ReadInt32();
				int v1 = r.ReadInt32();
				int v0 = r.ReadInt32();

				indices[i * 6 + 0] = v0;
				indices[i * 6 + 1] = v1;
				indices[i * 6 + 2] = v3;
				indices[i * 6 + 3] = v3;
				indices[i * 6 + 4] = v1;
				indices[i * 6 + 5] = v2;

				float nX = r.ReadSingle();
				float nY = r.ReadSingle();
				float nZ = r.ReadSingle();
			}

			double totalTime = t.Stop();

			Debug.Log("PLY import (" + totalTime + " ms) verts: " + vertCount + " quads: " + faceCount);
			
			result.vertPositions = vertPositions;
			result.triangles = indices;
		}

		return result;
	}
}