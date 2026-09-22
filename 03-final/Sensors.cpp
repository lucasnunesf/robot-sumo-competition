#include "Sensors.h"

#include "config.h"

void Sensors::begin() {
  pinMode(config::kLeftOpponentSensorPin, INPUT);
  pinMode(config::kRightOpponentSensorPin, INPUT);
}

void Sensors::calibrate() {
  leftEdgeThreshold_ = analogRead(config::kLeftEdgeSensorPin) - config::kEdgeMargin;
  rightEdgeThreshold_ = analogRead(config::kRightEdgeSensorPin) - config::kEdgeMargin;
}

SensorReadings Sensors::read() const {
  SensorReadings readings;
  // The reading drops over the white border.
  readings.edgeLeft = analogRead(config::kLeftEdgeSensorPin) < leftEdgeThreshold_;
  readings.edgeRight = analogRead(config::kRightEdgeSensorPin) < rightEdgeThreshold_;
  readings.opponentLeft = digitalRead(config::kLeftOpponentSensorPin) == HIGH;
  readings.opponentRight = digitalRead(config::kRightOpponentSensorPin) == HIGH;
  return readings;
}
