/**
 * @file    ReadVal.ino
 * @brief   Sensor calibration utility — prints LDR and rain sensor readings every second.
 * @board   Arduino Uno / Nano
 *
 * Flash this sketch BEFORE smart_window.ino.
 * Observe ADC values (0–1023) in all real conditions:
 *   · indoors (dark) and outdoors (bright) → note LDR range
 *   · dry surface and wet surface          → note Rain range
 *
 * Every 10 readings the sketch prints the observed min/max and threshold suggestions.
 * Use those values to set LIGHT_OPEN_THRESH / RAIN_CLOSE_THRESH in smart_window.ino.
 *
 * Tip: LDR  — low = bright, high = dark
 * Tip: Rain — low = dry,    high = wet
 */

// ─── PINS ─────────────────────────────────────────────────────────────────────
static const uint8_t LDR_PIN  = A0;
static const uint8_t RAIN_PIN = A1;

// ─── TIMING ───────────────────────────────────────────────────────────────────
static const unsigned long READ_INTERVAL_MS = 1000UL;
static const uint8_t       SAMPLE_COUNT     = 10;

// ─── GLOBALS ──────────────────────────────────────────────────────────────────
unsigned long lastReadMillis = 0;
uint8_t       sampleIndex   = 0;
int           ldrMin  = 1023, ldrMax  = 0;
int           rainMin = 1023, rainMax = 0;

// ─── ENTRY POINTS ─────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(9600);
    Serial.println(F("=== Smart Window — Calibration ==="));
    Serial.println(F("LDR: A0  Rain: A1  |  ADC range: 0-1023"));
    Serial.println(F("-----------------------------------------"));
}

void loop() {
    unsigned long now = millis();
    if (now - lastReadMillis < READ_INTERVAL_MS) return;
    lastReadMillis = now;

    int ldrVal  = analogRead(LDR_PIN);
    int rainVal = analogRead(RAIN_PIN);

    ldrMin  = min(ldrMin,  ldrVal);
    ldrMax  = max(ldrMax,  ldrVal);
    rainMin = min(rainMin, rainVal);
    rainMax = max(rainMax, rainVal);

    char buf[44];
    snprintf(buf, sizeof(buf), "[%4lus] LDR=%4d  Rain=%4d",
             now / 1000UL, ldrVal, rainVal);
    Serial.println(buf);

    if (++sampleIndex >= SAMPLE_COUNT) {
        sampleIndex = 0;

        char rbuf[48];
        Serial.println(F("-- Observed ranges -----------------------"));
        snprintf(rbuf, sizeof(rbuf), "  LDR  min=%4d  max=%4d", ldrMin, ldrMax);
        Serial.println(rbuf);
        snprintf(rbuf, sizeof(rbuf), "  Rain min=%4d  max=%4d", rainMin, rainMax);
        Serial.println(rbuf);
        Serial.println(F("  -> Set LIGHT_OPEN_THRESH  just above your bright min"));
        Serial.println(F("  -> Set RAIN_CLOSE_THRESH  just below your wet min"));
        Serial.println(F("-----------------------------------------"));

        ldrMin = 1023; ldrMax = 0;
        rainMin = 1023; rainMax = 0;
    }
}
