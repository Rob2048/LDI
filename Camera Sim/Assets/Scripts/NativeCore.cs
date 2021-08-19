using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using UnityEngine;
using System.Net;
using System.Net.Sockets;

using Debug = UnityEngine.Debug;

public class Blob {
	public Vector2 pos;
	public float size;
	public Color color;
	public string text;
};

public class CharucoMarker {
	public int id;
	public Vector2 pos;

	public CharucoMarker(int Id, Vector2 Pos) {
		id = Id;
		pos = Pos;
	}
};

public class CharucoBoard {
	public int id;
	public List<CharucoMarker> markers = new List<CharucoMarker>();
};

public class PoseEstimation {
	public Vector3 pos;
	public Matrix4x4 rotMat;
	public Matrix4x4 worldMat;

	// public PoseEstimation(Vector3 Pos, Matrix4x4 Rot) {
	// 	pos = Pos;
	// 	rotMat = Rot;
	// }
};

public class CalibratedCamera {
	public double[] mat = new double[9];
	public double[] distCoeffs = new double[8];	
	public Matrix4x4 r = Matrix4x4.identity;
	public Vector3 t = new Vector3();

	public Matrix4x4 k = Matrix4x4.identity;
	public Matrix4x4 rt = Matrix4x4.identity;
	public Matrix4x4 p = Matrix4x4.identity;

	public void CalculateMats() {
		k = Matrix4x4.identity;
		k.m00 = (float)mat[0]; k.m01 = (float)mat[1]; k.m02 = (float)mat[2];
		k.m10 = (float)mat[3]; k.m11 = (float)mat[4]; k.m12 = (float)mat[5];
		k.m20 = (float)mat[6]; k.m21 = (float)mat[7]; k.m22 = (float)mat[8];

		rt = Matrix4x4.TRS(t, r.rotation, Vector3.one);

		p = k * rt;
	}

	public Vector2 ProjectPoint(Vector3 Point) {
		// NOTE: Ignoring distortion.
		Vector2 result = new Vector2();

		Vector4 pos = k * (r * Point + new Vector4(t.x, t.y, t.z, 0));
		result.x = pos.x / pos.z;
		result.y = pos.y / pos.z;

		return result;
	}
}

public class NativeCore {

	private Process _process;
	private TcpClient _tcpClient;
	private BinaryWriter _bw;
	private BinaryReader _br;

	public void Init() {
		Debug.Log("Starting native core process...");

		ProcessStartInfo startInfo = new ProcessStartInfo("C:\\Projects\\LDI\\NativeCore\\x64\\Release\\NativeCore.exe");
		startInfo.RedirectStandardOutput = true;
		startInfo.RedirectStandardError = true;
		startInfo.UseShellExecute = false;

		_process = new Process();

		_process.StartInfo = startInfo;
		_process.OutputDataReceived += (Sender, Args) => {
			Debug.Log("NC: " + Args.Data);
		};

		_process.ErrorDataReceived += (Sender, Args) => {
			Debug.LogError("NC: " + Args.Data);
		};

		_process.Start();
		_process.BeginOutputReadLine();
		_process.BeginErrorReadLine();

		_StartConnection();
	}

	public void Destroy() {
		if (_tcpClient != null) {
			_tcpClient.Close();
		}

		if (_process != null) {
			_process.Kill();
		}
	}

	private void _StartConnection() {
		Debug.Log("Connecting TCP client to native core");

		_tcpClient = new TcpClient("localhost", 20000);

		_bw = new BinaryWriter(_tcpClient.GetStream());
		_br = new BinaryReader(_tcpClient.GetStream());
	}

