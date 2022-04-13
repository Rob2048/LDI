using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Text;
using UnityEngine;
using System.Threading;
using System.Diagnostics;

using Debug = UnityEngine.Debug;

//----------------------------------------------------------------------------------------------------------------
// Jobs.
//----------------------------------------------------------------------------------------------------------------
public abstract class JobTask {
	public abstract void Start(JobManager Manager);
	public abstract bool Update(JobManager Manager);
}

//----------------------------------------------------------------------------------------------------------------
// Delay.
//----------------------------------------------------------------------------------------------------------------
public class JobTaskDelay : JobTask {
	public Stopwatch stopwatch;
	public int delayMs;

	public JobTaskDelay(int Milliseconds) {
		delayMs = Milliseconds;
	}

	public override void Start(JobManager Manager) {
		stopwatch = new Stopwatch();
		stopwatch.Start();
	}

	public override bool Update(JobManager Manager) {
		if (stopwatch.Elapsed.TotalMilliseconds >= delayMs) {
			stopwatch.Stop();
			return true;
		}
		
		return false;
	}
}

//----------------------------------------------------------------------------------------------------------------
// Ping.
//----------------------------------------------------------------------------------------------------------------
public class JobTaskPing : JobTask {
	public override void Start(JobManager Manager) {
		Manager.controller.Ping();
	}

	public override bool Update(JobManager Manager) {
		return !Manager.controller.IsExecuting();
	}
}

//----------------------------------------------------------------------------------------------------------------
// Zero.
//----------------------------------------------------------------------------------------------------------------
public class JobTaskZero : JobTask {
	public override void Start(JobManager Manager) {
		Manager.controller.Zero();
	}

	public override bool Update(JobManager Manager) {
		return !Manager.controller.IsExecuting();
	}
}

//----------------------------------------------------------------------------------------------------------------
// Goto zero.
//----------------------------------------------------------------------------------------------------------------
public class JobTaskGotoZero : JobTask {
	public override void Start(JobManager Manager) {
		Manager.controller.GotoZero();
	}

	public override bool Update(JobManager Manager) {
		return !Manager.controller.IsExecuting();
	}
}

//----------------------------------------------------------------------------------------------------------------
// Modulate laser.
//----------------------------------------------------------------------------------------------------------------
public class JobTaskModulateLaser : JobTask {
	public int frequency;
	public int duty;

	public JobTaskModulateLaser(int Frequency, int Duty) {
		frequency = Frequency;
		duty = Duty;
	}

	public override void Start(JobManager Manager) {
		Manager.controller.ModulateLaser(frequency, duty);
	}

	public override bool Update(JobManager Manager) {
		return !Manager.controller.IsExecuting();
	}
}

//----------------------------------------------------------------------------------------------------------------
// Galvo preview.
//----------------------------------------------------------------------------------------------------------------
public class JobTaskGalvoPreview : JobTask {
	
	public JobTaskGalvoPreview() {
	}

	public override void Start(JobManager Manager) {
		Manager.controller.GalvoPreview();
	}

	public override bool Update(JobManager Manager) {
		return !Manager.controller.IsExecuting();
	}
}

//----------------------------------------------------------------------------------------------------------------
// Galvo move.
//----------------------------------------------------------------------------------------------------------------
public class JobTaskGalvoMove : JobTask {
	public float x;
	public float y;

	public JobTaskGalvoMove(float X, float Y) {
		x = X;
		y = Y;
	}

	public override void Start(JobManager Manager) {
		Manager.controller.GalvoMove(x, y);
	}

	public override bool Update(JobManager Manager) {
		return !Manager.controller.IsExecuting();
	}
}

//----------------------------------------------------------------------------------------------------------------
// Stop laser.
//----------------------------------------------------------------------------------------------------------------
public class JobTaskStopLaser : JobTask {
	public override void Start(JobManager Manager) {
		Manager.controller.StopLaser();
	}

	public override bool Update(JobManager Manager) {
		return !Manager.controller.IsExecuting();
	}
}

//----------------------------------------------------------------------------------------------------------------
// Simple move.
//----------------------------------------------------------------------------------------------------------------
public class JobTaskSimpleMove : JobTask {
	public int axis;
	public float position;
	public int speed;

	public JobTaskSimpleMove(int Axis, float Position, int Speed) {
		axis = Axis;
		position = Position;
		speed = Speed;
	}

