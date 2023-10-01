#pragma once

#define PIN_GALVO_CHIPSELECT	8

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