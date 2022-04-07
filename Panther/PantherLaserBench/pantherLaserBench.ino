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
// 5 3 (Why is this 5?)
// 3 23
// 2 22
// 1 21

//--------------------------------------------------------------------------------

#define PIN_STP_EN1			32
#define PIN_LASER_PWM		3

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

//--------------------------------------------------------------------------------
// Packet processing.
//--------------------------------------------------------------------------------

// Requests:
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
		st3.home(50, 30, true, 24000, 0, 24000);

		st1.moveTo(st1.stepsPerMm * 30, 200);
		st2.moveTo(st2.stepsPerMm * 3, 100);
		st3.moveTo(1000, 50);

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
		st1.moveTo(st1.stepsPerMm * 30, 200);
		st2.moveTo(st2.stepsPerMm * 3, 100);
		st3.moveTo(1000, 50);
		
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
	st3.moveTo(0, 50);

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
// Setup.
//--------------------------------------------------------------------------------
void setup() {
	Serial.begin(921600);
	
	if (!st1.init(32, 0, 900, 0.00625, 10000.0, true)) sendMessage("Bad driver X");
	if (!st2.init(32, 1, 600, 0.00625, 400.0, true)) sendMessage("Bad driver Y");
	if (!st3.init(32, 1, 600, 0.00625, 400.0, false)) sendMessage("Bad driver Z");

	// Enable steppers.
	pinMode(PIN_STP_EN1, OUTPUT);
	digitalWrite(PIN_STP_EN1, LOW);

	// Enable laser.
	pinMode(PIN_LASER_PWM, OUTPUT);
	digitalWrite(PIN_LASER_PWM, LOW);
}

//--------------------------------------------------------------------------------
// Loop.
//--------------------------------------------------------------------------------
void loop() {
	// Comms mode.
	sendMessage("Controller started");
	while (1) {
		updatePacketInput();
	}

	char buff[256];

	// Manual mode.
	while (1) {
		int b = Serial.read();

		if (b == 'l') {
			st1.currentStep = 0;
			Serial.println("Zeroed step counter");
		}

		if (b == 'i') {
			sprintf(buff, "X:%ld  Y:%ld  Z:%ld\r\n", st1.currentStep, st2.currentStep, st3.currentStep);
			Serial.print(buff);
		}

		if (b == '0') {
			st1.setMicrosteps(0);
			sprintf(buff, "Microsteps: 0 (%d)\r\n", st1.getMicrostepCount());
			Serial.print(buff);
		} else if (b == '9') {
			st1.setMicrosteps(32);
			sprintf(buff, "Microsteps: 32 (%d)\r\n", st1.getMicrostepCount());
			Serial.print(buff);
		}

		if (b == '1') {
			st1.moveDirectRelative(-1, 400);
			sprintf(buff, "Current step: %ld (%d) %d\r\n", st1.currentStep, st1.getMicrostepCount(), digitalRead(st1.limitPin));
			Serial.print(buff);
		} else if (b == '2') {
			st1.moveDirectRelative(1, 400);
			sprintf(buff, "Current step: %ld (%d) %d\r\n", st1.currentStep, st1.getMicrostepCount(), digitalRead(st1.limitPin));
			Serial.print(buff);
		}

		if (b == 'q') {
			st1.moveRelative(1000, 100);
			while (st1.updateStepper());
		} else if (b == 'w') {
			st1.moveRelative(-1000, 100);
			while (st1.updateStepper());
		}

		if (b == 'a') {
			st2.moveRelative(1000, 100);
			while (st2.updateStepper());
		} else if (b == 's') {
			st2.moveRelative(-1000, 100);
			while (st2.updateStepper());
		}

		if (b == 'z') {
			st3.moveRelative(1000, 30);
			while (st3.updateStepper());
		} else if (b == 'x') {
			st3.moveRelative(-1000, 30);
			while (st3.updateStepper());
		}

		if (b == 'h') {
			Serial.println("Homing...");
			homing();
			Serial.println("Done all homing");
		}
	}
}