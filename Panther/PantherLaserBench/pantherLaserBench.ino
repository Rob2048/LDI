//--------------------------------------------------------------------------------
// Teensy stepper motor controller.
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
// NOTES:
//--------------------------------------------------------------------------------
// 4000 steps/s = 250us total per step action.
// Stepper is 1.8 deg per step, at 8 microsteps.
// Stepper per rev = 1600 steps.
// Diamater of wheel = 12.2mm  (Accuracy?)
// 279mm over 7 revolutions
// 39.86mm of belt per revolution (12.688 diameter on motor pulley)
// 40.1434 steps/mm
// 0.02491 mm/step

// (8 usteps) 1600 steps per 40mm = 25um per step
// (32 usteps) 6400 steps per 40mm = 6.25um per step

//--------------------------------------------------------------------------------

// Steppers
// 1 cs 33 stp 34 dir 35
// 2 cs 37 stp 38 dir 39
// 3 cs 14 stp 15 dir 16
// 4 cs 20 stp 19 dir 18
// 5 cs 2 stp 1 dir 0
// 6 cs 8 stp 7 dir 6
// 7 cs 27 stp 26 dir 25
// 8 cs 31 stp 30 dir 29

// Endstops
// 1 36
// 2 9
// 3 17
// 4 10
// 5 5
// 6 24
// 7 4
// 8 28

// Aux
// 5 3
// 3 23
// 2 22
// 1 21

/*
Motion planning prep.

100 us per 25 um dot maybe.

250 mm/s
1 step = 0.00625 mm
4 steps per dot space?

Current process:
- stepHigh
- delay 5 us
- stepLow
- delay Speed (100 us)
- delay offTime (1000 us)
- laserHigh
- delay onTime (120 us)
- laserLow

New process:
- Accelerate
- At full speed and over start position, start laser pulses
- After end position, stop laser pulses
- Decelerate
- Move to next line

time per step at full speed (250 mm/s):
40 000 steps per second
25 us per step.

// PWM control on steps??
// Interrupts

*/

#define PIN_STP_EN1			32
#define PIN_LASER_PWM		3

#include <SPI.h>
#include <stdint.h>
#include <math.h>
#include "stepper.h"

#define AXIS_ID_X			0
#define AXIS_ID_Y			1

abStepper st1 = abStepper(0, 33, 34, 35, 36);
abStepper st2 = abStepper(0, 37, 38, 39, 9);
abStepper st3 = abStepper(0, 14, 15, 16, 17);

//--------------------------------------------------------------------------------
// Setup.
//--------------------------------------------------------------------------------
unsigned long _log[16000];
char _log2[16000];
int32_t _logCounts = 0;

float _laserPos[1024];
int _laserPosCount = 0;

int32_t _laserTiming[4096];
int _laserTimingCount = 0;

