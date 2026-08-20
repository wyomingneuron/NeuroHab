/* IMPORT STATEMENTS ************************************************************************************************************************/
#include <NeuroHab_Control.h>

// NH Class Initialization
NH_Control NH;

void setup() {
  Serial.begin(9600);

  // Setup NH device.
  NH.begin();

  // put your setup code here, to run once:
  // FLUSH LINES
  Serial.println("\nFlushing lines...");
  // Turn all valves off
  float flushTime = 100000;
  float flushTimestamp = millis();
  int valve = 1;
  while (millis() - flushTimestamp < flushTime) {
      digitalWrite(VALVE_1, LOW);
      digitalWrite(VALVE_2, LOW);
      digitalWrite(VALVE_3, LOW);
      delay(100);
      if (valve == 1) {
          digitalWrite(VALVE_1, HIGH);
      }
      if (valve == 2) {
          digitalWrite(VALVE_2, HIGH);
      }
      if (valve == 3) {
          digitalWrite(VALVE_3, HIGH);
      }
      delay(5000);

      valve += 1;
      if (valve > 3) {
          valve = 1;
      }
  }

  digitalWrite(VALVE_1, LOW);
  digitalWrite(VALVE_2, LOW);
  digitalWrite(VALVE_3, LOW);
  Serial.println("Finished flushing lines.");
  // END FLUSH LINES
}

void loop() {
  return;
}
