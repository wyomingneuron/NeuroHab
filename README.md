# QUICK START GUIDE
This README contains the documentation for setup, addresses FAQ's, hardware installation, calibration, and more. Generally after a new problem is solved it is documented here.<br>
Check for solutions here and if this README does not solve your problem email me, Samuel Crouse at scrouse2@uwyo.edu.<br><br>

## ARDUINO IDE DEPENDENCIES
FED3 by Lex Kravitz : 1.16.3<br>
CapacitiveSensor by Paul Bagder : 0.5.1<br>
UART CP210 drivers on installed. Via CP210x VCP Windows Installer. This allows the ESP32 to connect properly via USB. <br><br>

# NeuroHab Basic Setup and Use
## NeuroHab_firm
<b>NeuroHab_Lib</b> goes in your arduino library folder. Probably: "C:\Users\username\Documents\Arduino\libraries\PLACE_HERE"

<b>The following go in your sketches folder.</b> <br>

NeuroHab-ESP_Logging <br>
NeuroHab-ESP_Logging_Setclock <br>
NeuroHab-MEGA_Example <br>
NeuroHab-MEGA_basic_lick <br>
NeuroHab-MEGA_flush_lines <br><br>


## Arduino IDE Board Selection
Files labeled -ESP will use <b>ESP32 Dev Module</b> <br>
Files labeled -MEGA will use <b>Arduino Mega or Mega 2560</b> <br><br>

## Flashing ESP32
Ensure UART CP210 drivers on installed. Via CP210x VCP Windows Installer. <br><br>


## Flashing ESP32 Time Clock
Upload NeuroHab-ESP_Logging_Setclock to the Core to set the clock on the LCD screen. You MUST now reflash with NeuroHab-ESP_Logging.<br>
After selecting upload if the board does not automatically connect you made need to hold the boot button on the board. *The small button to the right of the micro-usb connection cable.<b><br><br>


## Flashing Arduino Mega 2560
Select the Arduino Mega 2560 COM6 (or COMX) from the drop down and select upload.<br><br>


## Removing Air Bubbles from Lines (flush lines)
To remove air bubbles from your lines you need to flash the Arduino Mega (lower square port) with <b>NeuroHab-MEGA_flush_lines</b>.<br>
By default this program will cycle open and closing each valve individually for 5 seconds. If this does not flush the lines properly, change the following lines of code.<br>

      if (valve == 1) {
          digitalWrite(VALVE_1, HIGH);
      }
      if (valve == 2) {
          digitalWrite(VALVE_2, HIGH);
      }
      if (valve == 3) {
          digitalWrite(VALVE_3, HIGH);
      }

      to

      if (valve == 1) {
          digitalWrite(VALVE_1, HIGH);
      }
      if (valve == 2) {
          digitalWrite(VALVE_1, HIGH);
      }
      if (valve == 3) {
          digitalWrite(VALVE_1, HIGH);
      }

Change VALVE_1 to VALVE_2 or VALVE_3 to flush each line completely. You must flash the Arduino each time you change the valve number.
#### You must reflash the Arduino with your desired code to resume experiment behavior.<br><br>


## Lickport Water Delivery Quantity Calibration
This code is found within "NeuroHab/NeuroHab_firm/NeuroHab_Lib/NeuroHab_Control.h" or locally "C:\Users\username\Documents\Arduino\libraries\NeuroHab_Lib\NeuroHab_Control.h"
One of the main limitations of the NeuroHab is the inability to communicate quantities between the Arduino MEGA controller and the ESP32 logger. This is overcome by hard coding the liquid delivery amount within:<br>

      NeuroHab-ESP_logging at the line
      float WD_QTY=0.006667 * 0.65;  // ml per WD

      and

      libraries/NeuroHab_lib/NeuroHab_Control.h at the line
      float lickTime = (250 * 0.65) - PULSE_SEPAR_DELAY_MS;	// How long the lickport valve is open before closing. Pulse delay to account for pulse time while valve is open.

The lickTime variable dictates how long the port stays open upon activation. IE greater times deliver more liquid and short times less.<br>
WD_QTY is the quantity that is recorded in the logging system. 0.65 is the scalar used to adjust delivery amount without ruining calibration approximately. IE at 250 ms 0.006667 ml is delivered and we scale the delivery time and amount by 0.65 to reduce the delivery amount without having to recalibrate delivery.<br>

However, if you need to change the delivery amount I recommend recalibration and this is the method.

1. Set the lickTime to the estimated time delivery amount. (100ms is generally about right to get a medium sized droplet delivered.)
2. Use a micropipette to measure the size of the droplet. (You may also program or manually deliver 10-50-100-X droplets into a container to have a greater amount to more easily measure. Remember to divide by the total number to get the average delivery size.)
3. If you found that your delivery amount is 0.002ml per delivery, replace <b>float WD_QTY=0.006667 * 0.65;</b> with <b>float WD_QTY=0.002;</b>
4. The scalar 0.65 is not mandatory and may be removed on recalibration, but adding your scalar or replacing with 1.0 is a good way to easily change delivery amount without recalibration.
5. After recalibration with a 0.002ml delivery amount your final code lines may look like this:

Example:

      NeuroHab-ESP_logging at the line
      float WD_QTY=0.002 * 1.0;  // ml per WD

      and

      libraries/NeuroHab_lib/NeuroHab_Control.h at the line
      float lickTime = (85 * 1.0) - PULSE_SEPAR_DELAY_MS;	// How long the lickport valve is open before closing. Pulse delay to account for pulse time while valve is open.

#### PULSE_SEPAR_DELAY_MS should not be removed as that delay is added during logging and removing it here will actually inflate the time the valves are open. Similarly, the minimum time a valve may be open is the PULSE_SEPAR_DELAY_MS which by default is 60ms. This also caps the logging at ~16HZ. I do not recommend changing PULSE_SEPAR_DELAY_MS at reducing times may cause events to be missed.<br><br>


## Lickports Activating Multiple Times? Reduce Sensitivity Here.
This code is found within "NeuroHab/NeuroHab_firm/NeuroHab_Lib/NeuroHab_Control.h" or locally "C:\Users\username\Documents\Arduino\libraries\NeuroHab_Lib\NeuroHab_Control.h"
<img width="794" height="14" alt="image" src="https://github.com/user-attachments/assets/8f6bbea2-b444-4bdc-be26-858d040545fd" /> <br>
At 20 this is quite sensitive. 25 is good, and 30 may be too restrictive for mice, but it works well when manually activating it with a finger. Please test this value in your own setup.


# Hardware Installation
## Lickport Installation
Each of 3 lickports comes with the port and magnetic housing. The housing inner diameter is ~21mm and the holes are sized for 3mm screws.<br>
To install I recommend placing the magnetic housing on the wall of your homecage and marking both mounting holes and the center 21mm hole with a sharpie. Then, use a 3.5mm drill bit to drill the mounting holes and a 20mm saw bit or similar to drill the center hole. Use the provided screws to mount the lickport housing on the wall. The lickport itself should now easily fit through the wall and allow liquid delivery into the homecage on activation.<br><br>

