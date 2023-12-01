//--------------------------------------------------------------------------------
// Teensy 4.0
//--------------------------------------------------------------------------------

#define PIN_02				A0
#define PIN_D1				0

#define PIN_LASER			12
#define PIN_CAMERA			9

#include <stdint.h>
#include <math.h>

// #include <ADCL_t4.h>

// ADCL* adc = new ADCL();
IntervalTimer timer;

void setup() {
	Serial.begin(921600);
	Serial.println("Starting");

	// analogReadResolution(12);

	// Setup output debug pin
	pinMode(PIN_D1, OUTPUT);
	digitalWrite(PIN_D1, LOW);

	pinMode(PIN_LASER, OUTPUT);
	digitalWrite(PIN_LASER, LOW);

	pinMode(PIN_CAMERA, OUTPUT);
	digitalWrite(PIN_CAMERA, LOW);

 	// adc->setResolution(10);
	// adc->setAdcClockSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED);
	// adc->setAveraging(2, 0);
	// adc->setConversionSpeed(ADC_CONVERSION_SPEED::MED_SPEED, 0);
	// adc->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED, 0);

	// timer.begin(takeMeasurement, 2);
}

void takeMeasurement() {
	// int a = adc->analogRead(PIN_02, 0);
	int a = 0;

	// Convert to single byte.
	a = a >> 2;
	
	if (a > 150) {
		digitalWrite(PIN_D1, HIGH);
	} else {
		digitalWrite(PIN_D1, LOW);
	}

	Serial.write(a);
}

int state = 0;

void loop() {
	while (1) {
		// No faster than 30 Hz.

		digitalWrite(PIN_CAMERA, HIGH);
		delayMicroseconds(10);
		digitalWrite(PIN_CAMERA, LOW);

		delayMicroseconds(750 - 100);

		digitalWrite(PIN_LASER, HIGH);
		delayMicroseconds(70);
		digitalWrite(PIN_LASER, LOW);

		delay(40);
	}
	
	while (1) {
		// Pulse delay is 16ms.
		// Starts about ~190 us after high.
		// Min pulse width is 80 us. (~4V)
		// 200 us = ~6V
		// Takes ~15ms to settle.
		// Max voltage ~6V


		digitalWrite(PIN_LASER, HIGH);
		delayMicroseconds(100);
		digitalWrite(PIN_LASER, LOW);
		delay(20);
		// delayMicroseconds(1000);
	}

	uint32_t timeout = millis() + 1000;
	int count = 0;
	int lastV = 0;

	while (true) {
		digitalWrite(PIN_D1, HIGH);
		// int a = adc->analogRead(PIN_02, 0);
		int a = 0;

		// Convert to single byte.
		a = a >> 2;
		Serial.write(a);

		digitalWrite(PIN_D1, LOW);

		// if (a > 30) {
			// digitalWrite(PIN_D1, HIGH);
		// } else {
			// digitalWrite(PIN_D1, LOW);
		// }

		// if (a > lastV + 10 || a < lastV - 10) {
		// 	Serial.printf("%10d %10d\n", count, a);
		// 	lastV = a;
		// }

		// int cc = 0;
		// while (adc->isConverting(0)) { ++cc; }

		// if (state == 0) {
		// 	if (a > 1) {
		// 		state = 1;
		// 		digitalWrite(PIN_D1, HIGH);
		// 	}
		// } else if (state == 1) {
		// 	if (a == 0) {
		// 		state = 0;
		// 		digitalWrite(PIN_D1, LOW);
		// 	}
		// }

		// ++count;
		// if (millis() > timeout) {
		// 	timeout = millis() + 20000;
		// 	Serial.printf("%10d %10d %10d\n", count, a);
		// 	count = 0;
		// }
	}

	// float v = (a / 4096.0f) * 3.3f;
	// _filtered = v;
	// _filtered = _filtered * 0.9f + v * 0.1f;
	// Serial.print(_count++);
	// Serial.print("    ");
	// Serial.print(v);
	// Serial.print(" V    ");
	// Serial.print(_filtered);
	// Serial.println(" V");

	// Serial.printf("%10d %10.3f V %10.3f V\n", _count++, v, _filtered);

	// 0.162 V in neutral state.

	// delay(10);
}