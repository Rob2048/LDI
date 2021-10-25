using System.Collections;
using System.Collections.Generic;
using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using UnityEngine;

public class Server {

	public const int FrameWidth = 1664;
	public const int FrameHeight = 1232;

	public System.Object frameDataLock = new System.Object();
	public volatile bool frameDataNew = false;
	public volatile Byte[] frameData = new Byte[FrameWidth * FrameHeight];

	private Socket _serverSocket;
	private Socket _clientSocket;
	private Thread _thread;

	private volatile bool _runThread = true;
	private Byte[] _tempFrameData = new Byte[FrameWidth * FrameHeight];

	private int HandleCmdFrame(Socket Client) {
		Byte[] data = new Byte[4];
		if (ReadData(Client, 4, data) != 0) return -1;
		int frameSize = (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);

		if (frameSize != FrameWidth * FrameHeight) {
			Debug.LogError("Frame size mismatch!");
			return - 1;
		}

		if (ReadData(Client, FrameWidth * FrameHeight, _tempFrameData) != 0) return -1;

		lock (frameDataLock) {
			Array.Copy(_tempFrameData, frameData, FrameWidth * FrameHeight);
			frameDataNew = true;
		}

		return 0;
	}

	private int ReadData(Socket Client, int Size, Byte[] Data) {
		int dataRecvd = 0;

		try {
			while (dataRecvd != Size) {
				int iResult = Client.Receive(Data, dataRecvd, Size - dataRecvd, SocketFlags.None);
				
				if (iResult > 0) {
					dataRecvd += iResult;
				} else if (iResult == 0) {
					Debug.Log("Client connection closing");
					return -1;
				}
			}
		} catch (Exception e) {
			Debug.LogError("Client connection error: " + e);
			return -1;
		}

		return 0;
}

	private void ProcessClient(Socket Client) {
		int cmd = 0;
		Byte[] data = new Byte[4];

		while (_runThread) {
			if (ReadData(Client, 4, data) != 0) return;

			cmd = (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);

			// Debug.Log("Cmd: " + cmd);

			if (cmd == 1) {
				if (HandleCmdFrame(Client) != 0) return;
			}
		}
	}

	private void ThreadProc() {

		while (_runThread) {
			_serverSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
			IPAddress hostIP = Dns.GetHostEntry(Dns.GetHostName()).AddressList[0];
			//IPAddress hostIP = Dns.GetHostEntry("127.0.0.1").AddressList[0];
			Debug.Log("Host IP: " + hostIP.ToString());
			IPEndPoint ep = new IPEndPoint(IPAddress.Any, 50001);

			try {
				_serverSocket.Bind(ep);
				_serverSocket.Listen(10);
				_clientSocket = _serverSocket.Accept();
				_serverSocket.Close();
				Debug.Log("Accepted client");
				ProcessClient(_clientSocket);
				Debug.Log("Done processing client");
				_clientSocket.Close();
			} catch (Exception e) {
				Debug.LogError("Trying to host: " + e);
				_serverSocket.Close();
				_serverSocket = null;
			}

			Debug.Log("Client accept terminated");
		}

		Debug.Log("Server thread terminated");
	}

	public void Init() {
		_thread = new Thread(new ThreadStart(ThreadProc));
		_thread.Start();
	}

	public void Destroy() {
		_thread.Abort();
		_runThread = false;
	}
}