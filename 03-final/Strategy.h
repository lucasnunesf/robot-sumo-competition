#pragma once

#include <stddef.h>
#include <stdint.h>

// What the strategy needs to know about the world, already decided as
// true/false by the Sensors module.
struct SensorReadings {
  bool edgeLeft = false;
  bool edgeRight = false;
  bool opponentLeft = false;
  bool opponentRight = false;
};

struct MotorCommand {
  int left;
  int right;
};

inline bool operator==(MotorCommand a, MotorCommand b) {
  return a.left == b.left && a.right == b.right;
}

enum class Side : uint8_t { Left, Right };

enum class Tactic : uint8_t {
  Radar,    // short dash to the centre, then sweep
  Flank,    // curve around the opponent's front and hit it from the side
  Counter,  // stay still and let the opponent come, then react
};

struct StrategyOptions {
  Tactic tactic = Tactic::Radar;
  Side side = Side::Right;  // first sweep direction, and flank side
};

// Match strategy as an explicit state machine.
//
// Priority on every update: stay in the ring, then attack, then search.
// No hardware access and no blocking calls: it receives sensor readings and
// the current time and returns a motor command, so it can be unit-tested on
// a PC (see test/).
class Strategy {
 public:
  enum class State : uint8_t { Opening, Search, Attack, EscapeReverse, EscapeTurn };

  void configure(const StrategyOptions& options) { options_ = options; }
  const StrategyOptions& options() const { return options_; }

  void start(uint32_t nowMs);
  MotorCommand update(const SensorReadings& in, uint32_t nowMs);

  State state() const { return state_; }

 private:
  // One timed step of an opening move.
  enum class Move : uint8_t { Straight, SpinTowardSide, SpinAwayFromSide };
  struct Step {
    Move move;
    uint32_t durationMs;
  };
  struct OpeningPlan {
    const Step* steps;
    size_t count;
  };

  static OpeningPlan openingFor(Tactic tactic);

  void enter(State next, uint32_t nowMs);
  uint32_t elapsed(uint32_t nowMs) const { return nowMs - stateStartMs_; }

  MotorCommand runOpening(uint32_t nowMs);
  MotorCommand moveCommand(Move move) const;
  MotorCommand sweep(Side toward) const;
  MotorCommand attack(const SensorReadings& in);
  MotorCommand escapeTurn() const;

  StrategyOptions options_;
  State state_ = State::Search;
  uint32_t stateStartMs_ = 0;
  size_t openingStep_ = 0;
  Side lastSeen_ = Side::Right;  // side the opponent was last seen on
  Side edgeSide_ = Side::Left;   // side that triggered the current escape
  bool edgeBoth_ = false;
};
