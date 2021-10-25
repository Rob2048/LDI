#include "stepper.h"

abStepper::abStepper(int8_t Type, int32_t ChipSelectPin, int32_t StepPin, int32_t DirPin, int32_t LimitPin) :	
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
		// Serial.println("Init TMC2130 stepper");
		
		_tmc->begin();
		_tmc->SilentStepStick2130(Current);
		_tmc->stealthChop(StealthChop);
		_tmc->microsteps(MicroSteps);

		uint32_t tmcVersion = _tmc->version();

		if (tmcVersion != 17)
			success = false;
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

	return success;
}

bool abStepper::updateStepper() {
	uint32_t currentTime = micros();

	remainingSteps = stepTarget - currentStep;
	if (remainingSteps < 0)
		remainingSteps = -remainingSteps;

	if (remainingSteps <= 0)
		return false;

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

	// Calculate next step time.
	// NOTE: Roughly 15us to calculate

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

void abStepper::moveRelative(int32_t StepTarget, float StartVelocity, float EndVelocity, float MaxVelocity) {
	StepTarget = currentStep + StepTarget;
	moveTo(StepTarget, StartVelocity, EndVelocity, MaxVelocity);
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

void abStepper::moveTo(int32_t StepTarget, float StartVelocity, float EndVelocity, float MaxVelocity) {
	uint32_t t0 = micros();
	// char buff[256];
	// sprintf(buff, "Motion block: %ld -> %ld (%.2f, %.2f, %.2f)\r\n", currentStep, StepTarget, StartVelocity, EndVelocity, MaxVelocity);
	// Serial.print(buff);
	
	int32_t tSteps = StepTarget - currentStep;  

	if (tSteps == 0) {
		return;
	}

	if (tSteps < 0) {
		setDirection(false);
	} else {
		setDirection(true);
	}

	moveStartTimeUs = micros();
	stepperTimeUs = moveStartTimeUs;
	maxVelocity = MaxVelocity;
	s = mmPerStep;
	a = globalAcceleration;
	u = StartVelocity;
	t = 0;
	remainingSteps = tSteps;
	totalSteps = tSteps;

	// Time to accelerate from start to max.
	// float accelT = (maxVelocity - StartVelocity) / a;
	// float accelS = (a * (accelT * accelT)) / 2;
	// float moveSteps = accelS * STEPS_PER_MM;
	// stepperState.accelSteps = (int32_t)moveSteps;

	// Time to decellerate from max to end.
	float accelT = (maxVelocity - EndVelocity) / a;
	float accelS = (a * (accelT * accelT)) / 2;
	float moveSteps = accelS * stepsPerMm;
	decelSteps = (int32_t)moveSteps;

	stepTarget = StepTarget;

	t0 = micros() - t0;
	//Serial.println(t0);
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

		// pulseStepper();
		digitalWrite(stepPin, HIGH);
		delayMicroseconds(5);
		digitalWrite(stepPin, LOW);

		delayMicroseconds(Speed);
	}
}

void abStepper::zero() {
	currentStep = 0;
}