# NeuroHab Training Documentation

## Overview

NeuroHab systems consist of three components:

| Component | Role |
|---|---|
| **Arduino MEGA** | Executes the experiment control flow — timing, variables, liquid delivery, conditioned stimulus, and logging requests. |
| **ESP32 DevModule** | Logs events based on TTL input. |
| **FED3 (Feather M0)** | Automated pellet feeding. |

---

## 1. Arduino MEGA

Executes the control flow of the experiment (timing, variables, liquid delivery, liquid conditioned stimulus, and initiating logging requests).

**Core library files** (place in the `NeuroHab_Lib` folder inside the Arduino Library):
- `NeuroHab_Control.h` — function instantiation and variable initialization
- `NeuroHab_Control.cpp` — function definitions

**Mutually exclusive sketches** (only one can be flashed onto the Arduino MEGA at a time):
- `NeuroHab-MEGA_basic_lick.ino` — simple basic licking script
- `NeuroHab-MEGA-Desktop-V1.ino` — HTML interface for visual scripting

---

## 2. ESP32 DevModule

Handles logging of events based on TTL input.

**Sketch to flash:**
- `NeuroHab-ESP_Logging.ino`

**BNC Ports** (numbered 1–5, right to left when facing the NeuroCore black box from the front):

| Port | Function |
|---|---|
| 1 | NeuroHab TTL Out — passthrough from Arduino pulsing |
| 2 | TTL Blank — customizable I/O port |
| 3–5 | BNC input to internal logging; timestamps aligned with NeuroHab microsecond time |

---

## 3. FED3 (Automated Pellet Feeding)

**Core library files** (replace the FED3 source defaults):
- `FED3.h`
- `FED3.cpp`

**Mutually exclusive sketches** (only one can be flashed onto the FED3 Feather M0 at a time):
- `NeuroHab_Classic.ino` — pretested basic FED3 programs
- `FED3-Desktop-V1.ino` — HTML interface for visual scripting

---

## Experiment Workflow

### Step 1 — Water Calibration Protocol

1. Open `NeuroHab_MEGA_calibrate_dispense.ino`.
2. Fill reservoir syringe barrels to the desired volume (recommended: 60 mL).
3. Place a preweighed tray under the FW port to catch dispensed liquid.
4. Flash `NeuroHab_MEGA_calibrate_dispense.ino` to the Arduino MEGA.
5. Edit the count dispensed in `Loop()` to dispense more or less.
   - Set this to the quantity you expect will be needed during the experiment. For example, if the mouse will drink ~3 mL, set the count to 100 — this lets you calibrate across a range of dispensed volumes for the best results.
6. Weigh the dispensed volume in grams.
7. Divide the measured volume by the total count dispensed (e.g., 100) to get the per-dispense volume in mL.
8. Set this calculated value as `WD_QTY` — for example: `WD_QTY = 0.02242;` — in:
   - `NeuroHab_Control.h` (open as `.txt`, edit `WD_QTY`, then save/close)
   - `NeuroHab-ESP_Logging.ino` (open in the Arduino IDE and edit `WD_QTY`)
9. Reflash the Arduino MEGA with the desired experiment code to update the internal value.
10. Reflash `NeuroHab-ESP_Logging.ino` with the new `WD_QTY`.

> **Optional — Validation:** Repeat steps 1–5 on a new NeuroHab file and verify that `totalWD_QTY` in `BNC_1` matches the weighed volume. It should be within 5%.

---

### Step 2 — NeuroHab to HTML Desktop Control: Setup

1. Open `NeuroHab_Desktop_Control_V1.html` (**Chrome required**).
2. Ensure `NeuroHab-MEGA-Desktop-V1` is flashed to the NeuroHab Arduino MEGA.
3. Ensure `NeuroHab-ESP_Logging` is flashed to the ESP32 DevModule.
   - `NeuroHab-ESP_Logging_Setclock` may be flashed first if the displayed clock is incorrect.
   - `NeuroHab_ESP_Logging_FP3002-MODIFIED` may be flashed in place of `NeuroHab-ESP_Logging.ino` if BNC Port 5 (for `BNC_4.csv`) needs to receive a single "high" start pulse.
4. Ensure `FED3-Desktop-V1` is flashed to the FED3, and that `src` `.h`/`.cpp` files are updated.

#### Important Dependencies

If COM ports do not appear or are incorrectly labeled for some devices, ensure the following are installed:
- Arduino IDE
- UART CP210 drivers (via the CP210x VCP Windows Installer)
- Adafruit SAMD Boards (via the Arduino Boards Manager)

#### Connecting Devices

1. In the NeuroHab Desktop Control **Devices** panel, navigate to **NeuroHab 1**. Find **ARDUINO** and select **Connect**.
2. Identify the COM port for the Arduino by plugging it in/out and waiting for the panel to update — or unplug other COM-port devices so only the Arduino is connected.
3. The Arduino is now connected.
4. Repeat the previous steps for the **ESP32** first, then the **FED3**.
5. Once all devices are connected, proceed to Experiment Execution.

---

### Step 3 — NeuroHab to HTML Desktop Control: Experiment Execution

1. Complete the HTML setup steps until all devices are connected.
2. Test device connections by pressing **Reset** in the Devices tab for the ESP32, and by uploading (**Send & Run** → **Selected NeuroHab**) simple stimulus LED scripts to the NeuroHab and FED3.
3. Design block programs or select one from the NeuroHab Program Library or FED3 Program Library.
   - NeuroHab and FED3 programs are block-scripted in separate windows.
4. Hit **Reset** on the ESP32 for the selected NeuroHab in the Devices tab. This resets the ESP32 and creates a new blank folder with `.csv` files.
5. Upload the FED3 program to the FED3 device by hitting **Send & Run**.
   - The FED3 must be powered on and **not** in bootloader mode.
6. Upload the NeuroHab program to NeuroHab 1, 2, 3, or **All Devices**.

> **Notes:**
> - Uploading serial communication to the NeuroHab may cause an erroneous RP (1 pulse) event — ignore this if it's the very first event.
> - The `millis` timestamp in `BNC_1` is based on the **ESP32 clock**, not the Arduino, and starts from ESP32 reset — not from **Send & Run**.

#### Concluding the Experiment

1. Select **Reset** on each active ESP32 device and verify the LCD screen on the NeuroHab Core turns off and opens a new file. Writing to the previous experiment file is now complete.
2. Select **Pause**, then upload the **Reset All** block in the Setup block-programming window to each NeuroHab to end execution of the existing paradigm.
3. Once events have ended, it is safe to remove the micro-SD card from the desired NeuroHab and evaluate the `.csv` files.

---

## Output File Reference

| File | Contents |
|---|---|
| `BNC_1.csv` | Experiment event data |
| `BNC_2.csv` – `BNC_4.csv` | BNC input event data, aligned with `BNC_1` (default) |
