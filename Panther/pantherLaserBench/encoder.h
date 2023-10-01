#pragma once

// 8192 * 4 = 32768
// 360 / 32768 = 0.010986328125 degs per count.

#define PIN_ENC_Z				21
#define PIN_ENC_B				22
#define PIN_ENC_A				23

volatile int encStateA = 0;
volatile int encStateB = 0;
volatile int encStateZ = 0;

volatile int encCount = 0;

void interrupt_encA() {
	encStateA = digitalRead(PIN_ENC_A);

	if (encStateA == 1) {
		if (encStateB == 0) {
			++encCount;
		} else {
			--encCount;
		}
	} else {
		if (encStateB == 1) {
			++encCount;
		} else {
			--encCount;
		}
	}
}

void interrupt_encB() {
	encStateB = digitalRead(PIN_ENC_B);

	if (encStateB == 1) {
		if (encStateA == 1) {
			++encCount;
		} else {
			--encCount;
		}
	} else {
		if (encStateA == 0) {
			++encCount;
		} else {
			--encCount;
		}
	}
}

void interrupt_encZ() {
	encStateZ = digitalRead(PIN_ENC_Z);
}

void encoderInit() {
	pinMode(PIN_ENC_A, INPUT);
	pinMode(PIN_ENC_B, INPUT);
	pinMode(PIN_ENC_Z, INPUT);

	attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), interrupt_encA, CHANGE);
	attachInterrupt(digitalPinToInterrupt(PIN_ENC_B), interrupt_encB, CHANGE);
	attachInterrupt(digitalPinToInterrupt(PIN_ENC_Z), interrupt_encZ, CHANGE);
}