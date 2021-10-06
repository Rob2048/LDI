using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Text;
using UnityEngine;
using System.Threading;

public class JobManager {

	public enum JobTask {
		NONE,
		IDLE,
		TEST,
		BUSY,
		PING,
		ZERO,
		SIMPLE_MOVE,
		SIMPLE_MOVE_RELATIVE,
		GOTO_ZERO,
		START_LASER,
		RASTER_IMAGE,
		LINE_TEST,
		RASTER_LINE,
	};

	public Controller _controller;
	private Thread _thread;
	private bool _running = true;

	private bool _executingTask = false;
	private JobTask _nextTask = JobTask.NONE;
	private JobTask _currentTask = JobTask.NONE;
	private int _taskAxis;
	private float _taskPosition;
	private int _taskSpeed;
	private int _taskCounts;
	private int _taskOnTime;
	private int _taskOffTime;
	private int _taskStepTime;
	private int _taskState;
	private float[] _taskImgData;
	private float _taskOffsetX;
	private float _taskOffsetY;
	private bool _taskCancel;
	private int _taskWidth;
	private int _taskHeight;

	private int _rasterState = 0;
	private int _lineCount = 0;
	private int _lineState = 0;
	private int _lineTotal = 0;

	public void Init() {
		_thread = new Thread(_ThreadStart);
		_thread.Start();
	}

	public void Destroy() {
		_controller.running = false;
		_running = false;
	}

	private bool _UpdateGotoZeroProgram() {
		if (_taskState == 0) {
			_controller.SimpleMove(0, 0, 200);
			_taskState = 1;
		} else if (_taskState == 1) {
			_controller.SimpleMove(1, 0, 200);
			_taskState = 2;
		} else if (_taskState == 2) {
			_controller.SimpleMove(2, 0, 25);
			_taskState = 3;
		} else if (_taskState == 3) {
			return true;
		}

		return false;
	}

	private int _GetNextX(int Y, int LastX) {
		LastX++;

		if (Y >= 1000 || LastX >= 1000) {
			return -1;
		}

		for (int i = LastX; i < 1000; ++i) {
			if (_taskImgData[Y * 1000 + i] < 1.0f) {
				return i;
			}
		}

		return -1;
	}

	private bool _UpdateRasterImageProgramBkp() {
		if (_taskCancel == true) {
			return true;
		}

		if (_lineTotal >= 100) {
			return true;
		} else {
			if (_rasterState == 0) {
				// Start of new line.
				Debug.Log("Line: " + _lineTotal);
				_controller.SimpleMove(0, 0, 200);
				_rasterState = 1;
				_lineCount = -1;
				_lineState = 0;
			} else if (_rasterState == 1) {
				_controller.SimpleMove(1, _lineTotal * 0.05f, 200);
				_rasterState = 2;
			} else if (_rasterState == 2) {
				// Plot out line.
				if (_lineState == 0) {
					// Get next X.
					int nextX = _GetNextX(_lineTotal, _lineCount);

					if (nextX == -1 || nextX >= 400) {
						// Move to next line
						_rasterState = 0;
						++_lineTotal;
					} else {
						// Move to next X position.
						_lineCount = nextX;
						_controller.SimpleMove(0, _lineCount * 0.05f, 200);
						_lineState = 1;
					}
				} else if (_lineState == 1) {
					// Fire laser.
					_controller.StartLaser(1, 180, 1000);
					// 250 for toner.
					_lineState = 0;
				}
			}
		}

		return false;
	}

	private bool _UpdateRasterImageProgram() {
		if (_taskCancel == true) {
			return true;
		}

		int maxX = _taskWidth;
		int maxY = _taskHeight;
		float moveX = 0.05f;
		float moveY = 0.05f;

		if (_lineTotal >= maxY) {
			return true;
		} else {
			if (_rasterState == 0) {
				// Start of new line.
				Debug.Log("Line: " + _lineTotal);
				_controller.SimpleMove(0, _taskOffsetX, 200);
				_rasterState = 1;
				_lineCount = -1;
				_lineState = 0;
			} else if (_rasterState == 1) {
				_controller.SimpleMove(1, _taskOffsetY + _lineTotal * moveY, 200);
				_rasterState = 2;
			} else if (_rasterState == 2) {
				// Plot out line.
				if (_lineState == 0) {
					// Get next X.
					int nextX = _GetNextX(_lineTotal, _lineCount);

					if (nextX == -1 || nextX >= maxX) {
						// Move to next line
						_rasterState = 0;
						++_lineTotal;
					} else {
						// Move to next X position.
						_lineCount = nextX;
						_controller.SimpleMove(0, _taskOffsetX + _lineCount * moveX, 200);
						_lineState = 1;
					}
				} else if (_lineState == 1) {
					// Fire laser.
					_controller.StartLaser(1, 100, 2000);
					// 40.3
					// 100
					// 125
					// 150
					// 175
					// 200
					// 225
					_lineState = 0;
				}
			}
		}

		return false;
	}

