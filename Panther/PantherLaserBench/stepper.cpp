#include "stepper.h"

abStepper::abStepper(int8_t Type, int32_t ChipSelectPin, int32_t StepPin, int32_t DirPin, int32_t LimitPin) :	
	active(false),
	type(Type),
	stepPin(StepPin),
	dirPin(DirPin),
	limitPin(LimitPin),
	invert(false),
	currentDir(-1),
	currentStep(0) {

	if (Type == 0) {
		_tmc = new TMC2130Stepper(0xFFFF, DirPin, StepPin, ChipSelectPin);
	}
}

bool abStepper::init(int MicroSteps, int StealthChop, int Current, float MmPerStep, float Acceleration, bool Invert) {
	bool success = true;
	mmPerStep = MmPerStep;
	stepsPerMm = 1.0 / MmPerStep;
	globalAcceleration = Acceleration;
	invert = Invert;

	if (type == 0) {
		_tmc->begin();
		_tmc->SilentStepStick2130(Current);
		//_tmc->stealthChop(StealthChop);
		_tmc->microsteps(MicroSteps);

		// Set stall gaurd stuff.
		// TCOOLTHRS
		// THIGH = 0;
		// _tmc->coolstep_min_speed(0xFFFFF); // 20bit max
		// _tmc->THIGH(0);

		// _tmc->toff(3);
		// _tmc->tbl(1);
		// _tmc->hysteresis_start(4);
		// _tmc->hysteresis_end(-2);
		// _tmc->rms_current(Current); // mA
		// _tmc->microsteps(MicroSteps);
		// _tmc->diag1_stall(1);
		// _tmc->diag1_active_high(1);
		// _tmc->coolstep_min_speed(0xFFFFF); // 20bit max
		// _tmc->THIGH(0);
		// _tmc->semin(5);
		// _tmc->semax(2);
		// _tmc->sedn(0b01);
		// _tmc->sg_stall_value(0); // -64 to 63

		uint32_t tmcVersion = _tmc->version();

		if (tmcVersion != 17) {
			success = false;
		}
		
		// Serial.print("TMC version: ");
		// Serial.println(tmcVersion);
	} else {
		// Serial.println("Init simple stepper");
		pinMode(dirPin, OUTPUT);
		pinMode(stepPin, OUTPUT);
	}

	if (limitPin > 0) {
		pinMode(limitPin, INPUT_PULLDOWN);
	}

	active = success;

	return success;
}

void abStepper::setMicrosteps(int Steps) {
	_tmc->microsteps(Steps);
}

uint16_t abStepper::getMicrostepCount() {
	return _tmc->MSCNT();
}

uint32_t abStepper::getDriverStatus() {
	return _tmc->DRV_STATUS();
}

bool abStepper::getSfilt() {
	return _tmc->sfilt();
}

int8_t abStepper::getSgt() {
	return _tmc->sgt();
}

bool abStepper::updateStepper() {
	uint32_t currentTime = micros();

	int32_t remainingSteps = stepTarget - currentStep;

	if (remainingSteps == 0)
		return false;

	if (remainingSteps < 0)
		remainingSteps = -remainingSteps;

	if (currentTime >= stepperTimeUs) {
		pulseStepper();
		//Serial.println(remainingSteps);
		//Serial.print(" ");
		//Serial.print(currentTime - moveStartTimeUs);
		//Serial.print(" V ");
		//Serial.println(u, 2);    
		
		if (totalSteps < 0) {
			--currentStep;
		} else {
			++currentStep;
		}
	} else {
		return true;
	}

	// TODO: Only calc accel during accel phase.
	// TODO: Keep velocity after accel phase during cruise phase.
	// TODO: Final step travel is not accounted for?
	
	// Calculate next step time.
	// NOTE: Roughly 15us to calculate on Teensy 3.5.
	if (remainingSteps <= decelSteps) {
		// NOTE: Deacceleration phase.
		t = (sqrtf(2 * -a * s + u * u) - u) / -a;
		float v = u + -a * t;
		u = v;
	
		uint32_t delayTimeUs = (uint32_t)(t * 1000000.0);
		stepperTimeUs += delayTimeUs;		
	} else if (u >= maxVelocity) {
		// NOTE: Cruising phase.
		u = maxVelocity;
		
		uint32_t cruiseStepDelay = (uint32_t)((mmPerStep / maxVelocity) * 1000000.0);
		stepperTimeUs = stepperTimeUs + cruiseStepDelay;		
	} else {
		// NOTE: Acceleration phase.
		t = (sqrtf(2 * a * s + u * u) - u) / a;
		float v = u + a * t;
		u = v;
	
		uint32_t delayTimeUs = (uint32_t)(t * 1000000.0);
		stepperTimeUs += delayTimeUs;
	}

	return true;
}

