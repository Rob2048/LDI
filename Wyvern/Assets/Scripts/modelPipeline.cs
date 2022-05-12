using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ModelPipeline {

	public MeshInfo startMesh;
	
	public ModelPipeline() {

	}
	
	public void Start(MeshInfo Mesh) {
		startMesh = Mesh;

		Figure importedPreview = new Figure();
		importedPreview.GenerateDisplayMesh(startMesh, core.singleton.BasicMaterial);

		StlImporter.Save("Content/remesh/source.stl", startMesh);

		// Voxelize mesh.
		// PolyMender-clean.exe source.stl 10 0.9 voxel.ply
		System.Diagnostics.Process process = new System.Diagnostics.Process();

		process.StartInfo.FileName = "C:\\Projects\\LDI\\Wyvern\\Content\\remesh\\PolyMender-clean.exe";
		process.StartInfo.Arguments = "source.stl 10 0.9 voxel.ply";
		process.StartInfo.WorkingDirectory = "C:\\Projects\\LDI\\Wyvern\\Content\\remesh\\";
		process.StartInfo.RedirectStandardOutput = true;
		process.StartInfo.RedirectStandardError = true;
		process.StartInfo.UseShellExecute = false;

		process.OutputDataReceived += (Sender, Args) => {
			Debug.Log("PolyMenderStdout: " + Args.Data);
		};

		process.ErrorDataReceived += (Sender, Args) => {
			Debug.LogError("PolyMenderStderr: " + Args.Data);
		};

		process.Start();
		process.BeginOutputReadLine();
		process.BeginErrorReadLine();
		process.WaitForExit();

		// Quadremesh mesh.
		//"Instant Meshes.exe" voxel.ply -o output.ply --scale 0.02
		process = new System.Diagnostics.Process();

		process.StartInfo.FileName = "C:\\Projects\\LDI\\Wyvern\\Content\\remesh\\Instant Meshes.exe";
		process.StartInfo.Arguments = "voxel.ply -o output.ply --scale 0.02";
		process.StartInfo.WorkingDirectory = "C:\\Projects\\LDI\\Wyvern\\Content\\remesh\\";
		process.StartInfo.RedirectStandardOutput = true;
		process.StartInfo.RedirectStandardError = true;
		process.StartInfo.UseShellExecute = false;

		process.OutputDataReceived += (Sender, Args) => {
			Debug.Log("InstantMeshes: " + Args.Data);
		};

		process.ErrorDataReceived += (Sender, Args) => {
			Debug.LogError("InstantMeshes: " + Args.Data);
		};

		process.Start();
		process.BeginOutputReadLine();
		process.BeginErrorReadLine();
		process.WaitForExit();
	}

	public void Update() {

	}

	public void Stop() {

	}
}
