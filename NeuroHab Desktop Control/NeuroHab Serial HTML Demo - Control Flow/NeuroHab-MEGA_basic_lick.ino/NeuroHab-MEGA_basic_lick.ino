/* IMPORT STATEMENTS ************************************************************************************************************************/
#include <NeuroHab_Control.h>

// NH Class Initialization
NH_Control NH;

bool isRunning = false;

void pause() {
  Serial.println("[STOPPED]");
  isRunning = false;
}

void resume() {
  Serial.println("[RUNNING]");
  isRunning = true;
}

void getState() {
  Serial.println(isRunning ? "[RUNNING]" : "[STOPPED]");
}

void reset() {
  void (*resetFunc)(void) = 0;  // declare a function pointer to address 0
  resetFunc();                   // jump to bootloader / reset vector
}

// ── JSON command parser ────────────────────────────────────────────────────

void handleSerialCommand(const String& json) {
  // Extract value of "cmd" key  e.g. {"cmd":"pause"}
  int cmdStart = json.indexOf("\"cmd\"");
  if (cmdStart == -1) return;

  int colonPos = json.indexOf(':', cmdStart);
  if (colonPos == -1) return;

  int quoteOpen  = json.indexOf('"', colonPos + 1);
  if (quoteOpen == -1) return;

  int quoteClose = json.indexOf('"', quoteOpen + 1);
  if (quoteClose == -1) return;

  String cmd = json.substring(quoteOpen + 1, quoteClose);

  if      (cmd == "pause")     pause();
  else if (cmd == "resume")    resume();
  else if (cmd == "get_state") getState();
  else if (cmd == "reset")     reset();
  else Serial.println("unknown cmd: " + cmd);
}

void basic_lick() {
  if (NH.RW_lick()) {
    NH.RW_dispense();
    // Serial.println("RW");
  }
  if (NH.LW_lick()) {
    NH.LW_dispense();
    // Serial.println("LW");
  }
  if (NH.FW_lick()) {
    NH.FW_dispense();
    // Serial.println("FW");
  }
}

void checkSerial() {
  if (Serial.available()) {
    String incoming = Serial.readStringUntil('\n');
    incoming.trim();
    if (incoming.length() > 0) {
      handleSerialCommand(incoming);
    }
  }
}

void setup() {
  Serial.begin(9600);
  delay(100);
  Serial.println(isRunning);
  Serial.println("waiting to start...");

  while (!isRunning) {
    checkSerial();
  }

  // Setup NH device.
  NH.begin();

  Serial.println("Starting...");
}

void loop() {
  checkSerial();

  if (isRunning) {
    // Detect licks, dispense, and log.
    NH.run();

    basic_lick();
  }
}
