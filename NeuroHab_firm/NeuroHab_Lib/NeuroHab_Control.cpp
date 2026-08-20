/*
AUTHOR: Sam Crouse
Email: scrouse2@uwyo.edu
Organization: Sun Lab Wyoming SBC
*/

// Import libraries.
#include <Arduino.h>
#include <CapacitiveSensor.h>
#include "NeuroHab_Control.h"
#include <string.h>


/* FUNCTION DEFINITIONS ************************************************************************************************************************/
void NH_Control::begin(float lick_threshold) {
    // SERIAL LOGGING.
    Serial.begin(9600);

    // PINMODE SETUP
    // Setup output pins.
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PINOUT_1, OUTPUT);      // TO ESP32
    pinMode(PINOUT_2, OUTPUT);      // TO EXTERNAL LOGGING

    digitalWrite(PINOUT_1, HIGH);   // DEFAULT HIGH FOR DRAIN SIGNALING (GND PULSES)
    digitalWrite(PINOUT_2, HIGH);

    pinMode(VALVE_1, OUTPUT);
    pinMode(VALVE_2, OUTPUT);
    pinMode(VALVE_3, OUTPUT);

    pinMode(PINOUT_2, INPUT);       // SET EXTERNAL LOGGING PIN TO FLOATING (So that external device can pull low.)

    // Conditioned stimulus pins.
    pinMode(R1, OUTPUT);
    pinMode(G1, OUTPUT);
    pinMode(B1, OUTPUT);
    pinMode(BUZ1, OUTPUT);
    pinMode(R2, OUTPUT);
    pinMode(G2, OUTPUT);
    pinMode(B2, OUTPUT);
    pinMode(BUZ2, OUTPUT);
    pinMode(R3, OUTPUT);
    pinMode(G3, OUTPUT);
    pinMode(B3, OUTPUT);
    pinMode(BUZ3, OUTPUT);

    // LICKPORT SETUP
    // Initialize lick sensors.
    S1.set_CS_Timeout_Millis(CS_timeoutmillis);
    S1.set_CS_AutocaL_Millis(500);
    S1.reset_CS_AutoCal();

    S2.set_CS_Timeout_Millis(CS_timeoutmillis);
    S2.set_CS_AutocaL_Millis(500);
    S2.reset_CS_AutoCal();

    S3.set_CS_Timeout_Millis(CS_timeoutmillis);
    S3.set_CS_AutocaL_Millis(500);
    S3.reset_CS_AutoCal();

    triggerThreshold = lick_threshold;

    // VALVE SETUP
    // Turn all valves off.
    digitalWrite(VALVE_1, LOW);
    digitalWrite(VALVE_2, LOW);
    digitalWrite(VALVE_3, LOW);

    // First serial log before initialization.
    Serial.print("Initializing for ");
    Serial.print(initialization_delay);
    Serial.println(" milliseconds");
    delay(100);

    // Final delay before start.
    initialization_start = millis();
    while(millis() - initialization_start < initialization_delay) {
        canDetectPort1 = false;
        canDetectPort2 = false;
        canDetectPort3 = false;
        run();  // Fill buffers, etc and let system initialize.
    }

    canDetectPort1 = true;
    canDetectPort2 = true;
    canDetectPort3 = true;

    // First serial log before start.
    Serial.print("Lickport sensitivity threshold: ");
    Serial.println(lick_threshold);

    Serial.println("Initialization complete!");
    delay(100);
}

void NH_Control::setColor(float r_color, int r_pin, float g_color, int g_pin, float b_color, int b_pin, float brightness = 1) {

    if (brightness > 1) { brightness = 1; }

    analogWrite(r_pin, r_color * brightness);
    analogWrite(g_pin, g_color * brightness);
    analogWrite(b_pin, b_color * brightness);
}

void NH_Control::pulsePin(int pin, int count, int delay_ms) {
    /*
      Pulses the given pin, count times, with a delay_ms before and after the pulse.
    */
    // Serial.print("pulsing ");
    // Serial.print(count);
    // Serial.println(" times.");

    for (int i = 0; i < count; i++) {
        digitalWrite(pin, HIGH);
        delayMicroseconds(delay_ms);
        digitalWrite(pin, LOW);
        delayMicroseconds(delay_ms);
    }
}

