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

#define PIN_STP_EN1         32
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

void WriteDacA(int Value) {
	if (Value < 0) Value = 0;
	else if (Value > 4095) Value = 4095;

	uint16_t cmd = 0;
	// Gain to 1x.
	cmd |= 1 << 13;
	// Turn on.
	cmd |= 1 << 12;
	// Output: 0 - 4095.
	cmd |= Value;

	// NOTE: 2MHz due to prototype setup.
	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
	digitalWrite(8, LOW);
	SPI.transfer16(cmd);
	digitalWrite(8, HIGH);
	SPI.endTransaction();
}

void WriteDacB(int Value) {
	if (Value < 0) Value = 0;
	else if (Value > 4095) Value = 4095;

	uint16_t cmd = 0;
	// Write DACb
	cmd |= 1 << 15;
	// Gain to 1x.
	cmd |= 1 << 13;
	// Turn on.
	cmd |= 1 << 12;
	// Output: 0 - 4095.
	cmd |= Value;

	// NOTE: 2MHz due to prototype setup.
	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
	digitalWrite(8, LOW);
	SPI.transfer16(cmd);
	digitalWrite(8, HIGH);
	SPI.endTransaction();
}

//--------------------------------------------------------------------------------
// Setup.
//--------------------------------------------------------------------------------
void setup() {
	Serial.begin(921600);
	
	while (!Serial);
	Serial.println("Galvo test");

	// Setup.
	pinMode(8, OUTPUT);
	digitalWrite(8, HIGH);
	SPI.begin();

	//WriteDacA(2048);
	//WriteDacB(2048);
	//-5.0
	//3.33

	float t = 0.0f;
	unsigned long prevTime = micros();

	unsigned long timer0 = 0;

	double totalTime = 0;

	double timeAvg = 0;
	int timeAvgCount = 0;

	
	// Square test
	int sqrSize = 200;
	int timing = 4000;
	// while (1) {
	// 	unsigned long m0 = micros();
	// 	unsigned long tD = m0 - prevTime;
	// 	prevTime = m0;
	// 	totalTime += tD / 1000.0;
		
	// 	t = totalTime * 0.01;
		
	// 	float sX = sin(t * 0.5) * 1000;
	// 	float sY = cos(t * 0.5) * 1000;

	// 	int oX = 2000;// + (int)sX;
	// 	int oY = 2000 + (int)sY;
	// 	sqrSize = 100;// + sin(t * 0.1f) * 50;

	// 	WriteDacA(oX - sqrSize);
	// 	WriteDacB(oY - sqrSize);
	// 	delayMicroseconds(timing);

	// 	WriteDacA(oX + sqrSize);
	// 	WriteDacB(oY - sqrSize);
	// 	delayMicroseconds(timing);

	// 	WriteDacA(oX + sqrSize);
	// 	WriteDacB(oY + sqrSize);
	// 	delayMicroseconds(timing);

	// 	WriteDacA(oX - sqrSize);
	// 	WriteDacB(oY + sqrSize);
	// 	delayMicroseconds(timing);
	// }

	int offsetX = 0;
	int offsetY = 0;
	int dacOY = 2048;
	int dacOX = 2048;

	// Square.
	// while (1) {
	// 	unsigned long m0 = micros();
	// 	unsigned long tD = m0 - prevTime;
	// 	prevTime = m0;
	// 	totalTime += tD / 1000.0;
		
	// 	int linePoints = 100;
	// 	float pointWidth = 30;
	// 	int timing = 50;

	// 	WriteDacA(dacOY - (int)(linePoints / 2.0 * pointWidth));
	// 	for (int i = 0; i < linePoints; ++i) {
	// 		WriteDacB(dacOX - (int)((-linePoints / 2.0 + i) * pointWidth));
	// 		delayMicroseconds(timing);
	// 	}

	// 	delayMicroseconds(timing);

	// 	for (int i = 0; i < linePoints; ++i) {
	// 		WriteDacA(dacOY + (int)((-linePoints / 2.0 + i) * pointWidth));
	// 		delayMicroseconds(timing);
	// 	}
	// 	delayMicroseconds(timing);

	// 	for (int i = 0; i < linePoints; ++i) {
	// 		WriteDacB(dacOX - (int)((-linePoints / 2.0 + (linePoints - 1 - i)) * pointWidth));
	// 		delayMicroseconds(timing);
	// 	}
	// 	delayMicroseconds(timing);

	// 	for (int i = 0; i < linePoints; ++i) {
	// 		WriteDacA(dacOY + (int)((-linePoints / 2.0 + (linePoints - 1 - i)) * pointWidth));
	// 		delayMicroseconds(timing);
	// 	}
	// 	delayMicroseconds(timing);
	// }

	// Scan test.
	int steps = 200;
	const float maxRangeUm = 50000.0;
	const float stepSizeUm = 50.0;
	const float dacPerStep = 4095.0 / (maxRangeUm / stepSizeUm);

	offsetX = 0;
	offsetY = 0;
	dacOY = 600;
	dacOX = 2048;

	while (1) {
		for (int j = 0; j < 800; ++j) {
			WriteDacA(dacOY + (int)((j + offsetY) * dacPerStep));
			//WriteDacA(4095);
			
			for (int i = 0; i < steps; ++i) {
				// if (i > 50 && i < 150) {
				// 	continue;
				// }

				//WriteDacA(2000 + (int)(i * dacPerStep));
				if (j % 2 == 0) {
					WriteDacB(dacOX + (int)((i + offsetX) * dacPerStep));
				} else {
					WriteDacB(dacOX + (int)((steps - 1 - i + offsetX) * dacPerStep));
				}
				delayMicroseconds(50);
			}
		}
	}

	while (1) {
		unsigned long m0 = micros();
		unsigned long tD = m0 - prevTime;
		prevTime = m0;
		totalTime += tD / 1000.0;
		
		t = totalTime * 0.01;

		float s1 = sin(t * 0.025f) * 0.5 + 0.5;
		float s2 = cos(t) * 0.5 + 0.5;
		
		int outputA = 1000 + (int)(s1 * 2000);
		int outputB = 1000 + (int)(s2 * 2000);

		unsigned long t0 = micros();
		WriteDacA(outputA);
		t0 = micros() - t0;
		timeAvg += t0;
		++timeAvgCount;

		WriteDacB(outputB);

		delayMicroseconds(100);
	}
		
	return;
	
	if (!st1.init(32, 1, 600, 0.00625, 800.0, true)) sendMessage("Bad driver X");
	if (!st2.init(32, 1, 600, 0.00625, 800.0, false)) sendMessage("Bad driver Y");
	if (!st3.init(256, 1, 200, 0.00625, 800.0, true)) sendMessage("Bad driver A");

	// Enable steppers.
	pinMode(PIN_STP_EN1, OUTPUT);
	digitalWrite(PIN_STP_EN1, LOW);

	// Enable laser.
	pinMode(PIN_LASER_PWM, OUTPUT);
	digitalWrite(PIN_LASER_PWM, LOW);

	sendMessage("Controller started");
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
	uint8_t buffer[4] = { FRAME_START, 1, 10, FRAME_END };
	Serial.write(buffer, 4);
}

