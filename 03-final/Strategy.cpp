#include "Strategy.h"

#include "config.h"

using namespace config;

namespace {

Side opposite(Side side) { return side == Side::Left ? Side::Right : Side::Left; }

// Spinning toward the right means the left wheel goes forward and the right
// wheel backward.
MotorCommand spin(Side toward, int forward, int backward) {
  return toward == Side::Right ? MotorCommand{forward, backward}
                               : MotorCommand{backward, forward};
}

}  // namespace

// ---------------------------------------------------------------------------
// Opening plans
// ---------------------------------------------------------------------------

Strategy::OpeningPlan Strategy::openingFor(Tactic tactic) {
  static constexpr Step kRadar[] = {
      {Move::Straight, kRadarDashMs},
  };
  static constexpr Step kFlank[] = {
      {Move::SpinTowardSide, kFlankTurnOutMs},
      {Move::Straight, kFlankDriveMs},
      {Move::SpinAwayFromSide, kFlankTurnInMs},
  };

  switch (tactic) {
    case Tactic::Radar:
      return {kRadar, sizeof(kRadar) / sizeof(kRadar[0])};
    case Tactic::Flank:
      return {kFlank, sizeof(kFlank) / sizeof(kFlank[0])};
    case Tactic::Counter:
    default:
      return {nullptr, 0};
  }
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

void Strategy::start(uint32_t nowMs) {
  lastSeen_ = options_.side;
  openingStep_ = 0;
  edgeBoth_ = false;
  enter(openingFor(options_.tactic).count > 0 ? State::Opening : State::Search, nowMs);
}

MotorCommand Strategy::update(const SensorReadings& in, uint32_t nowMs) {
  const bool edge = in.edgeLeft || in.edgeRight;
  const bool opponent = in.opponentLeft || in.opponentRight;

  // 1. Staying in the ring comes first, in every state.
  if (edge) {
    if (state_ != State::EscapeReverse) {
      edgeSide_ = in.edgeLeft ? Side::Left : Side::Right;
      edgeBoth_ = false;
    }
    edgeBoth_ = edgeBoth_ || (in.edgeLeft && in.edgeRight);
    // (Re)starting the timer keeps the robot reversing until the edge clears.
    enter(State::EscapeReverse, nowMs);
    return {kEscapeReverseSpeed, kEscapeReverseSpeed};
  }

  switch (state_) {
    case State::EscapeReverse:
      if (elapsed(nowMs) < kEscapeReverseMs) {
        return {kEscapeReverseSpeed, kEscapeReverseSpeed};
      }
      enter(State::EscapeTurn, nowMs);
      return escapeTurn();

    case State::EscapeTurn: {
      const uint32_t turnMs = edgeBoth_ ? kEscapeTurnBothMs : kEscapeTurnMs;
      if (!opponent && elapsed(nowMs) < turnMs) {
        return escapeTurn();
      }
      break;  // turn finished, or opponent already in view
    }

    case State::Opening:
      // Opening moves are committed: only the edge can interrupt them.
      if (openingStep_ < openingFor(options_.tactic).count) {
        return runOpening(nowMs);
      }
      break;

    case State::Search:
    case State::Attack:
      break;
  }

  // 2. Attack when the opponent is in view.
  if (opponent) {
    if (state_ != State::Attack) enter(State::Attack, nowMs);
    return attack(in);
  }

  // 3. Otherwise search.
  if (state_ != State::Search) enter(State::Search, nowMs);

  if (options_.tactic == Tactic::Counter && elapsed(nowMs) < kCounterPatienceMs) {
    return {kMotorStop, kMotorStop};
  }
  return sweep(lastSeen_);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void Strategy::enter(State next, uint32_t nowMs) {
  state_ = next;
  stateStartMs_ = nowMs;
}

MotorCommand Strategy::runOpening(uint32_t nowMs) {
  const OpeningPlan plan = openingFor(options_.tactic);

  // Advance through every step whose time is up (unsigned subtraction keeps
  // this correct when millis() wraps around).
  while (openingStep_ < plan.count && elapsed(nowMs) >= plan.steps[openingStep_].durationMs) {
    stateStartMs_ += plan.steps[openingStep_].durationMs;
    ++openingStep_;
  }

  if (openingStep_ >= plan.count) {
    enter(State::Search, nowMs);
    return sweep(lastSeen_);
  }
  return moveCommand(plan.steps[openingStep_].move);
}

MotorCommand Strategy::moveCommand(Move move) const {
  switch (move) {
    case Move::SpinTowardSide:
      return spin(options_.side, kOpeningSpinForward, kOpeningSpinBackward);
    case Move::SpinAwayFromSide:
      return spin(opposite(options_.side), kOpeningSpinForward, kOpeningSpinBackward);
    case Move::Straight:
    default:
      return {kOpeningSpeed, kOpeningSpeed};
  }
}

// Wide arc toward one side: both wheels forward, the outer one faster.
MotorCommand Strategy::sweep(Side toward) const {
  return toward == Side::Right ? MotorCommand{kSearchFastSide, kSearchSlowSide}
                               : MotorCommand{kSearchSlowSide, kSearchFastSide};
}

// Full push when both sensors see the opponent; when only one does, steer
// toward it while still pushing, so the robot ends up square to it.
MotorCommand Strategy::attack(const SensorReadings& in) {
  if (in.opponentLeft && in.opponentRight) {
    return {kAttackSpeed, kAttackSpeed};
  }
  if (in.opponentLeft) {
    lastSeen_ = Side::Left;
    return {kAttackInnerSpeed, kAttackSpeed};
  }
  lastSeen_ = Side::Right;
  return {kAttackSpeed, kAttackInnerSpeed};
}

// Turn away from the edge. If both sensors hit it, turn toward where the
// opponent was last seen instead.
MotorCommand Strategy::escapeTurn() const {
  const Side toward = edgeBoth_ ? lastSeen_ : opposite(edgeSide_);
  return spin(toward, kEscapeTurnForward, kEscapeTurnBackward);
}
