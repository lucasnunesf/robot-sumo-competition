// 03 - Final
//
// Complete match firmware, split into modules:
//   Drive        motors
//   Sensors      edge + opponent sensors, with calibration
//   StartModule  IR start module (Ready / Start / Stop)
//   Strategy     match logic as a state machine, testable without hardware
//
// This file only wires them together and handles the PS4 controller.
// OPTIONS cycles LOCKED (red) -> AUTONOMOUS (tactic colour) -> MANUAL (blue).

#include <ESP32Servo.h>
#include <PS4Controller.h>

#include "Drive.h"
#include "Sensors.h"
#include "StartModule.h"
#include "Strategy.h"
#include "config.h"

namespace {

enum class Mode { Locked, Autonomous, Manual };
enum class MatchState { Stopped, Ready, Running };

struct Colour {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

Drive drive;
Sensors sensors;
StartModule startModule(config::kIrReceiverPin);
Strategy strategy;
StrategyOptions options;

Mode mode = Mode::Locked;
MatchState matchState = MatchState::Stopped;
bool optionsWasPressed = false;
uint32_t lastBlinkMs = 0;
bool blinkOn = false;

// ---------------------------------------------------------------------------
// Controller LED
// ---------------------------------------------------------------------------

void setControllerLed(Colour c) {
  PS4.setLed(c.red, c.green, c.blue);
  PS4.sendToController();
}

constexpr Colour kOff{0, 0, 0};
constexpr Colour kRed{config::kLedLevel, 0, 0};
constexpr Colour kBlue{0, 0, config::kLedLevel};

// In autonomous mode the LED shows which tactic is selected.
Colour tacticColour(Tactic tactic) {
  switch (tactic) {
    case Tactic::Flank:   return {0, config::kLedLevel, config::kLedLevel};  // cyan
    case Tactic::Counter: return {config::kLedLevel, config::kLedLevel, 0};  // yellow
    case Tactic::Radar:
    default:              return {0, config::kLedLevel, 0};                  // green
  }
}

// ---------------------------------------------------------------------------
// Autonomous mode
// ---------------------------------------------------------------------------

// Settings are accepted only before the start.
//   Triangle = Radar, Square = Flank, Circle = Counter
//   D-pad left/right = side for the first sweep and the flank
void readTacticSettings() {
  const Tactic previous = options.tactic;

  if (PS4.Triangle()) options.tactic = Tactic::Radar;
  if (PS4.Square()) options.tactic = Tactic::Flank;
  if (PS4.Circle()) options.tactic = Tactic::Counter;
  if (PS4.Left()) options.side = Side::Left;
  if (PS4.Right()) options.side = Side::Right;

  if (options.tactic != previous) {
    setControllerLed(tacticColour(options.tactic));
  }
}

void handleStartSignal(StartSignal signal, uint32_t nowMs) {
  switch (signal) {
    case StartSignal::Ready:
      if (matchState == MatchState::Stopped) {
        matchState = MatchState::Ready;
        sensors.calibrate();
      }
      break;
    case StartSignal::Start:
      if (matchState == MatchState::Ready) {
        matchState = MatchState::Running;
        setControllerLed(tacticColour(options.tactic));
        strategy.configure(options);
        strategy.start(nowMs);
      }
      break;
    case StartSignal::Stop:
      if (matchState != MatchState::Stopped) {
        matchState = MatchState::Stopped;
        drive.stop();
        setControllerLed(tacticColour(options.tactic));
      }
      break;
    case StartSignal::None:
      break;
  }
}

void blinkReadyLed(uint32_t nowMs) {
  if (nowMs - lastBlinkMs < config::kReadyBlinkMs) return;
  lastBlinkMs = nowMs;
  blinkOn = !blinkOn;
  setControllerLed(blinkOn ? tacticColour(options.tactic) : kOff);
}

void runAutonomous(uint32_t nowMs) {
  handleStartSignal(startModule.read(), nowMs);

  switch (matchState) {
    case MatchState::Stopped:
      drive.stop();
      readTacticSettings();
      break;
    case MatchState::Ready:
      drive.stop();
      readTacticSettings();
      blinkReadyLed(nowMs);
      break;
    case MatchState::Running: {
      const MotorCommand command = strategy.update(sensors.read(), nowMs);
      drive.set(command.left, command.right);
      break;
    }
  }
}

// ---------------------------------------------------------------------------
// Manual mode
// ---------------------------------------------------------------------------

void runManual() {
  using namespace config;

  int forward = PS4.R2Value();
  const int reverse = PS4.L2Value();
  int steering = PS4.LStickX();

  if (abs(steering) < kStickDeadzone) steering = 0;
  if (forward < kTriggerDeadzone) forward = 0;

  if (PS4.Cross()) {
    forward = PS4.Square() ? kBoostForwardValue : kSlowForwardValue;
  } else if (PS4.Circle()) {
    forward = -kSlowForwardValue;
  }

  const int base = map(forward, 0, 255, kMotorStop, kMotorStop + kForwardRange);
  const int brake = map(reverse, 0, 255, 0, kReverseRange);
  const int turn = map(steering, -127, 127, -kSteeringRange, kSteeringRange);

  drive.set(constrain(base - brake + turn, kMotorMin, kMotorMax),
            constrain(base - brake - turn, kMotorMin, kMotorMax));
}

// ---------------------------------------------------------------------------
// Mode selection
// ---------------------------------------------------------------------------

void enterMode(Mode next) {
  mode = next;
  matchState = MatchState::Stopped;
  drive.stop();
  switch (mode) {
    case Mode::Locked:     setControllerLed(kRed); break;
    case Mode::Autonomous: setControllerLed(tacticColour(options.tactic)); break;
    case Mode::Manual:     setControllerLed(kBlue); break;
  }
}

void handleModeButton() {
  const bool pressed = PS4.Options();
  if (pressed && !optionsWasPressed) {
    switch (mode) {
      case Mode::Locked:     enterMode(Mode::Autonomous); break;
      case Mode::Autonomous: enterMode(Mode::Manual); break;
      case Mode::Manual:     enterMode(Mode::Locked); break;
    }
  }
  optionsWasPressed = pressed;
}

}  // namespace

// ---------------------------------------------------------------------------
// Arduino entry points
// ---------------------------------------------------------------------------

void setup() {
  // Put the ESCs in neutral before anything else.
  drive.begin(config::kLeftMotorPin, config::kRightMotorPin);
  sensors.begin();
  startModule.begin();

  PS4.begin(config::kPs4PairedAddress);
  while (!PS4.isConnected()) {
    delay(250);
  }
  enterMode(Mode::Locked);
}

void loop() {
  // Losing the controller always stops the robot, in any mode.
  if (!PS4.isConnected()) {
    drive.stop();
    return;
  }

  handleModeButton();

  const uint32_t nowMs = millis();
  switch (mode) {
    case Mode::Autonomous: runAutonomous(nowMs); break;
    case Mode::Manual:     runManual(); break;
    case Mode::Locked:     break;
  }
}