	public List<CharucoBoard> FindCharuco(int ImageWidth, int ImageHeight, byte[] ImageData, int Id) {
		List<CharucoBoard> result = new List<CharucoBoard>();

		_bw.Write((int)(1 + 4 + 4 + 4 + ImageWidth * ImageHeight));
		_bw.Write((byte)2);
		_bw.Write((int)ImageWidth);
		_bw.Write((int)ImageHeight);
		_bw.Write((int)Id);
		_bw.Write(ImageData, 0, ImageWidth * ImageHeight);

		int boardCount = _br.ReadInt32();
		
		for (int i = 0; i < boardCount; ++i) {
			int markerCount = _br.ReadInt32();
			
			if (markerCount > 0) {
				Debug.Log("Board: " + i);
				CharucoBoard board = new CharucoBoard();
				board.id = i;

				result.Add(board);
				for (int j = 0; j < markerCount; ++j) {
					int id = _br.ReadInt32();
					float x = _br.ReadSingle();
					float y = 1080.0f - _br.ReadSingle();
					board.markers.Add(new CharucoMarker(id, new Vector2(x, y)));
				}
			}
		}

		return result;
	}

	public List<Vector2> FindScanline(int ImageWidth, int ImageHeight, byte[] ImageData, int Id, List<Vector3> BoundryPoints, CalibratedCamera Cam, Matrix4x4 PoseMat) {
		_bw.Write((int)(1 + 4 + 4 + 4 + ImageWidth * ImageHeight + 4 + BoundryPoints.Count * 12 + 17 * 8 + 12 * 8));
		_bw.Write((byte)7);

		for (int i = 0 ; i < 9; ++i) {
			_bw.Write(Cam.mat[i]);
		}

		for (int i = 0 ; i < 8; ++i) {
			_bw.Write(Cam.distCoeffs[i]);
		}

		// TVec.
		_bw.Write((double)PoseMat.m03);
		_bw.Write((double)PoseMat.m13);
		_bw.Write((double)PoseMat.m23);

		// RVec.
		_bw.Write((double)PoseMat.m00);
		_bw.Write((double)PoseMat.m01);
		_bw.Write((double)PoseMat.m02);
		_bw.Write((double)PoseMat.m10);
		_bw.Write((double)PoseMat.m11);
		_bw.Write((double)PoseMat.m12);
		_bw.Write((double)PoseMat.m20);
		_bw.Write((double)PoseMat.m21);
		_bw.Write((double)PoseMat.m22);

		// Image.
		_bw.Write((int)ImageWidth);
		_bw.Write((int)ImageHeight);
		_bw.Write((int)Id);
		_bw.Write(ImageData, 0, ImageWidth * ImageHeight);
		_bw.Write((int)BoundryPoints.Count);

		// Boundry.
		for (int i = 0; i < BoundryPoints.Count; ++i) {
			_bw.Write(BoundryPoints[i].x);
			_bw.Write(BoundryPoints[i].y);
			_bw.Write(BoundryPoints[i].z);
		}

		int scanPointCount = _br.ReadInt32();
		
		List<Vector2> result = new List<Vector2>();

		for (int i = 0 ; i < scanPointCount; ++i) {
			Vector2 scanPoint;

			scanPoint.x = _br.ReadSingle();
			scanPoint.y = _br.ReadSingle();

			result.Add(scanPoint);
		}

		return result;
	}

