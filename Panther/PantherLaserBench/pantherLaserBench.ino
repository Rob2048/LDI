//--------------------------------------------------------------------------------
// Laser test bench controller.
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
// Hardware pinouts.
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

// Axis info:

// X:
// Y:
// Z:
// C:

// A:
// 90:1 gearbox.
// 32 * 200 * 90 = 576000
// 0.000625 steps per deg
// 1600 steps per deg

//--------------------------------------------------------------------------------

#include <SPI.h>
#include <stdint.h>
#include <math.h>
#include "stepper.h"
#include "encoder.h"
#include "laser.h"
#include "galvo.h"

#define PIN_STP_EN1				32
#define PIN_DBG1				21
#define PIN_DBG2				22
#define PIN_DBG3				23

#define PIN_LIMIT_C_INNER		21 // Analog 7
#define PIN_LIMIT_C_OUTER		22 // Analog 8

#define AXIS_ID_X			0
#define AXIS_ID_Y			1
#define AXIS_ID_Z			2
#define AXIS_ID_C			3
#define AXIS_ID_A			4

#define AXIS_COUNT			5

ldiStepper steppers[5] = {
	ldiStepper(0, 33, 34, 35, 36),
	ldiStepper(0, 37, 38, 39,  9),
	ldiStepper(0, 14, 15, 16, 17),
	ldiStepper(0, 20, 19, 18, 10),
	ldiStepper(0,  2,  1,  0,  5),
};

