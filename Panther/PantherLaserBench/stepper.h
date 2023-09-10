#pragma once

#include "Arduino.h"
// TODO: Need to remove this library due to viral AGPLv3 license (License was updated to MIT recently?).
// Alternative: https://github.com/teemuatlut/TMCStepper
#include <TMC2130Stepper.h>

struct ldiStepTableEntry {
	int16_t step;
	int16_t sgValue;
};

struct ldiStepTable {
	int32_t size;
	ldiStepTableEntry entries[32000];
};

class abStepper {
public:

	// Config.
	bool		active;
  	int8_t		type;
	int32_t 	stepPin;
	int32_t 	dirPin;	
	int32_t		limitPin;
	bool 		invert;
	float		mmPerStep;
	float		stepsPerMm;
	float		globalAcceleration;
	int32_t		minStep = 0;
	int32_t		maxStep = 0;

	int32_t 	currentDir = -1;
	int32_t 	currentStep = 0;

	// Move command data.
	uint32_t 	moveStartTimeUs;
	float 		maxVelocity;
	float 		s;
	float 		a;
	float 		u;
	float 		t;
	int32_t		accelSteps;
	int32_t 	decelSteps;
	int32_t 	stepTarget;
	int32_t 	totalSteps;
	uint32_t 	stepperTimeUs;

	abStepper(int8_t Type, int32_t ChipSelectPin, int32_t StepPin, int32_t DirPin, int32_t LimitPin);
	bool init(int MicroSteps, int StealthChop, int Current, float MmPerStep, float Acceleration, bool Invert);
	void setDirection(bool Dir);
	void setMicrosteps(int Steps);
	uint16_t getMicrostepCount();
	uint32_t getDriverStatus();
	bool getSfilt();
	int8_t getSgt();

	void zero();
	void home(int SlowSpeed, int FastSpeed, bool HomeDir, int CurrentStep, int MinStep, int MaxStep);

	void moveTo(int32_t StepTarget, float MaxVelocity);
	void moveRelative(int32_t StepTarget, float MaxVelocity);
	bool updateStepper();
	bool updateStepperLog(ldiStepTable* StepLogTable);
	
	void moveSimple(float Position, int32_t Speed);
	void moveDirect(int32_t StepTarget, int32_t Speed);
	void moveDirectRelative(int32_t Steps, int32_t Delay);
	
	inline void pulseStepper();

private:
	TMC2130Stepper* _tmc;
};

inline void abStepper::pulseStepper() {
	digitalWrite(stepPin, HIGH);
	delayMicroseconds(1);
	digitalWrite(stepPin, LOW);
}
