using System.Collections;
using System.Collections.Generic;
using System.Text;

using UnityEngine;

using NativeWebSocket;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

public class MachineFrame {
	public float x;
	public float y;
	public float z;
	public float a;
	public float b;

	public MachineFrame() {
	}

	public MachineFrame(float X, float Y, float Z, float A, float B) {
		x = X;
		y = Y;
		z = Z;
		a = A;
		b = B;
	}
}

public abstract class PNCCmd {
	public Core core;

	public PNCCmd(Core gCore) {
		core = gCore;
	}

	public abstract bool init();
	public abstract bool run();
}

public class MoveCmd : PNCCmd {
	public float X;
	public float Y;
	public float Z;
	public float A;
	public float B;

	public MoveCmd(Core gCore) : base(gCore) {

	}

	public bool compareFloats(float A, float B) {
		float epsilon = 0.0001f;
		return (A + epsilon > B && A - epsilon < B);
	}

	public bool AreWeThereYet() {
		if (compareFloats(core.Pnc.gX, X) &&
		compareFloats(core.Pnc.gY, Y) &&
		compareFloats(core.Pnc.gZ, Z) &&
		compareFloats(core.Pnc.gA, A) &&
		compareFloats(core.Pnc.gB, B)) {
			//Debug.Log("We are there!");
			return true;
		}

		return false;
	}

	public override bool init() {
		// Check if we are already completed.
		if (!run()) {
			return false;
		}

		core.Pnc.sendMoveGcode(X, Y, Z, A, B);
		
		return true;
	}

	public override bool run() {
		if (AreWeThereYet()) {
			return false;
		}

		return true;
	}
}

public class DelayCmd : PNCCmd {
	public float timeout;

	public DelayCmd(Core gCore, float TimeoutMs) : base(gCore) {
		timeout = TimeoutMs;
	}

	public override bool init()	{
		return true;
	}

	public override bool run() {
		timeout -= Time.deltaTime * 1000.0f;
		return (timeout > 0);
	}
}

public class SaveCameraFrameCmd : PNCCmd {
	string fileName;
	bool datFile;
	string datFileName;
	MachineFrame frame;

	public SaveCameraFrameCmd(Core gCore, string FileName, bool DatFile = false, string DatFileName = "", MachineFrame Frame = null) : base(gCore) {
		fileName = FileName;
		datFile = DatFile;
		datFileName = DatFileName;
		frame = Frame;
	}

	public override bool init()	{
		core.SaveMainCamImage(fileName, datFile, datFileName, frame);
		return false;
	}

	public override bool run() {
		return false;
	}
}

public class pocketNc : MonoBehaviour {
	WebSocket webSocket;

	public Core gCore;

	public float LinearUnits = 1.0f;

	public GameObject AxisX;
	public GameObject AxisY;
	public GameObject AxisZ;
	public GameObject AxisA;
	public GameObject AxisB;

	// public PNCCmd command;
	public Queue<PNCCmd> commands = new Queue<PNCCmd>();
	public PNCCmd executingCmd = null;

	public float gX = 0.0f;
	public float gY = 0.0f;
	public float gZ = 0.0f;
	public float gA = 0.0f;
	public float gB = 0.0f;

	public void SetAxisPositions(float X, float Y, float Z, float A, float B) {
		AxisX.transform.localPosition = new Vector3(-X / 10, 0, 0);
		AxisY.transform.localPosition = new Vector3(0, -Y / 10, 0);
		AxisZ.transform.localPosition = new Vector3(0, 0, Z / 10);
		AxisA.transform.localRotation = Quaternion.AngleAxis(A, new Vector3(1, 0, 0));
		AxisB.transform.localRotation = Quaternion.AngleAxis(-B, new Vector3(0, 1, 0));
	}

	async void Start() {
		webSocket = new WebSocket("ws://192.168.3.18:8000/websocket/linuxcnc", "linuxcnc");

		webSocket.OnOpen += () => {
			Debug.Log("Connection open");

			SendWebSocketMessage();
		};

		webSocket.OnError += (e) => {
			Debug.Log("Error: " + e);
		};

		webSocket.OnClose += (e) => {
			Debug.Log("Close: " + e);
		};

		webSocket.OnMessage += (data) => {
			string msg = Encoding.UTF8.GetString(data, 0, data.Length);

			// Debug.Log("Message: " + msg);

			//JObject o = JsonConvert.DeserializeObject(msg);
			JObject o = JObject.Parse(msg);
			string id = (string)o.SelectToken("id");

			//Debug.Log("Deserialized: " + id);

			if (id == "actual_position" ) {
				gX = (float)o.SelectToken("data[0]") / LinearUnits;
				gY = (float)o.SelectToken("data[1]") / LinearUnits;
				gZ = (float)o.SelectToken("data[2]") / LinearUnits;
				gA = (float)o.SelectToken("data[3]");
				gB = (float)o.SelectToken("data[4]");

				SetAxisPositions(gX, gY, gZ, gA, gB);

			} else if (id == "linear_units") {
				LinearUnits = (float)o.SelectToken("data");
				Debug.Log("Units: " + LinearUnits);
			}
		};

		await webSocket.Connect();
		// NOTE: Connect is blocking until termination.
	}

	void Update() {
		webSocket.DispatchMessageQueue();
		RunCommands();
	}

	public async void sendMoveGcode(float X, float Y, float Z, float A, float B) {
		await webSocket.SendText("{ \"id\": \"mdi\", \"command\": \"put\", \"name\": \"mdi_cmd\", \"0\": \"G1 X" + X + " Y" + Y + " Z" + Z + " A" + A + " B" + B + " F5000\" }");

		// 	id: 'mdi',
		// command: 'put',
		// name: 'mdi_cmd',
		// 0: 'G1 A90 F1500'
	}

	private async void OnApplicationQuit() {
		await webSocket.Close();
	}

	async void SendWebSocketMessage() {
		if (webSocket.State != WebSocketState.Open) 
			return;
		
		Debug.Log("Sending message...");
		// await webSocket.Send(new byte[] { 10, 20, 30 });

		await webSocket.SendText("{ \"id\": \"login\", \"user\": \"default\", \"password\": \"default\" }");
		
		await webSocket.SendText("{ \"id\": \"linear_units\", \"command\": \"get\", \"name\": \"linear_units\" }");

		await webSocket.SendText("{ \"id\": \"actual_position\", \"command\": \"watch\", \"name\": \"actual_position\" }");
	}

	public void QueueCommand(PNCCmd Cmd) {
		// if (command != null) {
		// 	Debug.LogError("Command already executing!");
		// 	return;
		// }

		commands.Enqueue(Cmd);

		if (executingCmd == null) {
			StartNextCommand();
		}
	}

	void StartNextCommand() {
		//Debug.Log("Start next command: " + commands.Count);
		if (commands.Count == 0) {
			return;
		}

		PNCCmd cmd = commands.Dequeue();
		
		if (cmd.init()) {
			executingCmd = cmd;
			RunCommands();
		} else {
			StartNextCommand();
		}
	}

	void RunCommands() {
		if (executingCmd != null) {
			if (!executingCmd.run()) {
				executingCmd = null;
				StartNextCommand();
			}
		}
	}

	public void MoveToPosition(float X, float Y, float Z, float A, float B) {
		MoveCmd cmd = new MoveCmd(gCore);
		cmd.X = X;
		cmd.Y = Y;
		cmd.Z = Z;
		cmd.A = A;
		cmd.B = B;
		QueueCommand(cmd);
	}
}
