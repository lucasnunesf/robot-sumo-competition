# 01 · PS4 integration

First step of the robot: driving it by hand with a PS4 controller over Bluetooth. It was used to check wiring, motor direction and ESC response before any autonomous code existed.

The whole program is a single file on purpose. The later projects split the code into modules as it grows.

## Controls

Press **OPTIONS** to switch between the two modes. The controller LED shows the current mode.

| Mode   | LED  | Behaviour                         |
|--------|------|-----------------------------------|
| Locked | Red  | Motors off. Safe state at power-up |
| Manual | Blue | Driven from the controller        |

| Input          | Action                                         |
|----------------|------------------------------------------------|
| R2             | Forward, proportional to how far it is pressed |
| L2             | Reverse / brake, proportional                  |
| Left stick X   | Steering                                       |
| Cross          | Fixed slow forward speed                       |
| Cross + Square | Fixed fast forward speed                       |
| Circle         | Fixed slow reverse speed                       |

If the controller disconnects, the motors stop immediately.

## How the driving mix works

Each wheel gets `base - brake ± turn`:
- `base` comes from R2, from stopped (90) up to 150;
- `brake` comes from L2 and pulls both wheels down toward reverse;
- `turn` comes from the stick and adds to one wheel while subtracting from the other.

The result is clamped to the range the ESCs accept (30 to 150).

## Hardware

| Part            | Connection |
|-----------------|------------|
| ESP32           | Controller |
| Left motor ESC  | GPIO 27    |
| Right motor ESC | GPIO 26    |
| PS4 controller  | Bluetooth  |

## Build

Arduino IDE with the ESP32 board package and the libraries `ESP32Servo` and `PS4Controller`.

1. Pair the PS4 controller with the Bluetooth address in the `config` block at the top of the file (for example with SixaxisPairTool).
2. Open `01-ps4-integration.ino`, select the ESP32 board and port, and upload.
