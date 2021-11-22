using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Text;
using System.Threading;
using UnityEngine;

public class TriangleRect {
	public Vector2 v0;
	public Vector2 v1;
	public Vector2 v2;
	public Rect rect;
	public int id;
}

public class PBCommand {
	public byte[] packet;
	
	public PBCommand(byte[] Packet) {
		packet = Packet;
	}
}

public class Controller {
	
	public enum PBAxis {
		X = 0,
		Y = 1,
		Z = 2,
		A = 3,
		B = 4
	};

	public enum PBCommandState {
		IDLE,
		SENDING_CMD
	};

	public enum PBRecvState {
		WAIT_FOR_START,
		WAIT_FOR_COUNT,
		WAIT_FOR_PAYLOAD,
		WAIT_FOR_END
	};

	const byte FRAME_START = 255;
	const byte FRAME_END = 254;

	public bool running = true;

	public float[] _lastAxisPos = new float[3];

	private SerialPort _serial;

	private PBCommand _cmd;
	private PBCommandState _commandState = PBCommandState.IDLE;

	private byte[] _recvBuffer = new byte[255];
	private int _recvLen = 0;
	private int _recvPayloadSize = 0;
	private PBRecvState _recvState = PBRecvState.WAIT_FOR_START;
	
	public void Init() {
		Debug.Log("Controller startup.");

		Connect();
	}

	public bool IsExecuting() {
		return (_commandState != PBCommandState.IDLE);
	}

	public void writeCmdMove(BinaryWriter W, UInt16 X, UInt16 Y) {
		W.Write((UInt16)1);
		W.Write((UInt16)Y);
		W.Write((UInt16)X);
	}

	public void writeCmdDelay(BinaryWriter W, UInt16 DelayUs) {
		W.Write((UInt16)2);
		W.Write((UInt16)DelayUs);
	}

	public void writeCmdLaser(BinaryWriter W, UInt16 Pwm) {
		W.Write((UInt16)3);
		W.Write((UInt16)Pwm);
	}

	public void writeCmdEnd(BinaryWriter W) {
		W.Write((UInt16)0);
	}

	public void moveTo(BinaryWriter W, float X, float Y, UInt16 Delay = 300) {
		// 0 to 1 = 3968 to 128 (3840)
		float max = 128;
		float min = 3968;
		float diff = max - min;

		X = Mathf.Clamp01(X);
		Y = Mathf.Clamp01(Y);

		float x = X * diff + min;
		float y = Y * diff + min;

		writeCmdMove(W, (UInt16)x, (UInt16)y);

		if (Delay > 0) {
			writeCmdDelay(W, Delay);
		}
	}

	public void pulseLaser(BinaryWriter W, UInt16 Pwm, UInt16 OnTime) {
		writeCmdLaser(W, Pwm);
		writeCmdDelay(W, OnTime);
		writeCmdLaser(W, 0);
	}

	public void lineTo(BinaryWriter W, float X, float Y) {
	}