void timerTest() {
	Serial.println();
	Serial.println("Timer testing");

	float speed = 125.0f;
	float totalDistance = 20.0f;
	// Initial laser pos list. 400 positions over 20 mm (50 um each)
	for (int i = 0; i < 400; ++i) {
		_laserPos[_laserPosCount++] = i * 0.050;
	}

	float totalTimeUs = (totalDistance / speed) * 1000000.0f;

	// Build laser timing list.
	for (int i = 0; i < _laserPosCount; ++i) {
		_laserTiming[_laserTimingCount++] = _laserPos[i] / totalDistance * totalTimeUs;
	}

	int laserState = 0;
	uint32_t laserNextT = 0;
	int laserPulses = 0;
	int laserOnTimeUs = 120;

	float stepsToMm = 0.00625f;
	float mmToSteps = 1.0 / stepsToMm;
	int motorSteps = totalDistance * mmToSteps;
	int motorCurrent = 0;
	uint32_t nextMotorT = 0;
	int motorState = 0;
	int stepDelayNs = 1000000000 / (speed * mmToSteps);

	//Serial.printf("Step delay: %d (%f)\n", stepDelayNs, (1000000.0 / (speed * mmToSteps)));

	if (_laserTimingCount > 0) {
		laserNextT = _laserTiming[laserPulses];
	}

	uint32_t t = micros();

	while (true) {
		uint32_t nt = micros() - t;
		
		if (laserPulses < _laserTimingCount) {
			if (nt >= laserNextT) {
				if (laserState == 0) {
					// High
					digitalWrite(st1.stepPin, HIGH);
					_log[_logCounts] = nt;
					_log2[_logCounts++] = 'L';
					laserNextT += laserOnTimeUs;
					laserState = 1;
				} else {
					// Low
					digitalWrite(st1.stepPin, LOW);
					_log[_logCounts] = nt;
					_log2[_logCounts++] = 'l';
					laserState = 0;
					++laserPulses;

					if (_laserTimingCount > 0) {
						laserNextT = _laserTiming[laserPulses];
					}
				}
			}
		}

		if (nt >= (nextMotorT + 500) / 1000) {
		//if (ns >= nextMotorT) {
			if (motorState == 0) {
				if (motorCurrent >= motorSteps) {
					_log[_logCounts] = nt;
					_log2[_logCounts++] = 'D';
					break;
				}

				// High
				_log[_logCounts] = nt;
				_log2[_logCounts++] = 'M';
				nextMotorT += 2000;
				motorState = 1;
			} else {
				// Low
				_log[_logCounts] = nt;
				_log2[_logCounts++] = 'm';
				nextMotorT += stepDelayNs - 2000;
				motorState = 0;
				++motorCurrent;
			}
		}
	}

	t = micros() - t;

	for (int i = 0; i < _logCounts; ++i) {
		Serial.print(_log[i]);
		Serial.print("\t");
		Serial.println(_log2[i]);
	}

	Serial.printf("Motor steps: %d\n", motorCurrent);
	Serial.printf("Laser pulses: %d\n", laserPulses);
	Serial.printf("Log size: %d\n", _logCounts);
	Serial.printf("Step delay: %d\n", stepDelayNs);

	// for (int i = 0; i < _laserPosCount; ++i) {
	// 	Serial.print(_laserPos[i]);
	// 	Serial.print("\t");
	// 	Serial.println(_laserTiming[i]);
	// }
}

void setup() {
	Serial.begin(921600);
	// while (!Serial);
	
	//if (!st1.init(32, 0, 900, 0.00625, 2000.0, true)) sendMessage("Bad driver X");
	if (!st1.init(32, 0, 900, 0.00625, 16000.0, true)) sendMessage("Bad driver X");
	if (!st2.init(32, 1, 900, 0.00625, 2000.0, false)) sendMessage("Bad driver Y");
	if (!st3.init(32, 1, 600, 0.00625, 1400.0, false)) sendMessage("Bad driver Z");

	// Enable steppers.
	pinMode(PIN_STP_EN1, OUTPUT);
	digitalWrite(PIN_STP_EN1, LOW);

	// Enable laser.
	pinMode(PIN_LASER_PWM, OUTPUT);
	digitalWrite(PIN_LASER_PWM, LOW);

	//timerTest();
}

/*
	Command:
		AxisId:
			0 - X
			1 - Y
			2 - Z
			3 - A
			4 - B

		CmdIds:
			[requests]
			0 - home
			1 - move
			2 - stop (cancel all stepper actions)
			3 - zero
			4 - simple move
			5 - burst laser
			[responses]
			10 - cmd recv success.
			11 - cmd recv failure. (expects command resend)
			12 - cmd invalid.
			13 - general failure.
			14 - utf8 message.

		Zero: [1 start][1 size][1 cmdId][1 end]

		Home: [1 start][1 size][1 cmdId][1 axisId][1 end]

		Move: [1 start][1 size][1 cmdId][1 numAxis][1 axisId][1 moveMode][4 steps][4 startSpeed][4 endSpeed][4 maxSpeed][1 end]

		MoveSimple: [1 start][1 size][1 cmdId][1 axisId][

		Response: [1 start][1 size][1 cmdId][1 end]
*/

