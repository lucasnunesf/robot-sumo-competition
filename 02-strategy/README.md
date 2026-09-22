# 02 · Strategy

Adds the autonomous match mode to the base from [01](../01-ps4-integration). The robot is armed and started by an IR start module, searches for the opponent, charges when it sees it, and backs away from the ring border.

## Modes

Press **OPTIONS** to cycle **Locked** (red) → **Autonomous** (green) → **Manual** (blue). Manual driving works the same as in 01.

## Match flow

| IR command | Effect                                                        |
|------------|---------------------------------------------------------------|
| Ready      | Arms the robot and calibrates the edge sensors (LED blinks)   |
| Start      | Starts the match                                              |
| Stop       | Stops the robot at any time                                   |

Before the start, the D-pad sets the tactic:

| Button     | Setting                                   |
|------------|-------------------------------------------|
| Left/Right | Direction of the first sweep              |
| Up         | Open with a short straight dash (default) |
| Down       | Skip the dash                             |

## Behaviour

Every loop follows the same priority: **stay in the ring → attack → search**.

```
 START ──► OPENING DASH ──► SEARCH ──opponent seen──► ATTACK
                              ▲                          │
                              └──── opponent lost ───────┘

 edge seen (any state) ──► REVERSE ──► TURN AWAY ──► SEARCH
```

- **Search:** sweeps in a wide arc until an opponent sensor fires. After each contact, the next sweep goes the other way.
- **Attack:** full speed forward while the opponent stays in view.
- **Edge escape:** reverses until the sensor is off the white border, keeps reversing a little longer, then turns away from the side that saw it.

The loop is a non-blocking state machine: once the match starts there are no `delay()` calls, so the IR stop command and the sensors are read on every cycle.

## Edge sensor calibration

The analog reading drops over the white border. When the **Ready** command arrives, with the robot on the black ring, each sensor stores its current reading minus a margin as its threshold. This adapts to the lighting and the ring surface at each match.

## Hardware

| Part             | Connection                     |
|------------------|--------------------------------|
| ESP32            | Controller                     |
| Left motor ESC   | GPIO 27                        |
| Right motor ESC  | GPIO 26                        |
| Opponent sensors | GPIO 17 (left), 16 (right)     |
| Edge sensors     | GPIO 34 (left), 35 (right)     |
| IR start module  | GPIO 23                        |

The edge sensors must be on ADC1 pins (GPIO 32 to 39), because ADC2 does not work while the ESP32 radio is in use. All pins can be changed in `config.h`.

## Build

Arduino IDE with the ESP32 board package and the libraries `ESP32Servo`, `PS4Controller` and `IRremote` **version 2.x** (version 3 and later changed the API).
