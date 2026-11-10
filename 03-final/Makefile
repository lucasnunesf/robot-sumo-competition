# Host-side build: tests and simulator, no Arduino needed.
CXX ?= g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Werror -I.

BIN = build

.PHONY: test sim clean

test: $(BIN)/config_test $(BIN)/strategy_test $(BIN)/match_test
	./$(BIN)/config_test
	./$(BIN)/strategy_test
	./$(BIN)/match_test

sim: $(BIN)/run_matches
	./$(BIN)/run_matches

$(BIN):
	mkdir -p $(BIN)

$(BIN)/config_test: test/config_test.cpp config.h | $(BIN)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BIN)/%: test/%.cpp Strategy.cpp Strategy.h config.h sim/ring_sim.h | $(BIN)
	$(CXX) $(CXXFLAGS) $< Strategy.cpp -o $@

$(BIN)/run_matches: sim/run_matches.cpp Strategy.cpp Strategy.h config.h sim/ring_sim.h | $(BIN)
	$(CXX) $(CXXFLAGS) $< Strategy.cpp -o $@

clean:
	rm -rf $(BIN)