	private void _cubeRenderLoop() {
		System.Diagnostics.Stopwatch sw = new System.Diagnostics.Stopwatch();

		float time = 0.0f;

		float cubeHSize = 1.0f;

		Vector3[] points = {
			new Vector3(-cubeHSize, -cubeHSize, -cubeHSize),
			new Vector3(cubeHSize, -cubeHSize, -cubeHSize),
			new Vector3(cubeHSize, -cubeHSize, cubeHSize),
			new Vector3(-cubeHSize, -cubeHSize, cubeHSize),

			new Vector3(-cubeHSize, cubeHSize, -cubeHSize),
			new Vector3(cubeHSize, cubeHSize, -cubeHSize),
			new Vector3(cubeHSize, cubeHSize, cubeHSize),
			new Vector3(-cubeHSize, cubeHSize, cubeHSize),
		};

		List<int[]> segs = new List<int[]>();

		segs.Add(new[]{ 0, 1, 2, 3, 0 });
		segs.Add(new[]{ 4, 5, 6, 7, 4 });
		segs.Add(new[]{ 0, 4 });
		segs.Add(new[]{ 1, 5 });
		segs.Add(new[]{ 2, 6 });
		segs.Add(new[]{ 3, 7 });

		Vector3[] screenPoints = new Vector3[points.Length];

		Matrix4x4 viewMat = Matrix4x4.TRS(new Vector3(0, 0, 5), Quaternion.identity, Vector3.one);
		Matrix4x4 projMat = Matrix4x4.Perspective(65.0f, 1.0f, 0.1f, 100.0f);

		while (running) {
			sw.Restart();

			BinaryWriter bw = new BinaryWriter(new MemoryStream());

			float s = (Mathf.Sin(time * 10.0f) * 0.5f + 0.5f) * 0.5f + 0.5f;
			Matrix4x4 modelMat = Matrix4x4.TRS(new Vector3(0, 0, 0), Quaternion.Euler(time * 50.0f, time * 1000.0f, 0), new Vector3(s, s, s));

			// Transform all points to screen space.
			for (int i = 0; i < points.Length; ++i) {
				Vector4 p = new Vector4(points[i].x, points[i].y, points[i].z, 1.0f);

				p = projMat * viewMat * modelMat * p;

				p.x /= p.w;
				p.y /= p.w;
				p.z /= p.w;

				p.x = p.x * 0.5f + 0.5f;
				p.y = p.y * 0.5f + 0.5f;
				
				screenPoints[i] = p;
			}

			float y = Mathf.Sin(time) * 0.5f + 0.5f;

			for (int i = 0; i < segs.Count; ++i) {
				int[] segList = segs[i];

				for (int j = 0; j < segList.Length; ++j) {
					Vector3 p = screenPoints[segList[j]];

					moveTo(bw, p.x, p.y, 300);
					writeCmdLaser(bw, 255);
					writeCmdDelay(bw, 10);
				}

				writeCmdLaser(bw, 0);
			}

			writeCmdEnd(bw);

			BinaryWriter hw = new BinaryWriter(new MemoryStream());
			hw.Write((byte)0xAA);
			hw.Write((byte)0xAA);
			hw.Write((byte)0xAA);
			hw.Write((byte)0xAA);
			hw.Write((byte)1);
			hw.Write((int)bw.BaseStream.Position);

			// Debug.Log("Galvo command size: " + ((float)bw.BaseStream.Position / 1024.0f) + " kB");

			// Limit frame to 100kB.
			if ((int)bw.BaseStream.Position > 1024 * 100) {
				Debug.LogError("Galvo commands too big.");
			} else {
				_serial.Write(((MemoryStream)hw.BaseStream).GetBuffer(), 0, (int)hw.BaseStream.Position);
				_serial.Write(((MemoryStream)bw.BaseStream).GetBuffer(), 0, (int)bw.BaseStream.Position);
			}

			sw.Stop();

			int sleepTime = 20 - (int)sw.Elapsed.TotalMilliseconds;
			Thread.Sleep(sleepTime);

			Debug.Log("Frame " + ((float)bw.BaseStream.Position / 1024.0f) + " kB " + sw.Elapsed.TotalMilliseconds + " ms");

			time += 0.016f;
		}
	}

	private void _rasterizeTestLoop() {
		System.Diagnostics.Stopwatch sw = new System.Diagnostics.Stopwatch();

		_serial.ReadTimeout = 1;

		int loops = 0;

		while (running) {
			sw.Restart();

			BinaryWriter bw = new BinaryWriter(new MemoryStream());

			//------------------------------------------------------------------------------------------------------------------------------------------
			// Commands.
			//------------------------------------------------------------------------------------------------------------------------------------------
			// 5cm = 1000 x 50um steps
			// 2cm = 400 steps

			float y = (loops % 400) * 0.001f;
			for (int i = 0; i < 400; ++i) {
				moveTo(bw, 0.0f + i * 0.001f, y, 50);
				writeCmdLaser(bw, 255);
				writeCmdDelay(bw, 250);
				writeCmdLaser(bw, 0);
			}

			++loops;

			y = (loops % 400) * 0.001f;
			for (int i = 399; i >= 0; --i) {
				moveTo(bw, 0.0f + i * 0.001f, y, 50);
				writeCmdLaser(bw, 255);
				writeCmdDelay(bw, 250);
				writeCmdLaser(bw, 0);
			}

			++loops;

			writeCmdLaser(bw, 0);

			//------------------------------------------------------------------------------------------------------------------------------------------
			// Packet header and sending.
			//------------------------------------------------------------------------------------------------------------------------------------------
			writeCmdEnd(bw);
			
			BinaryWriter hw = new BinaryWriter(new MemoryStream());
			hw.Write((byte)0xAA);
			hw.Write((byte)0xAA);
			hw.Write((byte)0xAA);
			hw.Write((byte)0xAA);
			hw.Write((byte)1);
			hw.Write((int)bw.BaseStream.Position);

			// Limit frame to 100kB.
			if ((int)bw.BaseStream.Position > 1024 * 100) {
				Debug.LogError("Galvo commands too big.");
			} else {
				_serial.Write(((MemoryStream)hw.BaseStream).GetBuffer(), 0, (int)hw.BaseStream.Position);
				_serial.Write(((MemoryStream)bw.BaseStream).GetBuffer(), 0, (int)bw.BaseStream.Position);
			}

			double t1 = sw.Elapsed.TotalMilliseconds;

			// Wait for frame complete response.
			while (true) {
				if (_serial.BytesToRead >= 5) {
					BinaryReader sr = new BinaryReader(_serial.BaseStream);
					sr.ReadBytes(5);
					break;
				}
			}

			sw.Stop();

			// int sleepTime = 1000 - (int)sw.Elapsed.TotalMilliseconds;
			Thread.Sleep(4);

			Debug.Log("Frame " + ((float)bw.BaseStream.Position / 1024.0f) + " kB " + sw.Elapsed.TotalMilliseconds + " ms " + t1);
		}
	}

