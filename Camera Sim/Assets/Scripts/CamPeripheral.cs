using System.Collections;
using System.Collections.Generic;
using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using UnityEngine;

public static class TransformExtensions
{
	public static void FromMatrix(this Transform transform, Matrix4x4 matrix)
	{
		transform.localScale = matrix.ExtractScale();
		transform.rotation = matrix.ExtractRotation();
		transform.position = matrix.ExtractPosition();
	}
}

public static class MatrixExtensions
{
	public static Quaternion ExtractRotation(this Matrix4x4 matrix)
	{
		Vector3 forward;
		forward.x = matrix.m02;
		forward.y = matrix.m12;
		forward.z = matrix.m22;
 
		Vector3 upwards;
		upwards.x = matrix.m01;
		upwards.y = matrix.m11;
		upwards.z = matrix.m21;
 
		return Quaternion.LookRotation(forward, upwards);
	}
 
	public static Vector3 ExtractPosition(this Matrix4x4 matrix)
	{
		Vector3 position;
		position.x = matrix.m03;
		position.y = matrix.m13;
		position.z = matrix.m23;
		return position;
	}
 
	public static Vector3 ExtractScale(this Matrix4x4 matrix)
	{
		Vector3 scale;
		scale.x = new Vector4(matrix.m00, matrix.m10, matrix.m20, matrix.m30).magnitude;
		scale.y = new Vector4(matrix.m01, matrix.m11, matrix.m21, matrix.m31).magnitude;
		scale.z = new Vector4(matrix.m02, matrix.m12, matrix.m22, matrix.m32).magnitude;
		return scale;
	}
}

public struct CalibPoint {
	public int id;
	public int boardId;
	public int fullId;
	public Vector3 pos;
	public string name;

	public CalibPoint(int Id, Vector3 Pos, int BoardId) {
		id = Id;
		pos = Pos;
		boardId = BoardId;
		fullId = boardId * 10 + id;
		name = "cube_" + boardId + "_" + id + "_" + fullId;
	}

	public CalibPoint(int FullId, Vector3 Pos) {
		fullId = FullId;
		pos = Pos;
		boardId = FullId / 10;
		id = FullId % 10;
		name = "cube_" + boardId + "_" + id + "_" + fullId;
	}
}

public class CamPeripheral {

	public int width;
	public int height;
	public Texture2D frameTexture;
	public CalibratedCamera calibration;

	public CamPeripheral(int Width, int Height) {
		frameTexture = new Texture2D(Width, Height, TextureFormat.RGB24, false);
		// _camFrameTex = new Texture2D(1920, 1080, TextureFormat.RGBA32, 1, true);
		// _camFrameTex.filterMode = FilterMode.Point;
		width = Width;
		height = Height;

		// NOTE: Original sim camera.
		// calibration = new CalibratedCamera();
		// calibration.mat[0] = 1437.433;
		// calibration.mat[1] = 0;
		// calibration.mat[2] = 960;
		// calibration.mat[3] = 0;
		// calibration.mat[4] = 1437.433;
		// calibration.mat[5] = 540;
		// calibration.mat[6] = 0;
		// calibration.mat[7] = 0;
		// calibration.mat[8] = 1;

/*
		fovX = 67.47453
		fovY = 52.14369 41.11209
		dimX = 3.74
		dimY = 2.74
		iX = 1920
		iY = 1080

		X 1,437.7047
		1,440.00002
		Y 1,103.64953

		x 37.13591
		y 21.4

		2,857.87

*/
		// NOTE: 1920x1080 at 37.13591 x 21.4
		calibration = new CalibratedCamera();
		calibration.mat[0] = 2857.87;
		calibration.mat[1] = 0;
		calibration.mat[2] = 960;
		calibration.mat[3] = 0;
		calibration.mat[4] = 2857.87;
		calibration.mat[5] = 540;
		calibration.mat[6] = 0;
		calibration.mat[7] = 0;
		calibration.mat[8] = 1;

		calibration.distCoeffs[0] = 0;
		calibration.distCoeffs[1] = 0;
		calibration.distCoeffs[2] = 0;
		calibration.distCoeffs[3] = 0;
		calibration.distCoeffs[4] = 0;
		calibration.distCoeffs[5] = 0;
		calibration.distCoeffs[6] = 0;
		calibration.distCoeffs[7] = 0;
	}

