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

//--------------------------------------------------------------------------------
// Setup.
//--------------------------------------------------------------------------------
void setup() {
	Serial.begin(921600);
	// while (!Serial);
	
	//if (!st1.init(32, 1, 600, 0.00625, 800.0, true)) sendMessage("Bad driver X");
	//if (!st2.init(32, 1, 600, 0.00625, 800.0, false)) sendMessage("Bad driver Y");
	//if (!st3.init(16, 0, 800, 0.000625, 200.0, true)) {
	if (!st3.init(256, 1, 400, 0.000625, 10000.0, true)) {
	 Serial.println("Bad driver A");
	}

	// Enable steppers.
	pinMode(PIN_STP_EN1, OUTPUT);
	digitalWrite(PIN_STP_EN1, LOW);

	// Enable laser.
	pinMode(PIN_LASER_PWM, OUTPUT);
	digitalWrite(PIN_LASER_PWM, HIGH);

	Serial.println("Controller started");
}

int stepCount = 0;
int subStep = 0;
bool lastDir = false;

inline void setLaser(int Value) {
	digitalWrite(PIN_LASER_PWM, Value);
}

void runLine(bool Dir, int Steps, int StartDelay, int EndDelay, int StepDelay, int Laze, int Offset) {

	st3.setDirection(Dir);

	delayMicroseconds(StartDelay);

	//setLaser(1 & Laze);

	for (int i = 0; i < Steps; ++i) {
		digitalWrite(15, HIGH);
		delayMicroseconds(1);
		digitalWrite(15, LOW);

		if ((i < 100 || i > Steps - 50)) {
			//setLaser(1 & Laze);
			delayMicroseconds(StepDelay);
			//setLaser(0 & Laze);
		} else if ((i + Offset) % 2 == 0) {
			
			setLaser(1 & Laze);
			//delayMicroseconds(1);
			//volatile int j = 0;
			//for (j = 0; j < 3; ++j);
			 
			
			delayMicroseconds(1);
			setLaser(0 & Laze);

			delayMicroseconds(StepDelay - 1);
			
			
		} else {
			delayMicroseconds(StepDelay);
		}

		// int div = i % 10;

		// if (div > 8) {
		// 	setLaser(1 & Laze);
		// } else {
		// 	setLaser(0 & Laze);
		// }
	}

	setLaser(0 & Laze);

	delayMicroseconds(EndDelay);
}

void loop() {
	// while (1) {
	// 	st3.moveTo(4000, 0, 0, 50);
	// 	while (st3.updateStepper());

	// 	st3.moveTo(0, 0, 0, 50);
	// 	while (st3.updateStepper());
	// }

	// while (1) {
	// 	st3.setDirection(true);

	// 	for (int i = 0; i < 1000; ++i) {
	// 		digitalWrite(15, HIGH);
	// 		delayMicroseconds(1);
	// 		digitalWrite(15, LOW);
		
	// 		delayMicroseconds(200);
	// 	}

	// 	delayMicroseconds(100000);

	// 	st3.moveRelative(-1000, 0, 0, 40);
	// 	while (st3.updateStepper());

	// 	delayMicroseconds(100000);
	// }

	int targetT = millis() + 1000;
	int lines = 0;

	while (1) {
		//runLine(true, 450, 0, 100, 110);
		//runLine(false, 450, 0, 100, 110);

		// int t = millis();
		// if (t >= targetT) {
		// 	Serial.print("Lines: ");
		// 	Serial.println(lines);
		// 	lines = 0;
		// 	targetT = t + 1000;
		// }

		runLine(true, 350, 0, 10000, 250, 1, 0);
		// -54 400ma 250us
		runLine(false, 350, 0, 10000, 250, 1, 0);
		// lines += 2;
	}

	while (1) {
		
		digitalWrite(15, HIGH);
		delayMicroseconds(1);
		digitalWrite(15, LOW);
		if (lastDir) {
			delayMicroseconds(200);
		} else {
			delayMicroseconds(20);
		}
		++stepCount;

		if (stepCount % 10 == 0) {
			//delay(100);
		}

		if (stepCount == 500) {
			stepCount = 0;
			lastDir = !lastDir;
			st3.setDirection(lastDir);
			//delayMicroseconds(10000);
			if (lastDir) {
				delayMicroseconds(100000);
			} else {
				delayMicroseconds(100);
			}
		}

	}

	while (1) {
		// Max move is 30 degrees.
		// 12800 steps per 30 degrees.
		// 77.656 mm covered with 30 degrees at 150mm.
		// 1553 x 50 um steps.
		// ~8 steps per 50um.

		int b = Serial.read();

		// if (b == 'a') {
		// 	st3.setDirection(true);
		// 	for (int i = 0; i < 1000; ++i) {
		// 		digitalWrite(15, HIGH);
		// 		delayMicroseconds(5);
		// 		digitalWrite(15, LOW);
		// 		delayMicroseconds(1);
		// 	}

		// 	st3.setDirection(lastDir);
		// } else if (b == 'b') {
		// 	st3.setDirection(true);
		// 	for (int i = 0; i < 10000; ++i) {
		// 		digitalWrite(15, HIGH);
		// 		delayMicroseconds(5);
		// 		digitalWrite(15, LOW);
		// 		delayMicroseconds(1);
		// 	}

		// 	st3.setDirection(lastDir);
		// }

		// st3.pulseStepper();
		int accel = abs(subStep - 80);
		accel /= 2;
		digitalWrite(15, HIGH);
		delayMicroseconds(5);
		digitalWrite(15, LOW);
		delayMicroseconds(15 + accel);
		++stepCount;
		++subStep;

		if (subStep == 160) {
			subStep = 0;
			delayMicroseconds(2000);
			//delay(50);
		}

		if (stepCount == 6400) {
			stepCount = 0;
			lastDir = !lastDir;
			st3.setDirection(lastDir);
			//delayMicroseconds(10000);
			//delay(50);
		}
	}
}