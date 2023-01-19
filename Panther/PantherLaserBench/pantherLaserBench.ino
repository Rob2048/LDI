//--------------------------------------------------------------------------------
// Laser test bench controller.
//--------------------------------------------------------------------------------

// Steppers:
// 1 cs 33 stp 34 dir 35
// 2 cs 37 stp 38 dir 39
// 3 cs 14 stp 15 dir 16
// 4 cs 20 stp 19 dir 18
// 5 cs 2 stp 1 dir 0
// 6 cs 8 stp 7 dir 6
// 7 cs 27 stp 26 dir 25
// 8 cs 31 stp 30 dir 29

// Endstops:
// 1 36
// 2 9
// 3 17
// 4 10
// 5 5
// 6 24
// 7 4
// 8 28

// Aux pins:
// 1 21 (A7)
// 2 22 (A8)
// 3 23 (A9)
// 4 NC
// 5 3

// Test limit input.
// 1 21 (A7)
// 2 22 (A8)


//--------------------------------------------------------------------------------

#define PIN_STP_EN1				32
#define PIN_LASER_PWM			3
#define PIN_GALVO_CHIPSELECT	8
#define PIN_DBG1				21
#define PIN_DBG2				22
#define PIN_DBG3				23

#include <SPI.h>
#include <stdint.h>
#include <math.h>
#include "stepper.h"

#define AXIS_ID_X			0
#define AXIS_ID_Y			1
#define AXIS_ID_Z			2
#define AXIS_ID_A			3
#define AXIS_ID_B			4

abStepper st1 = abStepper(0, 33, 34, 35, 36);
abStepper st2 = abStepper(0, 37, 38, 39, 9);
abStepper st3 = abStepper(0, 14, 15, 16, 17);
abStepper st4 = abStepper(0, 20, 19, 18, 10);

ldiStepTable stepLogTable;

//--------------------------------------------------------------------------------
// Galvo
//--------------------------------------------------------------------------------
// NOTE: DAC can go at 20Mhz SPI.
// DAC takes 4.5us to settle.
// 83us for SPI and settle.

int _galvoStepX = 2048;
int _galvoStepY = 2048;

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
	digitalWrite(PIN_GALVO_CHIPSELECT, LOW);
	SPI.transfer16(cmd);
	digitalWrite(PIN_GALVO_CHIPSELECT, HIGH);
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
	digitalWrite(PIN_GALVO_CHIPSELECT, LOW);
	SPI.transfer16(cmd);
	digitalWrite(PIN_GALVO_CHIPSELECT, HIGH);
	SPI.endTransaction();
}

void galvoInit() {
	// Enable chip select.
	pinMode(PIN_GALVO_CHIPSELECT, OUTPUT);
	digitalWrite(PIN_GALVO_CHIPSELECT, LOW);

	SPI.begin();

	_galvoStepX = 2048;
	_galvoStepY = 2048;

	WriteDacA(_galvoStepX);
	WriteDacB(_galvoStepY);
}

void galvoMoveSteps(int StepX, int StepY) {
	int wait = 0;
	int maxSteps = max(abs(_galvoStepX - StepX), abs(_galvoStepY - StepY));

	if (_galvoStepX	!= StepX) {
		_galvoStepX = StepX;
		WriteDacB(_galvoStepX);
	}

	if (_galvoStepY	!= StepY) {
		_galvoStepY = StepY;
		WriteDacA(_galvoStepY);
	}

	if (maxSteps >= 1024) {
		wait = 50000;
	} else if (maxSteps >= 512) {
		wait = 3000;
	} else if (maxSteps > 0) {
		wait = 800;
	}

	delayMicroseconds(wait);
}

int galvoGetStep(float Value) {
	// 0 to 4095 = 0mm to 40mm
	float stepsPerMm = 4095.0 / 40.0;
	int result = (int)(Value * stepsPerMm + 0.5f);

	return result;
}

// Movement value in mm from 0 to 40.
void galvoMove(float X, float Y) {
	int targetStepX = galvoGetStep(X);
	int targetStepY = galvoGetStep(40.0f - Y);
	galvoMoveSteps(targetStepX, targetStepY);
}

void galvoPreviewSquare() {
	digitalWrite(PIN_LASER_PWM, LOW);
	pinMode(PIN_LASER_PWM, OUTPUT);
	analogWriteFrequency(PIN_LASER_PWM, 10000);
	analogWrite(PIN_LASER_PWM, 40);

	for (int j = 0; j < 100; ++j) {
		for (int i = 0; i <= 128; ++i) {
			WriteDacA(i * 32);
			delayMicroseconds(50);
		}

		for (int i = 0; i <= 128; ++i) {
			WriteDacB(i * 32);
			delayMicroseconds(50);
		}

		for (int i = 0; i <= 128; ++i) {
			WriteDacA(4096 - i * 32);
			delayMicroseconds(50);
		}

		for (int i = 0; i <= 128; ++i) {
			WriteDacB(4096 - i * 32);
			delayMicroseconds(50);
		}
	}

	pinMode(PIN_LASER_PWM, OUTPUT);
	digitalWrite(PIN_LASER_PWM, LOW);

	WriteDacA(2048);
	WriteDacB(2048);
	_galvoStepX = 2048;
	_galvoStepY = 2048;
}