	public virtual void Update(float DeltaTime) {}
	public virtual bool StoreLatestFrame() { return false; }
}

public class VirtualCam : CamPeripheral {

	public Camera cam;
	public RenderTexture camRt;
	
	private Texture2D _bufferTexture;

	public VirtualCam(Camera Cam, int Width, int Height) : base(Width, Height) {
		cam = Cam;
		_bufferTexture = new Texture2D(Width, Height, TextureFormat.RGB24, false);
		camRt = new RenderTexture(Width, Height, 24, UnityEngine.Experimental.Rendering.GraphicsFormat.R8G8B8A8_UNorm);
		cam.targetTexture = camRt;
	}

	private void _CaptureScreen() {
		cam.Render();

		RenderTexture.active = camRt;
		_bufferTexture.ReadPixels(new Rect(0, 0, 1920, 1080), 0, 0, false);
		_bufferTexture.Apply();
		RenderTexture.active = null;
		
		// Apply contrast etc changes when img recvd?
		// Apply contrast etc in shader for speed?
		// Final output is texture capable for viewing in UI.

		// TODO: Should do the Y flip here.

		Graphics.CopyTexture(_bufferTexture, frameTexture);
	}

	public override bool StoreLatestFrame() {
		_CaptureScreen();

		return true;
	}
}

public class PhysicalCam : CamPeripheral {
	private enum ERecvState	{
		RS_NONE,
		RS_HEADER,
		RS_PAYLOAD,
		RS_DONE,
		RS_ERROR
	};

	private Int32 _dataSize;
	private Int32 _readData;
	private byte[] _dataBuffer = new byte[1024 * 1024 * 16];
	private ERecvState _recvState = ERecvState.RS_HEADER;

	public int sentBytes = 0;
	public int recvBytes = 0;

	public byte[] _frameBuffer = new byte[1024 * 1024 * 16];
	private bool _newFrame = false;

	private Socket _socket;

	public PhysicalCam(Socket TcpSocket, int Width, int Height) : base(Width, Height) {
		_socket = TcpSocket;
	}

	public bool GetFrameData(ref byte[] Data) {
		// private Color32[] _frameColors = new Color32[1920 * 1080];
		// private Texture2D _camFrameTex;
		// private void _ApplyFrameData(byte[] Frame) {
		// 	for (int i = 0; i < 1920 * 1080; ++i) {
		// 		float value = (Frame[i] * BrightnessSlider.value + ContrastSlider.value);
		// 		byte f = (byte)(Mathf.Clamp(value, 0, 255));
		// 		_frameColors[i] = new Color32(f, f, f, 255);
		// 	}

		// 	_camFrameTex.SetPixels32(_frameColors);
		// 	_camFrameTex.Apply(false);
		// 	//Debug.Log("New frame");
		// }

		// Color32[] CaptureLastFrame() {
		// 	return _frameColors;
		// }

		if (_newFrame) {
			_newFrame = false;

			// Array.Copy(_frameBuffer, 0, Data, 0, 1920 * 1080);
			Data = _frameBuffer;
			return true;
		}

		Data = _frameBuffer;

		return false;
	}

	private void HandlePacket(byte[] Data, int Size) {
		if (Size < 1) {
			return;
		}

		int Opcode = Data[0];

		if (Opcode == 1) {
			// NOTE: New camera image.
			Array.Copy(Data, 1, _frameBuffer, 0, 1920 * 1080);
			_newFrame = true;
		}

		// Debug.Log("Op: " + Opcode);
	}