	public CalibratedCamera CalibrateCameraCharuco(int ImageWidth, int ImageHeight, List<CharucoBoard> Boards) {
		int totalPacketSize = 1 + 4;

		for (int i = 0; i < Boards.Count; ++i) {
			totalPacketSize += 4 + Boards[i].markers.Count * 12;
		}

		_bw.Write((int)totalPacketSize);
		_bw.Write((byte)3);
		_bw.Write((int)Boards.Count);

		for (int i = 0; i < Boards.Count; ++i) {
			_bw.Write((int)Boards[i].markers.Count);

			for (int j = 0; j < Boards[i].markers.Count; ++j) {
				_bw.Write((int)Boards[i].markers[j].id);
				_bw.Write(Boards[i].markers[j].pos.x);
				// NOTE: Invert Y here to return to original OpenCV value.
				_bw.Write(1080.0f - Boards[i].markers[j].pos.y);
			}
		}

		double[] camMat = new double[9];
		double[] distMat = new double[8];

		double calibrationError = _br.ReadDouble();

		for (int i = 0; i < 9; ++i) {
			camMat[i] = _br.ReadDouble();
			Debug.Log("Cam " + i + ": " + camMat[i]);
		}

		for (int i = 0; i < 8; ++i) {
			distMat[i] = _br.ReadDouble();
			Debug.Log("Dist " + i + ": " + distMat[i]);
		}

		Debug.Log("Calibration error: " + calibrationError);

		int boardCount = _br.ReadInt32();

		List<PoseEstimation> poses = new List<PoseEstimation>();
		
		for (int i = 0; i < boardCount; ++i) {
			Vector3 tVec;
			tVec.x = (float)_br.ReadDouble();
			tVec.y = (float)_br.ReadDouble();
			tVec.z = (float)_br.ReadDouble();

			Matrix4x4 rMat = Matrix4x4.identity;

			rMat.m00 = (float)_br.ReadDouble();
			rMat.m01 = (float)_br.ReadDouble();
			rMat.m02 = (float)_br.ReadDouble();

			rMat.m10 = (float)_br.ReadDouble();
			rMat.m11 = (float)_br.ReadDouble();
			rMat.m12 = (float)_br.ReadDouble();

			rMat.m20 = (float)_br.ReadDouble();
			rMat.m21 = (float)_br.ReadDouble();
			rMat.m22 = (float)_br.ReadDouble();

			PoseEstimation pe = new PoseEstimation();
			pe.worldMat = Matrix4x4.TRS(tVec, rMat.rotation, Vector3.one);
			poses.Add(pe);
		}

		CalibratedCamera cam = new CalibratedCamera();
		cam.mat = camMat;
		cam.distCoeffs = distMat;
		
		return cam;
	}

	public bool GetCharucoPose(CalibratedCamera Cam, CharucoBoard Board, ref PoseEstimation Pose) {
		_bw.Write((int)(1 + 4 + 12 * Board.markers.Count + 8 * 17));
		_bw.Write((byte)4);

		for (int i = 0 ; i < 9; ++i) {
			_bw.Write(Cam.mat[i]);
		}

		for (int i = 0 ; i < 8; ++i) {
			_bw.Write(Cam.distCoeffs[i]);
		}

		_bw.Write((int)Board.markers.Count);
		for (int i = 0; i < Board.markers.Count; ++i) {
			_bw.Write(Board.markers[i].id);
			_bw.Write(Board.markers[i].pos.x);
			// NOTE: Y is inverted here from original OpenCV value.
			_bw.Write(Board.markers[i].pos.y);
		}

		int poseFound = _br.ReadInt32();

		

		if (poseFound == 1) {
			// NOTE: Some sign flips to convert from OpenCV to Unity co-ords. Y-Axis is inverted.
			// NOTE: Not needed if image space marker Y is inverted.
			// Vector3 tVec;
			// tVec.x = (float)_br.ReadDouble();
			// tVec.y = -(float)_br.ReadDouble();
			// tVec.z = (float)_br.ReadDouble();

			// Matrix4x4 rMat = Matrix4x4.identity;

			// rMat.m00 = (float)_br.ReadDouble();
			// rMat.m01 = -(float)_br.ReadDouble();
			// rMat.m02 = (float)_br.ReadDouble();

			// rMat.m10 = -(float)_br.ReadDouble();
			// rMat.m11 = (float)_br.ReadDouble();
			// rMat.m12 = -(float)_br.ReadDouble();

			// rMat.m20 = (float)_br.ReadDouble();
			// rMat.m21 = -(float)_br.ReadDouble();
			// rMat.m22 = (float)_br.ReadDouble();

			Vector3 tVec;
			tVec.x = (float)_br.ReadDouble();
			tVec.y = (float)_br.ReadDouble();
			tVec.z = (float)_br.ReadDouble();

			Matrix4x4 rMat = Matrix4x4.identity;

			rMat.m00 = (float)_br.ReadDouble();
			rMat.m01 = (float)_br.ReadDouble();
			rMat.m02 = (float)_br.ReadDouble();

			rMat.m10 = (float)_br.ReadDouble();
			rMat.m11 = (float)_br.ReadDouble();
			rMat.m12 = (float)_br.ReadDouble();

			rMat.m20 = (float)_br.ReadDouble();
			rMat.m21 = (float)_br.ReadDouble();
			rMat.m22 = (float)_br.ReadDouble();

			Pose.pos = tVec;
			Pose.rotMat = rMat;

			return true;
		}

		return false;
	}