enum RecvState {
	RS_START,
	RS_COUNT0,
	RS_COUNT1,
	RS_PAYLOAD,
	RS_END
};

uint8_t _recvBuffer[16384];
uint32_t _recvLen = 0;
uint16_t _recvPayloadSize = 0;
RecvState _recvState = RS_START;

const uint8_t FRAME_START = 255;
const uint8_t FRAME_END = 254;

// Mirror raster buffer
int32_t _targetSteps[4096];

void sendMessage(const char* Message) {
	uint8_t buffer[256];
	int strLen = 0;

	while (1) {
		if (Message[strLen] == 0)
			break;

		buffer[3 + strLen] = Message[strLen];
		++strLen;
	}

	buffer[0] = FRAME_START;
	buffer[1] = strLen + 1;
	buffer[2] = 13;
	buffer[strLen + 3] = FRAME_END;

	Serial.write(buffer, strLen + 4);
}

void sendCmdSuccess() {
	//uint8_t buffer[4] = { FRAME_START, 1, 10, FRAME_END };
	//Serial.write(buffer, 4);

	uint8_t buffer[16];
	buffer[0] = FRAME_START;
	buffer[1] = 13;
	buffer[2] = 10;
	buffer[sizeof(buffer) - 1] = FRAME_END;

	float s1p = st1.currentStep * st1.mmPerStep;
	float s2p = st2.currentStep * st2.mmPerStep;
	float s3p = st3.currentStep * st3.mmPerStep;

	memcpy(buffer + 3, &s1p, 4);
	memcpy(buffer + 7, &s2p, 4);
	memcpy(buffer + 11, &s3p, 4);
	
	Serial.write(buffer, sizeof(buffer));
}

bool moveStepperUntilLimit(abStepper* Stepper) {
	int p = digitalRead(Stepper->limitPin);

	if (p == 0) {
		return false;
	}

	Stepper->updateStepper();

	return true;
}

