// Host-side tests for Strategy. No hardware or Arduino libraries needed.
//
// Build and run from the 03-final/ folder:
//   g++ -std=c++17 -I. test/strategy_test.cpp Strategy.cpp -o strategy_test && ./strategy_test

#include <cstdio>
#include <cstdlib>

#include "Strategy.h"
#include "config.h"

using namespace config;
using State = Strategy::State;

namespace {

int failures = 0;
int checks = 0;

void check(bool ok, const char* what, int line) {
  ++checks;
  if (!ok) {
    std::printf("FAIL (line %d): %s\n", line, what);
    ++failures;
  }
}
#define CHECK(expr) check((expr), #expr, __LINE__)

constexpr SensorReadings kNothing{};
constexpr SensorReadings kOpponentAhead{false, false, true, true};
constexpr SensorReadings kOpponentLeft{false, false, true, false};
constexpr SensorReadings kOpponentRight{false, false, false, true};
constexpr SensorReadings kEdgeLeft{true, false, false, false};
constexpr SensorReadings kEdgeBoth{true, true, false, false};

constexpr MotorCommand kStop{kMotorStop, kMotorStop};
constexpr MotorCommand kDash{kOpeningSpeed, kOpeningSpeed};
constexpr MotorCommand kSweepRight{kSearchFastSide, kSearchSlowSide};
constexpr MotorCommand kSweepLeft{kSearchSlowSide, kSearchFastSide};
constexpr MotorCommand kPush{kAttackSpeed, kAttackSpeed};
constexpr MotorCommand kReverse{kEscapeReverseSpeed, kEscapeReverseSpeed};
constexpr MotorCommand kTurnRight{kEscapeTurnForward, kEscapeTurnBackward};
constexpr MotorCommand kTurnLeft{kEscapeTurnBackward, kEscapeTurnForward};

Strategy started(Tactic tactic, Side side, uint32_t nowMs = 0) {
  Strategy s;
  s.configure({tactic, side});
  s.start(nowMs);
  return s;
}

void radarOpensWithDashThenSweeps() {
  Strategy s = started(Tactic::Radar, Side::Right);
  CHECK(s.update(kOpponentAhead, 100) == kDash);  // opening ignores the opponent
  CHECK(s.state() == State::Opening);
  CHECK(s.update(kNothing, kRadarDashMs) == kSweepRight);
  CHECK(s.state() == State::Search);
}

void flankRunsItsThreeSteps() {
  Strategy s = started(Tactic::Flank, Side::Left);
  const MotorCommand spinLeft{kOpeningSpinBackward, kOpeningSpinForward};
  const MotorCommand spinRight{kOpeningSpinForward, kOpeningSpinBackward};

  uint32_t t = 0;
  CHECK(s.update(kNothing, t) == spinLeft);
  t += kFlankTurnOutMs;
  CHECK(s.update(kNothing, t) == kDash);
  t += kFlankDriveMs;
  CHECK(s.update(kNothing, t) == spinRight);
  t += kFlankTurnInMs;
  CHECK(s.update(kNothing, t) == kSweepLeft);
  CHECK(s.state() == State::Search);
}

void counterWaitsThenSweeps() {
  Strategy s = started(Tactic::Counter, Side::Right);
  CHECK(s.update(kNothing, 0) == kStop);
  CHECK(s.update(kNothing, kCounterPatienceMs - 1) == kStop);
  CHECK(s.update(kNothing, kCounterPatienceMs) == kSweepRight);
}

void counterReactsImmediately() {
  Strategy s = started(Tactic::Counter, Side::Right);
  CHECK(s.update(kOpponentAhead, 10) == kPush);
  CHECK(s.state() == State::Attack);
}

void attackSteersTowardOpponent() {
  Strategy s = started(Tactic::Counter, Side::Right);
  CHECK(s.update(kOpponentLeft, 0) == (MotorCommand{kAttackInnerSpeed, kAttackSpeed}));
  CHECK(s.update(kOpponentRight, 10) == (MotorCommand{kAttackSpeed, kAttackInnerSpeed}));
}

void searchFollowsLastSeenSide() {
  Strategy s = started(Tactic::Radar, Side::Right, 0);
  s.update(kNothing, kRadarDashMs);
  s.update(kOpponentLeft, kRadarDashMs + 10);  // opponent seen on the left, then lost
  CHECK(s.update(kNothing, kRadarDashMs + 20) == kSweepLeft);
}

void edgeOverridesAttack() {
  Strategy s = started(Tactic::Counter, Side::Right);
  s.update(kOpponentLeft, 0);
  SensorReadings halfOnOpponent = kOpponentLeft;  // only one sensor on it
  halfOnOpponent.edgeLeft = true;
  CHECK(s.update(halfOnOpponent, 10) == kReverse);
  CHECK(s.state() == State::EscapeReverse);
}

void pushThroughEdgeWhenLockedOn() {
  Strategy s = started(Tactic::Counter, Side::Right);
  s.update(kOpponentAhead, 0);
  SensorReadings opponentGoingOut = kOpponentAhead;
  opponentGoingOut.edgeLeft = true;
  opponentGoingOut.edgeRight = true;
  CHECK(s.update(opponentGoingOut, 10) == kPush);
  CHECK(s.state() == State::Attack);
}

void edgeWinsWhenNotYetAttacking() {
  // Seeing the opponent and the edge in the same reading, from search:
  // there is no push under way, so the edge still comes first.
  Strategy s = started(Tactic::Counter, Side::Right);
  SensorReadings both = kOpponentAhead;
  both.edgeRight = true;
  CHECK(s.update(both, 0) == kReverse);
}

void rightEdgeTurnsLeft() {
  Strategy s = started(Tactic::Counter, Side::Right);
  s.update(SensorReadings{false, true, false, false}, 0);
  CHECK(s.update(kNothing, kEscapeReverseMs) == kTurnLeft);
}

void edgeInterruptsOpening() {
  Strategy s = started(Tactic::Flank, Side::Right);
  s.update(kNothing, 0);
  CHECK(s.update(kEdgeLeft, 50) == kReverse);
  s.update(kNothing, 50 + kEscapeReverseMs);
  // after the escape the robot searches; the opening is not resumed
  CHECK(s.update(kNothing, 50 + kEscapeReverseMs + kEscapeTurnMs) == kSweepRight);
  CHECK(s.state() == State::Search);
}

void secondMatchStartsClean() {
  Strategy s = started(Tactic::Radar, Side::Left);
  s.update(kEdgeBoth, 0);  // first match ends mid-escape
  s.start(5000);
  CHECK(s.state() == State::Opening);
  CHECK(s.update(kNothing, 5000) == kDash);
}

void escapeReversesThenTurnsAway() {
  Strategy s = started(Tactic::Counter, Side::Right);
  uint32_t t = 0;
  CHECK(s.update(kEdgeLeft, t) == kReverse);
  t += 100;
  CHECK(s.update(kEdgeLeft, t) == kReverse);  // still on the edge: timer restarts
  CHECK(s.update(kNothing, t + kEscapeReverseMs - 1) == kReverse);
  t += kEscapeReverseMs;
  CHECK(s.update(kNothing, t) == kTurnRight);  // left edge -> turn right
  CHECK(s.state() == State::EscapeTurn);
  t += kEscapeTurnMs;
  CHECK(s.update(kNothing, t) == kStop);  // back to Counter search: wait
  CHECK(s.state() == State::Search);
}

void bothEdgesTurnTowardLastSeen() {
  Strategy s = started(Tactic::Radar, Side::Left);
  uint32_t t = 0;
  s.update(kEdgeBoth, t);
  t += kEscapeReverseMs;
  CHECK(s.update(kNothing, t) == kTurnLeft);
  CHECK(s.update(kNothing, t + kEscapeTurnMs) == kTurnLeft);  // longer turn
  CHECK(s.update(kNothing, t + kEscapeTurnBothMs) == kSweepLeft);
}

void opponentInterruptsEscapeTurn() {
  Strategy s = started(Tactic::Radar, Side::Right);
  s.update(kEdgeLeft, 0);
  s.update(kNothing, kEscapeReverseMs);
  CHECK(s.update(kOpponentAhead, kEscapeReverseMs + 10) == kPush);
}

void timingSurvivesMillisWrap() {
  Strategy s = started(Tactic::Radar, Side::Right, 0xFFFFFF00u);
  CHECK(s.update(kNothing, 0x00000010u) == kDash);        // 272 ms after start
  CHECK(s.update(kNothing, 0x00000040u) == kSweepRight);  // 320 ms after start
}

}  // namespace

int main() {
  radarOpensWithDashThenSweeps();
  flankRunsItsThreeSteps();
  counterWaitsThenSweeps();
  counterReactsImmediately();
  attackSteersTowardOpponent();
  searchFollowsLastSeenSide();
  edgeOverridesAttack();
  pushThroughEdgeWhenLockedOn();
  edgeWinsWhenNotYetAttacking();
  rightEdgeTurnsLeft();
  edgeInterruptsOpening();
  secondMatchStartsClean();
  escapeReversesThenTurnsAway();
  bothEdgesTurnTowardLastSeen();
  opponentInterruptsEscapeTurn();
  timingSurvivesMillisWrap();

  if (failures == 0) {
    std::printf("All strategy tests passed (%d checks).\n", checks);
    return EXIT_SUCCESS;
  }
  std::printf("%d of %d checks failed.\n", failures, checks);
  return EXIT_FAILURE;
}