void galvoPreviewTarget() {
	digitalWrite(PIN_LASER_PWM, LOW);
	pinMode(PIN_LASER_PWM, OUTPUT);
	analogWriteFrequency(PIN_LASER_PWM, 10000);
	analogWrite(PIN_LASER_PWM, 0);

	WriteDacA(0);
	WriteDacB(2048);
	delayMicroseconds(600);

	for (int j = 0; j < 100; ++j) {
		analogWrite(PIN_LASER_PWM, 40);
		for (int i = 0; i <= 128; ++i) {
			WriteDacA(i * 32);
			delayMicroseconds(50);
		}
		analogWrite(PIN_LASER_PWM, 0);
		WriteDacA(2048);
		WriteDacB(0);
		delayMicroseconds(600);

		analogWrite(PIN_LASER_PWM, 40);
		for (int i = 0; i <= 128; ++i) {
			WriteDacB(i * 32);
			delayMicroseconds(50);
		}
		analogWrite(PIN_LASER_PWM, 0);

		WriteDacA(0);
		WriteDacB(2048);
		delayMicroseconds(600);
	}
	
	pinMode(PIN_LASER_PWM, OUTPUT);
	digitalWrite(PIN_LASER_PWM, LOW);
}

//--------------------------------------------------------------------------------
// Functionality.
//--------------------------------------------------------------------------------
void homing() {
	st1.moveRelative(-100000, 50);
	st2.moveRelative(-100000, 50);
	st3.moveRelative(100000, 50);

	bool s1 = true;
	bool s2 = true;
	bool s3 = true;

	while (s1 || s2 || s3) {
		if (s1) { s1 = moveStepperUntilLimit(&st1); }
		if (s2) { s2 = moveStepperUntilLimit(&st2); }
		if (s3) { s3 = moveStepperUntilLimit(&st3); }
	}

	st1.home(50, 100, false, 0, 0, 20000);
	st2.home(50, 100, false, 0, 0, 15000);
	st3.home(50, 30, true, 17000, 0, 17000);

	// Move to home position.
	st1.moveTo(15000, 200);
	st2.moveTo(5000, 100);
	st3.moveTo(14400, 50);

	s1 = true;
	s2 = true;
	s3 = true;

	while (s1 || s2 || s3) {
		s1 = st1.updateStepper();
		s2 = st2.updateStepper();
		s3 = st3.updateStepper();
	}

	// Zero home position.
	st1.currentStep = 0;
	st2.currentStep = 0;
	st3.currentStep = 0;
}

void homingTest() {
	st1.moveRelative(-100000, 10);
	
	while (moveStepperUntilLimit(&st1));

	float dist = st1.currentStep * st1.mmPerStep;
	dist *= 1000.0f;

	// Target count for X axis: 420 TMC steps.
	int tmcSteps = st1.getMicrostepCount();
	int tmcDiffSteps = 420 - tmcSteps;
	int actualSteps = tmcDiffSteps / 8;

	char buff[256];
	sprintf(buff, "Current step: %ld Dist: %.0f um (%d) Actual: %d\r\n", st1.currentStep, dist, st1.getMicrostepCount(), actualSteps);
	Serial.print(buff);

	st1.home(50, 100, false, 0, 0, 20000);

	st1.moveTo(st1.stepsPerMm * 30, 200);
	while (st1.updateStepper());
}

void testLimitSwitches() {
	int lim1 = digitalRead(st1.limitPin);
	int lim2 = digitalRead(st2.limitPin);
	int lim3 = digitalRead(st3.limitPin);

	Serial.print(lim1);
	Serial.print(" ");
	Serial.print(lim2);
	Serial.print(" ");
	Serial.println(lim3);
	Serial.print(" ");
}

//--------------------------------------------------------------------------------
// Packet processing.
//--------------------------------------------------------------------------------

// Requests:
// 0: ping
// 1: depreacted move with accel
// 3: move relative (no accel)
// 4: move (no accel)
// 5: burst laser
// 6: pulse laser
// 7: stop laser
// 8: raster move/laser (no accel)
// 10: home all axes
// 11: move to center
// 13: modulate laser
// 15: halftone raster
// 16: galvo preview
// 17: move galvo
// 18: galvo burn batch points
		
// Responses:
// 10: cmd recv success.
// 13: utf8 message.

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
float _targetStartX;

void sendMessage(const char* Message) {
	uint8_t buffer[256];
	int strLen = 0;

	while (1) {
		char c = Message[strLen];

		buffer[3 + strLen] = Message[strLen];
		++strLen;

		if (c == 0)
			break;
	}

	buffer[0] = FRAME_START;
	buffer[1] = strLen + 1;
	buffer[2] = 13;
	buffer[strLen + 3] = FRAME_END;

	Serial.write(buffer, strLen + 4);
}

unsigned long _t0 = 0;
unsigned long _sendTimer = 0;

