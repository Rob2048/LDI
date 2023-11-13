#include <iostream>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <pigpio.h>

// GPIO 4 (PIN 16) (BCM 23)
#define TRIGGER_GPIO_PIN 23

#define LED_GPIO_PIN 24

//-------------------------------------------------------------------------
// Main application.
//-------------------------------------------------------------------------
int main() {
	std::cout << "Starting trigger app\n";
	// int64_t t0 = platformGetMicrosecond();
	// t0 = platformGetMicrosecond() - t0;
	// std::cout << "Wrote to " << clients.size() << " clients in " << (t0 / 1000.0) << " ms\n";

	int gv = gpioInitialise();

	if (gv == PI_INIT_FAILED) {
		std::cout << "PIGPIO failed to init.\n";
		return 1;
	}
	
	gpioSetMode(TRIGGER_GPIO_PIN, PI_OUTPUT);
	gpioWrite(TRIGGER_GPIO_PIN, PI_HIGH);

	gpioSetMode(LED_GPIO_PIN, PI_OUTPUT);
	gpioWrite(LED_GPIO_PIN, PI_LOW);

	while (true) {
		usleep(1000 * 300);

		gpioWrite(TRIGGER_GPIO_PIN, PI_HIGH);
		usleep(2);
		gpioWrite(TRIGGER_GPIO_PIN, PI_LOW);

		usleep(1500);

		gpioWrite(LED_GPIO_PIN, PI_HIGH);
		
		usleep(500);
		gpioWrite(LED_GPIO_PIN, PI_LOW);
	}

	return 0;
}