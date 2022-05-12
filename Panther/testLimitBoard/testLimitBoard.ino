//--------------------------------------------------------------------------------
// Teensy 4.0 Limit Board Tester.
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
// NOTES:
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------

#define PIN_02				A0

#include <stdint.h>
#include <math.h>

void setup() {

	Serial.begin(921600);
	Serial.println("Starting");

	analogReadResolution(12);
}

float _filtered = 0;
int _count = 0;

void loop() {
	int a = analogRead(PIN_02);

	float v = (a / 4096.0f) * 3.3f;

	_filtered = _filtered * 0.9f + v * 0.1f;

	// Serial.print(_count++);
	// Serial.print("    ");
	// Serial.print(v);
	// Serial.print(" V    ");
	// Serial.print(_filtered);
	// Serial.println(" V");

	char buf[256];
	sprintf(buf, "%10d %10.3f V %10.3f V\n", _count++, v, _filtered);
	Serial.print(buf);

	// 0.162 V in neutral state.

	delay(10);
}