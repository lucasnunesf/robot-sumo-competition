// End-to-end tests: the real Strategy plays whole matches in the simulator
// (sim/ring_sim.h). Seeds are fixed, so every run gives the same matches.
//
//   g++ -std=c++17 -O2 -I. test/match_test.cpp Strategy.cpp -o match_test && ./match_test

#include <cstdio>
#include <cstdlib>
#include <random>

#include "../sim/ring_sim.h"

namespace {

int failures = 0;

const char* name(Tactic t) {
  return t == Tactic::Radar ? "Radar" : t == Tactic::Flank ? "Flank" : "Counter";
}

struct Tally {
  int wins = 0, losses = 0, draws = 0;
};

Tally play(Tactic tactic, sim::Opponent opponent, int matches, unsigned seed) {
  std::mt19937 rng(seed);
  Tally t;
  for (int i = 0; i < matches; ++i) {
    switch (sim::runMatch(sim::randomSetup(rng, tactic, opponent)).result) {
      case sim::Result::Win:  ++t.wins; break;
      case sim::Result::Loss: ++t.losses; break;
      case sim::Result::Draw: ++t.draws; break;
    }
  }
  return t;
}

void expect(bool ok, const char* what) {
  std::printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) ++failures;
}

}  // namespace

int main() {
  constexpr int kMatches = 300;
  constexpr unsigned kSeed = 2024;

  // Against a still opponent nothing can push the robot, so any loss means
  // it drove itself out of the ring.
  for (Tactic tactic : {Tactic::Radar, Tactic::Flank, Tactic::Counter}) {
    const Tally t = play(tactic, sim::Opponent::Dummy, kMatches, kSeed);
    char what[96];
    std::snprintf(what, sizeof what, "%-7s never leaves the ring on its own (%d wins / %d)",
                  name(tactic), t.wins, kMatches);
    expect(t.losses == 0, what);
    std::snprintf(what, sizeof what, "%-7s pushes a still opponent out in >= 85%% of matches",
                  name(tactic));
    expect(t.wins * 100 >= kMatches * 85, what);
  }

  // Against a robot that searches and attacks too, Radar must win more than
  // it loses. (It still loses some, mostly when shoved sideways toward the border.)
  const Tally radar = play(Tactic::Radar, sim::Opponent::Chaser, kMatches, kSeed);
  expect(radar.wins > 3 * radar.losses, "Radar wins at least 3x more than it loses to a chaser");

  if (failures == 0) {
    std::puts("All match tests passed.");
    return EXIT_SUCCESS;
  }
  std::printf("%d match test(s) failed.\n", failures);
  return EXIT_FAILURE;
}