//--------------------------------------------------------------------------------
// Functionality.
//--------------------------------------------------------------------------------
void printLimitSwitchStatus() {
	int lim1 = digitalRead(steppers[0].limitPin);
	int lim2 = digitalRead(steppers[1].limitPin);
	int lim3 = digitalRead(steppers[2].limitPin);

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

unsigned long _sendTimer = 0;

void sendCmdSuccess() {
	static unsigned long prevProcessTime = 0;
	unsigned long t0 = micros();

	uint8_t buffer[20];
	buffer[0] = FRAME_START;
	buffer[1] = 17;
	buffer[2] = 10;
	buffer[sizeof(buffer) - 1] = FRAME_END;

	float s1p = steppers[0].currentStep * steppers[0].mmPerStep;
	float s2p = steppers[1].currentStep * steppers[1].mmPerStep;
	float s3p = steppers[2].currentStep * steppers[2].mmPerStep;

	memcpy(buffer + 3, &s1p, 4);
	memcpy(buffer + 7, &s2p, 4);
	memcpy(buffer + 11, &s3p, 4);
	memcpy(buffer + 15, &prevProcessTime, 4);
	
	Serial.write(buffer, sizeof(buffer));

	prevProcessTime = micros() - t0;
}

void sendCmdPosition() {
	uint8_t buffer[16];
	buffer[0] = FRAME_START;
	buffer[1] = 13;
	buffer[2] = 69;
	buffer[sizeof(buffer) - 1] = FRAME_END;

	float s1p = steppers[0].currentStep * steppers[0].mmPerStep;
	float s2p = steppers[1].currentStep * steppers[1].mmPerStep;
	float s3p = steppers[2].currentStep * steppers[2].mmPerStep;

	memcpy(buffer + 3, &s1p, 4);
	memcpy(buffer + 7, &s2p, 4);
	memcpy(buffer + 11, &s3p, 4);
	
	Serial.write(buffer, sizeof(buffer));
}

bool moveStepperUntilLimit(ldiStepper* Stepper) {
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
		sprintf(buff, "Panther diagnostics:\nsteppers[0]: %d\nsteppers[1]: %d\nsteppers[2]: %d\nunsigned long: %d", steppers[0].active, steppers[1].active, steppers[2].active, sizeof(unsigned long));
		sendMessage(buff);
		sendCmdSuccess();
	} else if (cmdId == 2) {
		int axisId = Buffer[1];
		int32_t stepTarget;
		float maxVelocity;

		memcpy(&stepTarget, Buffer + 2, 4);
		memcpy(&maxVelocity, Buffer + 6, 4);

		//sprintf(buff, "Cmd2: axisId: %d stepTarget: %ld maxVelocity: %f", axisId, stepTarget, maxVelocity);
		//sendMessage(buff);

		ldiStepper* stepper;

		if (axisId == 0) {
			stepper = &steppers[0];
		} else if (axisId == 1) {
			stepper = &steppers[1];
		} else if (axisId == 2) {
			stepper = &steppers[2];
		} else if (axisId == 3) {
			stepper = &steppers[3];
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
			steppers[0].moveDirect((int32_t)(position * steppers[0].stepsPerMm) + steppers[0].currentStep, speed);
		} else if (axisId == 1) {
			// Y
			steppers[1].moveDirect((int32_t)(position * steppers[1].stepsPerMm) + steppers[1].currentStep, speed);
		} else if (axisId == 2) {
			// Z
			steppers[2].moveDirect((int32_t)(position * steppers[2].stepsPerMm) + steppers[2].currentStep, speed);
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
			steppers[0].moveSimple(position, speed);
		} else if (axisId == 1) {
			// Y
			steppers[1].moveSimple(position, speed);
		} else if (axisId == 2) {
			// Z
			steppers[2].moveSimple(position, speed);
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
			_targetSteps[i] = (int32_t)round(position * steppers[0].stepsPerMm);
		}

		t = micros() - t;

		sprintf(buff, "Raster (%ld %ld %ld %ld) computed in %ld us", stopCounts, onTime, offTime, stepTime, t);
		sendMessage(buff);

		for (int i = 0; i < stopCounts; ++i) {
			steppers[0].moveDirect(_targetSteps[i], stepTime);
			delayMicroseconds(offTime);
			digitalWrite(PIN_LASER_PWM, HIGH);
			delayMicroseconds(onTime);
			digitalWrite(PIN_LASER_PWM, LOW);
		}

		sendCmdSuccess();
	} else if (cmdId == 11) {
		// Move to zero.
		steppers[0].moveTo(0, 200);
		steppers[1].moveTo(0, 100);
		steppers[2].moveTo(0, 50);
		
		bool s1 = true;
		bool s2 = true;
		bool s3 = true;

		while (s1 || s2 || s3) {
			s1 = steppers[0].updateStepper();
			s2 = steppers[1].updateStepper();
			s3 = steppers[2].updateStepper();
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
			int32_t targetStep = (int32_t)round(pos * steppers[0].stepsPerMm);

			steppers[0].moveDirect(targetStep, stepTime);
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
		// Move galvo and burn at each point.
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

		ldiStepper* stepper = &steppers[1];

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

		ldiStepper* stepper;

		if (axisId == 0) {
			stepper = &steppers[0];
		} else if (axisId == 1) {
			stepper = &steppers[1];
		} else if (axisId == 2) {
			stepper = &steppers[2];
		} else {
			// TODO: Cmd failed, invalid axis.
		}

		stepper->moveDirectRelative(stepTarget, delay);
		
		sendCmdSuccess();
	} else if (cmdId == 23) {
		// steppers[0].moveRelative(-10000, 10);
		// while (moveStepperUntilLimit(&steppers[0]));
		// sendCmdSuccess();

		// float dist = steppers[0].currentStep * steppers[0].mmPerStep;
		// dist *= 1000.0f;

		// // Target count for X axis: 420 TMC steps.
		// int tmcSteps = steppers[0].getMicrostepCount();
		// int tmcDiffSteps = 420 - tmcSteps;
		// int actualSteps = tmcDiffSteps / 8;

		// char buff[256];
		// sprintf(buff, "Current step: %ld Dist: %.0f um (%d) Actual: %d\r\n", steppers[0].currentStep, dist, steppers[0].getMicrostepCount(), actualSteps);
		// Serial.print(buff);

		bool homeDir[] = {
			false,
			false,
			true
		};

		for (int i = 0; i < 3; ++i) {
			steppers[i].home(10, 30, homeDir[i], 0, 0, 20000);
			int tmcSteps = steppers[i].getMicrostepCount();
			sprintf(buff, "Step pos: %d", tmcSteps);
			sendMessage(buff);
		}

		// Move to home position.
		steppers[0].moveTo(60000, 30);
		steppers[1].moveTo(90000, 30);
		steppers[2].moveTo(-80000, 30);

		bool s1 = true;
		bool s2 = true;
		bool s3 = true;

		while (s1 || s2 || s3) {
			s1 = steppers[0].updateStepper();
			s2 = steppers[1].updateStepper();
			s3 = steppers[2].updateStepper();
		}

		// Zero home position.
		steppers[0].currentStep = 0;
		steppers[1].currentStep = 0;
		steppers[2].currentStep = 0;

		//steppers[0].moveTo(steppers[0].stepsPerMm * 30, 30);
		//while (steppers[0].updateStepper());

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

	steppers[AXIS_ID_X].init(32, 0, 900, 0.00125, 200.0, false);
	steppers[AXIS_ID_Y].init(32, 0, 900, 0.00125, 200.0, false);
	steppers[AXIS_ID_Z].init(32, 0, 900, 0.00125, 200.0, false);
	steppers[AXIS_ID_C].init(32, 0, 400, 0.00125, 200.0, false);
	steppers[AXIS_ID_A].init(32, 1, 900, 0.00125, 50.0, true);

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
}

//--------------------------------------------------------------------------------
// Loop.
//--------------------------------------------------------------------------------
void loop() {
	// Comms mode.
	// while (1) {
	// 	updatePacketInput();
	// }

	// Manual mode.
	while (1) {
		int b = Serial.read();

		if (b == 'q') {
			steppers[AXIS_ID_A].moveRelative(10000, 20);
			while (steppers[AXIS_ID_A].updateStepper());
		} else if (b == 'w') {
			steppers[AXIS_ID_A].moveRelative(-10000, 20);
			while (steppers[AXIS_ID_A].updateStepper());
		}
		
		if (b == 'a') {
			steppers[AXIS_ID_A].moveRelative(1600, 20);
			while (steppers[AXIS_ID_A].updateStepper());
		} else if (b == 's') {
			steppers[AXIS_ID_A].moveRelative(-1600, 20);
			while (steppers[AXIS_ID_A].updateStepper());
		}

		if (b == 'z') {
			steppers[AXIS_ID_A].moveRelative(160, 20);
			while (steppers[AXIS_ID_A].updateStepper());
		} else if (b == 'x') {
			steppers[AXIS_ID_A].moveRelative(-160, 20);
			while (steppers[AXIS_ID_A].updateStepper());
		}
	}
}

int homeAxisC() {
	int state = 0;
	int startStep = 0;
	int endStep = 0;

	// int innerLimit = analogRead(7);
	// int outerLimit = analogRead(8);
	// int innerLimit = digitalRead(PIN_LIMIT_C_INNER);
	// int outerLimit = digitalRead(PIN_LIMIT_C_OUTER);

	ldiStepper* stepper = &steppers[AXIS_ID_C];

	// Align step to micro phase.
	stepper->currentStep = (stepper->getMicrostepCount() - 4) / 8;

	Serial.printf("Start home - Step: %d\n", stepper->currentStep);

	// NOTE: Back off if we are already inside limit.
	{
		int outerLimit = digitalRead(PIN_LIMIT_C_OUTER);

		if (outerLimit == 1) {
			Serial.println("Inside outer limit");

			stepper->moveRelative(-7000, 20.0f);
			while (stepper->updateStepper());
		}
	}

	Serial.printf("Start home forward - Step: %d\n", stepper->currentStep);

	// NOTE: Only search for one full turn.
	int lastVal = 0;
	stepper->setDirection(true);
	for (int i = 0; i < 192000; ++i) {
		stepper->pulseStepper();
		uint16_t step = stepper->getMicrostepCount();

		delayMicroseconds(200);

		int outerLimit = digitalRead(PIN_LIMIT_C_OUTER);

		if (stepper->currentDir) {
			--stepper->currentStep;
		} else {
			++stepper->currentStep;
		}

		if (outerLimit != lastVal) {
			lastVal = outerLimit;
			Serial.printf("%6d %3d %4d\n", stepper->currentStep, outerLimit, step);
		}

		if (state == 0) {
			if (outerLimit == 1) {
				startStep = stepper->currentStep;
				state = 1;
			}
		} else if (state == 1) {
			if (abs(stepper->currentStep - startStep) > 1000 && outerLimit == 0) {
				endStep = stepper->currentStep;
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
		stepper->setDirection(false);
		lastVal = 0;
		state = 0;
		for (int i = 0; i < 3000; ++i) {
			stepper->pulseStepper();
			uint16_t step = stepper->getMicrostepCount();
			delayMicroseconds(400);
			int innerLimit = digitalRead(PIN_LIMIT_C_INNER);

			if (stepper->currentDir) {
				--stepper->currentStep;
			} else {
				++stepper->currentStep;
			}

			if (innerLimit != lastVal) {
				lastVal = innerLimit;
				Serial.printf("%6d %3d %4d\n", stepper->currentStep, innerLimit, step);
			}

			if (state == 0) {
				if (innerLimit == 1) {
					startStep = stepper->currentStep;
					state = 1;
				}
			} else if (state == 1) {
				if (abs(stepper->currentStep - startStep) > 100 && innerLimit == 0) {
					endStep = stepper->currentStep;
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

			stepper->moveTo(newStepTarget, 20.0f);
			while (stepper->updateStepper());

			uint16_t ms = stepper->getMicrostepCount();
			Serial.printf("Final position: %d (%d)\n", stepper->currentStep, ms);

			// Move to zero pos and set as 0 step.
			stepper->moveRelative(-10000, 20.0f);
			while (stepper->updateStepper());
			stepper->currentStep = 0;

		} else {
			Serial.printf("Could not find inner limit\n");
		}
	} else {
		Serial.printf("Could not find outer limit\n");
	}
}