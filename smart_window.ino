/**
 * @file    smart_window.ino
 * @brief   Automatic window controller — opens when bright & dry, closes when dark or raining.
 * @board   Arduino Uno / Nano
 *
 * SENSORS
 *   A0 (pin 0) — LDR (light-dependent resistor)  : low value = bright, high = dark
 *   A1 (pin 1) — Rain / PCB moisture sensor       : low value = dry,   high = wet
 *
 * ACTUATOR
 *   D8 — SG90 servo (0° = closed, 90° = fully open)
 *
 * CHANGES:
 *  - [BUG]    delay(1000) caused servo jitter with Servo library on some boards
 *             → replaced with millis()-based non-blocking timer
 *  - [BUG]    Hard threshold at 100 causes rapid window chatter when sensor
 *             readings hover near 100 → added hysteresis band (OPEN_THRESH / CLOSE_THRESH)
 *  - [DESIGN] Magic numbers → named constants with units in comments
 *  - [DESIGN] Window state machine with explicit WindowState enum (avoids repeated servo writes)
 *  - [DESIGN] Serial logging of every state transition for easier debugging
 *  - [DESIGN] Indentation and formatting made consistent
 *  - [DESIGN] const int for pin numbers (never change at runtime)
 */

#include <Servo.h>

// ─── HARDWARE CONSTANTS ───────────────────────────────────────────────────────
static const uint8_t LDR_PIN   = A0; // Light sensor
static const uint8_t RAIN_PIN  = A1; // Rain sensor
static const uint8_t SERVO_PIN = 8;  // SG90 signal wire

// ─── SERVO POSITIONS ──────────────────────────────────────────────────────────
static const uint8_t SERVO_OPEN   = 90; // degrees
static const uint8_t SERVO_CLOSED = 0;  // degrees

// ─── SENSOR THRESHOLDS ────────────────────────────────────────────────────────
// Readings are 10-bit ADC (0–1023).
// Calibrate by running ReadVal.ino and observing real values indoors/outdoors, dry/wet.
//
// LDR:  low value → bright light (window can open)
//       Hysteresis: open when < LIGHT_OPEN_THRESH, close when > LIGHT_CLOSE_THRESH
static const int LIGHT_OPEN_THRESH  = 80;   // below this → bright enough to open
static const int LIGHT_CLOSE_THRESH = 120;  // above this → too dark, close

// Rain: high value → rain detected (close immediately)
//       Hysteresis: close when > RAIN_CLOSE_THRESH, re-allow open when < RAIN_CLEAR_THRESH
static const int RAIN_CLOSE_THRESH = 120;  // above this → raining, close
static const int RAIN_CLEAR_THRESH = 80;   // below this → dry, allow open

// ─── TIMING ───────────────────────────────────────────────────────────────────
static const unsigned long READ_INTERVAL_MS = 1000UL; // sensor poll period

// ─── TYPES ────────────────────────────────────────────────────────────────────
enum class WindowState : uint8_t { CLOSED, OPEN };

// ─── GLOBALS ──────────────────────────────────────────────────────────────────
Servo       windowServo;
WindowState currentState    = WindowState::CLOSED;
unsigned long lastReadMillis = 0;

// ─── HELPERS ──────────────────────────────────────────────────────────────────
/**
 * @brief Moves servo only if state actually changes (avoids jitter from constant writes).
 */
void applyState(WindowState next) {
    if (next == currentState) return; // no change — do nothing

    if (next == WindowState::OPEN) {
        windowServo.write(SERVO_OPEN);
        Serial.println(F("[Window] OPEN  (bright + dry)"));
    } else {
        windowServo.write(SERVO_CLOSED);
        Serial.println(F("[Window] CLOSED"));
    }
    currentState = next;
}

/**
 * @brief Decides window state using hysteresis to prevent chatter.
 *
 * Decision table:
 *   bright AND dry  → OPEN
 *   dark  OR  wet   → CLOSED
 *   between thresholds → keep current state (hysteresis)
 */
WindowState decideState(int ldrVal, int rainVal, WindowState prev) {
    bool isRaining = (rainVal > RAIN_CLOSE_THRESH);
    bool rainClear = (rainVal < RAIN_CLEAR_THRESH);
    bool isBright  = (ldrVal  < LIGHT_OPEN_THRESH);
    bool isDark    = (ldrVal  > LIGHT_CLOSE_THRESH);

    // Rain takes priority — close immediately
    if (isRaining) return WindowState::CLOSED;

    // Definitely bright and definitely dry → open
    if (isBright && rainClear) return WindowState::OPEN;

    // Definitely dark → close
    if (isDark) return WindowState::CLOSED;

    // Readings are in hysteresis band — hold current state
    return prev;
}

// ─── ARDUINO ENTRY POINTS ─────────────────────────────────────────────────────
void setup() {
    Serial.begin(9600);
    Serial.println(F("=== Smart Window ==="));
    Serial.printf("LDR pin: A%d  Rain pin: A%d  Servo pin: D%d\n",
                  LDR_PIN - A0, RAIN_PIN - A0, SERVO_PIN);

    windowServo.attach(SERVO_PIN);
    windowServo.write(SERVO_CLOSED); // safe initial position
    currentState = WindowState::CLOSED;

    Serial.printf("Thresholds — light open:<%-4d close:>%-4d | rain close:>%-4d clear:<%-4d\n",
                  LIGHT_OPEN_THRESH, LIGHT_CLOSE_THRESH,
                  RAIN_CLOSE_THRESH, RAIN_CLEAR_THRESH);
}

void loop() {
    unsigned long now = millis();
    if (now - lastReadMillis < READ_INTERVAL_MS) return; // non-blocking interval
    lastReadMillis = now;

    int ldrVal  = analogRead(LDR_PIN);
    int rainVal = analogRead(RAIN_PIN);

    Serial.printf("[Sensor] LDR=%4d  Rain=%4d  State=%s\n",
                  ldrVal, rainVal,
                  (currentState == WindowState::OPEN) ? "OPEN" : "CLOSED");

    WindowState next = decideState(ldrVal, rainVal, currentState);
    applyState(next);
}
