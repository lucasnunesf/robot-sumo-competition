# 03 · Final

The complete firmware, split into modules, with three selectable tactics and a strategy that is unit-tested on a PC.

## Code structure

| File           | Responsibility                                                      |
|----------------|---------------------------------------------------------------------|
| `03-final.ino` | Wires the modules together; modes and PS4 controller handling       |
| `config.h`     | Every pin, speed, time and IR code as a `constexpr` constant        |
| `Drive.*`      | The two wheel motors                                                |
| `Sensors.*`    | Edge and opponent sensors, with calibration                         |
| `StartModule.*`| Decodes the IR start module into `Ready` / `Start` / `Stop`         |
| `Strategy.*`   | Match logic as a state machine, with no hardware access             |
| `test/`        | Strategy tests that run on a PC                                     |

The key design choice is that `Strategy` never touches the hardware. It receives a `SensorReadings` struct and the current time, and returns a `MotorCommand`. The whole match logic can therefore be tested without the robot, and the hardware modules stay small.

## Tactics

In autonomous mode, choose the tactic before the start. The controller LED shows which one is selected.

| Button   | Tactic  | LED    | Idea |
|----------|---------|--------|------|
| Triangle | Radar   | Green  | Short dash to take the centre of the ring, then sweep until the opponent is found |
| Square   | Flank   | Cyan   | Curve around the opponent's front and hit it from the side, avoiding a head-on push |
| Circle   | Counter | Yellow | Stay still and let an aggressive opponent come in, then react. Starts sweeping if nothing arrives within 2 s |

The D-pad (left or right) picks the side used for the flank and for the first sweep.

## State machine

On every update the priority is **stay in the ring → attack → search**.

```
            ┌─────────┐   plan done    ┌────────┐  opponent seen  ┌────────┐
 START ───► │ OPENING │ ─────────────► │ SEARCH │ ──────────────► │ ATTACK │
            └─────────┘                └────────┘ ◄────────────── └────────┘
                                           ▲       opponent lost
                                           │
 edge seen in ANY state                    │ turn done (or opponent seen)
        │                                  │
        ▼                                  │
 ┌────────────────┐  edge clear + 450 ms  ┌─────────────┐
 │ ESCAPE REVERSE │ ────────────────────► │ ESCAPE TURN │
 └────────────────┘                       └─────────────┘
```

- **Opening:** each tactic has a plan made of timed steps (spin, straight, spin). Only the edge can interrupt it.
- **Search:** sweeps toward the side where the opponent was last seen. The Counter tactic waits before sweeping.
- **Attack:** full push when both sensors see the opponent. When only one does, the robot steers toward it while pushing, so it ends up square to the opponent.
- **Escape:** reverses while on the border and for a short time after, then turns away from the side that saw it. If both sensors hit the border, it turns further, toward where the opponent was last seen.

All timing uses `millis()` with unsigned subtraction, so it stays correct when the counter wraps around. There are no blocking delays during a match.

## Tests

```bash
g++ -std=c++17 -I. test/strategy_test.cpp Strategy.cpp -o strategy_test && ./strategy_test
```

The tests cover each opening plan, the Counter waiting time, attack steering, edge priority over attack, the escape sequence and `millis()` wrap-around.

## Status

The Radar behaviour and the edge escape come from code that ran on the robot. The Flank and Counter tactics are new in this version and are still to be tuned on the ring. Their timings in `config.h` are starting values.

## Hardware and build

Same wiring and libraries as [02](../02-strategy): ESP32, two ESCs (GPIO 27/26), opponent sensors (GPIO 17/16), edge sensors on ADC1 (GPIO 34/35) and an IR start module (GPIO 23). Libraries: `ESP32Servo`, `PS4Controller` and `IRremote` 2.x.