	public bool UpdateConnection(float DeltaTime) {
		try {
			if (!_socket.Connected) {
				Debug.LogError("Network Disconnection");
				return false;
			}

			while (_socket.Available != 0) {
				//Console.WriteLine("New Data: " + _socket.Available);
				//Debug.Log("New Data: " + _socket.Available);

				switch (_recvState)
				{
					case ERecvState.RS_HEADER: {
						_dataSize = 0;

						if (_socket.Available < 4)
							return true;

						int bytesRead = _socket.Receive(_dataBuffer, 4, SocketFlags.None);
						recvBytes += bytesRead;

						_dataSize = BitConverter.ToInt32(_dataBuffer, 0);
						_readData = 0;

						// Debug.Log("Got data size: " + _dataSize);

						if (_dataSize < _dataBuffer.Length - 64 && _dataSize > 0) {
							_recvState = ERecvState.RS_PAYLOAD;
						} else {
							Debug.LogError("Warning! Bad Packet Size: " + _dataSize);
							return false;
						}

						break;
					}

					case ERecvState.RS_PAYLOAD: {
						int bytesToRead = Math.Min(_socket.Available, (_dataSize - _readData));
						
						try {
							int bytesRead = _socket.Receive(_dataBuffer, _readData, bytesToRead, SocketFlags.None);
							_readData += bytesRead;
							recvBytes += bytesRead;

							if (_readData == _dataSize) {
								_recvState = ERecvState.RS_HEADER;
								// Debug.Log("Got packet: " + _dataSize);
								HandlePacket(_dataBuffer, _dataSize);
							}
						} catch (Exception e) {
							Debug.LogError(_readData + " " + bytesToRead + " " + _dataSize);
							throw(e);
						}

						break;
					}

					default:
						break;
				}
			}
		} catch (Exception e) {
			Debug.LogError("Updating Socket: " + e.Message);
		}

		return true;
	}
}

public class CameraServer
{
	private UdpClient _UdpClient;
	private Socket _TcpServer;

	public List<PhysicalCam> _cams = new List<PhysicalCam>();

	private float _BroadcastTimer = 0;

	public void Init() {
		_UdpClient = new UdpClient();
		//_UdpClient.Client.Bind(new IPEndPoint(IPAddress.Any, 45454));
		_UdpClient.EnableBroadcast = true;

		_StartServer(8000);
	}

	private bool _StartServer(int Port) {
		_TcpServer = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
		IPAddress hostIP = Dns.GetHostEntry(Dns.GetHostName()).AddressList[0];
		//IPAddress hostIP = Dns.GetHostEntry("127.0.0.1").AddressList[0];
		Debug.Log("Host IP: " + hostIP.ToString());
		IPEndPoint ep = new IPEndPoint(IPAddress.Any, Port);

		try {
			_TcpServer.Bind(ep);
			_TcpServer.Listen(10);
			_TcpServer.Blocking = false;
			return true;
		} catch (Exception e) {
			Debug.LogError("Trying to host: " + e.Message);
			_TcpServer = null;
			return false;
		}
	}

	private void _UpdateServer(float DeltaTime) {
		if (_TcpServer == null) {
			return;
		}

		try {
			Socket socket = _TcpServer.Accept();

			if (socket != null) {
				Debug.Log("Connection Accepted!");
				_cams.Add(new PhysicalCam(socket, 1920, 1080));
			}
		} catch (Exception e) {
		}

		for (int i = 0; i < _cams.Count; ++i) {
			_cams[i].UpdateConnection(DeltaTime);
		}
	}

	private void _BroadCastLocation() {
		// Debug.Log("Broadcasting...");
		byte[] data = Encoding.UTF8.GetBytes("Server:192.168.3.14");
		_UdpClient.Send(data, data.Length, "255.255.255.255", 45454);
	}

	public void Update(float DeltaTime) {
		_BroadcastTimer += DeltaTime;

		if (_BroadcastTimer >= 1.0f) {
			_BroadcastTimer = 0;
			_BroadCastLocation();
		}

		_UpdateServer(DeltaTime);
	}

	public bool GetLatestFrame(ref byte[] Data) {
		if (_cams.Count > 0) {
			return _cams[0].GetFrameData(ref Data);
		}

		return false;
	}
}
