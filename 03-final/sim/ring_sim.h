#pragma once

// A small 2D simulation of a sumo ring, used to run the real Strategy code
// through whole matches on a PC.
//
// It is deliberately simple: two round robots, a ring with a white border,
// beam sensors for the opponent and point sensors for the edge. It is not a
// physics engine. It answers one question: does the strategy, as written,
// find the opponent, push it out and stay in the ring?
//
// Geometry follows the standard sumo ring: 154 cm across, including a 5 cm
// white border. Robot size and speeds are rough values, not measured ones.

#include <cmath>
#include <cstdint>
#include <random>

#include "../Strategy.h"
#include "../config.h"

namespace sim {

constexpr double kPi = 3.14159265358979323846;

// ---- Ring ----
constexpr double kRingRadius = 0.77;   // m, outer edge
constexpr double kBorderWidth = 0.05;  // m, white line

// ---- Robots ----
constexpr double kBodyRadius = 0.10;   // m, both robots treated as circles
constexpr double kTrackWidth = 0.18;   // m, distance between wheels
constexpr double kTopSpeed = 1.0;      // m/s at full ESC command (150)
constexpr double kMotorLagMs = 60.0;   // ESC + motor response time constant

// ---- Sensors (robot frame: x forward, y left) ----
constexpr double kEdgeSensorX = 0.10, kEdgeSensorY = 0.08;
constexpr double kOpponentBeamY = 0.04;   // two beams, 8 cm apart
constexpr double kOpponentRange = 0.60;   // m

constexpr uint32_t kControlPeriodMs = 5;  // how often Strategy::update runs
constexpr uint32_t kMatchLimitMs = 15000;

// Dummy: a still box that has to be found and pushed out.
// Chaser: another robot that searches and attacks too, more slowly.
enum class Opponent { Dummy, Chaser };
enum class Result { Win, Loss, Draw };

struct Body {
  double x = 0, y = 0, heading = 0;  // m, m, rad
  double vLeft = 0, vRight = 0;      // m/s, after motor lag
};

struct MatchSetup {
  Tactic tactic = Tactic::Radar;
  Side side = Side::Right;
  Opponent opponent = Opponent::Dummy;
  Body robot;
  Body enemy;
};

struct MatchOutcome {
  Result result = Result::Draw;
  uint32_t timeMs = 0;
  uint32_t edgeEscapes = 0;  // times the robot reached the border and escaped
};

inline double wheelSpeed(int command) {
  return (command - config::kMotorStop) * kTopSpeed /
         double(config::kMotorMax - config::kMotorStop);
}

inline double radius(double x, double y) { return std::sqrt(x * x + y * y); }

// Point in the robot frame -> world frame.
inline void toWorld(const Body& b, double fx, double fy, double& wx, double& wy) {
  wx = b.x + fx * std::cos(b.heading) - fy * std::sin(b.heading);
  wy = b.y + fx * std::sin(b.heading) + fy * std::cos(b.heading);
}

// Does a forward beam starting at lateral offset `beamY` hit the other robot?
inline bool beamHits(const Body& self, double beamY, const Body& other) {
  double ox, oy;
  toWorld(self, kEdgeSensorX, beamY, ox, oy);
  const double dx = std::cos(self.heading), dy = std::sin(self.heading);
  const double px = other.x - ox, py = other.y - oy;
  const double along = px * dx + py * dy;
  if (along < 0 || along > kOpponentRange + kBodyRadius) return false;
  const double across = std::fabs(px * -dy + py * dx);
  return across <= kBodyRadius;
}

inline bool onBorder(const Body& b, double fy) {
  double wx, wy;
  toWorld(b, kEdgeSensorX, fy, wx, wy);
  return radius(wx, wy) > kRingRadius - kBorderWidth;
}

inline SensorReadings sense(const Body& robot, const Body& enemy) {
  SensorReadings s;
  s.edgeLeft = onBorder(robot, kEdgeSensorY);
  s.edgeRight = onBorder(robot, -kEdgeSensorY);
  s.opponentLeft = beamHits(robot, kOpponentBeamY, enemy);
  s.opponentRight = beamHits(robot, -kOpponentBeamY, enemy);
  return s;
}

// Differential drive with a first-order lag on each wheel.
inline void move(Body& b, double targetLeft, double targetRight, double dtMs) {
  const double k = dtMs / (kMotorLagMs + dtMs);
  b.vLeft += (targetLeft - b.vLeft) * k;
  b.vRight += (targetRight - b.vRight) * k;
  const double dt = dtMs / 1000.0;
  const double v = (b.vLeft + b.vRight) / 2;
  const double w = (b.vRight - b.vLeft) / kTrackWidth;
  b.heading += w * dt;
  b.x += v * std::cos(b.heading) * dt;
  b.y += v * std::sin(b.heading) * dt;
}

// The faster robot along the line of contact wins the shove.
inline void resolveContact(Body& a, Body& b) {
  const double dx = b.x - a.x, dy = b.y - a.y;
  const double d = radius(dx, dy);
  const double overlap = 2 * kBodyRadius - d;
  if (overlap <= 0 || d == 0) return;
  const double nx = dx / d, ny = dy / d;
  const double pushA = std::fmax(0.0, (a.vLeft + a.vRight) / 2 *
                                          (std::cos(a.heading) * nx + std::sin(a.heading) * ny));
  const double pushB = std::fmax(0.0, (b.vLeft + b.vRight) / 2 *
                                          -(std::cos(b.heading) * nx + std::sin(b.heading) * ny));
  const double total = pushA + pushB;
  const double shareB = total > 0 ? pushA / total : 0.5;  // how much B gets moved
  b.x += nx * overlap * shareB;
  b.y += ny * overlap * shareB;
  a.x -= nx * overlap * (1 - shareB);
  a.y -= ny * overlap * (1 - shareB);
}

// The active opponent is another robot with the same sensors, running a
// plain Radar search and attack, with motors at this fraction of ours.
constexpr double kChaserSpeedFactor = 0.6;

inline MatchOutcome runMatch(const MatchSetup& setup) {
  Body robot = setup.robot, enemy = setup.enemy;
  Strategy strategy;
  strategy.configure({setup.tactic, setup.side});
  strategy.start(0);

  Strategy chaser;
  chaser.configure({Tactic::Radar, Side::Left});
  chaser.start(0);
  MotorCommand enemyCmd{config::kMotorStop, config::kMotorStop};

  MatchOutcome out;
  MotorCommand cmd{config::kMotorStop, config::kMotorStop};
  bool wasEscaping = false;

  for (uint32_t t = 0; t <= kMatchLimitMs; ++t) {
    if (t % kControlPeriodMs == 0) {
      cmd = strategy.update(sense(robot, enemy), t);
      if (setup.opponent == Opponent::Chaser) enemyCmd = chaser.update(sense(enemy, robot), t);
      const bool escaping = strategy.state() == Strategy::State::EscapeReverse;
      if (escaping && !wasEscaping) ++out.edgeEscapes;
      wasEscaping = escaping;
    }
    move(robot, wheelSpeed(cmd.left), wheelSpeed(cmd.right), 1.0);

    move(enemy, wheelSpeed(enemyCmd.left) * kChaserSpeedFactor,
         wheelSpeed(enemyCmd.right) * kChaserSpeedFactor, 1.0);

    resolveContact(robot, enemy);

    const bool robotOut = radius(robot.x, robot.y) > kRingRadius;
    const bool enemyOut = radius(enemy.x, enemy.y) > kRingRadius;
    if (robotOut || enemyOut) {
      out.timeMs = t;
      out.result = robotOut ? Result::Loss : Result::Win;
      return out;
    }
  }
  out.timeMs = kMatchLimitMs;
  return out;
}

// Random but reproducible start: both robots inside the starting area,
// facing any direction, opponent somewhere in the ring.
inline MatchSetup randomSetup(std::mt19937& rng, Tactic tactic, Opponent opponent) {
  std::uniform_real_distribution<double> angle(-kPi, kPi);
  std::uniform_real_distribution<double> dist(0.15, 0.45);
  std::bernoulli_distribution coin(0.5);

  MatchSetup s;
  s.tactic = tactic;
  s.opponent = opponent;
  s.side = coin(rng) ? Side::Left : Side::Right;

  const double a = angle(rng);
  s.robot.x = 0.20 * std::cos(a);
  s.robot.y = 0.20 * std::sin(a);
  s.robot.heading = angle(rng);

  const double b = a + kPi + angle(rng) / 2;  // roughly on the other side
  const double r = dist(rng);
  s.enemy.x = r * std::cos(b);
  s.enemy.y = r * std::sin(b);
  s.enemy.heading = angle(rng);
  return s;
}

}  // namespace sim
