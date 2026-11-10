// Runs the real Strategy through simulated matches and prints a summary.
//
// Build and run from the 03-final/ folder:
//   g++ -std=c++17 -O2 -I. sim/run_matches.cpp Strategy.cpp -o run_matches && ./run_matches
//
// Optional: ./run_matches <matches per row> <seed> [--csv]

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>

#include "ring_sim.h"

namespace {

const char* name(Tactic t) {
  switch (t) {
    case Tactic::Radar: return "Radar";
    case Tactic::Flank: return "Flank";
    default:            return "Counter";
  }
}

const char* name(sim::Opponent o) { return o == sim::Opponent::Dummy ? "still" : "chasing"; }

const char* name(sim::Result r) {
  switch (r) {
    case sim::Result::Win:  return "win";
    case sim::Result::Loss: return "loss";
    default:                return "draw";
  }
}

}  // namespace

int main(int argc, char** argv) {
  const int matches = argc > 1 ? std::atoi(argv[1]) : 1000;
  const unsigned seed = argc > 2 ? unsigned(std::atoi(argv[2])) : 42u;
  const bool csv = argc > 3 && std::strcmp(argv[3], "--csv") == 0;

  if (csv) std::puts("tactic,opponent,match,result,time_ms,edge_escapes");
  else {
    std::printf("%d simulated matches per row, seed %u\n\n", matches, seed);
    std::printf("%-8s %-9s %5s %5s %5s %12s %13s\n", "tactic", "opponent", "wins", "losses",
                "draws", "median win", "edge escapes");
  }

  int selfExits = 0;  // losses against a still opponent: the robot drove itself out
  for (Tactic tactic : {Tactic::Radar, Tactic::Flank, Tactic::Counter}) {
    for (sim::Opponent opp : {sim::Opponent::Dummy, sim::Opponent::Chaser}) {
      std::mt19937 rng(seed);  // same starts for every tactic: a fair comparison
      int wins = 0, losses = 0, draws = 0;
      unsigned long escapes = 0;
      static uint32_t winTimes[100000];
      int nWin = 0;

      for (int i = 0; i < matches; ++i) {
        const sim::MatchOutcome o = sim::runMatch(sim::randomSetup(rng, tactic, opp));
        escapes += o.edgeEscapes;
        if (o.result == sim::Result::Win) { ++wins; winTimes[nWin++] = o.timeMs; }
        else if (o.result == sim::Result::Loss) ++losses;
        else ++draws;
        if (csv) std::printf("%s,%s,%d,%s,%u,%u\n", name(tactic), name(opp), i, name(o.result),
                             o.timeMs, o.edgeEscapes);
      }
      if (opp == sim::Opponent::Dummy) selfExits += losses;

      // median win time (simple sort, small arrays)
      for (int a = 1; a < nWin; ++a)
        for (int b = a; b > 0 && winTimes[b - 1] > winTimes[b]; --b) {
          const uint32_t tmp = winTimes[b]; winTimes[b] = winTimes[b - 1]; winTimes[b - 1] = tmp;
        }
      if (!csv) {
        char median[16] = "-";
        if (nWin > 0) std::snprintf(median, sizeof median, "%.2f s", winTimes[nWin / 2] / 1000.0);
        std::printf("%-8s %-9s %5d %5d %5d %12s %13lu\n", name(tactic), name(opp), wins, losses,
                    draws, median, escapes);
      }
    }
  }
  if (!csv) std::printf("\nleft the ring on its own: %d\n", selfExits);
  return selfExits == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