	public override void Start(JobManager Manager) {
		Manager.controller.SimpleMove(axis, position, speed);
	}

	public override bool Update(JobManager Manager) {
		return !Manager.controller.IsExecuting();
	}
}

//----------------------------------------------------------------------------------------------------------------
// Simple move relative.
//----------------------------------------------------------------------------------------------------------------
public class JobTaskSimpleMoveRelative : JobTask {
	public int axis;
	public float position;
	public int speed;

	public JobTaskSimpleMoveRelative(int Axis, float Position, int Speed) {
		axis = Axis;
		position = Position;
		speed = Speed;
	}

	public override void Start(JobManager Manager) {
		Manager.controller.SimpleMoveRelative(axis, position, speed);
	}

	public override bool Update(JobManager Manager) {
		return !Manager.controller.IsExecuting();
	}
}

//----------------------------------------------------------------------------------------------------------------
// Start laser.
//----------------------------------------------------------------------------------------------------------------
public class JobTaskStartLaser : JobTask {
	public int counts;
	public int onTime;
	public int offTime;

	public JobTaskStartLaser(int Counts, int OnTime, int OffTime) {
		counts = Counts;
		onTime = OnTime;
		offTime = OffTime;
	}

	public override void Start(JobManager Manager) {
		Manager.controller.StartLaser(counts, onTime, offTime);
	}

	public override bool Update(JobManager Manager) {
		return !Manager.controller.IsExecuting();
	}
}

//----------------------------------------------------------------------------------------------------------------
// Raster line.
//----------------------------------------------------------------------------------------------------------------
public class JobTaskRasterLine : JobTask {
	public int onTime;
	public int offTime;
	public int stepTime;
	public float[] stops;

	public JobTaskRasterLine(int OnTime, int OffTime, int StepTime, float[] Stops) {
		onTime = OnTime;
		offTime = OffTime;
		stepTime = StepTime;
		stops = Stops;
	}

	public override void Start(JobManager Manager) {
		Manager.controller.RasterLine(onTime, offTime, stepTime, stops);
	}

	public override bool Update(JobManager Manager) {
		return !Manager.controller.IsExecuting();
	}
}

//----------------------------------------------------------------------------------------------------------------
// Galvo batch burn.
//----------------------------------------------------------------------------------------------------------------
public class JobTaskGalvoBatchBurn : JobTask {
	public GalvoBatchCmd[] cmds;

	public JobTaskGalvoBatchBurn(GalvoBatchCmd[] Cmds) {
		cmds = Cmds;
	}

	public override void Start(JobManager Manager) {
		Manager.controller.GalvoBatchBurn(cmds);
	}

	public override bool Update(JobManager Manager) {
		return !Manager.controller.IsExecuting();
	}
}

//----------------------------------------------------------------------------------------------------------------
// Raster halftone line.
//----------------------------------------------------------------------------------------------------------------
public class JobTaskRasterHalftoneLine : JobTask {
	public float startX;
	public float stopSize;
	public int offTime;
	public int stepTime;
	public UInt16[] stops;

	public JobTaskRasterHalftoneLine(float StartX, float StopSize, int OffTime, int StepTime, UInt16[] Stops) {
		startX = StartX;
		stopSize = StopSize;
		offTime = OffTime;
		stepTime = StepTime;
		stops = Stops;
	}

	public override void Start(JobManager Manager) {
		Manager.controller.RasterHalftoneLine(startX, stopSize, offTime, stepTime, stops);
	}

	public override bool Update(JobManager Manager) {
		return !Manager.controller.IsExecuting();
	}
}

//----------------------------------------------------------------------------------------------------------------
// Incremental line.
//----------------------------------------------------------------------------------------------------------------
public class JobTaskIncrementalLine : JobTask {
	public int iterations;
	public int steps;
	public int startTime;
	public int timeIncrement;
	public int stepTime;
	public int afterMoveDelayTime;
	public float spacingMm;

	public JobTaskIncrementalLine(int Iterations, int Steps, int StartTime, int TimeIncrement, int StepTime, int AfterMoveDelayTime, float SpacingMm) {
		iterations = Iterations;
		steps = Steps;
		startTime = StartTime;
		timeIncrement = TimeIncrement;
		stepTime = StepTime;
		afterMoveDelayTime = AfterMoveDelayTime;
		spacingMm = SpacingMm;
	}

