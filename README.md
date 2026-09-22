# Robot Sumo Competition

Control firmware for an autonomous sumo robot, from my time competing in university robotics. The repository follows the robot in three stages, from driving it by hand to a complete match strategy. The code is rewritten in modern C++ to sharpen the language and revisit decisions made under deadline pressure.

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue) ![Embedded](https://img.shields.io/badge/Embedded-ESP32-lightgrey) ![Status](https://img.shields.io/badge/Status-Work%20in%20progress-orange)

## The problem

An autonomous sumo robot has to find an opponent inside a circular ring, push it out, and stay inside itself. Once the match starts nobody controls it: every decision comes from what the sensors reported a few milliseconds earlier.

That makes it a constraints problem as much as a programming one. The control loop has to react fast enough to matter, opponent detection has to tolerate noisy readings without hesitating, and edge detection has to override everything else, because leaving the ring loses the match instantly.

## Projects

| Project | What it adds |
|---------|--------------|
| [01 · PS4 integration](01-ps4-integration) | Manual driving with a PS4 controller over Bluetooth, used to validate motors and wiring. A single file. |
| [02 · Strategy](02-strategy) | Autonomous mode: IR start module, radar search, attack and edge escape, as a non-blocking state machine. |
| [03 · Final](03-final) | Modular code, three selectable tactics (Radar, Flank, Counter) and strategy unit tests that run on a PC. |

Each folder is a standalone Arduino sketch with its own README.

## How it works

The control loop runs on a fixed priority: staying in the ring comes first, attacking second, searching last.

```
  edge detected?     -> back off, turn, resume
  opponent in range? -> commit forward
  neither            -> run search pattern
```

The PS4 controller is used to switch modes, choose the tactic before a match and drive the robot manually for testing. During a match, the IR start module used by the referees arms, starts and stops the robot.

## What changed in the rewrite

The original code was written to work by a deadline, which is the right priority during a competition and the wrong one afterwards. Going back through it:

- Magic numbers replaced by named `constexpr` constants in one `config.h`
- Sensor reading separated from decision making, so the strategy is tested without hardware
- `enum class` for robot states instead of integer flags
- Nested loops and conditionals replaced by an explicit state machine
- No blocking `delay()` calls in the control loop, so the stop signal is always read
- Timing that stays correct when `millis()` wraps around

## Hardware

ESP32, two motors driven by ESCs, two digital opponent sensors, two analog edge sensors, an IR start module and a PS4 controller. Pin assignments are in each project's `config.h`.

## Status

Work in progress. Projects 01 and 02 are rewrites of code that ran on the robot. Project 03 adds new tactics that are still to be tuned on the ring.

## Author

Lucas Fernandes Nunes · [LinkedIn](https://linkedin.com/in/lucas-fernandes-nunes-335867256) · [Portfolio](https://lucasdata.notion.site/)