void sendCmdSuccess() {
	unsigned long t0 = micros();

	uint8_t buffer[20];
	buffer[0] = FRAME_START;
	buffer[1] = 17;
	buffer[2] = 10;
	buffer[sizeof(buffer) - 1] = FRAME_END;

	float s1p = st1.currentStep * st1.mmPerStep;
	float s2p = st2.currentStep * st2.mmPerStep;
	float s3p = st3.currentStep * st3.mmPerStep;

	memcpy(buffer + 3, &s1p, 4);
	memcpy(buffer + 7, &s2p, 4);
	memcpy(buffer + 11, &s3p, 4);
	memcpy(buffer + 15, &_t0, 4);
	
	Serial.write(buffer, sizeof(buffer));

	_t0 = micros() - t0;
}

void sendCmdPosition() {
	uint8_t buffer[16];
	buffer[0] = FRAME_START;
	buffer[1] = 13;
	buffer[2] = 69;
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
	} else if (cmdId == 1) {
		// NOTE: Status command.
		
		sprintf(buff, "Panther diagnostics:\nST1: %d\nST2: %d\nST3: %d\nunsigned long: %d", st1.active, st2.active, st3.active, sizeof(unsigned long));
		sendMessage(buff);

		sendCmdSuccess();


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
	} else if (cmdId == 2) {
		int axisId = Buffer[1];
		int32_t stepTarget;
		float maxVelocity;

		memcpy(&stepTarget, Buffer + 2, 4);
		memcpy(&maxVelocity, Buffer + 6, 4);

		//sprintf(buff, "Cmd2: axisId: %d stepTarget: %ld maxVelocity: %f", axisId, stepTarget, maxVelocity);
		//sendMessage(buff);

		abStepper* stepper;

		if (axisId == 0) {
			stepper = &st1;
		} else if (axisId == 1) {
			stepper = &st2;
		} else if (axisId == 2) {
			stepper = &st3;
		} else if (axisId == 3) {
			stepper = &st4;
		} else {
			// TODO: Cmd failed.
		}

		stepper->moveRelative(stepTarget, maxVelocity);
		while (stepper->updateStepper()) {
			unsigned long t0 = millis();

			if (t0 >= _sendTimer) {
				_sendTimer = t0 + 20;
				sendCmdPosition();
			}
		}

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
		// Pulse laser.
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
		pinMode(PIN_LASER_PWM, OUTPUT);
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

		homing();
		
		sendCmdSuccess();
	} else if (cmdId == 11) {
		// Move to zero.
		st1.moveTo(0, 200);
		st2.moveTo(0, 100);
		st3.moveTo(0, 50);
		
		bool s1 = true;
		bool s2 = true;
		bool s3 = true;

		while (s1 || s2 || s3) {
			s1 = st1.updateStepper();
			s2 = st2.updateStepper();
			s3 = st3.updateStepper();
		}

		sendCmdSuccess();
	} else if (cmdId == 13 && Len == 9) {
		// Modulate laser.
		int32_t frequency = 0;
		int32_t duty = 0;
		memcpy(&frequency, Buffer + 1, 4);
		memcpy(&duty, Buffer + 5, 4);

		digitalWrite(PIN_LASER_PWM, LOW);

		if (frequency > 0 && frequency <= 100000 && duty >= 0 && duty <= 255) {
			pinMode(PIN_LASER_PWM, OUTPUT);
			analogWriteFrequency(PIN_LASER_PWM, frequency);
			analogWrite(PIN_LASER_PWM, duty);
		}

		// sprintf(buff, "Laser: %ld %ld %ld", onTime, offTime, counts);
		// sendMessage(buff);

		sendCmdSuccess();
	} else if (cmdId == 15 && Len >= 8) {
		// Scan with stops at every pulse position.
		int32_t stopCounts = 0;
		int32_t offTime = 0;
		int32_t stepTime = 0;
		float startX = 0.0f;
		float stopSize = 0.0f;
		
		memcpy(&stopCounts, Buffer + 1, 4);
		memcpy(&offTime, Buffer + 5, 4);
		memcpy(&stepTime, Buffer + 9, 4);
		memcpy(&startX, Buffer + 13, 4);
		memcpy(&stopSize, Buffer + 17, 4);
		uint16_t* stopTimes = (uint16_t*)(Buffer + 21);

		for (int i = 0; i < stopCounts; ++i) {
			uint16_t stopTime = stopTimes[i];

			if (stopTime == 0) {
				continue;
			}

			float pos = startX + i * stopSize;
			int32_t targetStep = (int32_t)round(pos * st1.stepsPerMm);

			st1.moveDirect(targetStep, stepTime);
			delayMicroseconds(offTime);
			digitalWrite(PIN_LASER_PWM, HIGH);
			delayMicroseconds(stopTime);
			digitalWrite(PIN_LASER_PWM, LOW);
		}

		sendCmdSuccess();
	} else if (cmdId == 16) {
		galvoPreviewSquare();
		sendCmdSuccess();
	} else if (cmdId == 17) {
		float posX = 0.0f;
		float posY = 0.0f;

		memcpy(&posX, Buffer + 1, 4);
		memcpy(&posY, Buffer + 5, 4);

		galvoMove(posX, posY);

		sendCmdSuccess();
	} else if (cmdId == 18) {
		digitalWrite(PIN_DBG1, HIGH);
		sendCmdSuccess();

		int count = 0;

		memcpy(&count, Buffer + 1, 4);
		
		for (int i = 0; i < count; ++i) {
			digitalWrite(PIN_DBG2, HIGH);
			float x = 0;
			float y = 0;
			int burnTime = 0;

			memcpy(&x, Buffer + 5 + i * 10 + 0, 4);
			memcpy(&y, Buffer + 5 + i * 10 + 4, 4);
			memcpy(&burnTime, Buffer + 5 + i * 10 + 8, 2);

			digitalWrite(PIN_DBG3, HIGH);
			galvoMove(x, y);
			digitalWrite(PIN_DBG3, LOW);
			digitalWrite(PIN_LASER_PWM, HIGH);
			delayMicroseconds(burnTime);
			digitalWrite(PIN_LASER_PWM, LOW);

			digitalWrite(PIN_DBG2, LOW);
		}

		digitalWrite(PIN_DBG1, LOW);
	} else if (cmdId == 20) {
		// Read motor status test.

		abStepper* stepper = &st2;

		unsigned long t0 = micros();
		uint32_t status = stepper->getDriverStatus();
		t0 = micros() - t0;

		// bool standStill = status & (1 << 31);
		// bool openLoadPhaseB = status & (1 << 30);
		// bool openLoadPhaseA = status & (1 << 29);
		// bool shortPhaseB = status & (1 << 28);
		// bool shortPhaseA = status & (1 << 27);
		// bool overTempPreWarn = status & (1 << 26);
		// bool overTemp = status & (1 << 25);
		bool stallGuard = status & (1 << 24);
		// int actualCurrent = (status & 0x1F0000) >> 16;
		int stallGuardResult = status & 0x3FF;

		bool sfilt = stepper->getSfilt();
		int8_t sgt = stepper->getSgt();
		
		sprintf(buff, "Stall guard: %d %d %d %d %lu", stallGuard, stallGuardResult, sfilt, sgt, t0);
		sendMessage(buff);
		
		sendCmdSuccess();
	} else if (cmdId == 21) {
		// NOTE: Move direct relative.
		int axisId = Buffer[1];
		int32_t stepTarget;
		int32_t delay;

		memcpy(&stepTarget, Buffer + 2, 4);
		memcpy(&delay, Buffer + 6, 4);

		//sprintf(buff, "Cmd2: axisId: %d stepTarget: %ld maxVelocity: %f", axisId, stepTarget, maxVelocity);
		//sendMessage(buff);

		abStepper* stepper;

		if (axisId == 0) {
			stepper = &st1;
		} else if (axisId == 1) {
			stepper = &st2;
		} else if (axisId == 2) {
			stepper = &st3;
		} else {
			// TODO: Cmd failed, invalid axis.
		}

		stepper->moveDirectRelative(stepTarget, delay);
		
		sendCmdSuccess();
	} else if (cmdId == 22) {
		// NOTE: Move relative and output/record step log table.

		int axisId = Buffer[1];
		int32_t stepTarget;
		float maxVelocity;

		memcpy(&stepTarget, Buffer + 2, 4);
		memcpy(&maxVelocity, Buffer + 6, 4);

		//sprintf(buff, "Cmd2: axisId: %d stepTarget: %ld maxVelocity: %f", axisId, stepTarget, maxVelocity);
		//sendMessage(buff);

		abStepper* stepper;

		if (axisId == 0) {
			stepper = &st1;
		} else if (axisId == 1) {
			stepper = &st2;
		} else if (axisId == 2) {
			stepper = &st3;
		} else {
			// TODO: Cmd failed.
		}

		stepLogTable.size = 0;

		stepper->moveRelative(stepTarget, maxVelocity);
		while (stepper->updateStepperLog(&stepLogTable)) {
			unsigned long t0 = millis();
			if (t0 >= _sendTimer) {
				_sendTimer = t0 + 20;
				//sendCmdPosition();

				{
					static unsigned long prevTime = 0;
					unsigned long t0 = micros();

					uint8_t buffer[255];
					buffer[0] = 255;
					buffer[1] = 252;
					buffer[2] = 70;
					buffer[sizeof(buffer) - 1] = 254;
					memcpy(buffer + 3, &stepper->currentStep, 4);
					
					Serial.write(buffer, sizeof(buffer));

					prevTime = micros() - t0;
				}
			}
		}

		sendCmdSuccess();
	} else if (cmdId == 23) {
		// st1.moveRelative(-10000, 10);
		// while (moveStepperUntilLimit(&st1));
		// sendCmdSuccess();

		// float dist = st1.currentStep * st1.mmPerStep;
		// dist *= 1000.0f;

		// // Target count for X axis: 420 TMC steps.
		// int tmcSteps = st1.getMicrostepCount();
		// int tmcDiffSteps = 420 - tmcSteps;
		// int actualSteps = tmcDiffSteps / 8;

		// char buff[256];
		// sprintf(buff, "Current step: %ld Dist: %.0f um (%d) Actual: %d\r\n", st1.currentStep, dist, st1.getMicrostepCount(), actualSteps);
		// Serial.print(buff);

		abStepper* steppers[] = {
			&st1,
			&st2,
			&st3
		};

		bool homeDir[] = {
			false,
			false,
			true
		};

		for (int i = 0; i < 3; ++i) {
			steppers[i]->home(10, 30, homeDir[i], 0, 0, 20000);
			int tmcSteps = steppers[i]->getMicrostepCount();
			sprintf(buff, "Step pos: %d", tmcSteps);
			sendMessage(buff);
		}

		// Move to home position.
		st1.moveTo(60000, 30);
		st2.moveTo(90000, 30);
		st3.moveTo(-80000, 30);

		bool s1 = true;
		bool s2 = true;
		bool s3 = true;

		while (s1 || s2 || s3) {
			s1 = st1.updateStepper();
			s2 = st2.updateStepper();
			s3 = st3.updateStepper();
		}

		// Zero home position.
		st1.currentStep = 0;
		st2.currentStep = 0;
		st3.currentStep = 0;

		//st1.moveTo(st1.stepsPerMm * 30, 30);
		//while (st1.updateStepper());

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

//--------------------------------------------------------------------------------
// Setup.
//--------------------------------------------------------------------------------
void setup() {
	Serial.begin(921600);

	// NOTE: Y on LCS4.
	st1.init(32, 1, 900, 0.00125, 200.0, false);
	st2.init(32, 1, 900, 0.00125, 200.0, false);
	st3.init(32, 1, 900, 0.00125, 200.0, true);

	// A
	// 1:30
	st4.init(32, 1, 900, 0.00125, 200.0, false);

	// st1.init(32, 0, 900, 0.00625, 10000.0, true);

	// Enable debug pins
	// pinMode(PIN_DBG1, OUTPUT);
	// pinMode(PIN_DBG2, OUTPUT);
	// pinMode(PIN_DBG3, OUTPUT);

	// Enable steppers.
	pinMode(PIN_STP_EN1, OUTPUT);
	digitalWrite(PIN_STP_EN1, LOW);

	// Enable laser.
	pinMode(PIN_LASER_PWM, OUTPUT);
	digitalWrite(PIN_LASER_PWM, LOW);

	// Enable galvo.
	galvoInit();
	galvoMove(20, 20);
}

int getMicroPhase(int Step) {
	int result = ((Step * 8) % 1024) + 4;

	if (result < 0) {
		result += 1024;
	}

	return result;
}

//--------------------------------------------------------------------------------
// Loop.
//--------------------------------------------------------------------------------
void loop() {
	// Comms mode.
	while (1) {
		updatePacketInput();
	}

	// NOTE: Limit switch inputs.
	pinMode(21, INPUT_PULLDOWN);
	pinMode(22, INPUT_PULLDOWN);
	
	// while (1) {
	// 	int a0 = analogRead(7);
	// 	int a1 = analogRead(8);
	// 	// int a0 = digitalRead(21);
	// 	// int a1 = digitalRead(22);

	// 	st4.pulseStepper();
		
	// 	uint32_t t0 = millis();
	// 	if (t0 >= timeRep) {
	// 		timeRep = t0 + 20;
	// 		Serial.print(a0);
	// 		Serial.print(" ");
	// 		Serial.println(a1);
	// 	}

	// 	delayMicroseconds(300);
	// }

	uint16_t startMicrostep = st4.getMicrostepCount();
	st4.currentStep = (startMicrostep - 4) / 8;// % 128;

	Serial.printf("Step: %d Micro: %d\n", st4.currentStep, startMicrostep);

	// Manual mode.
	while (1) {
		int b = Serial.read();

		if (b == 'a' || b == 's') {
			st4.setDirection(b == 'a');
			uint32_t timeRep = 0;

			int state = 0;
			int startStep = 0;
			int endStep = 0;

			for (int i = 0; i < 6400; ++i) {
				
				// int a0 = digitalRead(21);
				// int a1 = digitalRead(22);

				st4.pulseStepper();
				uint16_t step = st4.getMicrostepCount();

				delayMicroseconds(50);
				int a0 = analogRead(7);
				int a1 = analogRead(8);

				if (st4.currentDir) {
					--st4.currentStep;
				} else {
					++st4.currentStep;
				}

				if (a0 >= 400 && a0 <= 700) {
					Serial.printf("%d:%d %d\n", st4.currentStep, step, a0);
				}

				delayMicroseconds(200);
			}

			Serial.println("------------------------------------------------------");
		}

		if (b == 'e' || b == 'r') {
			st4.setDirection(b == 'r');
			uint32_t timeRep = 0;

			int state = 0;
			int startStep = 0;
			int endStep = 0;
			int startMicroStep = 0;
			int endMicroStep = 0;

			for (int i = 0; i < 6400; ++i) {
				st4.pulseStepper();
				uint16_t step = st4.getMicrostepCount();

				delayMicroseconds(100);
				int a0 = analogRead(7);
				
				if (st4.currentDir) {
					--st4.currentStep;
				} else {
					++st4.currentStep;
				}

				if (state == 0) {
					if (a0 > 600) {
						startStep = st4.currentStep;
						startMicroStep = step;
						state = 1;
					}
				} else if (state == 1) {
					if (a0 < 600) {
						endStep = st4.currentStep;
						endMicroStep = step;
						state = 2;
					}
				}
			}

			if (state == 2) {
				int totalSteps = endStep - startStep;
				int midPoint = startStep + totalSteps / 2;
				int midMicro = startMicroStep + (endMicroStep - startMicroStep) / 2;
				Serial.printf("%d (%d) to %d (%d): Total: %d Mid: %d (%d)\n", startStep % 6400, startMicroStep, endStep % 6400, endMicroStep, totalSteps, midPoint % 6400, midMicro);
			} else {
				Serial.println("No limit detected");
			}

			// Serial.println("------------------------------------------------------");
		}

		if (b == 't' || b == 'y') {
			st4.setDirection(b == 'y');
			
			st4.pulseStepper();
			delayMicroseconds(100);

			if (st4.currentDir) {
				--st4.currentStep;
			} else {
				++st4.currentStep;
			}

			uint16_t step = st4.getMicrostepCount();
			int microPhase = getMicroPhase(st4.currentStep);

			Serial.printf("Step: %d Micro: %d Calc: %d\n", st4.currentStep, step, microPhase);
		}

		if (b == 'q' || b == 'w') {
			st4.setDirection(b == 'q');
			uint32_t timeRep = 0;

			int state = 0;
			int startStep = 0;
			int endStep = 0;

			for (int i = 0; i < 10000; ++i) {
				
				// int a0 = digitalRead(21);
				// int a1 = digitalRead(22);

				st4.pulseStepper();
				uint16_t step = st4.getMicrostepCount();

				delayMicroseconds(50);
				int a0 = analogRead(7);
				int a1 = analogRead(8);

				if (st4.currentDir) {
					--st4.currentStep;
				} else {
					++st4.currentStep;
				}

				if (state == 0) {
					if (a1 >= 600) {
						startStep = st4.currentStep;
						state = 1;
					}
				} else if (state == 1) {
					if (a1 >= 700) {
						state = 2;
					}
				} else if (state == 2) {
					if (a1 <= 600) {
						endStep = st4.currentStep;
						state = 3;
					}
				}

				// if (a1 >= 590 && a1 <= 610) {
				// 	Serial.printf("%d:%d %d\n", st4.currentStep, step, a1);
				// }

				// Serial.print(step);
				// Serial.print(" ");
				// Serial.print(a0);
				// Serial.print(" ");
				// Serial.println(a1);
				
				// uint32_t t0 = millis();
				// if (t0 >= timeRep) {
				// 	timeRep = t0 + 20;
				// 	Serial.print(step);
				// 	Serial.print(" ");
				// 	// Serial.print(a0);
				// 	// Serial.print(" ");
				// 	Serial.println(a1);
				// }

				delayMicroseconds(200);
			}

			if (state == 3) {
				int totalSteps = endStep - startStep;
				int midPoint = startStep + totalSteps / 2;
				Serial.printf("%d to %d: Total: %d Mid: %d\n", startStep, endStep, totalSteps, midPoint);
			} else {
				Serial.println("No limit detected");
			}

			// Serial.println("------------------------------------------------------");
		}

		if (b == 'p') {
			st4.setDirection(true);
			
			while (true) {
				delayMicroseconds(100);
				int a0 = analogRead(7);
				Serial.printf("%d\n", a0);

				if (a0 > 600) {
					break;
				}

				st4.pulseStepper();
				uint16_t step = st4.getMicrostepCount();
				
				if (st4.currentDir) {
					--st4.currentStep;
				} else {
					++st4.currentStep;
				}
			}

			// Serial.println("No limit detected");
		}

		if (b == 'o') {
			// st4.setDirection(true);

			float aAvg = 0.0f;
			
			while (true) {
				int c = Serial.read();

				if (c == 'o') {
					break;
				} else if (c == 'e' || c == 'r') {
					st4.setDirection(c == 'e');
					st4.pulseStepper();
					
					if (st4.currentDir) {
						--st4.currentStep;
					} else {
						++st4.currentStep;
					}
				}

				delay(20);
				// int a0 = analogRead(7);
				// aAvg = aAvg * 0.9f + (float)a0 * 0.1f;
				// Serial.printf("%d %f\n", st4.currentStep, aAvg);

				int innerLimit = digitalRead(21);
				int outerLimit = digitalRead(22);

				Serial.printf("%6d %3d %3d\n", st4.currentStep, innerLimit, outerLimit);
			}

			// Serial.println("No limit detected");
		}

		if (b == 'h') {
			// Find correct revolution.
			int state = 0;
			int startStep = 0;
			int endStep = 0;

			// int innerLimit = analogRead(7);
			// int outerLimit = analogRead(8);
			// int innerLimit = digitalRead(21);
			// int outerLimit = digitalRead(22);

			// Align step to micro phase.
			st4.currentStep = (st4.getMicrostepCount() - 4) / 8;

			Serial.printf("Start home - Step: %d\n", st4.currentStep);

			// NOTE: Back off if we are already inside limit.
			{
				int outerLimit = digitalRead(22);

				if (outerLimit == 1) {
					Serial.println("Inside outer limit");

					st4.moveRelative(-7000, 20.0f);
					while (st4.updateStepper());
				}
			}

			Serial.printf("Start home forward - Step: %d\n", st4.currentStep);

			// NOTE: Only search for one full turn.
			int lastVal = 0;
			st4.setDirection(true);
			for (int i = 0; i < 192000; ++i) {
				st4.pulseStepper();
				uint16_t step = st4.getMicrostepCount();

				delayMicroseconds(200);

				int outerLimit = digitalRead(22);

				if (st4.currentDir) {
					--st4.currentStep;
				} else {
					++st4.currentStep;
				}

				if (outerLimit != lastVal) {
					lastVal = outerLimit;
					Serial.printf("%6d %3d %4d\n", st4.currentStep, outerLimit, step);
				}

				if (state == 0) {
					if (outerLimit == 1) {
						startStep = st4.currentStep;
						state = 1;
					}
				} else if (state == 1) {
					if (abs(st4.currentStep - startStep) > 1000 && outerLimit == 0) {
						endStep = st4.currentStep;
						state = 2;
						break;
					}
				}
			}

			if (state == 2) {
				int midPointStep = startStep + (endStep - startStep) / 2;
				int midMicroPhase = getMicroPhase(midPointStep);

				Serial.printf("Found outer limit: %d to %d (%d to %d) Mid: %4d MidPhase: %4d\n", startStep, endStep, getMicroPhase(startStep), getMicroPhase(endStep), midPointStep, midMicroPhase);

				// TODO: Move to position that is far from inner limit.
				
				// Move back to find inner center.
				st4.setDirection(false);
				lastVal = 0;
				state = 0;
				for (int i = 0; i < 3000; ++i) {
					st4.pulseStepper();
					uint16_t step = st4.getMicrostepCount();
					delayMicroseconds(400);
					int innerLimit = digitalRead(21);

					if (st4.currentDir) {
						--st4.currentStep;
					} else {
						++st4.currentStep;
					}

					if (innerLimit != lastVal) {
						lastVal = innerLimit;
						Serial.printf("%6d %3d %4d\n", st4.currentStep, innerLimit, step);
					}

					if (state == 0) {
						if (innerLimit == 1) {
							startStep = st4.currentStep;
							state = 1;
						}
					} else if (state == 1) {
						if (abs(st4.currentStep - startStep) > 100 && innerLimit == 0) {
							endStep = st4.currentStep;
							state = 2;
							break;
						}
					}
				}

				if (state == 2) {
					int midPointStep = startStep + (endStep - startStep) / 2;
					int midMicroPhase = getMicroPhase(midPointStep);

					Serial.printf("Found inner limit: %d to %d (%d to %d) Mid: %4d MidPhase: %4d\n", startStep, endStep, getMicroPhase(startStep), getMicroPhase(endStep), midPointStep, midMicroPhase);

					// Get new step target from micro phase.
					int adjustedPhase = midMicroPhase - 4;
					int moveSteps = 0;

					if (adjustedPhase >= 512) {
						moveSteps = (1024 - adjustedPhase) / 8;
					} else {
						moveSteps = (-adjustedPhase) / 8;
					}

					int newStepTarget = midPointStep + moveSteps;

					Serial.printf("Move to step: %d (%d)\n", newStepTarget, getMicroPhase(newStepTarget));

					st4.moveTo(newStepTarget, 20.0f);
					while (st4.updateStepper());

					uint16_t ms = st4.getMicrostepCount();
					Serial.printf("Final position: %d (%d)\n", st4.currentStep, ms);

					// Move to zero pos and set as 0 step.
					st4.moveRelative(-10000, 10.0f);
					while (st4.updateStepper());
					st4.currentStep = 0;

				} else {
					Serial.printf("Could not find inner limit\n");
				}
			} else {
				Serial.printf("Could not find outer limit\n");
			}
		}
	}

	// NOTE: Test limit switch.
	// while (true) {
	// 	int x = digitalRead(st1.limitPin);
	// 	int y = digitalRead(st2.limitPin);
	// 	int z = digitalRead(st3.limitPin);

	// 	Serial.print("X ");
	// 	Serial.print(x);

	// 	Serial.print(" Y ");
	// 	Serial.print(y);

	// 	Serial.print(" Z ");
	// 	Serial.println(z);

	// 	delay(100);
	// }

	// digitalWrite(PIN_LASER_PWM, LOW);
	// pinMode(PIN_LASER_PWM, OUTPUT);
	// analogWriteFrequency(PIN_LASER_PWM, 1000);
	// analogWrite(PIN_LASER_PWM, 8);

	// galvoMove(20, 20);

	// Always 400 us?
	// Maybe 600us for max travel (0 to 4096).

	// while (1) {
	// 	// Grid
	// 	// galvoMove(0, 0);
	// 	// delay(100);

	// 	// for (int iY = 0; iY < 40; ++iY) {
	// 	// 	for (int iX = 0; iX < 40; ++iX) {
	// 	// 		if (iY % 2 == 0) {
	// 	// 			galvoMove(iX, iY);
	// 	// 		} else {
	// 	// 			galvoMove(39 - iX, iY);
	// 	// 		}
	// 	// 		delay(1);
	// 	// 	}
	// 	// }

	// 	galvoPreviewSquare();
	// 	delay(1000);
	// 	galvoPreviewTarget();
	// 	delay(1000);

		

	// 	// WriteDacB(1024);
	// 	// delayMicroseconds(500);
	// 	// WriteDacB(0);
	// 	// delayMicroseconds(500);


	// 	// for (int i = 0; i < 128; ++i) {
	// 	// 	WriteDacA(i * 32);
	// 	// 	delayMicroseconds(100);
	// 	// }

	// 	// for (int i = 0; i < 128; ++i) {
	// 	// 	WriteDacB(i * 32);
	// 	// 	delayMicroseconds(100);
	// 	// }

	// 	// for (int i = 0; i < 128; ++i) {
	// 	// 	WriteDacA(4096 - i * 32);
	// 	// 	delayMicroseconds(100);
	// 	// }

	// 	// for (int i = 0; i < 128; ++i) {
	// 	// 	WriteDacB(4096 - i * 32);
	// 	// 	delayMicroseconds(100);
	// 	// }

	// 	// WriteDacA(0);
	// 	// WriteDacB(0);
	// 	// delay(20);
	// 	// WriteDacA(4095);
	// 	// WriteDacB(0);
	// 	// delay(20);
	// 	// WriteDacA(4095);
	// 	// WriteDacB(4095);
	// 	// delay(20);
	// 	// WriteDacA(0);
	// 	// WriteDacB(4095);
	// 	// delay(20);

	// 	// {
	// 	// float pos = 0.0f;
	// 	// float inc = 0.05f;

	// 	// while (true) {
	// 	// 	int step = (pos / 0.050f) * 5.12f;

	// 	// 	WriteDacB(400 + step);
	// 	// 	delayMicroseconds(750);
			
	// 	// 	pos += inc;
	// 	// 	if (pos >= 40.0f) {
	// 	// 		break;
	// 	// 	}
	// 	// }
	// 	// }

	// 	// for (int i = 0; i < 3400; ++i) {
	// 	// 	// WriteDacA(200 + i);
	// 	// 	WriteDacB(400 + i);
	// 	// 	delayMicroseconds(750);
	// 	// }

	// // while (true) {
	// // 	WriteDacA(400);
	// // 	WriteDacB(400);
	// // 	delay(1000);

	// // 	WriteDacA(3600);
	// // 	WriteDacB(3600);
	// // 	delay(1000);
	// // }

	// }

	// char buff[256];

	// // Manual mode.
	// while (1) {
	// 	int b = Serial.read();

	// 	if (b == 'l') {
	// 		st1.currentStep = 0;
	// 		Serial.println("Zeroed step counter");
	// 	}

	// 	if (b == 'i') {
	// 		sprintf(buff, "X:%ld  Y:%ld  Z:%ld\r\n", st1.currentStep, st2.currentStep, st3.currentStep);
	// 		Serial.print(buff);
	// 	}

	// 	if (b == '0') {
	// 		st1.setMicrosteps(0);
	// 		sprintf(buff, "Microsteps: 0 (%d)\r\n", st1.getMicrostepCount());
	// 		Serial.print(buff);
	// 	} else if (b == '9') {
	// 		st1.setMicrosteps(32);
	// 		sprintf(buff, "Microsteps: 32 (%d)\r\n", st1.getMicrostepCount());
	// 		Serial.print(buff);
	// 	}

	// 	if (b == '1') {
	// 		st1.moveDirectRelative(-1, 400);
	// 		sprintf(buff, "Current step: %ld (%d) %d\r\n", st1.currentStep, st1.getMicrostepCount(), digitalRead(st1.limitPin));
	// 		Serial.print(buff);
	// 	} else if (b == '2') {
	// 		st1.moveDirectRelative(1, 400);
	// 		sprintf(buff, "Current step: %ld (%d) %d\r\n", st1.currentStep, st1.getMicrostepCount(), digitalRead(st1.limitPin));
	// 		Serial.print(buff);
	// 	}

	// 	if (b == 'q') {
	// 		st1.moveRelative(32000, 150);
	// 		while (st1.updateStepper());
	// 	} else if (b == 'w') {
	// 		st1.moveRelative(-32000, 150);
	// 		while (st1.updateStepper());
	// 	}

	// 	if (b == 'a') {
	// 		st1.moveRelative(8000, 150);
	// 		while (st1.updateStepper());
	// 	} else if (b == 's') {
	// 		st1.moveRelative(-8000, 150);
	// 		while (st1.updateStepper());
	// 	}

	// 	if (b == 'z') {
	// 		st1.moveRelative(8, 30);
	// 		while (st1.updateStepper());
	// 	} else if (b == 'x') {
	// 		st1.moveRelative(-8, 30);
	// 		while (st1.updateStepper());
	// 	}

	// 	if (b == 'h') {
	// 		Serial.println("Homing...");
	// 		homing();
	// 		Serial.println("Done all homing");
	// 	}
	// }
}