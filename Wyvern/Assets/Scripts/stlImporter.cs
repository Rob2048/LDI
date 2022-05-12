using System;
using System.IO;
using System.Text;
using System.Collections.Generic;
using UnityEngine;

public class StlImporter {

	public static void Save(string FilePath, MeshInfo Mesh) {
		using (BinaryWriter w = new BinaryWriter(File.Open(FilePath, FileMode.Create))) {
			byte[] header = new byte[80];
			w.Write(header);
			w.Write(Mesh.triangles.Length / 3);

			for (int i = 0; i < Mesh.triangles.Length / 3; ++i) {
				w.Write((float)0.0f);
				w.Write((float)1.0f);
				w.Write((float)0.0f);

				Vector3 v0 = Mesh.vertPositions[Mesh.triangles[i * 3 + 0]];
				Vector3 v1 = Mesh.vertPositions[Mesh.triangles[i * 3 + 1]];
				Vector3 v2 = Mesh.vertPositions[Mesh.triangles[i * 3 + 2]];

				w.Write(v0.x);
				w.Write(v0.y);
				w.Write(v0.z);

				w.Write(v1.x);
				w.Write(v1.y);
				w.Write(v1.z);

				w.Write(v2.x);
				w.Write(v2.y);
				w.Write(v2.z);

				w.Write((UInt16)0);
			}
		}
	}
}