void NH_Control::dualPulsePin_HIGH(int pin1, int pin2, int count, int delay_micros) {
    /*
      Pulses the given pin, count times, with a delay_micros before and after the pulse.
    */
    // Serial.print("pulsing ");
    // Serial.print(count);
    // Serial.println(" times.");

    for (int i = 0; i < count; i++) {
        digitalWrite(pin1, HIGH);
        digitalWrite(pin2, HIGH);
        delayMicroseconds(delay_micros);
        digitalWrite(pin1, LOW);
        digitalWrite(pin2, LOW);
        delayMicroseconds(delay_micros);
    }
}

void NH_Control::dualPulsePin_LOW(int pin1, int pin2, int count, int delay_micros) {
    /*
      Pulses the given pin, count times, with a delay_micros before and after the pulse.
    */
    // Serial.print("pulsing ");
    // Serial.print(count);
    // Serial.println(" times.");

    pinMode(pin1, OUTPUT);
    pinMode(pin2, OUTPUT);

    for (int i = 0; i < count; i++) {
        digitalWrite(pin1, LOW);
        digitalWrite(pin2, LOW);
        delayMicroseconds(delay_micros);
        digitalWrite(pin1, HIGH);
        digitalWrite(pin2, HIGH);
        delayMicroseconds(delay_micros);
    }

    pinMode(pin2, INPUT);  // Set BNC pin to input and assume external pullup.
}

// Function to insert a new value at the right, shifting left and discarding index 0
void NH_Control::insertAtRight(int* buffer, int size, int newValue) {
    if (size <= 0) return; // Safety check

    // Manually shift elements left (index 1 to 0, 2 to 1, ..., size-2 to size-1)
    for (int i = 0; i < size - 1; i++) {
        buffer[i] = buffer[i + 1];
    }

    // Insert new value at the right end (index size-1)
    buffer[size - 1] = newValue;
}

void NH_Control::insertAtRight(int* buffer, int size, int newValue, long& runningSum, int& head) {
    runningSum -= buffer[head];  // subtract the oldest value
    buffer[head] = newValue;     // overwrite it with the new value
    runningSum += newValue;      // add new value to sum
    head = (head + 1) % size;   // advance head, wrapping around
}

// Function to print the array for debugging (using Serial)
void NH_Control::printBuffer(int* buffer, int size) {
    for (int i = 0; i < size; i++) {
        Serial.print(buffer[i]);
        Serial.print(" ");
    }
    Serial.println();
}

// Function to calculate the average value of a long array, returning float
float NH_Control::calculateAverage(int* buffer, int size) {
    if (size <= 0) return 0; // Safety check for invalid size

    float sum = 0;
    for (int i = 0; i < size; i++) {
        sum += buffer[i];
    }

    return sum*1.0f / size; // Convert from integer division, prevent truncates decimal part
}

// Function to calculate the average value of an array excluding last n elements, returning float
float NH_Control::calculateAverageExclude(int* buffer, int size, int exclude_n) {
    if (size <= 0) return 0; // Safety check for invalid size

    float sum = 0;
    for (int i = 0; i < (size-exclude_n); i++) {
        sum += buffer[i];
    }

    return sum*1.0f / (size-exclude_n); // Convert from integer division, prevent truncates decimal part
}

float NH_Control::runningAverage(long running_sum, int size) {
    return float(running_sum) / float(size);
}

void NH_Control::condStim_ON_LED(bool rightW = false, bool leftW = false, bool frontW = false, float R_col = 0, float G_col = 0, float B_col = 0) {
    // Activate conditioned stimulus for each port.
    if (rightW) {
        setColor(R_col, R1, G_col, G1, B_col, B1, LED_BRIGHT);
	log_event("LED");
    }

    if (leftW) {
        setColor(R_col, R2, G_col, G2, B_col, B2, LED_BRIGHT);
	log_event("LED");
    }

    if (frontW) {
        setColor(R_col, R3, G_col, G3, B_col, B3, LED_BRIGHT);
	log_event("LED");
    }
}

void NH_Control::condStim_ON_BUZ(bool rightW = false, bool leftW = false, bool frontW = false, int volume = 0) {
    if (volume > 255) {
        volume = 255;
    }

    if (volume < 0) {
        volume = 0;
    }

    // Activate conditioned stimulus for each port.
    if (rightW) {
        analogWrite(BUZ1, volume);
	log_event("TONE");
    }

    if (leftW) {
        analogWrite(BUZ2, volume);
	log_event("TONE");
    }

    if (frontW) {
        analogWrite(BUZ3, volume);
	log_event("TONE");
    }
}

