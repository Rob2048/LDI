#include "stepper.h"

ldiStepper::ldiStepper(int8_t Type, int32_t ChipSelectPin, int32_t StepPin, int32_t DirPin, int32_t LimitPin) :	
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

bool ldiStepper::init(int MicroSteps, int StealthChop, int Current, float MmPerStep, float Acceleration, float MaxVelocity, bool Invert, bool Continuous) {
	bool success = true;
	mmPerStep = MmPerStep;
	stepsPerMm = 1.0 / MmPerStep;
	globalAcceleration = Acceleration;
	globalMaxVelocity = MaxVelocity;
	invert = Invert;
	continuous = Continuous;

	if (type == 0) {
		_tmc->begin();
		_tmc->SilentStepStick2130(Current);
		_tmc->stealthChop(StealthChop);
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

void ldiStepper::setMicrosteps(int Steps) {
	_tmc->microsteps(Steps);
}

uint16_t ldiStepper::getMicrostepCount() {
	return _tmc->MSCNT();
}

uint32_t ldiStepper::getDriverStatus() {
	return _tmc->DRV_STATUS();
}

bool ldiStepper::getSfilt() {
	return _tmc->sfilt();
}

int8_t ldiStepper::getSgt() {
	return _tmc->sgt();
}

bool ldiStepper::updateStepper() {
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
	} else if (u >= moveMaxVelocity) {
		// NOTE: Cruising phase.
		u = moveMaxVelocity;
		
		uint32_t cruiseStepDelay = (uint32_t)((mmPerStep / moveMaxVelocity) * 1000000.0);
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

void ldiStepper::moveRelative(int32_t StepTarget, float MaxVelocity) {
	StepTarget = currentStep + StepTarget;
	moveTo(StepTarget, MaxVelocity);
}

bool ldiStepper::validateMove(int32_t StepTarget) {
	if (continuous) {
		return true;
	}

	if (StepTarget < minStep || StepTarget > maxStep) {
		return false;
	}

	return true;
}

bool ldiStepper::validateMoveRelative(int32_t StepTarget) {
	StepTarget = currentStep + StepTarget;
	return validateMove(StepTarget);
}

void ldiStepper::setDirection(bool Dir) {
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

void ldiStepper::moveTo(int32_t StepTarget, float MaxVelocity) {
	uint32_t t0 = micros();

	if (MaxVelocity > globalMaxVelocity) {
		MaxVelocity = globalMaxVelocity;
	}

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

	moveMaxVelocity = MaxVelocity;
	s = mmPerStep;
	a = globalAcceleration;
	u = 0;
	t = 0;
	totalSteps = tSteps;

	// NOTE: Steps to accell/decell.
	float accelT = moveMaxVelocity / a;
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

void ldiStepper::moveSimple(float Position, int32_t Speed) {
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

void ldiStepper::moveDirect(int32_t StepTarget, int32_t Speed) {
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

void ldiStepper::moveDirectRelative(int32_t Steps, int32_t Delay) {
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


void ldiStepper::zero() {
	currentStep = 0;
}

void ldiStepper::home(int SlowSpeed, int FastSpeed, bool HomeDir, int FirstStageSteps) {
	int startStep = currentStep;
	
	if (HomeDir) {
		moveRelative(FirstStageSteps, SlowSpeed);
	} else {
		moveRelative(-FirstStageSteps, SlowSpeed);
	}

	while (true) {
		int p = digitalRead(limitPin);

		if (p == 0) {
			break;
		}

		// TODO: If we don't travel far enough, then this will loop forever.
		updateStepper();
	}

	if (HomeDir) {
		moveRelative(-1000, FastSpeed);
	} else {
		moveRelative(1000, FastSpeed);
	}

	while (updateStepper());

	// TODO: If the limit pin has not been released, then there is a problem.

	setDirection(HomeDir);
	while (true) {
		int p = digitalRead(limitPin);

		if (p == 0) {
			break;
		}

		pulseStepper();
		delayMicroseconds(2000);
	}

	// Serial.printf("Start: %d End: %d Diff:%d\n", startStep, currentStep, currentStep - startStep);

	currentStep = 0;
}

/*
void homing() {
	steppers[0].moveRelative(-100000, 50);
	steppers[1].moveRelative(-100000, 50);
	steppers[2].moveRelative(100000, 50);

	bool s1 = true;
	bool s2 = true;
	bool s3 = true;

	while (s1 || s2 || s3) {
		if (s1) { s1 = moveStepperUntilLimit(&steppers[0]); }
		if (s2) { s2 = moveStepperUntilLimit(&steppers[1]); }
		if (s3) { s3 = moveStepperUntilLimit(&steppers[2]); }
	}

	steppers[0].home(50, 100, false, 0, 0, 20000);
	steppers[1].home(50, 100, false, 0, 0, 15000);
	steppers[2].home(50, 30, true, 17000, 0, 17000);

	// Move to home position.
	steppers[0].moveTo(15000, 200);
	steppers[1].moveTo(5000, 100);
	steppers[2].moveTo(14400, 50);

	s1 = true;
	s2 = true;
	s3 = true;

	while (s1 || s2 || s3) {
		s1 = steppers[0].updateStepper();
		s2 = steppers[1].updateStepper();
		s3 = steppers[2].updateStepper();
	}

	// Zero home position.
	steppers[0].currentStep = 0;
	steppers[1].currentStep = 0;
	steppers[2].currentStep = 0;
}

void homingTest() {
	steppers[0].moveRelative(-100000, 10);
	
	while (moveStepperUntilLimit(&steppers[0]));

	float dist = steppers[0].currentStep * steppers[0].mmPerStep;
	dist *= 1000.0f;

	// Target count for X axis: 420 TMC steps.
	int tmcSteps = steppers[0].getMicrostepCount();
	int tmcDiffSteps = 420 - tmcSteps;
	int actualSteps = tmcDiffSteps / 8;

	char buff[256];
	sprintf(buff, "Current step: %ld Dist: %.0f um (%d) Actual: %d\r\n", steppers[0].currentStep, dist, steppers[0].getMicrostepCount(), actualSteps);
	Serial.print(buff);

	steppers[0].home(50, 100, false, 0, 0, 20000);

	steppers[0].moveTo(steppers[0].stepsPerMm * 30, 200);
	while (steppers[0].updateStepper());
}

*/