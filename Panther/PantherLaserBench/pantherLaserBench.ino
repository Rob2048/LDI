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

// Test limit input:
// 1 21 (A7)
// 2 22 (A8)

// Camera trigger:
// 24 

// Axis info:
// X: 32 * 200 = 8mm
// Y: 32 * 200 = 8mm
// Z: 32 * 200 = 8mm
// C: (30:1) 32 * 200 * 30 = 192000
// A: (90:1) 32 * 200 * 90 = 576000

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

#define PIN_SCAN_LASER_ENABLE	28

#define PIN_LIMIT_C_INNER		21 // Analog 7
#define PIN_LIMIT_C_OUTER		22 // Analog 8

#define PIN_CAMERA_TRIGGER		24

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
	ldiStepper(0, 20, 19, 18,  0),
	ldiStepper(0,  2,  1,  0,  5),
};

bool isHomed = false;

//--------------------------------------------------------------------------------
// Packet processing.
//--------------------------------------------------------------------------------
enum ldiPantherOpcode {
	PO_PING = 0,
	PO_DIAG = 1,
	
	PO_HOME = 10,
	PO_MOVE_RELATIVE = 11,
	PO_MOVE = 12,
	
	PO_LASER_BURST = 20,
	PO_LASER_PULSE = 21,
	PO_LASER_STOP = 22,
	PO_MODULATE_LASER = 23,

	PO_GALVO_PREVIEW_SQUARE = 30,
	PO_GALVO_MOVE = 31,
	PO_GALVO_BURN = 32,

	PO_SCAN_LASER_STATE = 60,

	PO_CMD_RESPONSE = 100,
	PO_POSITION = 101,
	PO_MESSAGE = 102,
	PO_SETUP = 103,
};

enum ldiPantherStatus {
	PS_OK = 0,
	PS_ERROR = 1,
	PS_NOT_HOMED = 2,
	PS_INVALID_PARAM = 3,
};

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

unsigned long _sendTimer = 0;

void sendMessage(const char* Message) {
	uint8_t buffer[1024];
	int strLen = 0;

	while (strLen < 1024) {
		char c = Message[strLen];

		buffer[3 + strLen] = Message[strLen];
		++strLen;

		if (c == 0)
			break;
	}

	buffer[0] = FRAME_START;
	buffer[1] = strLen + 1;
	buffer[2] = PO_MESSAGE;
	buffer[strLen + 3] = FRAME_END;

	Serial.write(buffer, strLen + 4);
}

void sendCmdSuccess(ldiPantherStatus Status) {
	// NOTE: takes about 6us to build and send.
	uint8_t buffer[32];
	buffer[0] = FRAME_START;
	buffer[1] = 29;
	buffer[2] = PO_CMD_RESPONSE;
	buffer[sizeof(buffer) - 1] = FRAME_END;

	int s1p = steppers[AXIS_ID_X].currentStep;
	int s2p = steppers[AXIS_ID_Y].currentStep;
	int s3p = steppers[AXIS_ID_Z].currentStep;
	int s4p = steppers[AXIS_ID_C].currentStep;
	int s5p = steppers[AXIS_ID_A].currentStep;
	int statusCode = (int)Status;
	int homed = (int)isHomed;
	
	memcpy(buffer + 3, &s1p, 4);
	memcpy(buffer + 7, &s2p, 4);
	memcpy(buffer + 11, &s3p, 4);
	memcpy(buffer + 15, &s4p, 4);
	memcpy(buffer + 19, &s5p, 4);
	memcpy(buffer + 23, &statusCode, 4);
	memcpy(buffer + 27, &homed, 4);
	
	Serial.write(buffer, sizeof(buffer));
}

void sendCmdPosition() {
	uint8_t buffer[24];
	buffer[0] = FRAME_START;
	buffer[1] = 21;
	buffer[2] = PO_POSITION;
	buffer[sizeof(buffer) - 1] = FRAME_END;

	int s1p = steppers[AXIS_ID_X].currentStep;
	int s2p = steppers[AXIS_ID_Y].currentStep;
	int s3p = steppers[AXIS_ID_Z].currentStep;
	int s4p = steppers[AXIS_ID_C].currentStep;
	int s5p = steppers[AXIS_ID_A].currentStep;

	memcpy(buffer + 3, &s1p, 4);
	memcpy(buffer + 7, &s2p, 4);
	memcpy(buffer + 11, &s3p, 4);
	memcpy(buffer + 15, &s4p, 4);
	memcpy(buffer + 19, &s5p, 4);
	
	Serial.write(buffer, sizeof(buffer));
}

