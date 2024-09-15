//--------------------------------------------------------------------------------
// Teensy 4.0
//--------------------------------------------------------------------------------

#define PIN_02 A0
#define PIN_D1 0

#define PIN_LASER 12
#define PIN_CAMERA 9

#include <stdint.h>
#include <math.h>

IntervalTimer timer;

void setup() {
  Serial.begin(921600);
  Serial.println("Starting");

  // Setup output debug pin
  pinMode(PIN_D1, OUTPUT);
  digitalWrite(PIN_D1, LOW);

  pinMode(PIN_CAMERA, OUTPUT);
  digitalWrite(PIN_CAMERA, LOW);

  pinMode(PIN_LASER, OUTPUT);
  digitalWrite(PIN_LASER, LOW);
}

volatile int _laserStartDelay = -140;
volatile int _pulseSize = 300000;

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
  static int lightDelay = 800;
  static int lightOn = 1000;
  
  int b = Serial.read();
  incDec(b, 'q', 'w', 100, 0, 30000, &lightDelay);
  incDec(b, 'a', 's', 100, 0, 30000, &lightOn);

  Serial.printf("lightDelay: %d lightOn: %d\n", lightDelay, lightOn);

  digitalWrite(PIN_LASER, HIGH);

  delayMicroseconds(lightDelay);

  digitalWrite(PIN_CAMERA, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_CAMERA, LOW);

  delayMicroseconds(lightOn);
  digitalWrite(PIN_LASER, LOW);
  delay(200);
}

void loopA() {
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
    // while (micros() - lastTime < 40000) {}
    // lastTime = micros();

    while (micros() - lastTime < 500000) {}
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

    timer.begin(triggerCam, camTriggerPosition);
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
