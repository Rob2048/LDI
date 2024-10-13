//--------------------------------------------------------------------------------
// Teensy 4.0
//--------------------------------------------------------------------------------

#define PIN_02				A0
#define PIN_D1				0

#define PIN_LASER			12
#define PIN_CAMERA			9

#include <stdint.h>
#include <math.h>
#include "galvo.h"

// #include <ADCL_t4.h>

// ADCL* adc = new ADCL();
IntervalTimer timer;
IntervalTimer timer2;
IntervalTimer timer3;

// Teensy 4.1 & 4.0 GPIO to bank bit mapping:
// Bit      31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00

// GPIO6    27 26       21 20 23 22 16 17 41 40 15 14 18 19       25 24                          0  1
// GPIO7                                               7  8          32  9  6                   13 11 12 10
// GPIO8                            30 31          28 38 39 34 35 36 37
// GPIO9    29                                                                    5 33  4  3  2

volatile IMXRT_LPSPI_t* port = &IMXRT_LPSPI4_S;

#define IMXRT_BANK6_DIRECT  (*(volatile uint32_t *)0x42000000)
#define IMXRT_BANK7_DIRECT  (*(volatile uint32_t *)0x42004000)
#define IMXRT_BANK8_DIRECT  (*(volatile uint32_t *)0x42008000)
#define IMXRT_BANK9_DIRECT  (*(volatile uint32_t *)0x4200C000)

#define IMXRT_BANK06_GPIO0_BIT ((uint32_t)(1 << 3))

inline void setDebugPin() {
	IMXRT_BANK6_DIRECT = IMXRT_BANK6_DIRECT | IMXRT_BANK06_GPIO0_BIT;
}

inline void clearDebugPin() {
	IMXRT_BANK6_DIRECT = IMXRT_BANK6_DIRECT & ~IMXRT_BANK06_GPIO0_BIT;
}

