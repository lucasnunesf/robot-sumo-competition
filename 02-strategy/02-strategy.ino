// 02 - Strategy
//
// Adds the autonomous match mode to the PS4 base from 01:
//  - IR start module (Ready / Start / Stop), as used by referees
//  - edge sensors, calibrated on the ring before each match
//  - radar tactic: sweep until the opponent is seen, then charge
//  - edge escape, which overrides everything else
//
// OPTIONS cycles LOCKED (red) -> AUTONOMOUS (green) -> MANUAL (blue).
// The whole loop is non-blocking: no delay() once the robot is running.

#include <ESP32Servo.h>
#include <IRremote.h>
#include <PS4Controller.h>

#include "config.h"

namespace {

enum class Mode { Locked, Autonomous, Manual };
enum class MatchState { Stopped, Ready, Running };
enum class Behaviour { OpeningDash, Search, Attack, EscapeReverse, EscapeTurn };
enum class Side { Left, Right };

Servo leftMotor;
Servo rightMotor;
IRrecv irReceiver(config::kIrReceiverPin);
decode_results irResults;

Mode mode = Mode::Locked;
MatchState matchState = MatchState::Stopped;
Behaviour behaviour = Behaviour::Search;
uint32_t behaviourStartMs = 0;

// Tactic settings, chosen with the D-pad before the start.
Side searchSide = Side::Right;
bool openingDash = true;

Side edgeSide = Side::Left;  // which sensor triggered the current escape
int leftEdgeThreshold = 0;
int rightEdgeThreshold = 0;

bool optionsWasPressed = false;
uint32_t lastBlinkMs = 0;
bool blinkOn = false;

// ---------------------------------------------------------------------------
// Hardware helpers
// ---------------------------------------------------------------------------

void setMotors(int left, int right) {
  leftMotor.write(left);
  rightMotor.write(right);
}

void stopMotors() { setMotors(config::kMotorStop, config::kMotorStop); }

void setControllerLed(uint8_t red, uint8_t green, uint8_t blue) {
  PS4.setLed(red, green, blue);
  PS4.sendToController();
}

bool opponentSeen() {
  return digitalRead(config::kLeftOpponentSensorPin) == HIGH ||
         digitalRead(config::kRightOpponentSensorPin) == HIGH;
}

bool leftEdgeSeen() { return analogRead(config::kLeftEdgeSensorPin) < leftEdgeThreshold; }
bool rightEdgeSeen() { return analogRead(config::kRightEdgeSensorPin) < rightEdgeThreshold; }

// Robot must be sitting on the black ring when this runs.
void calibrateEdgeSensors() {
  leftEdgeThreshold = analogRead(config::kLeftEdgeSensorPin) - config::kEdgeMargin;
  rightEdgeThreshold = analogRead(config::kRightEdgeSensorPin) - config::kEdgeMargin;
}

// ---------------------------------------------------------------------------
// Autonomous behaviour (non-blocking state machine)
// ---------------------------------------------------------------------------

void enterBehaviour(Behaviour next, uint32_t nowMs) {
  behaviour = next;
  behaviourStartMs = nowMs;
}

uint32_t timeInBehaviour(uint32_t nowMs) { return nowMs - behaviourStartMs; }

void startMatch(uint32_t nowMs) {
  enterBehaviour(openingDash ? Behaviour::OpeningDash : Behaviour::Search, nowMs);
}

void runMatch(uint32_t nowMs) {
  using namespace config;

  // 1. Staying in the ring comes first.
  const bool leftEdge = leftEdgeSeen();
  const bool rightEdge = rightEdgeSeen();
  if ((leftEdge || rightEdge) && behaviour != Behaviour::EscapeReverse) {
    edgeSide = leftEdge ? Side::Left : Side::Right;
    enterBehaviour(Behaviour::EscapeReverse, nowMs);
  }

  switch (behaviour) {
    case Behaviour::EscapeReverse:
      setMotors(kEscapeReverseSpeed, kEscapeReverseSpeed);
      if (leftEdge || rightEdge) {
        behaviourStartMs = nowMs;  // keep reversing until the edge is clear
      } else if (timeInBehaviour(nowMs) >= kEscapeReverseMs) {
        enterBehaviour(Behaviour::EscapeTurn, nowMs);
      }
      return;

    case Behaviour::EscapeTurn:
      // Turn away from the side that saw the edge.
      if (edgeSide == Side::Left) {
        setMotors(kEscapeTurnForward, kEscapeTurnBackward);
      } else {
        setMotors(kEscapeTurnBackward, kEscapeTurnForward);
      }
      if (timeInBehaviour(nowMs) >= kEscapeTurnMs) {
        enterBehaviour(Behaviour::Search, nowMs);
      }
      return;

    case Behaviour::OpeningDash:
      setMotors(kOpeningDashSpeed, kOpeningDashSpeed);
      if (timeInBehaviour(nowMs) >= kOpeningDashMs) {
        enterBehaviour(Behaviour::Search, nowMs);
      }
      return;

    case Behaviour::Search:
    case Behaviour::Attack:
      break;
  }

  // 2. Attack if the opponent is in view, 3. otherwise search.
  if (opponentSeen()) {
    if (behaviour == Behaviour::Search) {
      // After each contact the next sweep goes the other way.
      searchSide = (searchSide == Side::Right) ? Side::Left : Side::Right;
      enterBehaviour(Behaviour::Attack, nowMs);
    }
    setMotors(kAttackSpeed, kAttackSpeed);
  } else {
    if (behaviour == Behaviour::Attack) {
      enterBehaviour(Behaviour::Search, nowMs);
    }
    if (searchSide == Side::Right) {
      setMotors(kSearchFastSide, kSearchSlowSide);
    } else {
      setMotors(kSearchSlowSide, kSearchFastSide);
    }
  }
}

// ---------------------------------------------------------------------------
// Match control: IR start module and D-pad settings
// ---------------------------------------------------------------------------

void readIrStartModule(uint32_t nowMs) {
  if (!irReceiver.decode(&irResults)) return;
  const uint32_t code = irResults.value;
  irReceiver.resume();

  if (code == config::kIrCodeReady && matchState == MatchState::Stopped) {
    matchState = MatchState::Ready;
    calibrateEdgeSensors();
  } else if (code == config::kIrCodeStart && matchState == MatchState::Ready) {
    matchState = MatchState::Running;
    setControllerLed(0, config::kLedLevel, 0);
    startMatch(nowMs);
  } else if (code == config::kIrCodeStop && matchState != MatchState::Stopped) {
    matchState = MatchState::Stopped;
    setControllerLed(0, config::kLedLevel, 0);
  }
}

void readTacticSettings() {
  if (PS4.Right()) searchSide = Side::Right;
  if (PS4.Left()) searchSide = Side::Left;
  if (PS4.Up()) openingDash = true;
  if (PS4.Down()) openingDash = false;
}

void blinkReadyLed(uint32_t nowMs) {
  if (nowMs - lastBlinkMs < config::kReadyBlinkMs) return;
  lastBlinkMs = nowMs;
  blinkOn = !blinkOn;
  setControllerLed(0, blinkOn ? config::kLedLevel : 0, 0);
}

void runAutonomous(uint32_t nowMs) {
  readIrStartModule(nowMs);

  switch (matchState) {
    case MatchState::Stopped:
      stopMotors();
      readTacticSettings();
      break;
    case MatchState::Ready:
      stopMotors();
      readTacticSettings();
      blinkReadyLed(nowMs);
      break;
    case MatchState::Running:
      runMatch(nowMs);
      break;
  }
}

// ---------------------------------------------------------------------------
// Manual mode (same as 01)
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

  setMotors(constrain(base - brake + turn, kMotorMin, kMotorMax),
            constrain(base - brake - turn, kMotorMin, kMotorMax));
}

// ---------------------------------------------------------------------------
// Mode selection
// ---------------------------------------------------------------------------

void enterMode(Mode next) {
  mode = next;
  matchState = MatchState::Stopped;
  stopMotors();
  switch (mode) {
    case Mode::Locked:     setControllerLed(config::kLedLevel, 0, 0); break;
    case Mode::Autonomous: setControllerLed(0, config::kLedLevel, 0); break;
    case Mode::Manual:     setControllerLed(0, 0, config::kLedLevel); break;
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

void setup() {
  leftMotor.attach(config::kLeftMotorPin);
  rightMotor.attach(config::kRightMotorPin);
  stopMotors();

  pinMode(config::kLeftOpponentSensorPin, INPUT);
  pinMode(config::kRightOpponentSensorPin, INPUT);
  irReceiver.enableIRIn();

  PS4.begin(config::kPs4PairedAddress);
  while (!PS4.isConnected()) {
    delay(250);
  }
  enterMode(Mode::Locked);
}

void loop() {
  if (!PS4.isConnected()) {
    stopMotors();
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
