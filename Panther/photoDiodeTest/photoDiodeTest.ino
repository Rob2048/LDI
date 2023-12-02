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

	// timer.begin(takeAnalogMeasurement, 2);
}

// void takeAnalogMeasurement() {
// 	// int a = adc->analogRead(PIN_02, 0);
// 	int a = 0;

// 	// Convert to single byte.
// 	a = a >> 2;
	
// 	if (a > 150) {
// 		digitalWrite(PIN_D1, HIGH);
// 	} else {
// 		digitalWrite(PIN_D1, LOW);
// 	}

// 	Serial.write(a);
// }

volatile int _frameStartDelay = -173;
volatile int _pulseSize = 70;

bool incDec(int Trigger, int DecTrigger, int IncTrigger, int ValDelta, int Min, int Max, int* Val) {
	if (Trigger == DecTrigger) {
		*Val -= ValDelta;
		if (*Val < Min) {
			*Val = Min;
		}
		return true;
	} else if (Trigger == IncTrigger) {
		*Val += ValDelta;
		if (*Val > Max) {
			*Val = Max;
		}
		return true;
	}

	return false;
}

void loop() {
	int camTriggerPosition = 5000;
	int frameStartDelay = _frameStartDelay;
	int pulseSize = _pulseSize;
	bool showUpdate = true;

	int sampleCount = 0;
	int sampleCountTotal = 200;
	int sampleUs = 1;

	unsigned long lastTime = micros();

	while (1) {
		// Keep loop at 25 Hz.
		while (micros() - lastTime < 40000);
		lastTime = micros();

		int b = Serial.read();
		if (incDec(b, 'a', 's', 100, 0, 30000, &camTriggerPosition)) { showUpdate = true; }
		if (incDec(b, 'q', 'w', 10, -1000, 1000, &frameStartDelay)) { showUpdate = true; }
		if (incDec(b, 'e', 'r', 1, -1000, 1000, &frameStartDelay)) { showUpdate = true; }
		if (incDec(b, 'o', 'p', 1, 0, 200, &pulseSize)) { showUpdate = true; }
		
		if (showUpdate) {
			Serial.printf("frameStartDelay: %d pulseSize: %d camTriggerPosition: %d\n", frameStartDelay, pulseSize, camTriggerPosition);
			showUpdate = false;
			noInterrupts();
			_frameStartDelay = frameStartDelay;
			_pulseSize = pulseSize;
			interrupts();
		}

		digitalWrite(PIN_D1, HIGH);
		delayMicroseconds(100);
		digitalWrite(PIN_D1, LOW);

		if (sampleCount < sampleCountTotal) {
			sampleCount++;
		} else {			
			sampleCount = 0;
		}

		int sampleDelayUs = sampleCount * sampleUs;

		timer.begin(triggerCam, camTriggerPosition);

		// Start galvo moves.
		// ...
		delayMicroseconds(5000 + 800 - sampleDelayUs);
		
		// digitalWrite(PIN_LASER, HIGH);
		// delayMicroseconds(_pulseSize);
		// digitalWrite(PIN_LASER, LOW);
	}
}

void triggerCam() {
	// digitalWrite(PIN_LASER, HIGH);
	// delayMicroseconds(10);
	// digitalWrite(PIN_LASER, LOW);
	digitalWrite(PIN_CAMERA, HIGH);
	delayMicroseconds(10);
	digitalWrite(PIN_CAMERA, LOW);

	timer.begin(triggerLaser, 750 + _frameStartDelay);
}

void triggerLaser() {
	timer.end();

	digitalWrite(PIN_LASER, HIGH);
	delayMicroseconds(_pulseSize);
	digitalWrite(PIN_LASER, LOW);
}

// void loop() {
// 	int frameStartDelay = -173;
// 	int pulseSize = 70;

// 	bool showUpdate = false;

// 	while (1) {
// 		// No faster than 30 Hz.

// 		int b = Serial.read();
// 		if (b == 'q') {
// 			frameStartDelay -= 10;
// 			showUpdate = true;
// 		} else if (b == 'w') {
// 			frameStartDelay += 10;
// 			showUpdate = true;
// 		}

// 		if (b == 'e') {
// 			frameStartDelay -= 1;
// 			showUpdate = true;
// 		} else if (b == 'r') {
// 			frameStartDelay += 1;
// 			showUpdate = true;
// 		}

// 		if (b == 'o') {
// 			pulseSize -= 1;
// 			showUpdate = true;
// 		} else if (b == 'p') {
// 			pulseSize += 1;
// 			showUpdate = true;
// 		}

// 		if (pulseSize < 0) {
// 			pulseSize = 0;
// 			showUpdate = true;
// 		} else if (pulseSize > 200) {
// 			pulseSize = 200;
// 			showUpdate = true;
// 		}

// 		if (showUpdate) {
// 			Serial.printf("frameStartDelay: %d pulseSize: %d\n", frameStartDelay, pulseSize);
// 			showUpdate = true;
// 		}

// 		digitalWrite(PIN_CAMERA, HIGH);
// 		delayMicroseconds(10);
// 		digitalWrite(PIN_CAMERA, LOW);

// 		delayMicroseconds(750 + frameStartDelay);

// 		digitalWrite(PIN_LASER, HIGH);
// 		delayMicroseconds(pulseSize);
// 		digitalWrite(PIN_LASER, LOW);

// 		delay(40);
// 	}
// }