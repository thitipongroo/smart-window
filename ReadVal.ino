/**
 * @file    ReadVal.ino
 * @brief   Sensor calibration utility — reads LDR and rain sensor, prints to Serial Monitor.
 * @board   Arduino Uno / Nano
 *
 * PURPOSE
 *   Run this sketch BEFORE deploying smart_window.ino.
 *   Observe the printed values in different real-world conditions (bright/dark, dry/wet)
 *   and use them to set LIGHT_OPEN_THRESH / RAIN_CLOSE_THRESH in smart_window.ino.
 *
 *
 * CHANGES:
 *  - [BUG]    int LDRPin / PCBPin → const uint8_t (pins never change, int wastes 2 bytes each)
 *  - [DESIGN] Adds readable calibration guidance in Serial output
 *  - [DESIGN] Prints min/max over 10 samples to help pick stable threshold values
 *  - [DESIGN] Non-blocking interval via millis() (still effectively 1 s, but doesn't block ISR)
 *  - [DESIGN] Named constants replace magic number 9600 and 1000
 */

// ─── HARDWARE CONSTANTS ───────────────────────────────────────────────────────
static const uint8_t LDR_PIN  = A0;  // Light-dependent resistor
static const uint8_t RAIN_PIN = A1;  // Moisture / rain sensor

// ─── TIMING ───────────────────────────────────────────────────────────────────
static const unsigned long READ_INTERVAL_MS = 1000UL; // one reading per second
static const uint8_t       SAMPLE_COUNT     = 10;     // samples before printing min/max

// ─── GLOBALS ──────────────────────────────────────────────────────────────────
unsigned long lastReadMillis = 0;
uint8_t       sampleIndex   = 0;
int           ldrMin  = 1023, ldrMax  = 0;
int           rainMin = 1023, rainMax = 0;

// ─── ARDUINO ENTRY POINTS ─────────────────────────────────────────────────────
void setup() {
    Serial.begin(9600);
    Serial.println(F("=== Smart Window — Sensor Calibration ==="));
    Serial.println(F("LDR  pin: A0 | Rain pin: A1"));
    Serial.println(F("ADC range: 0 (min) to 1023 (max)"));
    Serial.println(F(""));
    Serial.println(F("Tip: LDR  — low value = bright, high value = dark"));
    Serial.println(F("Tip: Rain — low value = dry,    high value = wet"));
    Serial.println(F("─────────────────────────────────────────"));
}

void loop() {
    unsigned long now = millis();
    if (now - lastReadMillis < READ_INTERVAL_MS) return;
    lastReadMillis = now;

    int ldrVal  = analogRead(LDR_PIN);
    int rainVal = analogRead(RAIN_PIN);

    // Track running min/max for calibration guidance
    ldrMin  = min(ldrMin,  ldrVal);
    ldrMax  = max(ldrMax,  ldrVal);
    rainMin = min(rainMin, rainVal);
    rainMax = max(rainMax, rainVal);

    Serial.printf("[%4lu s] LDR=%4d  Rain=%4d\n",
                  now / 1000UL, ldrVal, rainVal);

    // Every SAMPLE_COUNT readings, print observed range
    if (++sampleIndex >= SAMPLE_COUNT) {
        sampleIndex = 0;
        Serial.println(F("── Observed ranges ──────────────────────"));
        Serial.printf("  LDR  min=%4d  max=%4d\n", ldrMin, ldrMax);
        Serial.printf("  Rain min=%4d  max=%4d\n", rainMin, rainMax);
        Serial.println(F("  → Suggested: set LIGHT_OPEN_THRESH between LDR min and midpoint"));
        Serial.println(F("  → Suggested: set RAIN_CLOSE_THRESH  between Rain max-dry and min-wet"));
        Serial.println(F("─────────────────────────────────────────"));
        // Reset for next window
        ldrMin  = 1023; ldrMax  = 0;
        rainMin = 1023; rainMax = 0;
    }
}