	private void _lineTimeTestLoop() {
		System.Diagnostics.Stopwatch sw = new System.Diagnostics.Stopwatch();

		_serial.ReadTimeout = 1;

		while (running) {
			sw.Restart();

			BinaryWriter bw = new BinaryWriter(new MemoryStream());

			//------------------------------------------------------------------------------------------------------------------------------------------
			// Commands.
			//------------------------------------------------------------------------------------------------------------------------------------------
			// 5cm = 1000 x 50um spots
			// 2cm = 400 steps

			moveTo(bw, 0, 0, 1000);
			pulseLaser(bw, 64, 100);
			
			moveTo(bw, 1.0f, 0, 0);
			pulseLaser(bw, 256, 100);
			
			writeCmdDelay(bw, 1000);
			pulseLaser(bw, 64, 100);

			//------------------------------------------------------------------------------------------------------------------------------------------
			// Packet header and sending.
			//------------------------------------------------------------------------------------------------------------------------------------------
			writeCmdEnd(bw);
			
			BinaryWriter hw = new BinaryWriter(new MemoryStream());
			hw.Write((byte)0xAA);
			hw.Write((byte)0xAA);
			hw.Write((byte)0xAA);
			hw.Write((byte)0xAA);
			hw.Write((byte)1);
			hw.Write((int)bw.BaseStream.Position);

			// Limit frame to 100kB.
			if ((int)bw.BaseStream.Position > 1024 * 100) {
				Debug.LogError("Galvo commands too big.");
			} else {
				_serial.Write(((MemoryStream)hw.BaseStream).GetBuffer(), 0, (int)hw.BaseStream.Position);
				_serial.Write(((MemoryStream)bw.BaseStream).GetBuffer(), 0, (int)bw.BaseStream.Position);
			}

			double t1 = sw.Elapsed.TotalMilliseconds;

			// Wait for frame complete response.
			while (true) {
				if (_serial.BytesToRead >= 5) {
					BinaryReader sr = new BinaryReader(_serial.BaseStream);
					sr.ReadBytes(5);
					break;
				}
			}

			sw.Stop();

			int sleepTime = 16 - (int)sw.Elapsed.TotalMilliseconds;
			if (sleepTime > 0) {
				Thread.Sleep(sleepTime);
			}

			Debug.Log("Frame " + ((float)bw.BaseStream.Position / 1024.0f) + " kB " + sw.Elapsed.TotalMilliseconds + " ms " + t1);
		}
	}

	public void Connect() {
		Debug.Log("Controller connecting.");
		_serial = new SerialPort("\\\\.\\COM7", 921600);

		try {
			_serial.Open();
			_serial.DtrEnable = true;
			_serial.DiscardInBuffer();
			Debug.Log("Controller connected.");

			// _cubeRenderLoop(); // NOTE: Doesn't check for response yet!
			// _rasterizeTestLoop();
			// _lineTimeTestLoop();

			Ping();
		} catch (Exception Ex) {
			Debug.LogError("Controller serial connect failed: " + Ex.Message);
		}
	}

	private void _ProcessCommand(byte[] Buffer, int Len) {
		int cmdId = Buffer[0];

		if (Len == 0) {
			Debug.LogError("Zero payload size");
			return;
		}

		if (cmdId == 13) {
			string message = Encoding.UTF8.GetString(Buffer, 1, Len - 1);
			Debug.Log("Controller message: " + message);
		} else if (cmdId == 10) {
			//_currentCmd = null;
			_commandState = PBCommandState.IDLE;
			BinaryReader br = new BinaryReader(new MemoryStream(Buffer));
			br.BaseStream.Seek(1, SeekOrigin.Current);
			_lastAxisPos[0] = br.ReadSingle();
			_lastAxisPos[1] = br.ReadSingle();
			_lastAxisPos[2] = br.ReadSingle();

			// Debug.Log("Controller: Command received " + _lastAxisPos[0] + " " + _lastAxisPos[1] + " " + _lastAxisPos[2]);
		} else {
			Debug.LogError("Invalid command: " + cmdId);
		}
	}

