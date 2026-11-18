# Ring simulator on a PC, no Arduino needed.
CXX ?= g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Werror -I.

BIN = build

.PHONY: sim clean

sim: $(BIN)/run_matches
	./$(BIN)/run_matches

$(BIN):
	mkdir -p $(BIN)

$(BIN)/run_matches: sim/run_matches.cpp Strategy.cpp Strategy.h config.h sim/ring_sim.h | $(BIN)
	$(CXX) $(CXXFLAGS) $< Strategy.cpp -o $@

clean:
	rm -rf $(BIN)
