#pragma once

#include <stdint.h>

// All pins, tuning values and protocol codes in one place, so the robot can
// be retuned without touching the logic. This file has no Arduino
// dependencies, so the strategy tests can include it on a PC.
namespace config {

// ---- Pins (ESP32) ----
constexpr uint8_t kLeftMotorPin = 27;
constexpr uint8_t kRightMotorPin = 26;
constexpr uint8_t kLeftOpponentSensorPin = 17;
constexpr uint8_t kRightOpponentSensorPin = 16;
// Edge sensors must be on ADC1 (GPIO 32-39): ADC2 is shared with the radio.
constexpr uint8_t kLeftEdgeSensorPin = 34;
constexpr uint8_t kRightEdgeSensorPin = 35;
constexpr uint8_t kIrReceiverPin = 23;

constexpr const char* kPs4PairedAddress = "44:1c:a8:c6:41:74";

// ---- Motor commands (servo degrees sent to the ESCs) ----
// 90 = stopped, above 90 = forward, below 90 = reverse.
constexpr int kMotorStop = 90;
constexpr int kMotorMin = 30;
constexpr int kMotorMax = 150;

// ---- Edge detection ----
// The reading drops over the white border. Threshold = reading on the black
// ring at calibration minus this margin (12-bit ADC, 0-4095).
constexpr int kEdgeMargin = 1500;

// ---- Opening moves ----
constexpr int kOpeningSpeed = 120;
constexpr int kOpeningSpinForward = 115;
constexpr int kOpeningSpinBackward = 65;
constexpr uint32_t kRadarDashMs = 300;
// Flank: turn to one side, drive past the opponent's front, turn back in.
constexpr uint32_t kFlankTurnOutMs = 120;
constexpr uint32_t kFlankDriveMs = 350;
constexpr uint32_t kFlankTurnInMs = 250;

// ---- Search ----
constexpr int kSearchFastSide = 110;  // outer wheel while sweeping
constexpr int kSearchSlowSide = 95;   // inner wheel while sweeping
// Counter tactic: how long to stand still waiting for the opponent to come
// in before falling back to a sweep.
constexpr uint32_t kCounterPatienceMs = 2000;

// ---- Attack ----
constexpr int kAttackSpeed = 150;
constexpr int kAttackInnerSpeed = 125;  // inner wheel when only one sensor sees

// ---- Edge escape ----
constexpr int kEscapeReverseSpeed = 70;
constexpr uint32_t kEscapeReverseMs = 450;  // extra reverse after the edge clears
constexpr int kEscapeTurnForward = 100;
constexpr int kEscapeTurnBackward = 80;
constexpr uint32_t kEscapeTurnMs = 350;
constexpr uint32_t kEscapeTurnBothMs = 600;  // both sensors on the edge: turn further

// ---- Manual mode ----
constexpr int kTriggerDeadzone = 10;
constexpr int kStickDeadzone = 10;
constexpr int kSlowForwardValue = 63;
constexpr int kBoostForwardValue = 180;
constexpr int kForwardRange = 60;
constexpr int kReverseRange = 60;
constexpr int kSteeringRange = 20;

// ---- IR start module codes ----
constexpr uint32_t kIrCodeReady = 0x10;
constexpr uint32_t kIrCodeStart = 0x810;
constexpr uint32_t kIrCodeStop = 0x410;

// ---- Controller LED ----
constexpr uint8_t kLedLevel = 100;
constexpr uint32_t kReadyBlinkMs = 200;

}  // namespace config
