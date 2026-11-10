// Compile-time checks on config.h. If a retune breaks one of these rules,
// the build fails before the code reaches the robot.
//
//   g++ -std=c++17 -I. test/config_test.cpp -o config_test && ./config_test

#include <cstdio>

#include "config.h"

using namespace config;

constexpr bool inEscRange(int v) { return v >= kMotorMin && v <= kMotorMax; }
constexpr bool forward(int v) { return v > kMotorStop; }
constexpr bool reverse(int v) { return v < kMotorStop; }

// Every motor value the strategy can send is one the ESCs accept.
static_assert(inEscRange(kOpeningSpeed) && inEscRange(kOpeningSpinForward) &&
                  inEscRange(kOpeningSpinBackward) && inEscRange(kSearchFastSide) &&
                  inEscRange(kSearchSlowSide) && inEscRange(kAttackSpeed) &&
                  inEscRange(kAttackInnerSpeed) && inEscRange(kEscapeReverseSpeed) &&
                  inEscRange(kEscapeTurnForward) && inEscRange(kEscapeTurnBackward),
              "motor value outside the ESC range");

// Directions: a sign error here sends the robot the wrong way.
static_assert(forward(kOpeningSpeed) && forward(kAttackSpeed) && forward(kAttackInnerSpeed),
              "dash and attack must drive forward");
static_assert(reverse(kEscapeReverseSpeed), "escape must reverse");
static_assert(forward(kOpeningSpinForward) && reverse(kOpeningSpinBackward) &&
                  forward(kEscapeTurnForward) && reverse(kEscapeTurnBackward),
              "spins need one wheel forward and one backward");
static_assert(kSearchFastSide > kSearchSlowSide && forward(kSearchSlowSide),
              "search sweep is a forward arc: both wheels forward, outer one faster");
static_assert(kAttackInnerSpeed < kAttackSpeed, "steering while attacking slows the inner wheel");

// Hardware rules.
static_assert(kLeftEdgeSensorPin >= 32 && kLeftEdgeSensorPin <= 39 &&
                  kRightEdgeSensorPin >= 32 && kRightEdgeSensorPin <= 39,
              "edge sensors must be on ADC1 (GPIO 32-39): ADC2 stops working with Bluetooth on");
constexpr uint8_t kPins[] = {kLeftMotorPin,       kRightMotorPin,     kLeftOpponentSensorPin,
                             kRightOpponentSensorPin, kLeftEdgeSensorPin, kRightEdgeSensorPin,
                             kIrReceiverPin};
constexpr bool pinsUnique() {
  for (unsigned i = 0; i < sizeof kPins; ++i)
    for (unsigned j = i + 1; j < sizeof kPins; ++j)
      if (kPins[i] == kPins[j]) return false;
  return true;
}
static_assert(pinsUnique(), "two parts share a pin");
static_assert(kEdgeMargin > 0 && kEdgeMargin < 4096, "edge margin must fit the 12-bit ADC");

// Start module and timing.
static_assert(kIrCodeReady != kIrCodeStart && kIrCodeStart != kIrCodeStop &&
                  kIrCodeReady != kIrCodeStop,
              "IR codes must be distinct");
static_assert(kEscapeTurnBothMs > kEscapeTurnMs, "with both edges seen, the turn is longer");
static_assert(kRadarDashMs > 0 && kFlankTurnOutMs > 0 && kFlankDriveMs > 0 &&
                  kFlankTurnInMs > 0 && kEscapeReverseMs > 0 && kCounterPatienceMs > 0,
              "timings must be positive");

int main() {
  std::puts("config.h checks passed (12 rules, checked at compile time).");
  return 0;
}
