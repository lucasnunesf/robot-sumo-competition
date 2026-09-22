// 01 - PS4 integration
//
// First step of the robot: drive it by hand with a PS4 controller over
// Bluetooth. Used to check wiring, motor direction and ESC response before
// any autonomous code exists.
//
// OPTIONS toggles between LOCKED (red LED, motors off) and MANUAL (blue LED).

#include <ESP32Servo.h>
#include <PS4Controller.h>

namespace config {

// ---- Pins (ESP32) ----
constexpr uint8_t kLeftMotorPin = 27;
constexpr uint8_t kRightMotorPin = 26;

// Bluetooth address the PS4 controller is paired to (e.g. with SixaxisPairTool).
constexpr const char* kPs4PairedAddress = "44:1c:a8:c6:41:74";

// ---- Motor commands (servo degrees sent to the ESCs) ----
// 90 = stopped, above 90 = forward, below 90 = reverse.
constexpr int kMotorStop = 90;
constexpr int kMotorMin = 30;
constexpr int kMotorMax = 150;

// ---- Driving ----
constexpr int kTriggerDeadzone = 10;     // R2 readings below this are ignored
constexpr int kStickDeadzone = 10;       // |stick X| below this is ignored
constexpr int kSlowForwardValue = 63;    // Cross: fixed slow speed (trigger scale 0-255)
constexpr int kBoostForwardValue = 180;  // Cross + Square: fixed fast speed
constexpr int kForwardRange = 60;        // R2 fully pressed = stop + 60
constexpr int kReverseRange = 60;        // L2 fully pressed = stop - 60
constexpr int kSteeringRange = 20;       // stick fully sideways = +/-20 per wheel

constexpr uint8_t kLedLevel = 100;

}  // namespace config

namespace {

enum class Mode { Locked, Manual };

Servo leftMotor;
Servo rightMotor;

Mode mode = Mode::Locked;
bool optionsWasPressed = false;

void setMotors(int left, int right) {
  leftMotor.write(left);
  rightMotor.write(right);
}

void stopMotors() { setMotors(config::kMotorStop, config::kMotorStop); }

void setControllerLed(uint8_t red, uint8_t green, uint8_t blue) {
  PS4.setLed(red, green, blue);
  PS4.sendToController();
}

void enterMode(Mode next) {
  mode = next;
  stopMotors();
  if (mode == Mode::Locked) {
    setControllerLed(config::kLedLevel, 0, 0);
  } else {
    setControllerLed(0, 0, config::kLedLevel);
  }
}

// Acts on the press, not while the button is held.
void handleModeButton() {
  const bool pressed = PS4.Options();
  if (pressed && !optionsWasPressed) {
    enterMode(mode == Mode::Locked ? Mode::Manual : Mode::Locked);
  }
  optionsWasPressed = pressed;
}

void driveManually() {
  using namespace config;

  int forward = PS4.R2Value();        // 0..255
  const int reverse = PS4.L2Value();  // 0..255
  int steering = PS4.LStickX();       // -127..127

  if (abs(steering) < kStickDeadzone) steering = 0;
  if (forward < kTriggerDeadzone) forward = 0;

  // Fixed-speed buttons override the trigger.
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

}  // namespace

void setup() {
  // Put the ESCs in neutral before anything else.
  leftMotor.attach(config::kLeftMotorPin);
  rightMotor.attach(config::kRightMotorPin);
  stopMotors();

  PS4.begin(config::kPs4PairedAddress);
  while (!PS4.isConnected()) {
    delay(250);
  }
  enterMode(Mode::Locked);
}

void loop() {
  // Losing the controller always stops the robot.
  if (!PS4.isConnected()) {
    stopMotors();
    return;
  }

  handleModeButton();
  if (mode == Mode::Manual) {
    driveManually();
  }
}
