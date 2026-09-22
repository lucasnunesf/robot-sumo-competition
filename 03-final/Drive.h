#pragma once

#include <Arduino.h>
#include <ESP32Servo.h>

// Two ESC-driven wheels. Commands are always given as (left, right).
class Drive {
 public:
  void begin(uint8_t leftPin, uint8_t rightPin);
  void set(int left, int right);
  void stop();

 private:
  Servo left_;
  Servo right_;
};