void setup() {
	Serial.begin(921600);
	Serial.println("Starting");

	// analogReadResolution(12);

	// Setup output debug pin
	pinMode(PIN_D1, OUTPUT);
	digitalWrite(PIN_D1, LOW);

	pinMode(PIN_CAMERA, OUTPUT);
	digitalWrite(PIN_CAMERA, LOW);

	galvoInit();

	pinMode(PIN_LASER, OUTPUT);
	digitalWrite(PIN_LASER, LOW);

 	// adc->setResolution(10);
	// adc->setAdcClockSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED);
	// adc->setAveraging(2, 0);
	// adc->setConversionSpeed(ADC_CONVERSION_SPEED::MED_SPEED, 0);
	// adc->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED, 0);

	// timer.begin(takeAnalogMeasurement, 2);

	// PWM works: 5000hz, 5.5% (200us total, 11us on)
	// analogWriteFrequency(PIN_LASER, 5000);
	// analogWrite(PIN_LASER, 15);
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

// volatile int _laserStartDelay = -180;
volatile int _laserStartDelay = -140;
volatile int _pulseSize = 70;

// digitalWriteFast ??

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

int _currentFrame = 0;
int _camStartFrame = 0;
volatile bool _runningPath = false;

void runSampledPath() {
	int totalFrames = 100;
	int currentStartFrame = 0;

	// Time between camera samples = 34 ms = 29.412 FPS
	// Path needs to fit within 34 ms, including full reset (maybe 2 ms).

	for (int i = 0; i < totalFrames; ++i) {
		// Make sure we are not less than 34 ms since last iteration.
		// ...

		// Perform full reset.
		// ...

		_currentFrame = 0;
		_camStartFrame = i;
		_runningPath = true;
		timer.begin(primaryLoop, 5);

		// Wait for path to finish.
		while (_runningPath);
	}

	// For repetition, make sure we are at least 34 ms since last camera trigger.
	// ...
}

void loop() {
	// SPI to DAC notes:
	// At 4Mhz:
	// - ~4.5us to send SPI command to single channel
	// - ~9us to send SPI command to both channels, using WriteDacX functions.
	// - DAC will take 4.5us to settle after second channel.
	// - ~15us for both channels and settle.

	// Aim for 50kHz update rate, every 20us.

	// With 4096 DAC positions, at 100us per pixel, 80000us to complete a sweep, 20us for each DAC update. (500 mm/s)
	
	SPI.beginTransaction(SPISettings(5000000, MSBFIRST, SPI_MODE0));
	volatile IMXRT_LPSPI_t* volatile port = &IMXRT_LPSPI4_S;
	uint32_t tcr = port->TCR;
	// turn on 16 bit mode.
	port->TCR = (tcr & 0xfffff000) | LPSPI_TCR_FRAMESZ(15);

	timer.priority(120);
	timer2.priority(100);
	
	timer.begin(primaryLoop, 20);
	delayNanoseconds(4000);
	timer2.begin(updateCamTrigger, 20);
	
	while (true) {};
}

int _galvoDacA = 0;
int _galvoDacB = 0;

float _incX = 0.01;
float _galvoX = 0.0f;

float _cameraTriggerCycle = 0.0f;

void primaryLoop() {
	// setDebugPin();
	// clearDebugPin();

	uint16_t cmd = 0;
	// Gain to 1x.
	cmd |= 1 << 13;
	// Turn on.
	cmd |= 1 << 12;
	// Output: 0 - 4095.
	cmd |= (int)_galvoX;

	digitalWrite(PIN_GALVO_CHIPSELECT, LOW);
	port->TDR = cmd;

	// setDebugPin();

	_galvoX += _incX;
	
	if ((int)_galvoX >= 4096) {
		_galvoX = 0.0f;
	}

	if (_cameraTriggerCycle >= 3000.0f) {
		_cameraTriggerCycle = 0.0f;
	}

	delayNanoseconds((int)_cameraTriggerCycle);
	_cameraTriggerCycle += 0.001f;
	
	// clearDebugPin();

	while ((port->RSR & LPSPI_RSR_RXEMPTY));
	// NOTE: Important to read RDR to clear flag for next cycle.
	int test = port->RDR;

	digitalWrite(PIN_GALVO_CHIPSELECT, HIGH);

	// If end of path then terminate.
	// timer.end();
	
	// setDebugPin();
	// clearDebugPin();
}

void updateCamTrigger() {
	setDebugPin();
	delayNanoseconds(50);
	clearDebugPin();
}

void stopCamTrigger() {
	digitalWrite(PIN_D1, LOW);
	timer3.end();
}

void loop2() {
	int camTriggerPosition = 5100;
	int laserStartDelay = _laserStartDelay;
	int pulseSize = _pulseSize;
	bool showUpdate = true;

	int sampleCount = 0;
	int sampleCountTotal = 200;
	int sampleUs = 5;

	int gX = 2218;
	int gY = 1398;

	unsigned long lastTime = micros();

	while (1) {
		// Keep loop at 25 Hz.
		while (micros() - lastTime < 40000);
		lastTime = micros();

		int b = Serial.read();
		if (incDec(b, 'a', 's', 100, 0, 30000, &camTriggerPosition)) { showUpdate = true; }
		if (incDec(b, 'q', 'w', 10, -1000, 1000, &laserStartDelay)) { showUpdate = true; }
		if (incDec(b, 'e', 'r', 1, -1000, 1000, &laserStartDelay)) { showUpdate = true; }
		if (incDec(b, 'o', 'p', 1, 0, 200, &pulseSize)) { showUpdate = true; }

		if (incDec(b, 't', 'y', 10, 0, 4095, &gX)) { showUpdate = true; }
		if (incDec(b, 'g', 'h', 10, 0, 4095, &gY)) { showUpdate = true; }
		
		if (showUpdate) {
			Serial.printf("laserStartDelay: %d pulseSize: %d camTriggerPosition: %d\n", laserStartDelay, pulseSize, camTriggerPosition);
			Serial.printf("gX: %d gY: %d\n", gX, gY);
			showUpdate = false;
			noInterrupts();
			_laserStartDelay = laserStartDelay;
			_pulseSize = pulseSize;
			interrupts();
		}

		digitalWrite(PIN_D1, HIGH);
		delayMicroseconds(20);
		digitalWrite(PIN_D1, LOW);

		// ADC response:
		// 15us to adjust single dac channgel, 9us to write dac value, 6us to settle (max swing).
		// 24us to adjust both channels.

		// Diff output response:
		// ...

		// Galvo response:
		// ...

		// galvoPreviewSquare();

		// WriteDacA(gX);
		// WriteDacB(gY);
		// delayMicroseconds(10);

		// for (int i = 0; i < 10; ++i) {
		// 	WriteDacA(2048 + i * 200);
		// 	WriteDacB(2048 + i * 200);
		// 	delayMicroseconds(10);
		// }

		// WriteDacA(0);
		// WriteDacB(0);

		if (sampleCount < sampleCountTotal) {
			sampleCount++;
		} else {			
			sampleCount = 0;
		}

		int sampleDelayUs = sampleCount * sampleUs;

		timer.begin(triggerCam, camTriggerPosition);

		// Start galvo moves.
		// Starting position.
		WriteDacA(gX);
		WriteDacB(gY);

		delayMicroseconds(5000 + 770 - sampleDelayUs);

		int v = 8 * 6;

		WriteDacA(gX);
		WriteDacB(gY);
		delayMicroseconds(100);

		WriteDacA(gX + 333);
		delayMicroseconds(400);

		// for (int i = 0; i < 10; ++i) {
		// 	WriteDacA(gX + v * i);
		// 	// if ((i % 2) == 0) {
		// 	// 	WriteDacB(gY + v * 1);
		// 	// } else {
		// 	// 	WriteDacB(gY + v * -1);
		// 	// }
		// 	delayMicroseconds(400);
		// }
		
		// 50 counts = ~150px = 450um
		// 1 count = 3px = 9um
		
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
	// delayMicroseconds(10);
	timer.begin(tiggerCamStop, 10);
}

void tiggerCamStop() {
	digitalWrite(PIN_CAMERA, LOW);
	timer.begin(triggerLaser, 750 + _laserStartDelay);
}

void triggerLaser() {
	digitalWrite(PIN_LASER, HIGH);
	// delayMicroseconds(_pulseSize);
	timer.begin(triggerLaserStop, _pulseSize);
}

void triggerLaserStop() {
	timer.end();
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