	public override void Start(JobManager Manager) {
		Manager.controller.IncrementalLine(iterations, steps, startTime, timeIncrement, stepTime, afterMoveDelayTime, spacingMm);
	}

	public override bool Update(JobManager Manager) {
		return !Manager.controller.IsExecuting();
	}
}

//----------------------------------------------------------------------------------------------------------------
// Job manager.
//----------------------------------------------------------------------------------------------------------------
public class JobManager {
	public Controller controller;
	private Thread _thread;
	private bool _running = true;

	private bool _executingTask = false;
	private JobTask _currentTask = null;
	private List<JobTask> _taskList = null;
	private int _taskCurrentId = 0;
	private bool _taskCancel = false;

	public void Init() {
		_thread = new Thread(_ThreadStart);
		_thread.Start();
	}

	public void Destroy() {
		controller.running = false;
		_running = false;
	}

	public void SetTaskList(List<JobTask> Tasks) {
		if (_taskList != null) {
			Debug.LogError("Task list already executing!");
			return;
		}

		_ResetTaskList();
		_taskList = Tasks;
	}

	private void _ResetTaskList() {
		_taskList = null;
		_taskCurrentId = 0;
		_currentTask = null;
		_taskCancel = false;
	}

	public void CancelTask() {
		Debug.Log("Cancelling current task...");
		_taskCancel = true;
	}

	private void _StartNextTask() {
		if (_taskCancel == true) {
			_ResetTaskList();
		}

		if (_taskList == null) {
			return;
		}

		if (_taskCurrentId >= _taskList.Count) {
			_ResetTaskList();
			return;
		}

		_currentTask = _taskList[_taskCurrentId];
		_currentTask.Start(this);
		Debug.Log("Task: " + _taskCurrentId + "/" + _taskList.Count);
	}

	private void _EndCurrentTask() {
		++_taskCurrentId;
		_currentTask = null;
		_StartNextTask();
	}

	private void _ThreadStart() {
		Debug.Log("Job thread started.");

		controller = new Controller();
		controller.Init();
		
		try {
			while (_running) {

				if (_currentTask == null) {
					_StartNextTask();
				} else {
					if (_currentTask.Update(this) == true) {
						_EndCurrentTask();
					}
				}

				controller.Update();
			}

		} catch (Exception E) {
			Debug.LogError("Job thread exception: " + E.Message + " " + E.StackTrace);
		}
	}

	public void AppendTaskGotoZero(List<JobTask> Tasks) {
		Tasks.Add(new JobTaskSimpleMove(0, 0, 200));
		Tasks.Add(new JobTaskSimpleMove(1, 0, 200));
		Tasks.Add(new JobTaskSimpleMove(2, 0, 25));
	}

	public void StartTaskZero() {
		List<JobTask> taskList = new List<JobTask>();
		taskList.Add(new JobTaskZero());
		SetTaskList(taskList);
	}

	public void StartTaskGotoZero() {
		List<JobTask> taskList = new List<JobTask>();
		taskList.Add(new JobTaskGotoZero());
		SetTaskList(taskList);
	}

	public void StartTaskSimpleMoveRelative(int Axis, float Position, int Speed) {
		List<JobTask> taskList = new List<JobTask>();
		taskList.Add(new JobTaskSimpleMoveRelative(Axis, Position, Speed));
		SetTaskList(taskList);
	}

	public void StartTaskModulateLaser(int Frequency, float DutyPercent) {
		int duty = (int)(DutyPercent / 100.0f * 255);
		List<JobTask> taskList = new List<JobTask>();
		taskList.Add(new JobTaskModulateLaser(Frequency, duty));
		SetTaskList(taskList);
	}

	public void StartTaskGalvoPreview() {
		List<JobTask> taskList = new List<JobTask>();
		taskList.Add(new JobTaskGalvoPreview());
		SetTaskList(taskList);
	}

	public void StartTaskStopLaser() {
		List<JobTask> taskList = new List<JobTask>();
		taskList.Add(new JobTaskStopLaser());
		SetTaskList(taskList);
	}

