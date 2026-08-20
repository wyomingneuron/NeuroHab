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

void basic_cs() {
  int delay_ms = 200;

  Serial.println("RW CS");
  NH.RW_LED_ON();
  // NH.RW_BUZ_ON();
  delay(delay_ms);
  NH.RW_LED_OFF();
  NH.RW_BUZ_OFF();
  delay(500);

  Serial.println("LW CS");
  NH.LW_LED_ON();
  // NH.LW_BUZ_ON();
  delay(delay_ms);
  NH.LW_LED_OFF();
  NH.LW_BUZ_OFF();
  delay(500);

  Serial.println("FW CS");
  NH.FW_LED_ON();
  // NH.FW_BUZ_ON();
  delay(delay_ms);
  NH.FW_LED_OFF();
  NH.FW_BUZ_OFF();
  delay(500);
}

void setup() {
  Serial.begin(9600);

  // Setup NH device.
  NH.begin();

  // Test Variable Init
  int delay_ms = 200;

  // Test LED and buzzer controls.
  NH.set_LED_color(255, 100, 0);
  NH.set_LED_brightness(1.2);
  NH.set_BUZ_volume(1);

  NH.RW_LED_ON();
  NH.RW_BUZ_ON();
  delay(delay_ms);
  NH.LW_LED_ON();
  NH.LW_BUZ_ON();
  delay(delay_ms);
  NH.FW_LED_ON();
  NH.FW_BUZ_ON();
  delay(500);
  NH.RW_LED_OFF();
  NH.RW_BUZ_OFF();
  delay(delay_ms);
  NH.LW_LED_OFF();
  NH.LW_BUZ_OFF();
  delay(delay_ms);
  NH.FW_LED_OFF();
  NH.FW_BUZ_OFF();
  delay(1000);

  NH.set_LED_color(255, 0, 255);
  NH.set_LED_brightness(1.1);
  NH.set_BUZ_volume(2);

  NH.RW_LED_ON();
  NH.RW_BUZ_ON();
  delay(delay_ms);
  NH.LW_LED_ON();
  NH.LW_BUZ_ON();
  delay(delay_ms);
  NH.FW_LED_ON();
  NH.FW_BUZ_ON();
  delay(500);
  NH.RW_LED_OFF();
  NH.RW_BUZ_OFF();
  delay(delay_ms);
  NH.LW_LED_OFF();
  NH.LW_BUZ_OFF();
  delay(delay_ms);
  NH.FW_LED_OFF();
  NH.FW_BUZ_OFF();
  delay(1000);
}

void loop() {
  // Detect licks, dispense, and log.
  NH.run();

  basic_lick();

  // basic_cs


  // Serial.println("logging RW");
  // NH.log_event("RW");
  // // NH.RW_dispense();
  // // Serial.println("RW");
  // delay(1000);

  // Serial.println("logging LW");
  // NH.log_event("LW");
  // delay(1000);

  // Serial.println("logging FW");
  // NH.log_event("FW");
  // delay(1000);

  // Serial.print("logging WD ");
  // // Serial.println(micros());
  // NH.log_event("WD");
  // delay(1000);
}