void processCmd(uint8_t* Buffer, int Len) {
	if (Len == 0)
		return;

	char buff[1024];
	
	uint8_t cmdId = Buffer[0];

	if (cmdId == 0) {
		sendCmdSuccess();
	} else if (cmdId == 1 && Len == 20) {
		// int numAxis = Buffer[1];
		// int bufIdx = 2;

		// TODO: Check for length based on num axis.

		// sprintf(buff, "Move %d", numAxis);
		// sendMessage(buff);

		// for (int i = 0; i < numAxis; ++i) {
		// 	int axisId = Buffer[bufIdx++];
		// 	int moveMode = Buffer[bufIdx++];
		// 	int stepTarget = 0;
		// 	float startVelocity = 0.0f;
		// 	float endVelocity = 0.0f;
		// 	float maxVelocity = 0.0f;

		// 	memcpy(&stepTarget, Buffer + bufIdx, 4); bufIdx += 4;
		// 	memcpy(&startVelocity, Buffer + bufIdx, 4); bufIdx += 4;
		// 	memcpy(&endVelocity, Buffer + bufIdx, 4); bufIdx += 4;
		// 	memcpy(&maxVelocity, Buffer + bufIdx, 4); bufIdx += 4;

		// 	// Build stepper data here.

		// 	if (axisId == AXIS_ID_X) {
		// 		st4.moveTo(stepTarget, startVelocity, endVelocity, maxVelocity);
		// 		while (st4.updateStepper());
		// 	} else if (axisId == AXIS_ID_Y) {
		// 		st1.moveTo(stepTarget, startVelocity, endVelocity, maxVelocity);
		// 		while (st1.updateStepper());
		// 	} else if (axisId == AXIS_ID_Z) {
		// 		st2.moveTo(stepTarget, startVelocity, endVelocity, maxVelocity);
		// 		st3.moveTo(stepTarget, startVelocity, endVelocity, maxVelocity);
		// 		while (1) {
		// 			bool z0 = st2.updateStepper();
		// 			bool z1 = st3.updateStepper();

		// 			if (z0 == false && z1 == false)
		// 				break;
		// 		}
		// 	} else if (axisId == AXIS_ID_A) {
		// 		st5.moveTo(stepTarget, startVelocity, endVelocity, maxVelocity);
		// 		while (st5.updateStepper());
		// 	} else if (axisId == AXIS_ID_B) {
		// 		st6.moveTo(stepTarget, startVelocity, endVelocity, maxVelocity);
		// 		while (st6.updateStepper());
		// 	}

		// 	// sprintf(buff, "Axis: %d Mode: %d Step: %d Start: %f End: %f Max: %f", axisId, moveMode, stepTarget, startVelocity, endVelocity, maxVelocity);
		// 	// sendMessage(buff);
		// }

		sendCmdSuccess();
	} else if (cmdId == 3 && Len == 10) {
		// Simple move relative.
		int axisId = Buffer[1];
		float position = 0.0f;
		int32_t speed = 0;
		memcpy(&position, Buffer + 2, 4);
		memcpy(&speed, Buffer + 6, 4);

		if (axisId == 0) {
			// X
			st1.moveDirect((int32_t)(position * st1.stepsPerMm) + st1.currentStep, speed);
		} else if (axisId == 1) {
			// Y
			st2.moveDirect((int32_t)(position * st2.stepsPerMm) + st2.currentStep, speed);
		} else if (axisId == 2) {
			// Z
			st3.moveDirect((int32_t)(position * st3.stepsPerMm) + st3.currentStep, speed);
		}

		// sprintf(buff, "Simple move %d %f", axisId, position);
		// sendMessage(buff);

		sendCmdSuccess();
	} else if (cmdId == 4 && Len == 10) {
		// Simple move.
		int axisId = Buffer[1];
		float position = 0.0f;
		int32_t speed = 0;
		memcpy(&position, Buffer + 2, 4);
		memcpy(&speed, Buffer + 6, 4);

		if (axisId == 0) {
			// X
			st1.moveSimple(position, speed);
		} else if (axisId == 1) {
			// Y
			st2.moveSimple(position, speed);
		} else if (axisId == 2) {
			// Z
			st3.moveSimple(position, speed);
		}

		// sprintf(buff, "Simple move %d %f", axisId, position);
		// sendMessage(buff);

		sendCmdSuccess();
	} else if (cmdId == 5 && Len == 5) {
		// Burst laser.

		int32_t time = 0;
		memcpy(&time, Buffer + 1, 4);

		digitalWrite(PIN_LASER_PWM, HIGH);
		delayMicroseconds(time);
		digitalWrite(PIN_LASER_PWM, LOW);

		sendCmdSuccess();
	} else if (cmdId == 6 && Len == 13) {
		// Start laser.
		int32_t counts = 0;
		int32_t onTime = 0;
		int32_t offTime = 0;
		memcpy(&onTime, Buffer + 1, 4);
		memcpy(&offTime, Buffer + 5, 4);
		memcpy(&counts, Buffer + 9, 4);
		
		for (int i = 0; i < counts; ++i) {
			delayMicroseconds(offTime);
			digitalWrite(PIN_LASER_PWM, HIGH);
			delayMicroseconds(onTime);
			digitalWrite(PIN_LASER_PWM, LOW);
		}

		// sprintf(buff, "Laser: %ld %ld %ld", onTime, offTime, counts);
		// sendMessage(buff);

		sendCmdSuccess();
	} else if (cmdId == 7 && Len == 1) {
		// Stop laser.
		digitalWrite(PIN_LASER_PWM, LOW);
		sendCmdSuccess();
	} else if (cmdId == 8 && Len >= 8) {
		// Scan with stops at every pulse position.
		int32_t stopCounts = 0;
		int32_t onTime = 0;
		int32_t offTime = 0;
		int32_t stepTime = 0;

		memcpy(&stopCounts, Buffer + 1, 4);
		memcpy(&onTime, Buffer + 5, 4);
		memcpy(&offTime, Buffer + 9, 4);
		memcpy(&stepTime, Buffer + 13, 4);

		unsigned long t = micros();

		// Convert all stops to step target buffer.
		for (int i = 0; i < stopCounts; ++i) {
			float position = 0.0f;
			memcpy(&position, Buffer + 17 + i * 4, 4);
			_targetSteps[i] = (int32_t)round(position * st1.stepsPerMm);
		}

		t = micros() - t;

		sprintf(buff, "Raster (%ld %ld %ld %ld) computed in %ld us", stopCounts, onTime, offTime, stepTime, t);
		sendMessage(buff);

		for (int i = 0; i < stopCounts; ++i) {
			st1.moveDirect(_targetSteps[i], stepTime);
			delayMicroseconds(offTime);
			digitalWrite(PIN_LASER_PWM, HIGH);
			delayMicroseconds(onTime);
			digitalWrite(PIN_LASER_PWM, LOW);
		}

		sendCmdSuccess();
	} else if (cmdId == 10) {
		// Home all axes.
		sprintf(buff, "Homing");
		sendMessage(buff);

		st1.moveRelative(-100000, 0, 0, 50);
		st2.moveRelative(100000, 0, 0, 50);
		st3.moveRelative(100000, 0, 0, 50);

		bool s1 = true;
		bool s2 = true;
		bool s3 = true;

		while (s1 || s2 || s3) {
			if (s1) { s1 = moveStepperUntilLimit(&st1); }
			if (s2) { s2 = moveStepperUntilLimit(&st2); }
			if (s3) { s3 = moveStepperUntilLimit(&st3); }
		}

		st1.home(50, 100, false, 0, 0, 20000);
		st2.home(50, 100, true, 15000, 0, 15000);
		st3.home(50, 30, true, 24000, 0, 24000);

		st1.moveTo(10000, 0, 0, 200);
		st2.moveTo(7500, 0, 0, 100);
		st3.moveTo(1000, 0, 0, 50);

		s1 = true;
		s2 = true;
		s3 = true;

		while (s1 || s2 || s3) {
			s1 = st1.updateStepper();
			s2 = st2.updateStepper();
			s3 = st3.updateStepper();
		}

		sendCmdSuccess();
	} else if (cmdId == 11) {
		// Move to center?
		st1.moveTo(10000, 0, 0, 200);
		st2.moveTo(7500, 0, 0, 100);
		st3.moveTo(1000, 0, 0, 50);
		
		bool s1 = true;
		bool s2 = true;
		bool s3 = true;

		while (s1 || s2 || s3) {
			s1 = st1.updateStepper();
			s2 = st2.updateStepper();
			s3 = st3.updateStepper();
		}

		sendCmdSuccess();
	} else if (cmdId == 12) {
		// Scan with continuous movement.

		// X axis raster line.
		// 20 mm test
		
		// TODO: Move back to start position which accounts for accel phase.

		float speed = 125.0f;
		float totalDistance = 20.0f;
		int laserOnTimeUs = 120;
		int pulseCounts = 0;
		
		memcpy(&speed, Buffer + 1, 4);
		memcpy(&totalDistance, Buffer + 5, 4);
		memcpy(&laserOnTimeUs, Buffer + 9, 4);
		memcpy(&pulseCounts, Buffer + 13, 4);

		// sprintf(buff, "Len: %d\n", Len);
		// sendMessage(buff);

		// sprintf(buff, "Speed: %f\n", speed);
		// sendMessage(buff);
		// sprintf(buff, "totalDistance: %f\n", totalDistance);
		// sendMessage(buff);
		// sprintf(buff, "laserOnTimeUs: %d\n", laserOnTimeUs);
		// sendMessage(buff);
		// sprintf(buff, "pulseCounts: %d\n", pulseCounts);
		// sendMessage(buff);

		for (int i = 0; i < pulseCounts; ++i) {
			int time = 0;
			memcpy(&time, Buffer + 17 + i * 4, 4);
			_laserTiming[i] = time;

			//sprintf(buff, "%d: %d\n", i, time);
			//sendMessage(buff);
		}
		_laserTimingCount = pulseCounts;

		//sprintf(buff, "Speed: %f\n", speed);
		//sendMessage(buff);

		//--------------------------------------------------------------------------------
		// Calcs.
		//--------------------------------------------------------------------------------
		int motorSteps = totalDistance * st1.stepsPerMm;
		int absSteps = abs(motorSteps);
		float totalTimeUs = (totalDistance / speed) * 1000000.0f;

		int laserState = 0;
		uint32_t laserNextT = 0;
		int laserPulseCount = 0;
		
		int motorCurrent = 0;
		uint32_t nextMotorT = 0;
		int motorState = 0;
		int stepDelayNs = 1000000000 / (speed * st1.stepsPerMm);

		if (_laserTimingCount > 0) {
			laserNextT = _laserTiming[laserPulseCount];
		}

		//--------------------------------------------------------------------------------
		// Accel phase.
		//--------------------------------------------------------------------------------
		int dir = 1;

		if (motorSteps < 0) {
			dir = 0;
		}

		st1.moveRelative(motorSteps, 0, 0, speed);
		int preSteps = st1.accelSteps;

		if (dir == 0) {
			preSteps = -preSteps;
		}

		//sprintf(buff, "Pulse count: %d PreSteps: %d Speed: %d\n", _laserTimingCount, preSteps, speed);
		//sendMessage(buff);

		// Accel phase
		st1.moveRelative(preSteps, 0, speed, speed);
		while (st1.updateStepper());

		// TOOD: Drift.

		//sprintf(buff, "Accel done\n");
		//sendMessage(buff);

		//--------------------------------------------------------------------------------
		// Scan phase.
		//--------------------------------------------------------------------------------
		uint32_t t = micros();

		while (true) {
			uint32_t nt = micros() - t;
			
			// Control laser.
			if (laserPulseCount < _laserTimingCount) {
				if (nt >= laserNextT) {
					if (laserState == 0) {
						// High
						digitalWrite(PIN_LASER_PWM, HIGH);
						//_log[_logCounts] = nt;
						//_log2[_logCounts++] = 'L';
						laserNextT += laserOnTimeUs;
						laserState = 1;
					} else {
						// Low
						digitalWrite(PIN_LASER_PWM, LOW);
						//_log[_logCounts] = nt;
						//_log2[_logCounts++] = 'l';
						laserState = 0;
						++laserPulseCount;

						if (_laserTimingCount > 0) {
							laserNextT = _laserTiming[laserPulseCount];
						}
					}
				}
			}

			// Step motor.
			if (nt >= (nextMotorT + 500) / 1000) {
				if (motorState == 0) {
					if (motorCurrent >= absSteps) {
						//_log[_logCounts] = nt;
						//_log2[_logCounts++] = 'D';
						break;
					}

					// High
					digitalWrite(st1.stepPin, HIGH);
					//_log[_logCounts] = nt;
					//_log2[_logCounts++] = 'M';
					nextMotorT += 2000;
					motorState = 1;
				} else {
					// Low
					digitalWrite(st1.stepPin, LOW);
					//_log[_logCounts] = nt;
					//_log2[_logCounts++] = 'm';
					nextMotorT += stepDelayNs - 2000;
					motorState = 0;
					++motorCurrent;
				}
			}
		}

		t = micros() - t;

		// for (int i = 0; i < _logCounts; ++i) {
		// 	Serial.print(_log[i]);
		// 	Serial.print("\t");
		// 	Serial.println(_log2[i]);
		// }

		// sprintf(buff, "Motor steps: %d (%d)\n", motorCurrent, motorSteps);
		// sendMessage(buff);

		// sprintf(buff, "Laser pulses: %d\n", laserPulseCount);
		// sendMessage(buff);

		// sprintf(buff, "Log size: %ld\n", _logCounts);
		// sendMessage(buff);

		//sprintf(buff, "Step delay: %d\n", stepDelayNs);
		//sendMessage(buff);

		// for (int i = 0; i < _laserPosCount; ++i) {
		// 	Serial.print(_laserPos[i]);
		// 	Serial.print("\t");
		// 	Serial.println(_laserTiming[i]);
		// }

		// NOTE: Accounts for both drifting phases.
		if (dir) {
			st1.currentStep += motorCurrent;
		} else {
			st1.currentStep -= motorCurrent;
		}

		//--------------------------------------------------------------------------------
		// Deccel phase.
		//--------------------------------------------------------------------------------
		// TODO: Drift.

		st1.moveRelative(preSteps, speed, 0, speed);
		while (st1.updateStepper());

		sendCmdSuccess();
	} else {
		sprintf(buff, "Unknown cmd %d %d", cmdId, Len);
		sendMessage(buff);
	}
}

