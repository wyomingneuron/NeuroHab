# NeuroHab Ready? — Startup Checklist

Pre-session checklist for bringing a NeuroHab rig online. Work through the sections in order.

---

## 1. Fluid Lines

- [ ] Flush lines until water dispenses
  - [ ] Calibrate lines if not done recently
- [ ] Fill reservoir back to **60 mL**

---

## 2. Cords Connected

- [ ] Arduino — USB B
- [ ] Arduino — 12 V barrel jack
- [ ] ESP32 — micro USB
- [ ] FED3 Aux connected to the **correct** NeuroHab
- [ ] External BNC I/O

---

## 3. Connect All Devices

### 3a. FED3 Setup

Choose the branch that matches your setup.

**If using HTML FED3:**

- [ ] Turn on the FED3
- [ ] Connect Adafruit, Feather M0, or associated COM in Desktop Control
- [ ] Flash the HTML paradigm

> **Prerequisite:** `FED3_Desktop_V1.ino` must be flashed first.

**Else, using Classic FED3:**

- [ ] Flash FED3 with **NeuroHab Classic** or the desired paradigm
- [ ] Ensure `src` `.cpp` screen updates are turned **off** in `run()`

### 3b. Test FED3

- [ ] Trigger an event — it should display on the NeuroHab LCD screen
- [ ] **Non-HTML FED3:** power cycle it (off, then on) to reset the log
- [ ] **HTML FED3:** leave it on, or you will have to reconnect and *Send & Run* again

### 3c. COM Ports in Desktop Control (HTML Chrome Screen)

- [ ] Connect the **Arduino** COM port
  - Arduino (ATMEGA) — COM label reads `Arduino 2560` or similar
- [ ] Connect the **ESP32** COM port under *ESP32*
  - ESP32 — COM label reads `CP_UART…` or similar

---

## 4. SD Card

- [ ] SD card inserted
- [ ] `SD_CARD_ERROR` does **not** display on the NeuroHab LCD
- [ ] Display shows the current time

---

## 5. ESP32 and LEDs

- [ ] Reset the ESP32 once
- [ ] Confirm NeuroHab LEDs are **off**
  - If not: HTML → `ResetAll()` block → *Send & Run*
  - Then reset the ESP32 again

---

## 6. Run

- [ ] Import the desired NeuroHab HTML paradigm — **do not send it yet**
- [ ] Start External Data Collection - Sends Pulse to NeuroHab for Alignment
- [ ] *Send & Run* the NeuroHab HTML paradigm