#pragma once

#include <stdint.h>

// All pins, tuning values and protocol codes in one place, so the robot can
// be retuned without touching the logic.
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
// The sensor reading drops over the white border. The threshold is taken at
// calibration: reading on the black ring minus this margin (12-bit ADC).
constexpr int kEdgeMargin = 1500;

// ---- Radar tactic ----
constexpr uint32_t kOpeningDashMs = 300;
constexpr int kOpeningDashSpeed = 120;
constexpr int kSearchFastSide = 110;  // outer wheel while sweeping
constexpr int kSearchSlowSide = 95;   // inner wheel while sweeping
constexpr int kAttackSpeed = 150;

// ---- Edge escape ----
constexpr int kEscapeReverseSpeed = 70;
constexpr uint32_t kEscapeReverseMs = 450;  // extra reverse after the edge clears
constexpr int kEscapeTurnForward = 100;
constexpr int kEscapeTurnBackward = 80;
constexpr uint32_t kEscapeTurnMs = 350;

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
