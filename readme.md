# Robot Sumo Competition

Control firmware for an autonomous sumo robot, from my time competing in university robotics.
The original competition code is published here as it ran, and I am gradually rewriting parts of
it in modern C++ to sharpen the language and revisit decisions made under deadline pressure.

![C++](https://img.shields.io/badge/C++-17-blue)
![Embedded](https://img.shields.io/badge/Embedded-Microcontroller-lightgrey)
![Status](https://img.shields.io/badge/Status-Work%20in%20progress-orange)

---

## About this project

Personal project, built and used in real competitions. It is published for two reasons: the
competition code is worth keeping as a record of what actually worked in a match, and it is a good
base to practise on. Improvements are tested against the original behaviour rather than replacing
it blindly.

## The problem

An autonomous sumo robot has to find an opponent inside a circular ring, push it out, and stay
inside itself. There is no remote control: once the match starts, every decision comes from what
the sensors reported a few milliseconds earlier.

That makes it a constraints problem as much as a programming one. The control loop has to react
fast enough to matter, opponent detection has to tolerate noisy readings without hesitating, and
edge detection has to override everything else, because leaving the ring loses the match instantly.

## How it works

The control loop runs on a fixed priority: staying in the ring comes first, attacking second,
searching last.

```
  edge detected?   -> back off, turn, resume
  opponent in range? -> commit forward
  neither          -> run search pattern
```

## Repository structure

```
robot-sumo-competition/
├── competition/   # the firmware as it ran in competition, unchanged
├── src/           # the rewrite in progress
└── docs/
```

The `competition/` folder is deliberately frozen. It is the reference the rewrite is measured
against, not something to tidy up.

## What I am improving

The original was written to work by a deadline, which is the right priority during a competition
and the wrong one afterwards. Going back through it:

- Replacing magic numbers with named constants, so thresholds can be tuned in one place
- Separating sensor reading from decision making, so the strategy can be tested without hardware
- Using `enum class` for robot states instead of integer flags
- Replacing nested conditionals with an explicit state machine
- Keeping the control loop free of blocking delays

## Status

Work in progress. The competition firmware is the starting point; the rewrite is being done a
piece at a time, with hardware details, strategy notes and build instructions added as each part
is revisited.

## Author

**Lucas Fernandes Nunes**
[LinkedIn](https://linkedin.com/in/lucas-fernandes-nunes-335867256) · [Portfolio](https://lucasdata.notion.site)