bool abStepper::updateStepperLog(ldiStepTable* StepLogTable) {
	uint32_t currentTime = micros();

	int32_t remainingSteps = stepTarget - currentStep;

	if (remainingSteps == 0)
		return false;

	if (remainingSteps < 0)
		remainingSteps = -remainingSteps;

	if (currentTime >= stepperTimeUs) {
		pulseStepper();
		//Serial.println(remainingSteps);
		//Serial.print(" ");
		//Serial.print(currentTime - moveStartTimeUs);
		//Serial.print(" V ");
		//Serial.println(u, 2);
		
		if (totalSteps < 0) {
			--currentStep;
		} else {
			++currentStep;
		}

		//StepLogTable->entries[StepLogTable->size].step = currentStep;

		// {
		// 	static unsigned long prevTime = 0;
		// 	unsigned long t0 = micros();

		// 	uint8_t buffer[8];
		// 	buffer[0] = 255;
		// 	buffer[1] = 5;
		// 	buffer[2] = 70;
		// 	buffer[sizeof(buffer) - 1] = 254;

		// 	memcpy(buffer + 3, &prevTime, 4);
			
		// 	Serial.write(buffer, sizeof(buffer));

		// 	prevTime = micros() - t0;
		// }

	} else {
		return true;
	}

	// TODO: Only calc accel during accel phase.
	// TODO: Keep velocity after accel phase during cruise phase.
	// TODO: Final step travel is not accounted for?
	
	// Calculate next step time.
	// NOTE: Roughly 15us to calculate on Teensy 3.5.
	if (remainingSteps <= decelSteps) {
		// NOTE: Deacceleration phase.
		t = (sqrtf(2 * -a * s + u * u) - u) / -a;
		float v = u + -a * t;
		u = v;
	
		uint32_t delayTimeUs = (uint32_t)(t * 1000000.0);
		stepperTimeUs += delayTimeUs;		
	} else if (u >= maxVelocity) {
		// NOTE: Cruising phase.
		u = maxVelocity;
		
		uint32_t cruiseStepDelay = (uint32_t)((mmPerStep / maxVelocity) * 1000000.0);
		stepperTimeUs = stepperTimeUs + cruiseStepDelay;		
	} else {
		// NOTE: Acceleration phase.
		t = (sqrtf(2 * a * s + u * u) - u) / a;
		float v = u + a * t;
		u = v;
	
		uint32_t delayTimeUs = (uint32_t)(t * 1000000.0);
		stepperTimeUs += delayTimeUs;
	}

	return true;
}

void abStepper::moveRelative(int32_t StepTarget, float MaxVelocity) {
	StepTarget = currentStep + StepTarget;
	moveTo(StepTarget, MaxVelocity);
}

void abStepper::setDirection(bool Dir) {
	if (invert) {
		Dir = !Dir;
	}

	if (Dir) {
		if (currentDir != 0) {

			if (type == 0)
				_tmc->shaft_dir(0);
			else
				digitalWrite(dirPin, LOW);
			
			currentDir = 0;
		}
	} else {
		if (currentDir != 1) {
			
			if (type == 0)
				_tmc->shaft_dir(1);
			else
				digitalWrite(dirPin, HIGH);
			
			currentDir = 1;
		}
	}
}

// void abStepper::moveTo(int32_t StepTarget, float StartVelocity, float EndVelocity, float MaxVelocity) {
// 	uint32_t t0 = micros();
// 	// char buff[256];
// 	// sprintf(buff, "Motion block: %ld -> %ld (%.2f, %.2f, %.2f)\r\n", currentStep, StepTarget, StartVelocity, EndVelocity, MaxVelocity);
// 	// Serial.print(buff);
	
// 	int32_t tSteps = StepTarget - currentStep;

// 	if (tSteps == 0) {
// 		return;
// 	}

// 	if (tSteps < 0) {
// 		setDirection(false);
// 	} else {
// 		setDirection(true);
// 	}

// 	moveStartTimeUs = micros();
// 	stepperTimeUs = moveStartTimeUs;
// 	maxVelocity = MaxVelocity;
// 	s = mmPerStep;
// 	a = globalAcceleration;
// 	u = StartVelocity;
// 	t = 0;
// 	totalSteps = tSteps;

// 	// Time to accelerate from start to max.
// 	float accelT = (maxVelocity - StartVelocity) / a;
// 	float accelS = (a * (accelT * accelT)) / 2;
// 	float moveSteps = accelS * stepsPerMm;
// 	accelSteps = (int32_t)moveSteps;

// 	// Time to decellerate from max to end.
// 	accelT = (maxVelocity - EndVelocity) / a;
// 	accelS = (a * (accelT * accelT)) / 2;
// 	moveSteps = accelS * stepsPerMm;
// 	decelSteps = (int32_t)moveSteps;

