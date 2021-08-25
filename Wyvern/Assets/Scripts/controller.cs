using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Text;
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

	public void moveTo(BinaryWriter W, float X, float Y) {
		// 0 to 1 = 3968 to 128 (3840)
		float max = 128;
		float min = 3968;
		float diff = max - min;

		float x = X * diff + min;
		float y = Y * diff + min;

		writeCmdMove(W, (UInt16)x, (UInt16)y);
		writeCmdDelay(W, 300);
	}

	public void lineTo(BinaryWriter W, float X, float Y) {
	}

	public void Connect() {
		Debug.Log("Controller connecting.");
		_serial = new SerialPort("\\\\.\\COM7", 921600);

		try {
			_serial.Open();
			_serial.DtrEnable = true;
			_serial.DiscardInBuffer();
			Debug.Log("Controller connected.");

			BinaryWriter bw = new BinaryWriter(new MemoryStream());

			// NOTE: Make sure each command is 2byte aligned.

			// 128 - 3968 (3840)

			

			// for (int i = 0; i < 100; ++i) {
			// 	writeCmdMove(bw, (ushort)(128 + (i * 3840.0f / 100.0f)), 2048);
			// 	writeCmdDelay(bw, 100);
			// }

			// for (int i = 0; i < 100; ++i) {
			// 	writeCmdMove(bw, (ushort)(3968 - (i * 3840.0f / 100.0f)), 2048);
			// 	writeCmdDelay(bw, 100);
			// }

			moveTo(bw, 0, 0);
			writeCmdLaser(bw, 255);

			moveTo(bw, 0.25f, 0);
			moveTo(bw, 0.25f, 0.25f);
			moveTo(bw, 0f, 0.25f);
			moveTo(bw, 0, 0);

			writeCmdLaser(bw, 0);

			int segs = 16;
			float radius = 0.2f;

			for (int i = 0; i < segs + 1; ++i) {
				float rad = (Mathf.PI * 2) / segs * i;
				float x = (Mathf.Sin(rad) * 0.5f + 0.5f) * radius + 0.25f - radius * 0.5f;
				float y = (Mathf.Cos(rad) * 0.5f + 0.5f) * radius + 0.25f - radius * 0.5f;

				moveTo(bw, x, y);

				if (i == 0) {
					writeCmdLaser(bw, 255);
				}
			}

			writeCmdLaser(bw, 0);

			// Wait for end of frame
			writeCmdDelay(bw, 16000);
			
			writeCmdEnd(bw);

			BinaryWriter hw = new BinaryWriter(new MemoryStream());
			hw.Write((byte)0xAA);
			hw.Write((byte)0xAA);
			hw.Write((byte)0xAA);
			hw.Write((byte)0xAA);
			hw.Write((byte)1);
			hw.Write((int)bw.BaseStream.Position);

			Debug.Log("Galvo command size: " + ((float)bw.BaseStream.Position / 1024.0f) + " kB");

			if ((int)bw.BaseStream.Position > 1024 * 100) {
				Debug.LogError("Galvo commands too big.");
			} else {
				_serial.Write(((MemoryStream)hw.BaseStream).GetBuffer(), 0, (int)hw.BaseStream.Position);
				_serial.Write(((MemoryStream)bw.BaseStream).GetBuffer(), 0, (int)bw.BaseStream.Position);
			}

			// Ping();
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

			//Debug.Log("Controller: Command received " + _lastAxisPos[0] + " " + _lastAxisPos[1] + " " + _lastAxisPos[2]);
		} else {
			Debug.LogError("Invalid command: " + cmdId);
		}
	}

	public bool Update() {
		if (_serial != null && _serial.IsOpen) {
			_serial.ReadTimeout = 1;

			try {
				while (true) {
					byte b = (byte)_serial.ReadByte();

					Debug.Log("Serial recv: " + b);
				}
			} catch (TimeoutException Ex) {

			}

			return true;

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
