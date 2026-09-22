#pragma once

#include <Arduino.h>

#include "Strategy.h"

// Reads the edge and opponent sensors and turns them into true/false
// readings for the strategy.
class Sensors {
 public:
  void begin();

  // Must run with the robot sitting on the black ring.
  void calibrate();

  SensorReadings read() const;

 private:
  int leftEdgeThreshold_ = 0;
  int rightEdgeThreshold_ = 0;
};
