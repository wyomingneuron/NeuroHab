/* IMPORT STATEMENTS ************************************************************************************************************************/
#include <NeuroHab_Control.h>

// NH Class Initialization
NH_Control NH;

int count = 0;
float t1 = 0;
float t1_thresh = 330;

void setup() {
  Serial.begin(9600);

  // Setup NH device.
  NH.begin();
  delay(100);
}

void loop() {
  // Detect licks, dispense, and log.
  NH.run();

  while (count > 200) {  //650
    NH.run();
    delay(50);
  }

  if (millis()-t1 > t1_thresh) {
    Serial.println(count);
    
    t1 = millis();
    NH.log_event("FW");
    NH.FW_dispense();
    
    count += 1;
  }
}
