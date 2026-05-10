# Smart Window

An Arduino-based automatic window controller that opens when it is bright and dry, and closes when it gets dark or starts raining. Uses a dual-threshold hysteresis state machine to prevent chattering.

---

## Architecture

```text
                   Physical Environment
                    │              │
               Light level     Rain / moisture
                    │              │
                    ▼              ▼
              LDR (A0)       Rain sensor (A1)
              0–1023 ADC      0–1023 ADC
                    │              │
                    └──────┬───────┘
                           ▼
                   ┌───────────────┐
                   │  Arduino Uno  │
                   │               │
                   │ decideState() │  ← hysteresis FSM
                   │               │     rain has priority
                   │ applyState()  │  ← writes servo only on state change
                   └──────┬────────┘
                          │ PWM signal (D8)
                          ▼
                     SG90 Servo
                   0° = CLOSED
                  90° = OPEN
                          │
                          ▼
                    Window actuator
```

---

## Tech Stack

| Component | Part | Why |
| --------- | ---- | --- |
| MCU | Arduino Uno / Nano | Beginner-friendly, large library ecosystem, 6 analog inputs, 5 V tolerant — sufficient for LDR + rain sensor + servo |
| Light sensor | LDR (light-dependent resistor) | Analog output gives a full 0–1023 ADC dynamic range for calibration; wider range than a digital threshold module |
| Rain sensor | PCB moisture / rain sensor | Analog out lets you tune sensitivity via `RAIN_CLOSE_THRESH`; digital module is binary and non-adjustable |
| Actuator | SG90 servo | 0–90° range maps exactly to closed/open; inexpensive and widely available |
| Library | `Servo.h` (built-in) | Ships with the Arduino core; no extra install required |

---

## Project Structure

```text
smart-window/
├── smart_window/
│   └── smart_window.ino   # Main sketch — flash this to deploy
├── calibration/
│   └── ReadVal/
│       └── ReadVal.ino    # Calibration utility — flash first, then the main sketch
└── README.md
```

---

## System Flow

```text
Power-on → setup()
  ├─ Serial.begin(9600)
  ├─ Servo attach (D8)
  └─ Write servo to 0° (CLOSED) — safe default on every boot

loop() — runs every READ_INTERVAL_MS (1 000 ms)
  │
  ├─ analogRead(A0) → ldrVal   (0 = bright, 1023 = dark)
  ├─ analogRead(A1) → rainVal  (0 = dry,    1023 = wet)
  │
  ├─ Serial print: [Sensor] LDR=NNN  Rain=NNN  State=OPEN|CLOSED
  │
  └─ decideState(ldrVal, rainVal, currentState)
       │
       ├─ rainVal > RAIN_CLOSE_THRESH                       → CLOSED  (rain priority)
       ├─ ldrVal  < LIGHT_OPEN_THRESH
       │  AND rainVal < RAIN_CLEAR_THRESH                   → OPEN
       ├─ ldrVal  > LIGHT_CLOSE_THRESH                      → CLOSED
       └─ else                                              → keep currentState
             │
             ▼
       applyState(next)
         ├─ next == currentState → no-op (prevents servo jitter)
         ├─ OPEN   → servo.write(90°), print "[Window] OPEN"
         └─ CLOSED → servo.write(0°),  print "[Window] CLOSED"
```

---

## Decision Table

Rain takes priority over light:

| `rainVal` | `ldrVal` | Next state |
| --------- | -------- | ---------- |
| `> RAIN_CLOSE_THRESH` | any | **CLOSED** |
| `< RAIN_CLEAR_THRESH` | `< LIGHT_OPEN_THRESH` | **OPEN** |
| any | `> LIGHT_CLOSE_THRESH` | **CLOSED** |
| in band | in band | **hold current** |

---

## Hardware

| Component | Part | Pin |
| --------- | ---- | --- |
| MCU | Arduino Uno / Nano | — |
| Light sensor | LDR | A0 |
| Rain sensor | PCB moisture sensor | A1 |
| Actuator | SG90 servo | D8 |

**Servo positions:** 0° = closed, 90° = open

### Wiring

```text
Arduino A0  ── LDR middle leg  (other legs: 5 V and GND via 10 kΩ pull-down)
Arduino A1  ── Rain sensor AO pin
Arduino D8  ── SG90 signal wire (orange)
Arduino 5 V ── Rain sensor VCC, SG90 VCC (red)
Arduino GND ── Rain sensor GND, SG90 GND (brown/black)
```

---

## Getting Started

### Step 1 — Calibrate your sensors

Flash `calibration/ReadVal/ReadVal.ino` and open **Serial Monitor at 9600 baud**.

Move the board through all real conditions and note the ADC readings:

- **LDR:** dark room → bright sunlight
- **Rain sensor:** dry surface → wet surface

Every 10 readings the sketch prints the observed min/max range and threshold suggestions.

### Step 2 — Set thresholds

Open `smart_window/smart_window.ino` and adjust these constants:

```cpp
// LDR: low = bright, high = dark
static const int LIGHT_OPEN_THRESH  = 80;   // open  when LDR  < this
static const int LIGHT_CLOSE_THRESH = 120;  // close when LDR  > this

// Rain: low = dry, high = wet
static const int RAIN_CLOSE_THRESH  = 120;  // close when Rain > this
static const int RAIN_CLEAR_THRESH  = 80;   // allow open when Rain < this
```

Keep at least 20–30 ADC units of gap between each open/close pair to maintain a stable hysteresis band.

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

---

## Hysteresis Explained

Without hysteresis, a sensor reading hovering near a single threshold causes rapid open/close toggling (chattering), which strains the servo and rattles the window. Dual thresholds create a dead band:

```text
         LIGHT_OPEN_THRESH=80          LIGHT_CLOSE_THRESH=120
                 │                              │
  ───OPEN────────▼──── hold current state ──────▼────CLOSED───►
                                                        (LDR value)
```

Readings between 80 and 120 leave the window in whatever state it was already in.

---

## Tradeoffs

| Decision | Alternative | Reasoning |
| -------- | ----------- | --------- |
| Polling every 1 s | Interrupt-driven on analog comparator | Polling is simpler to reason about; 1 s latency is acceptable for a window |
| Analog LDR | Digital light sensor module (with fixed threshold pot) | Analog gives full 10-bit dynamic range; calibration adjusts for any environment without soldering |
| State resets to CLOSED on power cycle | Persist state in EEPROM | CLOSED is the safe default — keeps the window shut during unexpected reboots |
| `applyState()` no-op on same state | Always write to servo | Prevents constant PWM pulses that cause servo jitter and unnecessary wear |

---

## Scaling Considerations

| Concern | Approach |
| ------- | -------- |
| Remote monitoring | Add an ESP8266/ESP32 alongside the Arduino; publish sensor readings via MQTT to a dashboard (e.g. Home Assistant) |
| Scheduled overrides | Add a DS3231 RTC module to implement time-based rules (e.g. always close at 18:00) |
| Offline data logging | Log ADC readings to an SD card for post-calibration analysis |
| Multiple windows | Replace the single servo with a relay-controlled motor driver; use the same decision logic |
| Outdoor weatherproofing | Enclose the Arduino in an IP65 box; use a conformal-coated PCB rain sensor rated for outdoor use |