// 	stepTarget = StepTarget;

// 	// TODO: This does not account for uneven steps.
// 	// TOOD: This does not account for vastly differeing accel/decell step counts.
// 	// TODO: Check if it is possible to reach end velocity from start velocity.
// 	int32_t absTotalSteps = abs(totalSteps);

// 	if (accelSteps + decelSteps > absTotalSteps) {
// 		accelSteps = absTotalSteps / 2;
// 		decelSteps = absTotalSteps / 2;
// 	}

// 	t0 = micros() - t0;
// 	//Serial.println(t0);
// }

void abStepper::moveTo(int32_t StepTarget, float MaxVelocity) {
	uint32_t t0 = micros();

	float maxTime = mmPerStep * 1000000;
	maxTime /= MaxVelocity;
	
	// char buff[256];
	// sprintf(buff, "\r\nMove to: %ld -> %ld (%.2f mm/s) (%.2f us)\r\n", currentStep, StepTarget, MaxVelocity, maxTime);
	// Serial.print(buff);

	stepTarget = StepTarget;
	
	int32_t tSteps = StepTarget - currentStep;

	if (tSteps == 0) {
		return;
	}

	if (tSteps < 0) {
		setDirection(false);
	} else {
		setDirection(true);
	}

	maxVelocity = MaxVelocity;
	s = mmPerStep;
	a = globalAcceleration;
	u = 0;
	t = 0;
	totalSteps = tSteps;

	// NOTE: Steps to accell/decell.
	float accelT = maxVelocity / a;
	float accelS = (a * (accelT * accelT)) / 2;
	float moveSteps = accelS * stepsPerMm;
	int32_t totalAccelSteps = (int32_t)moveSteps;
	int32_t origAccelSteps = totalAccelSteps;

	// NOTE: If there are not enough steps to accel to max, then accel as far as we can.
	int32_t absTotalSteps = abs(totalSteps);

	if (totalAccelSteps * 2 > absTotalSteps) {
		totalAccelSteps = absTotalSteps / 2;
	}

	accelSteps = totalAccelSteps;
	decelSteps = totalAccelSteps;

	moveStartTimeUs = micros();
	stepperTimeUs = moveStartTimeUs;

	t0 = micros() - t0;
	//Serial.println(t0);

	// sprintf(buff, "Total: %ld Accel: %ld -> %ld (%ld us)\r\n", absTotalSteps, origAccelSteps, totalAccelSteps, t0);
	// Serial.print(buff);
}

void abStepper::moveSimple(float Position, int32_t Speed) {
	// Convert position to step target.
	int32_t stepTarget = (int32_t)(Position * stepsPerMm);

	// Step.
	while (currentStep != stepTarget) {
		if (currentStep > stepTarget) {
			setDirection(false);
			--currentStep;
		} else {
			setDirection(true);
			++currentStep;
		}

		pulseStepper();
		delayMicroseconds(Speed);
	}
}

void abStepper::moveDirect(int32_t StepTarget, int32_t Speed) {
	int32_t stepTarget = StepTarget;

	// Step.
	while (currentStep != stepTarget) {
		if (currentStep > stepTarget) {
			setDirection(false);
			--currentStep;
		} else {
			setDirection(true);
			++currentStep;
		}

		pulseStepper();
		delayMicroseconds(Speed);
	}
}

void abStepper::moveDirectRelative(int32_t Steps, int32_t Delay) {
	int32_t totalSteps = abs(Steps);

	if (Steps < 0) {
		setDirection(false);
	} else {
		setDirection(true);
	}

	for (int i = 0; i < totalSteps; ++i) {
		// pulseStepper();
		digitalWrite(stepPin, HIGH);
		delayMicroseconds(5);
		digitalWrite(stepPin, LOW);

		delayMicroseconds(Delay);
	}

	currentStep += Steps;
}


void abStepper::zero() {
	currentStep = 0;
}

void abStepper::home(int SlowSpeed, int FastSpeed, bool HomeDir, int CurrentStep, int MinStep, int MaxStep) {
	if (HomeDir) {
		moveRelative(100000, SlowSpeed);
	} else {
		moveRelative(-100000, SlowSpeed);
	}

	while (true) {
		int p = digitalRead(limitPin);

		if (p == 0) {
			break;
		}

		updateStepper();
	}

	if (HomeDir) {
		moveRelative(-1000, FastSpeed);
	} else {
		moveRelative(1000, FastSpeed);
	}

	while (updateStepper());

	setDirection(HomeDir);
	while (true) {
		int p = digitalRead(limitPin);

		if (p == 0) {
			break;
		}

		pulseStepper();
		delayMicroseconds(2000);
	}

	currentStep = CurrentStep;
	minStep = MinStep;
	maxStep = MaxStep;
}