	private bool _UpdateRasterImageProgramFast() {
		if (_taskCancel == true) {
			return true;
		}

		int maxX = _taskWidth;
		int maxY = _taskHeight;
		float moveX = 0.050f;
		float moveY = 0.050f;

		if (_lineTotal >= maxY) {
			return true;
		} else {
			if (_rasterState == 0) {
				// Start of new line.
				_controller.SimpleMove(0, _taskOffsetX, 200);
				_rasterState = 1;
				_lineCount = -1;
				_lineState = 0;
			} else if (_rasterState == 1) {
				// Move Y to correct line.
				_controller.SimpleMove(1, _taskOffsetY + _lineTotal * moveY, 200);
				_rasterState = 2;
			} else if (_rasterState == 2) {
				// Plot out line.
				List<float> data = new List<float>();

				while (true) {
					int nextX = _GetNextX(_lineTotal, _lineCount);
					_lineCount = nextX;

					if (nextX == -1 || nextX >= maxX) {

						if (data.Count > 0) {
							//_controller.RasterLine(250, 1000, 100, data.ToArray());
							_controller.RasterLine(800, 1000, 100, data.ToArray());

							// 300: thick white on blue/clear
							// 200: black on white - or leaves dark marks?

							// 50 25um
							// 100
							// 100 50um
							// 150
							// 200

							//36.5
							//5.375
							//Y 150
							//M 100 - super thin coat
							//C 120 - thin coat, trying a little more than magenta
							//K 120 - thin coat




						}

						// Move to next Y line
						_rasterState = 0;
						++_lineTotal;
						break;

					} else {
						data.Add(_taskOffsetX + nextX * moveX);
					}
				}
			}
		}

		return false;
	}

	private bool _UpdateRasterScanProgram() {
		if (_taskCancel == true) {
			return true;
		}

		int maxX = _taskWidth;
		int maxY = _taskHeight;
		float moveX = 0.05f;
		float moveY = 0.05f;

		float speed = 40.0f;
		float distance = _taskWidth * moveX;
		int laserTime = 100;

		float pixelStepUs = (float)(((distance / speed) / (double)_taskWidth) * 1000000.0);

		if (laserTime > pixelStepUs) {
			Debug.LogError("Laser time greater than pixel Step time");
			return true;
		}

		if (_lineTotal >= maxY) {
			return true;
		} else {
			if (_rasterState == 0) {
				// Start of new line.
				//_controller.SimpleMove(0, _taskOffsetX, 200);
				// NOTE: Should already be in position for new line.
				_rasterState = 1;
				_lineCount = -1;
				_lineState = 0;
			} else if (_rasterState == 1) {
				// Move Y to correct line.
				_controller.SimpleMove(1, _taskOffsetY + _lineTotal * moveY, 200);
				_rasterState = 2;

				// TODO: Wait after movement. Accel period should be enough time to cancel Y vibrations.
				
			} else if (_rasterState == 2) {
				// Plot out line.
				List<int> data = new List<int>();

				if (_lineTotal % 2 == 0) {
					for (int i = 0; i < _taskWidth; ++i) {
						if (_taskImgData[_lineTotal * 1000 + i] < 1.0f) {
							data.Add((int)(pixelStepUs * i));
						}
					}

					// for (int i = 0; i < 240; ++i) {
					// 	data.Add((int)(pixelStepUs * i));
					// }

					Debug.Log("Send scan line: " + speed + " " + distance + " " + laserTime + " " + data.Count);
					_controller.ScanLine(speed, distance, laserTime, data.ToArray());
				} else {
					for (int i = 0; i < _taskWidth; ++i) {
						if (_taskImgData[_lineTotal * 1000 + (_taskWidth - i - 1)] < 1.0f) {
							data.Add((int)(pixelStepUs * i));
						}
					}

					Debug.Log("Send scan line: " + speed + " " + distance + " " + laserTime + " " + data.Count);
					_controller.ScanLine(speed, -distance, laserTime, data.ToArray());
				}
				
				_rasterState = 0;
				++_lineTotal;
			}
		}

		return false;
	}