	public void AppendTaskImageRaster(List<JobTask> TaskList, float[] Data, float X, float Y, float Z, int Width, int Height, float ScaleMm, int OnTime, int OffTime, int Speed, int StartLine) {
		TaskList.Add(new JobTaskSimpleMove(2, Z, 400));

		for (int iY = StartLine; iY < Height; ++iY) {
			List<float> data = new List<float>();

			for (int iX = 0; iX < Width; ++iX) {
				if (Data[iY * Width + iX] < 1.0f) {
					data.Add(X + iX * ScaleMm);
				}
			}

			if (data.Count > 0) {
				TaskList.Add(new JobTaskSimpleMove(0, X, 100));
				TaskList.Add(new JobTaskSimpleMove(1, Y + iY * ScaleMm, 200));
				TaskList.Add(new JobTaskDelay(100));
				TaskList.Add(new JobTaskRasterLine(OnTime, OffTime, Speed, data.ToArray()));
			}
		}
	}

	public void AppendTaskImageHalftoneRaster(List<JobTask> TaskList, float[] Data, float X, float Y, float Z, int Width, int Height, float ScaleMm, int OffTime, int Speed, int StartLine) {
		TaskList.Add(new JobTaskSimpleMove(2, Z, 400));

		for (int iY = StartLine; iY < Height; ++iY) {
			List<UInt16> data = new List<UInt16>();

			for (int iX = 0; iX < Width; ++iX) {
				float value = 1.0f - Data[iY * Width + iX];
				UInt16 onTime = 0;

				if (value > 0) {
					onTime = (UInt16)(150.0f + (value * (300.0f - 150.0f))); // 0, 120 to 250
				}

				data.Add(onTime);
			}

			// if (iY == 0) {
			// 	for (int i = 0; i < data.Count; ++i) {
			// 		Debug.Log(data[i]);
			// 	}
			// }

			if (data.Count > 0) {
				TaskList.Add(new JobTaskSimpleMove(0, X, 100));
				TaskList.Add(new JobTaskSimpleMove(1, Y + iY * ScaleMm, 200));
				TaskList.Add(new JobTaskDelay(100));
				TaskList.Add(new JobTaskRasterHalftoneLine(X, ScaleMm, OffTime, Speed, data.ToArray()));
			}
		}
	}

	public void AppendTaskImageHalftoneGalvo(List<JobTask> TaskList, float[] Data, int Width, int Height, float ScaleMm) {
		List<GalvoBatchCmd> data = new List<GalvoBatchCmd>();

		float offX = 20.0f - (Width * ScaleMm) / 2.0f;
		float offY = 20.0f - (Height * ScaleMm) / 2.0f;

		for (int iY = 0; iY < Height; ++iY) {
			for (int iX = 0; iX < Width; ++iX) {
				float value = 1.0f - Data[iY * Width + iX];
				
				if (value > 0) {
					UInt16 onTime = (UInt16)(150.0f + (value * (300.0f - 150.0f))); // 0, 120 to 250
					
					data.Add(new GalvoBatchCmd(offX + iX * ScaleMm, offY + iY * ScaleMm, onTime));

					if (data.Count == 1000) {
						TaskList.Add(new JobTaskGalvoBatchBurn(data.ToArray()));
						data.Clear();
					}
				}
			}
		}

		// Add remaining cmds.
		if (data.Count > 0) {
			TaskList.Add(new JobTaskGalvoBatchBurn(data.ToArray()));
		}
	}

	public void AppendTaskImageGalvo(List<JobTask> TaskList, float[] Data, int Width, int Height, float ScaleMm, int OnTime) {
		List<GalvoBatchCmd> data = new List<GalvoBatchCmd>();

		float offX = 20.0f - (Width * ScaleMm) / 2.0f;
		float offY = 20.0f - (Height * ScaleMm) / 2.0f;

		for (int iY = 0; iY < Height; ++iY) {
			for (int iX = 0; iX < Width; ++iX) {
				if (Data[iY * Width + iX] < 1.0f) {
					data.Add(new GalvoBatchCmd(offX + iX * ScaleMm, offY + iY * ScaleMm, OnTime));

					if (data.Count == 1000) {
						TaskList.Add(new JobTaskGalvoBatchBurn(data.ToArray()));
						data.Clear();
					}
				}
			}
		}

		// Add remaining cmds.
		if (data.Count > 0) {
			TaskList.Add(new JobTaskGalvoBatchBurn(data.ToArray()));
		}
	}
}