	public bool Update() {
		if (_serial != null && _serial.IsOpen) {
			_serial.ReadTimeout = 1;

			// try {
			// 	while (true) {
			// 		byte b = (byte)_serial.ReadByte();

			// 		Debug.Log("Serial recv: " + b);
			// 	}
			// } catch (TimeoutException Ex) {

			// }

			// return true;

			try {
				while (true) {
					byte b = (byte)_serial.ReadByte();
					
					if (_recvState == PBRecvState.WAIT_FOR_START) {
						if (b == FRAME_START) {
							_recvState = PBRecvState.WAIT_FOR_COUNT;
							_recvLen = 0;
							_recvPayloadSize = 0;
						} else {
							Debug.LogError("Expected start byte.");
						}
					} else if (_recvState == PBRecvState.WAIT_FOR_COUNT) {
						_recvPayloadSize = b;						
						_recvState = PBRecvState.WAIT_FOR_PAYLOAD;
					} else if (_recvState == PBRecvState.WAIT_FOR_PAYLOAD) {
						_recvBuffer[_recvLen++] = b;
						
						if (_recvLen == _recvPayloadSize) {
							_recvState = PBRecvState.WAIT_FOR_END;
						}
					} else if (_recvState == PBRecvState.WAIT_FOR_END) {
						if (b == FRAME_END) {
							_ProcessCommand(_recvBuffer, _recvPayloadSize);
						} else {
							Debug.LogError("Recv packet failure.");
						}

						_recvState = PBRecvState.WAIT_FOR_START;
					}
				}
			} catch (TimeoutException Ex) {

			}

			// //if (_currentCmd != null && _commandState == PBCommandState.IDLE) {
			// if (_cmdQueue.Count > 0 && _commandState == PBCommandState.IDLE) {
			// 	_commandState = PBCommandState.SENDING_CMD;

			// 	PBCommand nextCmd = _cmdQueue.Peek();

			// 	_serial.Write(nextCmd.packet, 0, nextCmd.packet.Length);

			// 	// string t = "Send cmd [" + _currentCmd.Length + "]: ";
			// 	// for (int i = 0; i < _currentCmd.Length; ++i) {
			// 	// 	t += _currentCmd[i] + " ";
			// 	// }
			// 	// Debug.Log(t);
			// }

			return (_commandState == PBCommandState.IDLE);
		}

		return false;
	}

	private bool _SendCommand(byte[] Data) {
		return _SendCommand(Data, Data.Length);
	}

	private bool _SendCommand(byte[] Data, int Size) {
		if (_commandState == PBCommandState.SENDING_CMD) {
			Debug.LogError("Already sending command.");
			return false;
		}

		_commandState = PBCommandState.SENDING_CMD;
		_serial.Write(Data, 0, Size);
		//Debug.Log("Writing data size: " + Size);

		return true;
	}

	public bool Ping() {
		byte[] pkt = { FRAME_START, 1, 0, 0, FRAME_END };
		return _SendCommand(pkt);
	}

	public bool Zero() {
		byte[] pkt = { FRAME_START, 1, 0, 10, FRAME_END };
		return _SendCommand(pkt);
	}

	public bool GotoZero() {
		byte[] pkt = { FRAME_START, 1, 0, 11, FRAME_END };
		return _SendCommand(pkt);
	}

	public bool Home() {
		byte[] pkt = { FRAME_START, 2, 0, 0, 3, FRAME_END };
		return _SendCommand(pkt);
	}

	public bool SimpleMoveRelative(int Axis, float Position, int Speed) {
		byte[] pkt = new byte[14];

		BinaryWriter w = new BinaryWriter(new MemoryStream(pkt));

		w.Write(FRAME_START);
		w.Write((Int16)10); // Payload size
		w.Write((byte)3); // Cmd ID
		w.Write((byte)Axis); // Axis id
		w.Write(Position); // Relative position
		w.Write(Speed); // Speed
		w.Write(FRAME_END);

		return _SendCommand(pkt);
	}

	public bool SimpleMove(int Axis, float Position, int Speed) {
		byte[] pkt = new byte[14];

		BinaryWriter w = new BinaryWriter(new MemoryStream(pkt));

		w.Write(FRAME_START);
		w.Write((Int16)10); // Payload size
		w.Write((byte)4); // Cmd ID
		w.Write((byte)Axis); // Axis id
		w.Write(Position); // Position
		w.Write(Speed); // Speed
		w.Write(FRAME_END);

		return _SendCommand(pkt);
	}