void processCmd(uint8_t* Buffer, int Len) {
	if (Len == 0)
		return;

	char buff[1024];	
	
	uint8_t cmdId = Buffer[0];

	if (cmdId == 0 && Len == 2) {
		uint8_t axisId = Buffer[1];
		sprintf(buff, "Home on: %d", axisId);
		sendMessage(buff);

		// if (axisId == AXIS_ID_X) homeX();
		// else if (axisId == AXIS_ID_Y) homeY();
		// else if (axisId == AXIS_ID_Z) homeZ();
		// else if (axisId == AXIS_ID_A) homeA();
		// else if (axisId == AXIS_ID_B) homeB();
		// else return;

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
	} else if (cmdId == 3 && Len == 1) {
		// Zero.
		st1.zero();
		st2.zero();
		st3.zero();
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
			// M
			// 0 step is middle position, also 0.0 mm
			// 200 mm away from target.
			// convert position to steps.
			// 256 * 200 = 51200 steps per rev
			// Optical angle is double mechanical angle.
			// Gear ratio of 3:1
			// 0.00703125 degs per step

			const double focalDistance = 200.0;
			double rads = atan(position / focalDistance);

			double mechRads = (rads * 3.0) / 2.0;
			double stepsPerRad = 51200.0 / (360 * M_PI / 180.0);
			double steps = mechRads * stepsPerRad;
			int32_t targetStep = (int32_t)round(steps);

			// sprintf(buff, "M: %f %f %f %d", position, rads, steps, targetStep);
			// sendMessage(buff);
						
			st3.moveDirect(targetStep, speed);
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
		// Mirror raster line.
		int32_t stopCounts = 0;
		int32_t onTime = 0;
		int32_t offTime = 0;
		int32_t stepTime = 0;

		memcpy(&stopCounts, Buffer + 1, 4);
		memcpy(&onTime, Buffer + 5, 4);
		memcpy(&offTime, Buffer + 9, 4);
		memcpy(&stepTime, Buffer + 13, 4);

		// Convert all stops to step target buffer.
		const double focalDistance = 200.0;
		const double stepsPerRad = 51200.0 / (360 * M_PI / 180.0);

		unsigned long t = micros();

		for (int i = 0; i < stopCounts; ++i) {
			float position = 0.0f;
			memcpy(&position, Buffer + 17 + i * 4, 4);

			double rads = atan(position / focalDistance);
			double mechRads = (rads * 3.0) / 2.0;
			
			double steps = mechRads * stepsPerRad;
			_targetSteps[i] = (int32_t)round(steps);
		}

		t = micros() - t;

		sprintf(buff, "Mirror raster: %d %d %d %d %ld us", stopCounts, onTime, offTime, stepTime, t);
		sendMessage(buff);

		// digitalWrite(PIN_LASER_PWM, HIGH);

		for (int i = 0; i < stopCounts; ++i) {
			// sprintf(buff, "Stop: %d %d", i, _targetSteps[i]);
			// sendMessage(buff);

			st3.moveDirect(_targetSteps[i], stepTime);
			delayMicroseconds(offTime);
			digitalWrite(PIN_LASER_PWM, HIGH);
			delayMicroseconds(onTime);
			digitalWrite(PIN_LASER_PWM, LOW);

			// for (int j = 0; j < 4000; ++j) {
			// 	digitalWrite(PIN_LASER_PWM, HIGH);
			// 	delayMicroseconds(10);
			// 	digitalWrite(PIN_LASER_PWM, LOW);
			// 	delayMicroseconds(5);
			// }
		}

		// digitalWrite(PIN_LASER_PWM, LOW);

		// Run raster.
		// NOTE: offTime is delay before firing.
		
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

int stepCount = 0;
bool lastDir = true;

void loop() {
	while (1);

	while (1) {
		updatePacketInput();
	}

	while (1) {
		// Max move is 30 degrees.
		// 12800 steps per 30 degrees.
		// 77.656 mm covered with 30 degrees at 150mm.
		// 1553 x 50 um steps.
		// ~8 steps per 50um.

		int b = Serial.read();

		if (b == 'a') {
			st3.setDirection(true);
			for (int i = 0; i < 1000; ++i) {
				digitalWrite(15, HIGH);
				delayMicroseconds(5);
				digitalWrite(15, LOW);
				delayMicroseconds(1);
			}

			st3.setDirection(lastDir);
		} else if (b == 'b') {
			st3.setDirection(true);
			for (int i = 0; i < 10000; ++i) {
				digitalWrite(15, HIGH);
				delayMicroseconds(5);
				digitalWrite(15, LOW);
				delayMicroseconds(1);
			}

			st3.setDirection(lastDir);
		}

		// st3.pulseStepper();
		digitalWrite(15, HIGH);
		delayMicroseconds(5);
		digitalWrite(15, LOW);
		delayMicroseconds(1);
		++stepCount;

		if (stepCount % 8 == 0) {
			// Fire laser
			delayMicroseconds(250);

			// if ((stepCount / 100) % 2 == 0) {
				// delay(1);
			// }
			//Serial.println("Next");
		}

		// if (stepCount == 12800) {
		if (stepCount == 4000) {
			stepCount = 0;
			lastDir = !lastDir;
			st3.setDirection(lastDir);
			// delay(100);
		}
	}
}