	private bool _UpdateTestPatternProgram() {
		if (_taskCancel == true) {
			return true;
		}

		int maxX = 10;
		int maxY = 10;
		float moveX = 0.2f;
		float moveY = 0.2f;

		if (_lineTotal >= maxY) {
			return true;
		} else {
			if (_rasterState == 0) {
				// Start of new line.
				Debug.Log("Line: " + _lineTotal);
				_controller.SimpleMove(0, _taskOffsetX + 0, 200);
				_rasterState = 1;
				_lineCount = -1;
				_lineState = 0;
			} else if (_rasterState == 1) {
				_controller.SimpleMove(1, _taskOffsetY + _lineTotal * moveY, 200);
				_rasterState = 2;
			} else if (_rasterState == 2) {
				// Plot out line.
				if (_lineState == 0) {
					// Get next X.
					int nextX = _lineCount + 1;

					if (nextX == -1 || nextX >= maxX) {
						// Move to next line
						_rasterState = 0;
						++_lineTotal;
					} else {
						// Move to next X position.
						_lineCount = nextX;
						_controller.SimpleMove(0, _taskOffsetX + _lineCount * moveX, 200);
						_lineState = 1;
					}
				} else if (_lineState == 1) {
					// Fire laser.
					// NOTE: Just acts as a delay to chill after stepping.
					_controller.StartLaser(1, 0, 10000);
					_lineState = 2;
				} else if (_lineState == 2) {
					//_controller.StartLaser(200, 50, 50);
					_controller.StartLaser(1, 50, 50);

					// NOTE: ~9 cm to target
					// 2nd 500mw laser
					// Cuts on paper nicely
					// No PWM required.
					// _controller.StartLaser(1, 8000, 0);
					//_controller.StartLaser(10, 800, 200);

					_lineState = 0;
				}
			}
		}

		return false;
	}

	private bool _UpdateLineTestProgram() {
		if (_taskCancel == true) {
			return true;
		}

		// int maxX = 400;
		int maxY = 1;
		// float moveX = 0.05f;
		// float moveY = 0.05f;

		if (_lineTotal >= maxY) {
			return true;
		} else {
			if (_rasterState == 0) {
				// Start of new line.
				Debug.Log("Line: " + _lineTotal);
				_controller.SimpleMove(2, -20.0f, 50);
				_rasterState = 1;
				_lineCount = -1;
				_lineState = 0;
			} else if (_rasterState == 1) {
				// Y axis.
				_controller.SimpleMove(1, _lineTotal * 0.05f, 200);
				_rasterState = 2;
			} else if (_rasterState == 2) {
				// Plot out line.

				if (_lineState == 0) {
					if (_lineCount >= 800) {
						_rasterState = 0;
						++_lineTotal;
					} else {
						++_lineCount;
						_controller.SimpleMove(2, (_lineCount * 0.05f) - 20.0f, 50);
						_lineState = 1;
					}
					
				} else if (_lineState == 1) {
					// Fire laser.
					_controller.StartLaser(1, 250, 0);
					_lineState = 0;
				}

				// if (_lineState == 0) {
				// 	// Get next X.
				// 	int nextX = _GetNextX(_lineTotal, _lineCount);

				// 	if (nextX == -1 || nextX >= maxX) {
				// 		// Move to next line
				// 		_rasterState = 0;
				// 		++_lineTotal;
				// 	} else {
				// 		// Move to next X position.
				// 		_lineCount = nextX;
				// 		_controller.SimpleMove(0, _lineCount * moveX, 200);
				// 		_lineState = 1;
				// 	}
				// } else if (_lineState == 1) {
				// 	// Fire laser.
				// 	_controller.StartLaser(1, 250, 0);
				// 	// 250 for toner.
				// 	_lineState = 0;
				// }
			}
		}

		return false;
	}

