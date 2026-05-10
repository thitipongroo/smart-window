# Smart Window

An Arduino-based automatic window controller that opens when it is bright and dry, and closes when it gets dark or starts raining.

## How It Works

Two sensors feed ADC readings (0–1023) into a hysteresis state machine. Rain takes priority over light.

| Condition | Window |
| --------- | ------ |
| Bright **and** dry | Open |
| Raining | Close (priority) |
| Dark | Close |
| Readings in threshold band | Hold current state |

The hysteresis band between the open and close thresholds prevents the window from chattering when sensor values hover near the boundary.

## Hardware

| Component | Part | Pin |
| --------- | ---- | --- |
| Microcontroller | Arduino Uno / Nano | — |
| Light sensor | LDR (light-dependent resistor) | A0 |
| Rain sensor | PCB moisture / rain sensor | A1 |
| Actuator | SG90 servo motor | D8 |

**Servo positions:** 0° = closed, 90° = open

## Wiring

```text
Arduino A0  ── LDR middle leg  (other legs: 5V and GND via 10kΩ pull-down)
Arduino A1  ── Rain sensor AO pin
Arduino D8  ── SG90 signal wire (orange)
Arduino 5V  ── Rain sensor VCC, SG90 VCC (red)
Arduino GND ── Rain sensor GND, SG90 GND (brown/black)
```

## Project Structure

```text
smart-window/
├── smart_window/           # Main sketch — flash to deploy
│   └── smart_window.ino
├── calibration/            # Calibration utility — flash first
│   └── ReadVal/
│       └── ReadVal.ino
└── README.md
```

## Getting Started

### Step 1 — Calibrate your sensors

Flash `calibration/ReadVal/ReadVal.ino` and open the Serial Monitor at **9600 baud**.

Move the board through all real conditions and note the ADC readings:

- **LDR:** dark room → bright sunlight
- **Rain sensor:** dry surface → wet surface

Every 10 readings the sketch prints the observed min/max range and threshold suggestions.

### Step 2 — Set thresholds

Open `smart_window/smart_window.ino` and adjust these constants using your calibration data:

```cpp
// LDR: low = bright, high = dark
static const int LIGHT_OPEN_THRESH  = 80;   // open  when LDR  < this
static const int LIGHT_CLOSE_THRESH = 120;  // close when LDR  > this

// Rain: low = dry, high = wet
static const int RAIN_CLOSE_THRESH  = 120;  // close when Rain > this
static const int RAIN_CLEAR_THRESH  = 80;   // allow open when Rain < this
```

Keep at least 20–30 units of gap between each open/close pair to maintain a stable hysteresis band.

### Step 3 — Deploy

Flash `smart_window/smart_window.ino`. Monitor state transitions at **9600 baud**:

```text
=== Smart Window ===
LDR: A0  Rain: A1  Servo: D8
[Sensor] LDR=  54  Rain=  30  State=CLOSED
[Window] OPEN  (bright + dry)
[Sensor] LDR=  60  Rain= 145  State=OPEN
[Window] CLOSED
```
