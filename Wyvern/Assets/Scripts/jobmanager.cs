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
}

// private int _GetNextX(float[] Data, int Y, int LastX) {
	// 	LastX++;

	// 	if (Y >= 1000 || LastX >= 1000) {
	// 		return -1;
	// 	}

	// 	for (int i = LastX; i < 1000; ++i) {
	// 		if (Data[Y * 1000 + i] < 1.0f) {
	// 			return i;
	// 		}
	// 	}

	// 	return -1;
	// }

	// private bool _UpdateRasterImageProgramBkp() {
	// 	if (_taskCancel == true) {
	// 		return true;
	// 	}

	// 	if (_lineTotal >= 100) {
	// 		return true;
	// 	} else {
	// 		if (_rasterState == 0) {
	// 			// Start of new line.
	// 			Debug.Log("Line: " + _lineTotal);
	// 			_controller.SimpleMove(0, 0, 200);
	// 			_rasterState = 1;
	// 			_lineCount = -1;
	// 			_lineState = 0;
	// 		} else if (_rasterState == 1) {
	// 			_controller.SimpleMove(1, _lineTotal * 0.05f, 200);
	// 			_rasterState = 2;
	// 		} else if (_rasterState == 2) {
	// 			// Plot out line.
	// 			if (_lineState == 0) {
	// 				// Get next X.
	// 				int nextX = _GetNextX(_lineTotal, _lineCount);

	// 				if (nextX == -1 || nextX >= 400) {
	// 					// Move to next line
	// 					_rasterState = 0;
	// 					++_lineTotal;
	// 				} else {
	// 					// Move to next X position.
	// 					_lineCount = nextX;
	// 					_controller.SimpleMove(0, _lineCount * 0.05f, 200);
	// 					_lineState = 1;
	// 				}
	// 			} else if (_lineState == 1) {
	// 				// Fire laser.
	// 				_controller.StartLaser(1, 180, 1000);
	// 				// 250 for toner.
	// 				_lineState = 0;
	// 			}
	// 		}
	// 	}

	// 	return false;
	// }

	// private bool _UpdateRasterImageProgram() {
	// 	if (_taskCancel == true) {
	// 		return true;
	// 	}

	// 	int maxX = _taskWidth;
	// 	int maxY = _taskHeight;
	// 	float moveX = 0.05f;
	// 	float moveY = 0.05f;

	// 	if (_lineTotal >= maxY) {
	// 		return true;
	// 	} else {
	// 		if (_rasterState == 0) {
	// 			// Start of new line.
	// 			Debug.Log("Line: " + _lineTotal);
	// 			_controller.SimpleMove(0, _taskOffsetX, 200);
	// 			_rasterState = 1;
	// 			_lineCount = -1;
	// 			_lineState = 0;
	// 		} else if (_rasterState == 1) {
	// 			_controller.SimpleMove(1, _taskOffsetY + _lineTotal * moveY, 200);
	// 			_rasterState = 2;
	// 		} else if (_rasterState == 2) {
	// 			// Plot out line.
	// 			if (_lineState == 0) {
	// 				// Get next X.
	// 				int nextX = _GetNextX(_lineTotal, _lineCount);

	// 				if (nextX == -1 || nextX >= maxX) {
	// 					// Move to next line
	// 					_rasterState = 0;
	// 					++_lineTotal;
	// 				} else {
	// 					// Move to next X position.
	// 					_lineCount = nextX;
	// 					_controller.SimpleMove(0, _taskOffsetX + _lineCount * moveX, 200);
	// 					_lineState = 1;
	// 				}
	// 			} else if (_lineState == 1) {
	// 				// Fire laser.
	// 				_controller.StartLaser(1, 100, 2000);
	// 				// 40.3
	// 				// 100
	// 				// 125
	// 				// 150
	// 				// 175
	// 				// 200
	// 				// 225
	// 				_lineState = 0;
	// 			}
	// 		}
	// 	}

	// 	return false;
	// }

	// private bool _UpdateRasterImageProgramFast() {
	// 	if (_taskCancel == true) {
	// 		return true;
	// 	}

	// 	int maxX = _taskWidth;
	// 	int maxY = _taskHeight;
	// 	float moveX = 0.025f;
	// 	float moveY = 0.025f;

	// 	if (_lineTotal >= maxY) {
	// 		return true;
	// 	} else {
	// 		if (_rasterState == 0) {
	// 			// Start of new line.
	// 			_controller.SimpleMove(0, _taskOffsetX, 200);
	// 			_rasterState = 1;
	// 			_lineCount = -1;
	// 			_lineState = 0;
	// 		} else if (_rasterState == 1) {
	// 			// Move Y to correct line.
	// 			_controller.SimpleMove(1, _taskOffsetY + _lineTotal * moveY, 200);
	// 			_rasterState = 2;
	// 		} else if (_rasterState == 2) {
	// 			// Plot out line.
	// 			List<float> data = new List<float>();

	// 			while (true) {
	// 				int nextX = _GetNextX(_lineTotal, _lineCount);
	// 				_lineCount = nextX;

	// 				if (nextX == -1 || nextX >= maxX) {

	// 					if (data.Count > 0) {
	// 						//_controller.RasterLine(250, 1000, 100, data.ToArray());
	// 						// _controller.RasterLine(800, 1000, 100, data.ToArray());
	// 						_controller.RasterLine(350, 2000, 400, data.ToArray());

	// 						// 300: thick white on blue/clear
	// 						// 200: black on white - or leaves dark marks?

	// 						// 50 25um
	// 						// 100
	// 						// 100 50um
	// 						// 150
	// 						// 200

	// 						//36.5
	// 						//5.375
	// 						//Y 150
	// 						//M 100 - super thin coat
	// 						//C 120 - thin coat, trying a little more than magenta
	// 						//K 120 - thin coat
	// 					}

	// 					// Move to next Y line
	// 					_rasterState = 0;
	// 					++_lineTotal;
	// 					break;

	// 				} else {
	// 					data.Add(_taskOffsetX + nextX * moveX);
	// 				}
	// 			}
	// 		}
	// 	}

	// 	return false;
	// }

	// private bool _UpdateRasterScanProgram() {
	// 	if (_taskCancel == true) {
	// 		return true;
	// 	}

	// 	int maxX = _taskWidth;
	// 	int maxY = _taskHeight;
	// 	float moveX = 0.05f;
	// 	float moveY = 0.05f;

	// 	float speed = 40.0f;
	// 	float distance = _taskWidth * moveX;
	// 	int laserTime = 100;

	// 	float pixelStepUs = (float)(((distance / speed) / (double)_taskWidth) * 1000000.0);

	// 	if (laserTime > pixelStepUs) {
	// 		Debug.LogError("Laser time greater than pixel Step time");
	// 		return true;
	// 	}

	// 	if (_lineTotal >= maxY) {
	// 		return true;
	// 	} else {
	// 		if (_rasterState == 0) {
	// 			// Start of new line.
	// 			//_controller.SimpleMove(0, _taskOffsetX, 200);
	// 			// NOTE: Should already be in position for new line.
	// 			_rasterState = 1;
	// 			_lineCount = -1;
	// 			_lineState = 0;
	// 		} else if (_rasterState == 1) {
	// 			// Move Y to correct line.
	// 			_controller.SimpleMove(1, _taskOffsetY + _lineTotal * moveY, 200);
	// 			_rasterState = 2;

	// 			// TODO: Wait after movement. Accel period should be enough time to cancel Y vibrations.
				
	// 		} else if (_rasterState == 2) {
	// 			// Plot out line.
	// 			List<int> data = new List<int>();

	// 			if (_lineTotal % 2 == 0) {
	// 				for (int i = 0; i < _taskWidth; ++i) {
	// 					if (_taskImgData[_lineTotal * 1000 + i] < 1.0f) {
	// 						data.Add((int)(pixelStepUs * i));
	// 					}
	// 				}

	// 				// for (int i = 0; i < 240; ++i) {
	// 				// 	data.Add((int)(pixelStepUs * i));
	// 				// }

	// 				Debug.Log("Send scan line: " + speed + " " + distance + " " + laserTime + " " + data.Count);
	// 				_controller.ScanLine(speed, distance, laserTime, data.ToArray());
	// 			} else {
	// 				for (int i = 0; i < _taskWidth; ++i) {
	// 					if (_taskImgData[_lineTotal * 1000 + (_taskWidth - i - 1)] < 1.0f) {
	// 						data.Add((int)(pixelStepUs * i));
	// 					}
	// 				}

	// 				Debug.Log("Send scan line: " + speed + " " + distance + " " + laserTime + " " + data.Count);
	// 				_controller.ScanLine(speed, -distance, laserTime, data.ToArray());
	// 			}
				
	// 			_rasterState = 0;
	// 			++_lineTotal;
	// 		}
	// 	}

	// 	return false;
	// }

	// private bool _UpdateTestPatternProgram() {
	// 	if (_taskCancel == true) {
	// 		return true;
	// 	}

	// 	int maxX = 10;
	// 	int maxY = 10;
	// 	float moveX = 0.2f;
	// 	float moveY = 0.2f;

	// 	if (_lineTotal >= maxY) {
	// 		return true;
	// 	} else {
	// 		if (_rasterState == 0) {
	// 			// Start of new line.
	// 			Debug.Log("Line: " + _lineTotal);
	// 			_controller.SimpleMove(0, _taskOffsetX + 0, 200);
	// 			_rasterState = 1;
	// 			_lineCount = -1;
	// 			_lineState = 0;
	// 		} else if (_rasterState == 1) {
	// 			_controller.SimpleMove(1, _taskOffsetY + _lineTotal * moveY, 200);
	// 			_rasterState = 2;
	// 		} else if (_rasterState == 2) {
	// 			// Plot out line.
	// 			if (_lineState == 0) {
	// 				// Get next X.
	// 				int nextX = _lineCount + 1;

	// 				if (nextX == -1 || nextX >= maxX) {
	// 					// Move to next line
	// 					_rasterState = 0;
	// 					++_lineTotal;
	// 				} else {
	// 					// Move to next X position.
	// 					_lineCount = nextX;
	// 					_controller.SimpleMove(0, _taskOffsetX + _lineCount * moveX, 200);
	// 					_lineState = 1;
	// 				}
	// 			} else if (_lineState == 1) {
	// 				// Fire laser.
	// 				// NOTE: Just acts as a delay to chill after stepping.
	// 				_controller.StartLaser(1, 0, 10000);
	// 				_lineState = 2;
	// 			} else if (_lineState == 2) {
	// 				//_controller.StartLaser(200, 50, 50);
	// 				_controller.StartLaser(1, 50, 50);

	// 				// NOTE: ~9 cm to target
	// 				// 2nd 500mw laser
	// 				// Cuts on paper nicely
	// 				// No PWM required.
	// 				// _controller.StartLaser(1, 8000, 0);
	// 				//_controller.StartLaser(10, 800, 200);

	// 				_lineState = 0;
	// 			}
	// 		}
	// 	}

	// 	return false;
	// }

	// private bool _UpdateLineTestProgram() {
	// 	if (_taskCancel == true) {
	// 		return true;
	// 	}

	// 	// int maxX = 400;
	// 	int maxY = 1;
	// 	// float moveX = 0.05f;
	// 	// float moveY = 0.05f;

	// 	if (_lineTotal >= maxY) {
	// 		return true;
	// 	} else {
	// 		if (_rasterState == 0) {
	// 			// Start of new line.
	// 			Debug.Log("Line: " + _lineTotal);
	// 			_controller.SimpleMove(2, -20.0f, 50);
	// 			_rasterState = 1;
	// 			_lineCount = -1;
	// 			_lineState = 0;
	// 		} else if (_rasterState == 1) {
	// 			// Y axis.
	// 			_controller.SimpleMove(1, _lineTotal * 0.05f, 200);
	// 			_rasterState = 2;
	// 		} else if (_rasterState == 2) {
	// 			// Plot out line.

	// 			if (_lineState == 0) {
	// 				if (_lineCount >= 800) {
	// 					_rasterState = 0;
	// 					++_lineTotal;
	// 				} else {
	// 					++_lineCount;
	// 					_controller.SimpleMove(2, (_lineCount * 0.05f) - 20.0f, 50);
	// 					_lineState = 1;
	// 				}
					
	// 			} else if (_lineState == 1) {
	// 				// Fire laser.
	// 				_controller.StartLaser(1, 250, 0);
	// 				_lineState = 0;
	// 			}

	// 			// if (_lineState == 0) {
	// 			// 	// Get next X.
	// 			// 	int nextX = _GetNextX(_lineTotal, _lineCount);

	// 			// 	if (nextX == -1 || nextX >= maxX) {
	// 			// 		// Move to next line
	// 			// 		_rasterState = 0;
	// 			// 		++_lineTotal;
	// 			// 	} else {
	// 			// 		// Move to next X position.
	// 			// 		_lineCount = nextX;
	// 			// 		_controller.SimpleMove(0, _lineCount * moveX, 200);
	// 			// 		_lineState = 1;
	// 			// 	}
	// 			// } else if (_lineState == 1) {
	// 			// 	// Fire laser.
	// 			// 	_controller.StartLaser(1, 250, 0);
	// 			// 	// 250 for toner.
	// 			// 	_lineState = 0;
	// 			// }
	// 		}
	// 	}

	// 	return false;
	// }