void NH_Control::condStim_OFF_LED(bool rightW = false, bool leftW = false, bool frontW = false) {
    // Deactivate conditioned stimulus.
    if (rightW) {
        setColor(0, R1, 0, G1, 0, B1, 1);
    }

    if (leftW) {
        setColor(0, R2, 0, G2, 0, B2, 1);
    }

    if (frontW) {
        setColor(0, R3, 0, G3, 0, B3, 1);
    }
}

void NH_Control::condStim_OFF_BUZ(bool rightW = false, bool leftW = false, bool frontW = false) {
    // Deactivate conditioned stimulus.
    if (rightW) {
        analogWrite(BUZ1, 0);
    }

    if (leftW) {
        analogWrite(BUZ2, 0);
    }

    if (frontW) {
        analogWrite(BUZ3, 0);
    }
}

void NH_Control::log_event(const char* event) {
    int pulseCount = -1;

    if (strcmp(event, "RW") == 0)       { pulseCount = RW; }
    else if (strcmp(event, "LW") == 0)  { pulseCount = LW; }
    else if (strcmp(event, "FW") == 0)  { pulseCount = FW; }
    else if (strcmp(event, "WD") == 0)  { pulseCount = WD; }
    else if (strcmp(event, "LED") == 0)  { pulseCount = LED; }
    else if (strcmp(event, "TONE") == 0)  { pulseCount = TONE; }

    if (pulseCount == -1) { return; }

    dualPulsePin_LOW(PINOUT_1, PINOUT_2, pulseCount, pulse_delay_micros);
    delay(PULSE_SEPAR_DELAY_MS);
}

void NH_Control::set_LED_color(int r, int g, int b) {
    R = r;
    G = g;
    B = b;
}

void NH_Control::set_LED_brightness(float brightness) {
    if (brightness > 1) {
        brightness = 1;
    }

    if (brightness < 0.001) {
        brightness = 0.001;
    }
    
    LED_BRIGHT = brightness;
}

void NH_Control::set_BUZ_volume(int volume) {
    if (volume > 255) {
        volume = 255;
    }

    if (volume < 1) {
        volume = 1;
    }

    BUZ_VOL = volume;
}

bool NH_Control::RW_lick() {
    // Capacitive sense calculation.
    float base = runningAverage(val1_running_sum, capBufSize);

    if (base == 0) {
        return false;
    }

    // Detect a rising edge for the capacitance sensor.
    if (canDetectPort1 == true && val1 > (base + triggerThreshold)) {
        base_saved1 = base;
        canDetectPort1 = false;

        log_event("RW");

        delay(lickportTimeout);

        return true;
    }

    // Detect a falling edge back to baseline.
    if (canDetectPort1 == false && val1 <= (base_saved1 + resetThreshold)) {
        base_saved1 = -1;
        canDetectPort1 = true;

        return false;
    }

    return false;
}

bool NH_Control::LW_lick() {
    // Capacitive sense calculation.
    float base = runningAverage(val2_running_sum, capBufSize);

    if (base == 0) {
        return false;
    }

    // Detect a rising edge for the capacitance sensor.
    if (canDetectPort2 == true && val2 > (base + triggerThreshold)) {
        base_saved2 = base;
        canDetectPort2 = false;

        log_event("LW");

        delay(lickportTimeout);

        return true;
    }

    // Detect a falling edge back to baseline.
    if (canDetectPort2 == false && val2 <= (base_saved2 + resetThreshold)) {
        base_saved2 = -1;
        canDetectPort2 = true;

        return false;
    }

    return false;
}

bool NH_Control::FW_lick() {
    // Capacitive sense calculation.
    float base = runningAverage(val3_running_sum, capBufSize);

    if (base == 0) {
        return false;
    }

    // Detect a rising edge for the capacitance sensor.
    if (canDetectPort3 == true && val3 > (base + triggerThreshold)) {
        base_saved3 = base;
        canDetectPort3 = false;

        log_event("FW");

        delay(lickportTimeout);

        return true;
    }

    // Detect a falling edge back to baseline.
    if (canDetectPort3 == false && val3 <= (base_saved3 + resetThreshold)) {
        base_saved3 = -1;
        canDetectPort3 = true;

        return false;
    }

    return false;
}

void NH_Control::RW_dispense() {
    RW_ON = true;
}

