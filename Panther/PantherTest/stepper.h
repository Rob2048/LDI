#pragma once

#include "Arduino.h"
// TODO: Need to remove this library due to viral AGPLv3 license.
#include <TMC2130Stepper.h>

class abStepper {
public:
  	int8_t    	type;
	int32_t 	stepPin;
	int32_t 	dirPin;
	int32_t		limitPin;
	uint32_t 	stepperTimeUs;
	uint32_t 	moveStartTimeUs;
	float 		maxVelocity;
	float 		s;
	float 		a;
	float 		u;
	float 		t;
	int32_t 	decelSteps;
	int32_t 	remainingSteps;
	int32_t 	stepTarget;
	int32_t 	totalSteps;
	float		mmPerStep;
	float		stepsPerMm;
	float		globalAcceleration;
	bool 		invert;

	int32_t 	currentDir = -1;
	int32_t 	currentStep = 0;

	abStepper(int8_t Type, int32_t ChipSelectPin, int32_t StepPin, int32_t DirPin, int32_t LimitPin);
	bool init(int MicroSteps, int StealthChop, int Current, float MmPerStep, float Acceleration, bool Invert);
	bool updateStepper();
	void moveTo(int32_t StepTarget, float StartVelocity, float EndVelocity, float MaxVelocity);
  	void moveRelative(int32_t StepTarget, float StartVelocity, float EndVelocity, float MaxVelocity);
	void setDirection(bool Dir);
	void moveSimple(float Position, int32_t Speed);
	void moveDirect(int32_t StepTarget, int32_t Speed);
	void zero();
  	inline void pulseStepper();

private:
	TMC2130Stepper* _tmc;
};

inline void abStepper::pulseStepper() {
	digitalWrite(stepPin, HIGH);
	delayMicroseconds(10);
	digitalWrite(stepPin, LOW);
}