	private void _ThreadStart() {
		Debug.Log("Job thread started.");

		_controller = new Controller();
		_controller.Init();
		
		try {
			while (_running) {
				lock (this) {
					if (_nextTask != JobTask.NONE) {
						_currentTask = _nextTask;
						_nextTask = JobTask.NONE;

						if (_currentTask == JobTask.TEST) {
							_executingTask = true;
							_controller.StartLaser(1, 50, 950);
						} else if (_currentTask == JobTask.PING) {
							_executingTask = true;
							_controller.Ping();
						} else if (_currentTask == JobTask.ZERO) {
							_executingTask = true;
							_controller.Zero();
						} else if (_currentTask == JobTask.SIMPLE_MOVE_RELATIVE) {
							_executingTask = true;
							_controller.SimpleMoveRelative(_taskAxis, _taskPosition, _taskSpeed);
						} else if (_currentTask == JobTask.SIMPLE_MOVE) {
							_executingTask = true;
							_controller.SimpleMove(_taskAxis, _taskPosition, _taskSpeed);
						} else if (_currentTask == JobTask.GOTO_ZERO) {
							_executingTask = true;
							_controller.GotoZero();
						} else if (_currentTask == JobTask.START_LASER) {
							_executingTask = true;
							_controller.StartLaser(_taskCounts, _taskOnTime, _taskOffTime);
						} else if (_currentTask == JobTask.RASTER_IMAGE) {
							_executingTask = true;
							_taskState = 0;
							_rasterState = 0;
							_lineTotal = 0;
						} else if (_currentTask == JobTask.LINE_TEST) {
							_executingTask = true;
							_taskState = 0;
							_rasterState = 0;
							_lineTotal = 0;
						} else if (_currentTask == JobTask.RASTER_LINE) {
							_executingTask = true;
							_controller.RasterLine(_taskOnTime, _taskOffTime, _taskStepTime, _taskImgData);
						}
					}
				}

				bool ready = _controller.Update();
				// Thread.Sleep(1):

				// Check if we need to do the next part of the program.
				if (ready) {
					if (_currentTask == JobTask.RASTER_IMAGE) {
						//ready = _UpdateTestPatternProgram();
						ready = _UpdateRasterImageProgramFast();
						//ready = _UpdateRasterImageProgram();
						//ready = _UpdateRasterScanProgram();
					} else if (_currentTask == JobTask.LINE_TEST) {
						//ready = _UpdateLineTestProgram();
						//ready = _UpdateRasterScanProgram();
					}
				}

				lock (this) {
					if (_executingTask == true) {
						if (ready == true) {
							Debug.Log("Done task");
							_executingTask = false;
							_currentTask = JobTask.NONE;
						}
					}
				}
			}

		} catch (Exception E) {
			Debug.LogError("Job thread exception: " + E.Message + " " + E.StackTrace);
		}
	}

	public bool SetNextTask(JobTask Task) {
		lock (this) {
			if (_executingTask == true) {
				return false;
			}

			_nextTask = Task;

			return true;
		}
	}

	public bool SetTaskZero() {
		lock (this) {
			if (_executingTask == true) {
				return false;
			}

			_nextTask = JobTask.ZERO;
			
			return true;
		}
	}

	public bool SetTaskSimpleMove(int Axis, float Position, int Speed) {
		lock (this) {
			if (_executingTask == true) {
				return false;
			}

			_nextTask = JobTask.SIMPLE_MOVE;
			_taskAxis = Axis;
			_taskPosition = Position;
			_taskSpeed = Speed;
			
			return true;
		}
	}

		public bool SetTaskSimpleMoveRelative(int Axis, float Position, int Speed) {
		lock (this) {
			if (_executingTask == true) {
				return false;
			}

			_nextTask = JobTask.SIMPLE_MOVE_RELATIVE;
			_taskAxis = Axis;
			_taskPosition = Position;
			_taskSpeed = Speed;
			
			return true;
		}
	}

	public bool SetTaskGotoZero() {
		lock (this) {
			if (_executingTask == true) {
				return false;
			}

			_nextTask = JobTask.GOTO_ZERO;
			
			return true;
		}
	}

	public bool SetTaskStartLaser(int Counts, int OnTime, int OffTime) {
		lock (this) {
			if (_executingTask == true) {
				return false;
			}

			_nextTask = JobTask.START_LASER;
			_taskCounts = Counts;
			_taskOnTime = OnTime;
			_taskOffTime = OffTime;
			
			return true;
		}
	}

	public bool SetTaskRasterImage(float[] Data, float X, float Y, int Width, int Height) {
		lock (this) {
			if (_executingTask == true) {
				return false;
			}

			_taskCancel = false;
			_taskImgData = new float[1000 * 1000];
			Data.CopyTo(_taskImgData, 0);
			_taskOffsetX = X;
			_taskOffsetY = Y;
			_taskWidth = Width;
			_taskHeight = Height;
			_nextTask = JobTask.RASTER_IMAGE;
			
			return true;
		}
	}

	public bool SetTaskLineTest(float[] Data, float X, float Y, int Width, int Height) {
		lock (this) {
			if (_executingTask == true) {
				return false;
			}

			_taskCancel = false;
			_taskImgData = new float[1000 * 1000];
			Data.CopyTo(_taskImgData, 0);
			_taskOffsetX = X;
			_taskOffsetY = Y;
			_taskWidth = Width;
			_taskHeight = Height;
			_nextTask = JobTask.LINE_TEST;
			
			return true;
		}
	}

	public bool SetLineRaster(int OnTime, int OffTime, int StepTime, float[] Data) {
		lock (this) {
			if (_executingTask == true) {
				return false;
			}

			_taskCancel = false;
			_taskImgData = new float[Data.Length];
			Data.CopyTo(_taskImgData, 0);
			_taskOnTime = OnTime;
			_taskOffTime = OffTime;
			_taskStepTime = StepTime;
			_nextTask = JobTask.RASTER_LINE;
			
			return true;
		}
	}

	public void CancelTask() {
		_taskCancel = true;
	}

}