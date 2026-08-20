/* IMPORT STATEMENTS ************************************************************************************************************************/
#include <NeuroHab_Control.h>

// NH Class Initialization
NH_Control NH;

void basic_lick() {
  if (NH.RW_lick()) {
    NH.RW_dispense();
    Serial.println("RW");
  }
  if (NH.LW_lick()) {
    NH.LW_dispense();
    Serial.println("LW");
  }
  if (NH.FW_lick()) {
    NH.FW_dispense();
    Serial.println("FW");
  }
}

void setup() {
  Serial.begin(9600);

  // Setup NH device.
  NH.begin();
}

void loop() {
  // Detect licks, dispense, and log.
  NH.run();

  basic_lick();
}