void NH_Control::RW_control() {
    // Right water signal control.
    if (RW_ON && !RW_SIGNALED) {
        // Start dispensing water and log the event.
        digitalWrite(VALVE_1, HIGH);
        log_event("WD");
        RW_delivered += WD_QTY;

        // SET RW SIGNAL VALUES
        LICK_TIMER = millis();
        RW_SIGNALED = true;

        return;
    }

    if (millis() - LICK_TIMER > lickTime) {
        // Stop dispensing water.
        digitalWrite(VALVE_1, LOW);

        // RESET RW SIGNAL VALUES
        RW_ON = false;
        RW_SIGNALED = false;

        return;
    }
}

void NH_Control::RW_LED_ON() {
    condStim_ON_LED(true, false, false, R, G, B);
}

void NH_Control::RW_LED_OFF() {
    condStim_OFF_LED(true, false, false);
}

void NH_Control::RW_BUZ_ON() {
    condStim_ON_BUZ(true, false, false, BUZ_VOL);
}

void NH_Control::RW_BUZ_OFF() {
    condStim_OFF_BUZ(true, false, false);
}

void NH_Control::LW_dispense() {
    LW_ON = true;
}

void NH_Control::LW_control() {
    // Left water signal control.
    if (LW_ON && !LW_SIGNALED) {
        // Start dispensing water and log the event.
        digitalWrite(VALVE_2, HIGH);
        log_event("WD");
        LW_delivered += WD_QTY;

        // SET LW SIGNAL VALUES
        LICK_TIMER = millis();
        LW_SIGNALED = true;

        return;
    }

    if (millis() - LICK_TIMER > lickTime) {
        // Stop dispensing water.
        digitalWrite(VALVE_2, LOW);

        // RESET LW SIGNAL VALUES
        LW_ON = false;
        LW_SIGNALED = false;

        return;
    }
}

void NH_Control::LW_LED_ON() {
    condStim_ON_LED(false, true, false, R, G, B);
}

void NH_Control::LW_LED_OFF() {
    condStim_OFF_LED(false, true, false);
}

void NH_Control::LW_BUZ_ON() {
    condStim_ON_BUZ(false, true, false, BUZ_VOL);
}

void NH_Control::LW_BUZ_OFF() {
    condStim_OFF_BUZ(false, true, false);
}

void NH_Control::FW_dispense() {
    FW_ON = true;
}

void NH_Control::FW_control() {
    // Front water signal control.
    if (FW_ON && !FW_SIGNALED) {
        // Start dispensing water and log the event.
        digitalWrite(VALVE_3, HIGH);
        log_event("WD");
        FW_delivered += WD_QTY;

        // SET FW SIGNAL VALUES
        LICK_TIMER = millis();
        FW_SIGNALED = true;

        return;
    }

    if (millis() - LICK_TIMER > lickTime) {
        // Stop dispensing water.
        digitalWrite(VALVE_3, LOW);

        // RESET FW SIGNAL VALUES
        FW_ON = false;
        FW_SIGNALED = false;

        return;
    }
}

void NH_Control::FW_LED_ON() {
    condStim_ON_LED(false, false, true, R, G, B);
}

void NH_Control::FW_LED_OFF() {
    condStim_OFF_LED(false, false, true);
}

void NH_Control::FW_BUZ_ON() {
    condStim_ON_BUZ(false, false, true, BUZ_VOL);
}

void NH_Control::FW_BUZ_OFF() {
    condStim_OFF_BUZ(false, false, true);
}

void NH_Control::run() {
    /*
      Handles the lick detection and pulsing for each lickport.
    */

    // latency_stamp1 = micros();  // remove

    // Update the capacitive sensor values.
    val1 = S1.capacitiveSensor(samples);
    val2 = S2.capacitiveSensor(samples);
    val3 = S3.capacitiveSensor(samples);

    // Serial.print(val1);
    // Serial.print(' ');
    // Serial.print(val2);
    // Serial.print(' ');
    // Serial.print(val3);
    // Serial.println();

    if (val1 > 0) { insertAtRight(capBuffer1, capBufSize, val1, val1_running_sum, val1_head); }
    if (val2 > 0) { insertAtRight(capBuffer2, capBufSize, val2, val2_running_sum, val2_head); }
    if (val3 > 0) { insertAtRight(capBuffer3, capBufSize, val3, val3_running_sum, val3_head); }

    // Signal control for lickport dispense. (non-blocking)
    RW_control();
    LW_control();
    FW_control();
}