	public bool GetGeneralPose(CalibratedCamera Cam, List<Vector2> ImagePoints, List<Vector3> ObjectPoints, ref PoseEstimation Pose) {
		_bw.Write((int)(1 + 4 + 20 * ImagePoints.Count + 8 * 17));
		_bw.Write((byte)6);

		for (int i = 0 ; i < 9; ++i) {
			_bw.Write(Cam.mat[i]);
		}

		for (int i = 0 ; i < 8; ++i) {
			_bw.Write(Cam.distCoeffs[i]);
		}

		_bw.Write((int)ImagePoints.Count);

		for (int i = 0; i < ImagePoints.Count; ++i) {
			_bw.Write(ImagePoints[i].x);
			// NOTE: Y is inverted here from original OpenCV value.
			_bw.Write(ImagePoints[i].y);

			_bw.Write(ObjectPoints[i].x);
			_bw.Write(ObjectPoints[i].y);
			_bw.Write(ObjectPoints[i].z);
		}

		int solutions = _br.ReadInt32();

		Debug.Log("Solution count: " + solutions);

		for (int i = 0; i < solutions; ++i) {
			// NOTE: Some sign flips to convert from OpenCV to Unity co-ords. Y-Axis is inverted.
			// NOTE: Not needed if image space marker Y is inverted.
			// Vector3 tVec;
			// tVec.x = (float)_br.ReadDouble();
			// tVec.y = -(float)_br.ReadDouble();
			// tVec.z = (float)_br.ReadDouble();

			// Matrix4x4 rMat = Matrix4x4.identity;

			// rMat.m00 = (float)_br.ReadDouble();
			// rMat.m01 = -(float)_br.ReadDouble();
			// rMat.m02 = (float)_br.ReadDouble();

			// rMat.m10 = -(float)_br.ReadDouble();
			// rMat.m11 = (float)_br.ReadDouble();
			// rMat.m12 = -(float)_br.ReadDouble();

			// rMat.m20 = (float)_br.ReadDouble();
			// rMat.m21 = -(float)_br.ReadDouble();
			// rMat.m22 = (float)_br.ReadDouble();

			Vector3 tVec;
			tVec.x = (float)_br.ReadDouble();
			tVec.y = (float)_br.ReadDouble();
			tVec.z = (float)_br.ReadDouble();

			Matrix4x4 rMat = Matrix4x4.identity;

			rMat.m00 = (float)_br.ReadDouble();
			rMat.m01 = (float)_br.ReadDouble();
			rMat.m02 = (float)_br.ReadDouble();

			rMat.m10 = (float)_br.ReadDouble();
			rMat.m11 = (float)_br.ReadDouble();
			rMat.m12 = (float)_br.ReadDouble();

			rMat.m20 = (float)_br.ReadDouble();
			rMat.m21 = (float)_br.ReadDouble();
			rMat.m22 = (float)_br.ReadDouble();

			Pose.pos = tVec;
			Pose.rotMat = rMat;

			// Projections
			int projectedPoints = _br.ReadInt32();

			for (int j = 0; j < projectedPoints; ++j) {
				Vector2 p;

				p.x = _br.ReadSingle();
				p.y = _br.ReadSingle();

				// Debug.Log("Proj: " + i + " " + p.x + ", " + p.y);
			}

			float rms = _br.ReadSingle();

			Debug.Log("Error: " + rms);
		}

		if (solutions > 0) {
			return true;
		}

		return false;
	}