void updatePacketInput() {
	int b = Serial.read();

	if (b != -1) {
		if (_recvState == RS_START) {
			if (b == FRAME_START) {
				_recvState = RS_COUNT0;
				_recvLen = 0;
				_recvPayloadSize = 0;
			} else {
				// Serial.println("Expected start byte.");
			}
		} else if (_recvState == RS_COUNT0) {
			_recvPayloadSize = b;
			_recvState = RS_COUNT1;
		} else if (_recvState == RS_COUNT1) {
			_recvPayloadSize |= ((int16_t)b) << 8;
			_recvState = RS_PAYLOAD;
		} else if (_recvState == RS_PAYLOAD) {
			_recvBuffer[_recvLen++] = b;
			
			if (_recvLen == _recvPayloadSize) {
				_recvState = RS_END;
			}
		} else if (_recvState == RS_END) {
			if (b == FRAME_END) {
				processCmd(_recvBuffer, _recvPayloadSize);
			} else {
				// Serial.println("Recv packet failure.");
			}

			_recvState = RS_START;
		}
	}
}

void loop() {
	// No mode.
	//while (1) {
	//}

	// Comms mode.
	sendMessage("Controller started");
	while (1) {
		updatePacketInput();
	}

	// Manual mode.
	while (1) {
		int b = Serial.read();

		if (b == 'q') {
			st1.moveRelative(10000, 0, 0, 480);
			while (st1.updateStepper());
		} else if (b == 'w') {
			st1.moveRelative(-10000, 0, 0, 480);
			while (st1.updateStepper());
		}

		if (b == 'a') {
			st2.moveRelative(3000, 0, 0, 100);
			while (st2.updateStepper());
		} else if (b == 's') {
			st2.moveRelative(-3000, 0, 0, 100);
			while (st2.updateStepper());
		}

		if (b == 'z') {
			st3.moveRelative(1000, 0, 0, 30);
			while (st3.updateStepper());
		} else if (b == 'x') {
			st3.moveRelative(-1000, 0, 0, 30);
			while (st3.updateStepper());
		}

		if (b == 'h') {
			st1.home(50, 100, false, 0, 0, 20000);
			st2.home(50, 100, true, 15000, 0, 15000);
			st3.home(50, 30, true, 24000, 0, 24000);

			st1.moveTo(10000, 0, 0, 200);
			st2.moveTo(7500, 0, 0, 100);
			st3.moveTo(1000, 0, 0, 50);

			bool s1 = true;
			bool s2 = true;
			bool s3 = true;

			while (s1 || s2 || s3) {
				s1 = st1.updateStepper();
				s2 = st2.updateStepper();
				s3 = st3.updateStepper();
			}

			Serial.println("Done all homing");
		}
	}
}