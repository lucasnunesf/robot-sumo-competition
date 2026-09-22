#pragma once

#include <Arduino.h>
#include <IRremote.h>

enum class StartSignal { None, Ready, Start, Stop };

// IR start module used by the referee to arm, start and stop the robot.
class StartModule {
 public:
  explicit StartModule(uint8_t pin) : receiver_(pin) {}

  void begin() { receiver_.enableIRIn(); }
  StartSignal read();

 private:
  IRrecv receiver_;
  decode_results results_;
};