	public bool TriangulatePoints(Matrix4x4 CamProj0, Matrix4x4 CamProj1, List<Vector2> Points0, List<Vector2> Points1, ref List<Vector3> OutputPoints) {
		if (Points0.Count != Points1.Count) {
			Debug.LogError("Points lists must be same size");
			return false;			
		}

		_bw.Write((int)(1 + (16 * 8) + (16 * 8) + 4 + Points0.Count * 8 + Points1.Count * 8));
		_bw.Write((byte)5);

		for (int i = 0 ; i < 16; ++i) {
			_bw.Write((double)CamProj0[(i % 4) * 4 + (i / 4)]);
		}

		for (int i = 0 ; i < 16; ++i) {
			_bw.Write((double)CamProj1[(i % 4) * 4 + (i / 4)]);
		}

		_bw.Write((int)Points0.Count);

		for (int i = 0; i < Points0.Count; ++i) {
			_bw.Write(Points0[i].x);
			_bw.Write(Points0[i].y);
		}

		for (int i = 0; i < Points1.Count; ++i) {
			_bw.Write(Points1[i].x);
			_bw.Write(Points1[i].y);
		}

		int outputPoints = _br.ReadInt32();

		if (outputPoints != -1) {

			for (int i = 0; i < outputPoints; ++i) {
				Vector3 p;
				p.x = _br.ReadSingle();
				p.y = _br.ReadSingle();
				p.z = _br.ReadSingle();

				OutputPoints.Add(p);
			}

			return true;
		}

		return false;
	}

	public List<Blob> GenericAnalysis(int ImageWidth, int ImageHeight, byte[] ImageData) {
		Stopwatch sw = new Stopwatch();
		sw.Start();
		List<Blob> result = new List<Blob>();

		_bw.Write((int)(1 + 4 + 4 + ImageWidth * ImageHeight));
		_bw.Write((byte)1);
		_bw.Write((int)ImageWidth);
		_bw.Write((int)ImageHeight);
		_bw.Write(ImageData, 0, ImageWidth * ImageHeight);

		int pointCount = _br.ReadInt32();

		for (int i = 0; i < pointCount; ++i) {
			Blob blob = new Blob();

			blob.color = new Color(0.0f, 0.7f, 0.0f, 0.5f);
			blob.pos.x = _br.ReadSingle();
			blob.pos.y = _br.ReadSingle();
			blob.size = _br.ReadSingle();
			blob.text = i.ToString();

			// result.Add(blob);
		}

		int polygonCount = _br.ReadInt32();
		Color polygonColor = new Color(1.0f, 0, 0, 0.7f);

		for (int i = 0; i < polygonCount; ++i) {
			int polygonType = _br.ReadInt32();
			int polygonPointCount = _br.ReadInt32();

			if (polygonType == 0) {
				polygonColor = new Color(1.0f, 0, 0, 0.7f);

				for (int j = 0; j < polygonPointCount; ++j) {
					Blob blob = new Blob();

					blob.color = polygonColor;
					blob.pos.x = _br.ReadSingle();
					blob.pos.y = _br.ReadSingle();
					blob.size = 10;
					blob.text = "";

					//result.Add(blob);
				}

			} else {
				polygonColor = new Color(0.0f, 0.0f, 0.7f, 0.7f);
				int markerId = _br.ReadInt32();

				for (int j = 0; j < polygonPointCount; ++j) {
					Blob blob = new Blob();

					if (j == 0) {
						blob.color = new Color(1.0f, 0.0f, 1.0f, 0.7f);
					} else {
						blob.color = polygonColor;
					}
					blob.pos.x = _br.ReadSingle();
					blob.pos.y = _br.ReadSingle();
					blob.size = 10;
					blob.text = markerId.ToString();

					result.Add(blob);
				}
			}
		}

		sw.Stop();
		int t = sw.Elapsed.Milliseconds;

		Debug.Log("Find blobs - Time: " + t + " ms");

		return result;
	}

	public Ray FitLine(List<Vector3> Points) {
		_bw.Write((int)(1 + 4 + Points.Count * 12));
		_bw.Write((byte)8);
		_bw.Write((int)Points.Count);

		for (int i = 0; i < Points.Count; ++i) {
			Vector3 p = Points[i];
			_bw.Write(p.x);
			_bw.Write(p.y);
			_bw.Write(p.z);
		}

		Vector3 origin;
		origin.x = _br.ReadSingle();
		origin.y = _br.ReadSingle();
		origin.z = _br.ReadSingle();

		Vector3 dir;
		dir.x = _br.ReadSingle();
		dir.y = _br.ReadSingle();
		dir.z = _br.ReadSingle();

		dir.Normalize();

		return new Ray(origin, dir);
	}
}
