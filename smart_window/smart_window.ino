/**
 * @file    smart_window.ino
 * @brief   Automatic window controller — opens when bright & dry, closes when dark or raining.
 * @board   Arduino Uno / Nano
 *
 * PINS
 *   A0 — LDR (light-dependent resistor)  : low = bright, high = dark
 *   A1 — Rain / PCB moisture sensor       : low = dry,    high = wet
 *   D8 — SG90 servo (0° = closed, 90° = open)
 *
 * CALIBRATION
 *   Flash calibration/ReadVal/ReadVal.ino first, observe ADC values under real
 *   bright/dark and dry/wet conditions, then adjust the thresholds below.
 */

#include <Servo.h>

// ─── PINS ─────────────────────────────────────────────────────────────────────
static const uint8_t LDR_PIN   = A0;
static const uint8_t RAIN_PIN  = A1;
static const uint8_t SERVO_PIN = 8;

// ─── SERVO POSITIONS ──────────────────────────────────────────────────────────
static const uint8_t SERVO_OPEN   = 90; // degrees
static const uint8_t SERVO_CLOSED = 0;  // degrees

// ─── SENSOR THRESHOLDS (0–1023 ADC) ──────────────────────────────────────────
// LDR:  open when < LIGHT_OPEN_THRESH, close when > LIGHT_CLOSE_THRESH
static const int LIGHT_OPEN_THRESH  = 80;
static const int LIGHT_CLOSE_THRESH = 120;

// Rain: close when > RAIN_CLOSE_THRESH, re-allow open when < RAIN_CLEAR_THRESH
static const int RAIN_CLOSE_THRESH  = 120;
static const int RAIN_CLEAR_THRESH  = 80;

// ─── TIMING ───────────────────────────────────────────────────────────────────
static const unsigned long READ_INTERVAL_MS = 1000UL;

// ─── STATE ────────────────────────────────────────────────────────────────────
enum class WindowState : uint8_t { CLOSED, OPEN };

Servo         windowServo;
WindowState   currentState   = WindowState::CLOSED;
unsigned long lastReadMillis = 0;

// ─── HELPERS ──────────────────────────────────────────────────────────────────
// Moves servo only on a state change — avoids jitter from constant writes.
void applyState(WindowState next) {
    if (next == currentState) return;

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
 * Decision table (rain has priority):
 *   raining              → CLOSED
 *   bright AND dry       → OPEN
 *   dark                 → CLOSED
 *   readings in band     → keep current state (hysteresis)
 */
WindowState decideState(int ldrVal, int rainVal, WindowState prev) {
    if (rainVal > RAIN_CLOSE_THRESH)                                return WindowState::CLOSED;
    if (ldrVal  < LIGHT_OPEN_THRESH && rainVal < RAIN_CLEAR_THRESH) return WindowState::OPEN;
    if (ldrVal  > LIGHT_CLOSE_THRESH)                               return WindowState::CLOSED;
    return prev;
}

// ─── ENTRY POINTS ─────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(9600);
    Serial.println(F("=== Smart Window ==="));
    Serial.println(F("LDR: A0  Rain: A1  Servo: D8"));

    windowServo.attach(SERVO_PIN);
    windowServo.write(SERVO_CLOSED);
    currentState = WindowState::CLOSED;
}

void loop() {
    unsigned long now = millis();
    if (now - lastReadMillis < READ_INTERVAL_MS) return;
    lastReadMillis = now;

    int ldrVal  = analogRead(LDR_PIN);
    int rainVal = analogRead(RAIN_PIN);

    char buf[52];
    snprintf(buf, sizeof(buf), "[Sensor] LDR=%4d  Rain=%4d  State=%s",
             ldrVal, rainVal,
             currentState == WindowState::OPEN ? "OPEN" : "CLOSED");
    Serial.println(buf);

    applyState(decideState(ldrVal, rainVal, currentState));
}