	public bool BurstLaser(int Time) {
		byte[] pkt = new byte[9];

		BinaryWriter w = new BinaryWriter(new MemoryStream(pkt));

		w.Write(FRAME_START);
		w.Write((Int16)5); // Payload size
		w.Write((byte)5); // Cmd ID
		w.Write(Time);
		w.Write(FRAME_END);

		return _SendCommand(pkt);
	}

	public bool StartLaser(int Counts, int OnTime, int OffTime) {
		byte[] pkt = new byte[17];

		BinaryWriter w = new BinaryWriter(new MemoryStream(pkt));

		w.Write(FRAME_START);
		w.Write((Int16)13); // Payload size
		w.Write((byte)6); // Cmd ID
		w.Write(OnTime);
		w.Write(OffTime);
		w.Write(Counts);
		w.Write(FRAME_END);

		return _SendCommand(pkt);
	}

	public bool ModulateLaser(int Frequency, int Duty) {
		byte[] pkt = new byte[17];

		BinaryWriter w = new BinaryWriter(new MemoryStream(pkt));

		w.Write(FRAME_START);
		w.Write((Int16)9); // Payload size
		w.Write((byte)13); // Cmd ID
		w.Write(Frequency);
		w.Write(Duty);
		w.Write(FRAME_END);

		return _SendCommand(pkt);
	}

	public bool StopLaser() {
		byte[] pkt = new byte[17];

		BinaryWriter w = new BinaryWriter(new MemoryStream(pkt));

		w.Write(FRAME_START);
		w.Write((Int16)1); // Payload size
		w.Write((byte)7); // Cmd ID
		w.Write(FRAME_END);

		return _SendCommand(pkt);
	}

	public bool RasterLine(int OnTime, int OffTime, int StepTime, float[] Stops) {
		byte[] pkt = new byte[4 + 1 + 16 + Stops.Length * 4];

		BinaryWriter w = new BinaryWriter(new MemoryStream(pkt));

		w.Write(FRAME_START);
		w.Write((Int16)(1 + 16 + Stops.Length * 4)); // Payload size
		w.Write((byte)8); // Cmd ID
		w.Write((Int32)Stops.Length);
		w.Write((Int32)OnTime);
		w.Write((Int32)OffTime);
		w.Write((Int32)StepTime);

		for (int i = 0; i < Stops.Length; ++i) {
			w.Write(Stops[i]);
		}

		w.Write(FRAME_END);

		return _SendCommand(pkt);
	}

	public bool IncrementalLine(int Iterations, int Steps, int StartTime, int TimeIncrement, int StepTime, int AfterMoveDelayTime, float SpacingMm) {
		byte[] pkt = new byte[4 + 1 + 7 * 4];

		BinaryWriter w = new BinaryWriter(new MemoryStream(pkt));

		w.Write(FRAME_START);
		w.Write((Int16)(1 + 7 * 4)); // Payload size
		w.Write((byte)14); // Cmd ID
		w.Write((Int32)Iterations);
		w.Write((Int32)Steps);
		w.Write((Int32)StartTime);
		w.Write((Int32)TimeIncrement);
		w.Write((Int32)StepTime);
		w.Write((Int32)AfterMoveDelayTime);
		w.Write((float)SpacingMm);

		w.Write(FRAME_END);

		return _SendCommand(pkt);
	}

	public bool ScanLine(float Speed, float Distance, int LaserOnTime, int[] Pulses) {
		BinaryWriter w = new BinaryWriter(new MemoryStream(4096));

		w.Write(FRAME_START);
		w.Write((Int16)(1)); // Payload size
		w.Write((byte)12); // Cmd ID
				
		w.Write(Speed);
		w.Write(Distance);
		w.Write((Int32)LaserOnTime);
		w.Write((Int32)Pulses.Length);
		for (int i = 0; i < Pulses.Length; ++i) {
			w.Write(Pulses[i]);
		}

		w.Write(FRAME_END);

		// Payload size.
		w.Seek(1, SeekOrigin.Begin);
		w.Write((Int16)(w.BaseStream.Length - 4));

		//Debug.Log("Writing data size: " + (w.BaseStream.Length - 4));
		//Debug.Log("Writing data size: " + ((MemoryStream)w.BaseStream).GetBuffer().Length);

		return _SendCommand(((MemoryStream)w.BaseStream).GetBuffer(), (int)w.BaseStream.Length);
	}
}