void sendCmdSetup() {
	static unsigned long prevProcessTime = 0;
	unsigned long t0 = micros();

	uint8_t buffer[28];
	buffer[0] = FRAME_START;
	buffer[1] = 25;
	buffer[2] = PO_SETUP;
	buffer[sizeof(buffer) - 1] = FRAME_END;

	int s1p = steppers[AXIS_ID_X].currentStep;
	int s2p = steppers[AXIS_ID_Y].currentStep;
	int s3p = steppers[AXIS_ID_Z].currentStep;
	int s4p = steppers[AXIS_ID_C].currentStep;
	int s5p = steppers[AXIS_ID_A].currentStep;

	memcpy(buffer + 3, &s1p, 4);
	memcpy(buffer + 7, &s2p, 4);
	memcpy(buffer + 11, &s3p, 4);
	memcpy(buffer + 15, &s4p, 4);
	memcpy(buffer + 19, &s5p, 4);
	memcpy(buffer + 23, &prevProcessTime, 4);
	
	Serial.write(buffer, sizeof(buffer));

	prevProcessTime = micros() - t0;
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

	if (cmdId == PO_PING) {
		// TODO: Send setup response.
		sendCmdSetup();
		sendCmdSuccess(PS_OK);
	} else if (cmdId == PO_DIAG) {
		// NOTE: Status command.
		sprintf(buff, "Panther diagnostics:\nsteppers[0]: %d\nsteppers[1]: %d\nsteppers[2]: %d\nunsigned long: %d", steppers[0].active, steppers[1].active, steppers[2].active, sizeof(unsigned long));
		sendMessage(buff);
		sendCmdSuccess(PS_OK);
	} else if (cmdId == PO_HOME) {
		ldiPantherStatus status = homeAll();
		sendCmdSuccess(status);
	} else if (cmdId == PO_MOVE_RELATIVE) {
		if (!isHomed) {
			sendCmdSuccess(PS_NOT_HOMED);
			return;
		}

		int axisId = Buffer[1];
		int32_t stepTarget;
		float maxVelocity;

		memcpy(&stepTarget, Buffer + 2, 4);
		memcpy(&maxVelocity, Buffer + 6, 4);

		//sprintf(buff, "Cmd2: axisId: %d stepTarget: %ld maxVelocity: %f", axisId, stepTarget, maxVelocity);
		//sendMessage(buff);

		if (axisId < 0 || axisId >= AXIS_COUNT) {
			sendCmdSuccess(PS_INVALID_PARAM);
			return;
		}

		ldiStepper* stepper = &steppers[axisId];

		if (maxVelocity == 0.0f) {
			maxVelocity = stepper->globalMaxVelocity;
		}

		if (!stepper->validateMoveRelative(stepTarget)) {
			// sendMessage("Can't validate move.");
			sendCmdSuccess(PS_INVALID_PARAM);
			return;
		}

		stepper->moveRelative(stepTarget, maxVelocity);
		while (stepper->updateStepper()) {
			unsigned long t0 = millis();

			if (t0 >= _sendTimer) {
				_sendTimer = t0 + 20;
				sendCmdPosition();
			}
		}	
		
		sendCmdSuccess(PS_OK);
	} else if (cmdId == PO_MOVE) {
		if (!isHomed) {
			sendCmdSuccess(PS_NOT_HOMED);
			return;
		}
		
		int axisId = Buffer[1];
		int32_t stepTarget;
		float maxVelocity;

		memcpy(&stepTarget, Buffer + 2, 4);
		memcpy(&maxVelocity, Buffer + 6, 4);

		//sprintf(buff, "Cmd2: axisId: %d stepTarget: %ld maxVelocity: %f", axisId, stepTarget, maxVelocity);
		//sendMessage(buff);

		if (axisId < 0 || axisId >= AXIS_COUNT) {
			sendCmdSuccess(PS_INVALID_PARAM);
			return;
		}

		ldiStepper* stepper = &steppers[axisId];

		if (maxVelocity == 0.0f) {
			maxVelocity = stepper->globalMaxVelocity;
		}

		if (!stepper->validateMove(stepTarget)) {
			sendCmdSuccess(PS_INVALID_PARAM);
			return;
		}

		stepper->moveTo(stepTarget, maxVelocity);
		while (stepper->updateStepper()) {
			unsigned long t0 = millis();

			if (t0 >= _sendTimer) {
				_sendTimer = t0 + 20;
				sendCmdPosition();
			}
		}	
		
		sendCmdSuccess(PS_OK);
	} else if (cmdId == PO_LASER_BURST && Len == 5) {
		// Burst laser.
		int32_t time = 0;
		memcpy(&time, Buffer + 1, 4);

		digitalWrite(PIN_LASER_PWM, HIGH);
		delayMicroseconds(time);
		digitalWrite(PIN_LASER_PWM, LOW);

		sendCmdSuccess(PS_OK);
	} else if (cmdId == PO_LASER_PULSE && Len == 13) {
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

		sendCmdSuccess(PS_OK);
	} else if (cmdId == PO_LASER_STOP && Len == 1) {
		// Stop laser.
		pinMode(PIN_LASER_PWM, OUTPUT);
		digitalWrite(PIN_LASER_PWM, LOW);
		sendCmdSuccess(PS_OK);
	} else if (cmdId == PO_MODULATE_LASER && Len == 9) {
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

		sendCmdSuccess(PS_OK);
	} else if (cmdId == PO_GALVO_PREVIEW_SQUARE) {
		galvoPreviewSquare();
		sendCmdSuccess(PS_OK);
	} else if (cmdId == PO_GALVO_MOVE) {
		float posX = 0.0f;
		float posY = 0.0f;

		memcpy(&posX, Buffer + 1, 4);
		memcpy(&posY, Buffer + 5, 4);

		galvoMove(posX, posY);

		sendCmdSuccess(PS_OK);
	} else if (cmdId == PO_GALVO_BURN) {
		// Move galvo and burn at each point.
		// digitalWrite(PIN_DBG1, HIGH);
		int count = 0;

		memcpy(&count, Buffer + 1, 4);
		
		for (int i = 0; i < count; ++i) {
			// digitalWrite(PIN_DBG2, HIGH);
			float x = 0;
			float y = 0;
			int burnTime = 0;

			memcpy(&x, Buffer + 5 + i * 10 + 0, 4);
			memcpy(&y, Buffer + 5 + i * 10 + 4, 4);
			memcpy(&burnTime, Buffer + 5 + i * 10 + 8, 2);

			// digitalWrite(PIN_DBG3, HIGH);
			galvoMove(x, y);
			// digitalWrite(PIN_DBG3, LOW);
			digitalWrite(PIN_LASER_PWM, HIGH);
			delayMicroseconds(burnTime);
			digitalWrite(PIN_LASER_PWM, LOW);

			// digitalWrite(PIN_DBG2, LOW);
		}

		// digitalWrite(PIN_DBG1, LOW);
		sendCmdSuccess(PS_OK);
	} else if (cmdId == PO_SCAN_LASER_STATE) {
		int state = 0;
		memcpy(&state, Buffer + 1, 4);

		if (state == 0) {
			digitalWrite(PIN_SCAN_LASER_ENABLE, LOW);
		} else {
			digitalWrite(PIN_SCAN_LASER_ENABLE, HIGH);
		}
		
		sendCmdSuccess(PS_OK);

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
	
	isHomed = false;

	steppers[AXIS_ID_X].init(32, 0, 900, 0.00125, 200.0, 30, false, false);
	steppers[AXIS_ID_Y].init(32, 0, 900, 0.00125, 200.0, 30, false, false);
	steppers[AXIS_ID_Z].init(32, 0, 900, 0.00125, 200.0, 30, true, false);
	steppers[AXIS_ID_C].init(32, 0, 400, 0.00125, 200.0, 20, true, true);
	steppers[AXIS_ID_A].init(32, 1, 900, 0.00125, 50.0, 20, true, false);

	// Enable debug pins
	// pinMode(PIN_DBG1, OUTPUT);
	// pinMode(PIN_DBG2, OUTPUT);
	// pinMode(PIN_DBG3, OUTPUT);

	// Enable C axis limit pins.
	pinMode(PIN_LIMIT_C_INNER, INPUT_PULLUP);
	pinMode(PIN_LIMIT_C_OUTER, INPUT_PULLUP);

	// Enable steppers.
	pinMode(PIN_STP_EN1, OUTPUT);
	digitalWrite(PIN_STP_EN1, LOW);

	// Enable laser.
	pinMode(PIN_LASER_PWM, OUTPUT);
	digitalWrite(PIN_LASER_PWM, LOW);

	// Enable scan laser.
	pinMode(PIN_SCAN_LASER_ENABLE, OUTPUT);
	digitalWrite(PIN_SCAN_LASER_ENABLE, LOW);

	// Enable galvo.
	galvoInit();

	// Enable camera trigger.
	pinMode(PIN_CAMERA_TRIGGER, OUTPUT);
	digitalWrite(PIN_CAMERA_TRIGGER, LOW);
}

//--------------------------------------------------------------------------------
// Loop.
//--------------------------------------------------------------------------------
void loop() {

	// PWN works: 5000hz, 15% (200us total, 11us on)
	// analogWriteFrequency(PIN_LASER_PWM, 5000);
	// analogWrite(PIN_LASER_PWM, 15);

	// while (1){
	// }

	// while (1) {
	// 	digitalWrite(PIN_LASER_PWM, HIGH);
	// 	delayMicroseconds(15);
	// 	digitalWrite(PIN_LASER_PWM, LOW);
	// 	delayMicroseconds(250);
	// }

	int frameStartDelay = 0;

	// Camera trigger test.
	while (1) {
		int b = Serial.read();

		if (b == 'q') {
			frameStartDelay -= 100;
		} else if (b == 'w') {
			frameStartDelay += 100;
		}

		if (b == 'e') {
			frameStartDelay -= 10;
		} else if (b == 'r') {
			frameStartDelay += 10;
		}

		if (frameStartDelay < 0)
			frameStartDelay = 0;
		
		Serial.println(frameStartDelay);

		// Serial.println("Trigger camera");
		digitalWrite(PIN_CAMERA_TRIGGER, HIGH);
		delayMicroseconds(10);
		digitalWrite(PIN_CAMERA_TRIGGER, LOW);
		
		delayMicroseconds(frameStartDelay);

		for (int i = 0; i < 6; ++i) {
			digitalWrite(PIN_LASER_PWM, HIGH);
			delayMicroseconds(70);
			digitalWrite(PIN_LASER_PWM, LOW);
			delayMicroseconds(300);
		}

		// delayMicroseconds(6500);
		// digitalWrite(PIN_LASER_PWM, HIGH);
		// delayMicroseconds(90);
		// digitalWrite(PIN_LASER_PWM, LOW);
		
		delay(50);
	}

	// Comms mode.
	while (1) {
		updatePacketInput();
	}

	// Manual mode.
	while (1) {
		int b = Serial.read();

		int axis = AXIS_ID_A;

		if (b == 'q') {
			steppers[axis].moveRelative(10000, 20);
			while (steppers[axis].updateStepper());
			Serial.printf("Step: %d\n", steppers[axis].currentStep);
		} else if (b == 'w') {
			steppers[axis].moveRelative(-10000, 20);
			while (steppers[axis].updateStepper());
			Serial.printf("Step: %d\n", steppers[axis].currentStep);
		}
		
		if (b == 'a') {
			steppers[axis].moveRelative(1600, 20);
			while (steppers[axis].updateStepper());
			Serial.printf("Step: %d\n", steppers[axis].currentStep);
		} else if (b == 's') {
			steppers[axis].moveRelative(-1600, 20);
			while (steppers[axis].updateStepper());
			Serial.printf("Step: %d\n", steppers[axis].currentStep);
		}

		if (b == 'z') {
			steppers[axis].moveRelative(160, 20);
			while (steppers[axis].updateStepper());
			Serial.printf("Step: %d\n", steppers[axis].currentStep);
		} else if (b == 'x') {
			steppers[axis].moveRelative(-160, 20);
			while (steppers[axis].updateStepper());
			Serial.printf("Step: %d\n", steppers[axis].currentStep);
		}

		if (b == 'l') {
			while (true) {
				printLimitSwitchStatus();
				delay(1000);
			}
		}

		if (b == 'h') {
			homeAll();
		}
	}
}

//--------------------------------------------------------------------------------
// Platform functionality.
//--------------------------------------------------------------------------------
void printLimitSwitchStatus() {
	int limX = digitalRead(steppers[AXIS_ID_X].limitPin);
	int limY = digitalRead(steppers[AXIS_ID_Y].limitPin);
	int limZ = digitalRead(steppers[AXIS_ID_Z].limitPin);
	int limA = digitalRead(steppers[AXIS_ID_A].limitPin);
	int limC0 = digitalRead(PIN_LIMIT_C_INNER);
	int limC1 = digitalRead(PIN_LIMIT_C_OUTER);

	Serial.printf("X:%d Y:%d Z:%d A:%d C0:%d C1:%d\n", limX, limY, limZ, limA, limC0, limC1);
}

ldiPantherStatus homeAll() {
	isHomed = false;

	steppers[AXIS_ID_X].home(10, 20, false);

	steppers[AXIS_ID_Z].home(10, 20, true);
	steppers[AXIS_ID_Z].moveRelative(-100000, 30);
	while (steppers[AXIS_ID_Z].updateStepper());
	steppers[AXIS_ID_Z].currentStep = 0;
	steppers[AXIS_ID_Z].minStep = -64000;
	steppers[AXIS_ID_Z].maxStep = 64000;

	steppers[AXIS_ID_A].home(10, 20, false, 300000);
	steppers[AXIS_ID_A].moveRelative(20000, 20);
	while (steppers[AXIS_ID_A].updateStepper());
	steppers[AXIS_ID_A].currentStep = 0;
	steppers[AXIS_ID_A].minStep = -15000; // -9.375 degrees.
	steppers[AXIS_ID_A].maxStep = 300000; // 187.5 degrees.

	steppers[AXIS_ID_Y].home(10, 20, false);
	steppers[AXIS_ID_Y].moveRelative(76000, 30);
	while (steppers[AXIS_ID_Y].updateStepper());
	steppers[AXIS_ID_Y].currentStep = 0;
	steppers[AXIS_ID_Y].minStep = -64000;
	steppers[AXIS_ID_Y].maxStep = 64000;

	steppers[AXIS_ID_X].moveRelative(92000, 30);
	while (steppers[AXIS_ID_X].updateStepper());
	steppers[AXIS_ID_X].currentStep = 0;
	steppers[AXIS_ID_X].minStep = -64000;
	steppers[AXIS_ID_X].maxStep = 64000;

	if (!homeAxisC()) {
		return PS_ERROR;
	}
	
	isHomed = true;

	return PS_OK;
}

bool homeAxisC() {
	ldiStepper* stepper = &steppers[AXIS_ID_C];

	// Align step to micro phase.
	stepper->currentStep = (stepper->getMicrostepCount() - 4) / 8;

	// Serial.printf("Start home - Step: %d\n", stepper->currentStep);

	// NOTE: Back off if we are already inside limit.
	{
		int outerLimit = digitalRead(PIN_LIMIT_C_OUTER);

		if (outerLimit == 1) {
			// Serial.println("Inside outer limit");

			stepper->moveRelative(7000, 20.0f);
			while (stepper->updateStepper());
		}
	}

	// Serial.printf("Start home forward - Step: %d\n", stepper->currentStep);

	// NOTE: Only search for one full turn.
	int lastVal = 0;
	int state = 0;
	int startStep = 0;
	int endStep = 0;

	stepper->setDirection(false);
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
			// Serial.printf("%6d %3d %4d\n", stepper->currentStep, outerLimit, step);
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

		// Serial.printf("Found outer limit: %d to %d (%d to %d) Mid: %4d MidPhase: %4d\n", startStep, endStep, getMicroPhase(startStep), getMicroPhase(endStep), midPointStep, midMicroPhase);

		// TODO: Move to position that is far from inner limit.
		
		// Move back to find inner center.
		stepper->setDirection(true);
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
				// Serial.printf("%6d %3d %4d\n", stepper->currentStep, innerLimit, step);
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

			// Serial.printf("Found inner limit: %d to %d (%d to %d) Mid: %4d MidPhase: %4d\n", startStep, endStep, getMicroPhase(startStep), getMicroPhase(endStep), midPointStep, midMicroPhase);

			// Get new step target from micro phase.
			int adjustedPhase = midMicroPhase - 4;
			int moveSteps = 0;

			if (adjustedPhase >= 512) {
				moveSteps = (1024 - adjustedPhase) / 8;
			} else {
				moveSteps = (-adjustedPhase) / 8;
			}

			int newStepTarget = midPointStep + moveSteps;

			// Serial.printf("Move to step: %d (%d)\n", newStepTarget, getMicroPhase(newStepTarget));

			stepper->moveTo(newStepTarget, 20.0f);
			while (stepper->updateStepper());

			uint16_t ms = stepper->getMicrostepCount();
			// Serial.printf("Final position: %d (%d)\n", stepper->currentStep, ms);

			// Move to zero pos and set as 0 step.
			stepper->moveRelative(10000, 20.0f);
			while (stepper->updateStepper());
			stepper->currentStep = 0;

		} else {
			// Serial.printf("Could not find inner limit\n");
		}
	} else {
		// Serial.printf("Could not find outer limit\n");
	}

	return true;
}