#include "Drive.h"

#include "config.h"

void Drive::begin(uint8_t leftPin, uint8_t rightPin) {
  left_.attach(leftPin);
  right_.attach(rightPin);
  stop();
}

void Drive::set(int left, int right) {
  left_.write(left);
  right_.write(right);
}

void Drive::stop() { set(config::kMotorStop, config::kMotorStop); }
