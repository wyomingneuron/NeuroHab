/*
  NeuroHab-MEGA_block_program.ino
  AUTHOR: Sam Crouse  |  scrouse2@uwyo.edu  |  Sun Lab Wyoming SBC

  ── PROTOCOL ─────────────────────────────────────────────────────────────
  Commands are newline-terminated JSON objects:

    {"cmd":"pause"}
    {"cmd":"resume"}
    {"cmd":"get_state"}
    {"cmd":"reset"}
    {"cmd":"load_program",
     "name":"My Program",
     "setup":[<step>, ...],
     "loop":[<step>, ...]}

  load_program must be sent while paused. After loading, send resume.
  Serial is checked between every loop step (never mid-step).

  ── STEP TYPES ───────────────────────────────────────────────────────────

  {"type":"lick_detect",         "port":"RW"|"LW"|"FW"|"any", "timeout_ms":3000}
  {"type":"lick_detect_dispense","port":"RW"|"LW"|"FW"|"any"}
  {"type":"dispense",            "port":"RW"|"LW"|"FW"}
  {"type":"led",                 "port":"RW"|"LW"|"FW"|"all", "state":"on"|"off"}
  {"type":"buzzer",              "port":"RW"|"LW"|"FW"|"all", "state":"on"|"off"}
  {"type":"wait",                "ms":500}
  {"type":"wait_random",         "min_ms":200, "max_ms":2000}
  {"type":"set_threshold",       "val":5.0}
  {"type":"set_brightness",      "val":1.0}
  {"type":"log_msg",             "msg":"checkpoint"}
  {"type":"get_state"}

  {"type":"if_lick", "port":"RW"|"LW"|"FW"|"any", "steps":[<step>,...]}
    Instantaneous (non-blocking) lick check. If the port is licked right
    now, runs the inner steps (up to MAX_IF_STEPS=10). If not, skips
    silently. No else branch. No nesting.

  ── MEMORY BUDGET ────────────────────────────────────────────────────────
  ATmega2560 has 8 KB RAM. sizeof(Step)=32 bytes.
    Outer arrays:  MAX_STEPS(30) × 2 arrays × 32 B = 1920 B
    if_lick pool:  IF_POOL_SIZE(20)          × 32 B =  640 B
    Step arrays total                               = 2560 B
  Leaves ~5.6 KB for NH library, stack, and locals.
*/

#include <NeuroHab_Control.h>

NH_Control NH;

/* ── CONSTANTS ───────────────────────────────────────────────────────────── */

#define MAX_STEPS     30
#define MAX_IF_STEPS  10
#define IF_POOL_SIZE  20

/* ── STEP STRUCT ─────────────────────────────────────────────────────────── */

enum StepType : uint8_t {
  ST_LICK_DETECT = 0,
  ST_LICK_DISPENSE,
  ST_DISPENSE,
  ST_LED,
  ST_BUZZER,
  ST_SET_LED_COLOR,
  ST_SET_BUZ_VOLUME,
  ST_RESET_ALL,
  ST_WAIT,
  ST_WAIT_RANDOM,
  ST_SET_THRESH,
  ST_SET_BRIGHT,
  ST_LOG_MSG,
  ST_GET_STATE,
  ST_FLUSH_LINES,
  ST_IF_LICK,
  ST_UNKNOWN
};

#define PORT_RW  0x01
#define PORT_LW  0x02
#define PORT_FW  0x04
#define PORT_ALL 0x07

struct Step {
  StepType type;
  uint8_t  port;
  uint32_t ms;
  uint32_t ms2;
  float    fval;
  uint8_t  ledState;
  uint8_t r, g, b;   // for set_led_color
  char     msg[13];
  uint8_t  ifStepStart;
  uint8_t  ifStepCount;
};

Step ifStepPool[IF_POOL_SIZE];
uint8_t ifPoolUsed = 0;

Step setupSteps[MAX_STEPS];
Step loopSteps[MAX_STEPS];
uint8_t setupLen = 0;
uint8_t loopLen  = 0;

/* ── RUNTIME STATE ───────────────────────────────────────────────────────── */

bool    isRunning      = false;
bool    setupDone      = false;
uint8_t lastLickedPort = 0;

/* ── NON-BLOCKING SERIAL BUFFER ──────────────────────────────────────────── */
//
// Forward declaration needed because checkSerial() calls handleSerialCommand()
// which is defined later in the file.
void handleSerialCommand(const String& json);
//
// FIXED: The original code used Serial.readStringUntil('\n') which BLOCKS until
// a newline arrives. During that block, NH.RW_dispense() (or any other long
// operation) holds the CPU and the serial buffer (64 bytes on MEGA) overflows,
// dropping bytes from subsequent commands. pause/resume were being lost.
//
// Solution: accumulate incoming bytes one at a time in serialBuf. Only process
// a command when '\n' is seen. checkSerial() returns immediately if no bytes
// are waiting — it never blocks.

#define SERIAL_BUF_SIZE 2048   // enough for a full load_program JSON
static char   serialBuf[SERIAL_BUF_SIZE];
static int    serialBufLen = 0;
static bool   serialOverflow = false;

void checkSerial() {
  if (!Serial.available()) return;   // nothing here — bail immediately
  delay(20);                         // wait for the rest of the message to arrive at 9600 baud
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\r') continue;   // ignore CR (Windows line endings)

    if (c == '\n') {
      // Complete line received — process it
      if (serialOverflow) {
        Serial.println(F("[ERR] Serial overflow — command dropped"));
        serialOverflow = false;
      } else if (serialBufLen > 0) {
        serialBuf[serialBufLen] = '\0';
        handleSerialCommand(String(serialBuf));
      }
      serialBufLen = 0;
      serialOverflow = false;
      continue;
    }

    if (serialOverflow) continue;   // discard rest of overflowed line

    if (serialBufLen >= SERIAL_BUF_SIZE - 1) {
      serialOverflow = true;
      serialBufLen = 0;
      continue;
    }

    serialBuf[serialBufLen++] = c;
  }
}

/* ── SIMPLE JSON HELPERS ─────────────────────────────────────────────────── */

String jsonStr(const String& json, const char* key) {
  String search = String(F("\"")) + key + F("\"");
  int ki = json.indexOf(search);
  if (ki == -1) return "";
  int ci = json.indexOf(':', ki + search.length());
  if (ci == -1) return "";
  ci++;
  while (ci < (int)json.length() && json[ci] == ' ') ci++;
  if (json[ci] == '"') {
    int q2 = json.indexOf('"', ci + 1);
    return (q2 == -1) ? "" : json.substring(ci + 1, q2);
  }
  int end = ci;
  while (end < (int)json.length() && json[end] != ',' && json[end] != '}' && json[end] != ']') end++;
  return json.substring(ci, end);
}

float jsonFloat(const String& json, const char* key, float def = 0.0f) {
  String v = jsonStr(json, key);
  return v.length() ? v.toFloat() : def;
}

long jsonLong(const String& json, const char* key, long def = 0) {
  String v = jsonStr(json, key);
  return v.length() ? v.toInt() : def;
}

uint8_t parsePort(const String& s) {
  if (s == F("RW") || s == F("right")) return PORT_RW;
  if (s == F("LW") || s == F("left"))  return PORT_LW;
  if (s == F("FW") || s == F("front")) return PORT_FW;
  return PORT_ALL;
}

StepType parseType(const String& t) {
  if (t == F("lick_detect"))          return ST_LICK_DETECT;
  if (t == F("lick_detect_dispense")) return ST_LICK_DISPENSE;
  if (t == F("dispense"))             return ST_DISPENSE;
  if (t == F("led"))                  return ST_LED;
  if (t == F("buzzer"))               return ST_BUZZER;
  if (t == F("set_led_color"))   return ST_SET_LED_COLOR;
  if (t == F("set_buz_volume"))  return ST_SET_BUZ_VOLUME;
  if (t == F("reset_all"))       return ST_RESET_ALL;
  if (t == F("wait"))                 return ST_WAIT;
  if (t == F("wait_random"))          return ST_WAIT_RANDOM;
  if (t == F("set_threshold"))        return ST_SET_THRESH;
  if (t == F("set_brightness"))       return ST_SET_BRIGHT;
  if (t == F("log_msg"))              return ST_LOG_MSG;
  if (t == F("get_state"))            return ST_GET_STATE;
  if (t == F("flush_lines"))          return ST_FLUSH_LINES;
  if (t == F("if_lick"))              return ST_IF_LICK;
  return ST_UNKNOWN;
}

/* ── NON-BLOCKING DELAY ──────────────────────────────────────────────────── */

void nb_delay(uint32_t ms) {
  uint32_t start = millis();
  while (millis() - start < ms) {
    NH.run();
    checkSerial();
    if (!isRunning) return;
  }
}

/* ── STEP EXECUTOR ───────────────────────────────────────────────────────── */

void execStep(const Step& s) {
  switch (s.type) {

    case ST_LICK_DETECT: {
      uint32_t start = millis();
      while (true) {
        NH.run(); checkSerial(); if (!isRunning) return;
        if ((s.port & PORT_RW) && NH.RW_lick()) { lastLickedPort = PORT_RW; return; }
        if ((s.port & PORT_LW) && NH.LW_lick()) { lastLickedPort = PORT_LW; return; }
        if ((s.port & PORT_FW) && NH.FW_lick()) { lastLickedPort = PORT_FW; return; }
        if (s.ms > 0 && millis() - start >= s.ms) return;
      }
    }

    case ST_LICK_DISPENSE: {
      if ((s.port & PORT_RW) && NH.RW_lick()) { NH.RW_dispense(); lastLickedPort = PORT_RW; }
      if ((s.port & PORT_LW) && NH.LW_lick()) { NH.LW_dispense(); lastLickedPort = PORT_LW; }
      if ((s.port & PORT_FW) && NH.FW_lick()) { NH.FW_dispense(); lastLickedPort = PORT_FW; }
      checkSerial();
      return;
    }

    case ST_DISPENSE:
      if (s.port & PORT_RW) { NH.RW_dispense(); checkSerial(); }
      if (s.port & PORT_LW) { NH.LW_dispense(); checkSerial(); }
      if (s.port & PORT_FW) { NH.FW_dispense(); checkSerial(); }
      return;

    case ST_LED:
      if (s.ledState) {
        if (s.port & PORT_RW) NH.RW_LED_ON();
        if (s.port & PORT_LW) NH.LW_LED_ON();
        if (s.port & PORT_FW) NH.FW_LED_ON();
      } else {
        if (s.port & PORT_RW) NH.RW_LED_OFF();
        if (s.port & PORT_LW) NH.LW_LED_OFF();
        if (s.port & PORT_FW) NH.FW_LED_OFF();
      }
      return;

    case ST_BUZZER:
      if (s.ledState) {
        if (s.port & PORT_RW) NH.RW_BUZ_ON();
        if (s.port & PORT_LW) NH.LW_BUZ_ON();
        if (s.port & PORT_FW) NH.FW_BUZ_ON();
      } else {
        if (s.port & PORT_RW) NH.RW_BUZ_OFF();
        if (s.port & PORT_LW) NH.LW_BUZ_OFF();
        if (s.port & PORT_FW) NH.FW_BUZ_OFF();
      }
      return;

    case ST_SET_LED_COLOR:
      if (s.port & PORT_RW) NH.set_LED_color(s.r, s.g, s.b);
      if (s.port & PORT_LW) NH.set_LED_color(s.r, s.g, s.b);
      if (s.port & PORT_FW) NH.set_LED_color(s.r, s.g, s.b);
      return;

    case ST_SET_BUZ_VOLUME:
      if (s.port & PORT_RW) NH.set_BUZ_volume((int)s.fval);
      if (s.port & PORT_LW) NH.set_BUZ_volume((int)s.fval);
      if (s.port & PORT_FW) NH.set_BUZ_volume((int)s.fval);
      return;

    case ST_RESET_ALL:
      NH.set_LED_brightness(1.0f);
      NH.set_LED_color(0, 255, 0);
      NH.set_BUZ_volume(1);
      NH.RW_LED_OFF(); NH.RW_BUZ_OFF();
      NH.LW_LED_OFF(); NH.LW_BUZ_OFF();
      NH.FW_LED_OFF(); NH.FW_BUZ_OFF();
      return;

    case ST_WAIT:
      nb_delay(s.ms);
      return;

    case ST_WAIT_RANDOM: {
      uint32_t dur = s.ms + (uint32_t)random(s.ms2 - s.ms + 1);
      nb_delay(dur);
      return;
    }

    case ST_SET_THRESH:
      NH.triggerThreshold = s.fval;
      return;

    case ST_SET_BRIGHT:
      NH.set_LED_brightness(s.fval);
      return;

    case ST_LOG_MSG:
      Serial.print(F("[LOG] "));
      Serial.println(s.msg);
      return;

    case ST_GET_STATE:
      Serial.println(isRunning ? F("[RUNNING]") : F("[STOPPED]"));
      return;

    case ST_FLUSH_LINES: {
      Serial.println(F("[LOG] Flushing lines..."));
      uint32_t flushTimestamp = millis();

      uint8_t valveCount = (s.port == PORT_ALL) ? 3 : 1;
      uint8_t valvePins[3];
      if (s.port == PORT_ALL) {
        valvePins[0] = VALVE_1; valvePins[1] = VALVE_2; valvePins[2] = VALVE_3;
      } else if (s.port & PORT_RW) {
        valvePins[0] = VALVE_1;
      } else if (s.port & PORT_LW) {
        valvePins[0] = VALVE_2;
      } else {
        valvePins[0] = VALVE_3;
      }

      digitalWrite(VALVE_1, LOW);
      digitalWrite(VALVE_2, LOW);
      digitalWrite(VALVE_3, LOW);
      delay(100);
      if (isRunning) {
        for (uint8_t i = 0; i < valveCount; i++) {
          digitalWrite(valvePins[i], HIGH);
        }
      }
      while (millis() - flushTimestamp < s.ms) {
        delay(100);
        checkSerial();
        if (!isRunning) {
          digitalWrite(VALVE_1, LOW);
          digitalWrite(VALVE_2, LOW);
          digitalWrite(VALVE_3, LOW);
          break;
        }
      }

      digitalWrite(VALVE_1, LOW);
      digitalWrite(VALVE_2, LOW);
      digitalWrite(VALVE_3, LOW);
      Serial.println(F("[LOG] Flush done."));
      return;
    }

    case ST_IF_LICK: {
      bool hit = false;
      uint8_t hitPort = 0;
      if      ((s.port & PORT_RW) && NH.RW_lick()) { hit = true; hitPort = PORT_RW; }
      else if ((s.port & PORT_LW) && NH.LW_lick()) { hit = true; hitPort = PORT_LW; }
      else if ((s.port & PORT_FW) && NH.FW_lick()) { hit = true; hitPort = PORT_FW; }

      if (hit) {
        lastLickedPort = hitPort;
        for (uint8_t i = 0; i < s.ifStepCount; i++) {
          if (!isRunning) return;
          NH.run(); checkSerial();
          execStep(ifStepPool[s.ifStepStart + i]);
        }
      }
      return;
    }

    default: return;
  }
}

/* ── STEP VECTOR EXECUTION ───────────────────────────────────────────────── */

void runSteps(Step* steps, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    if (!isRunning) return;
    checkSerial();
    NH.run();
    execStep(steps[i]);
  }
}

/* ── PARSER ──────────────────────────────────────────────────────────────── */

bool parseOneStep(const String& blk, Step& step, bool innerOnly) {
  memset(&step, 0, sizeof(Step));
  step.type = parseType(jsonStr(blk, "type"));
  if (step.type == ST_UNKNOWN) return false;
  if (innerOnly && step.type == ST_IF_LICK) {
    Serial.println(F("[WARN] Nested if_lick skipped"));
    return false;
  }

  String port = jsonStr(blk, "port");
  step.port = port.length() ? parsePort(port) : PORT_ALL;

  switch (step.type) {
    case ST_LICK_DETECT:
      step.ms = (uint32_t)jsonLong(blk, "timeout_ms", 3000); break;
    case ST_LICK_DISPENSE:
      step.ms = (uint32_t)jsonLong(blk, "dur_ms", 2000); break;
    case ST_WAIT:
      step.ms = (uint32_t)jsonLong(blk, "ms", 500); break;
    case ST_WAIT_RANDOM:
      step.ms  = (uint32_t)jsonLong(blk, "min_ms", 200);
      step.ms2 = (uint32_t)jsonLong(blk, "max_ms", 2000);
      if (step.ms2 < step.ms) step.ms2 = step.ms;
      break;
    case ST_LED:
    case ST_BUZZER: {
      String st = jsonStr(blk, "state");
      step.ledState = (st == F("on") || st == F("1")) ? 1 : 0;
      break;
    }
    case ST_SET_LED_COLOR:
      step.r = (uint8_t)jsonLong(blk, "r", 0);
      step.g = (uint8_t)jsonLong(blk, "g", 255);
      step.b = (uint8_t)jsonLong(blk, "b", 0);
      break;
    case ST_SET_BUZ_VOLUME:
      step.fval = jsonFloat(blk, "val", 2.0f);
      break;
    case ST_SET_THRESH:
      step.fval = jsonFloat(blk, "val", 5.0f); break;
    case ST_SET_BRIGHT:
      step.fval = jsonFloat(blk, "val", 1.0f); break;
    case ST_LOG_MSG: {
      String m = jsonStr(blk, "msg");
      m.toCharArray(step.msg, sizeof(step.msg));
      break;
    }
    case ST_FLUSH_LINES: {
      step.ms = (uint32_t)jsonLong(blk, "time_ms", 100000);
      break;
    }
    case ST_IF_LICK:
      step.ifStepStart = 0;
      step.ifStepCount = 0;
      break;
    default: break;
  }
  return true;
}

void parseStepArray(const String& json, int arrStart, int arrEnd,
                    Step* out, uint8_t& count, uint8_t maxCount, bool innerOnly) {
  count = 0;
  int pos = arrStart + 1;

  while (pos < arrEnd && count < maxCount) {
    int bStart = json.indexOf('{', pos);
    if (bStart == -1 || bStart >= arrEnd) break;

    int depth = 1, bEnd = bStart + 1;
    while (bEnd < (int)json.length() && depth > 0) {
      if      (json[bEnd] == '{') depth++;
      else if (json[bEnd] == '}') depth--;
      bEnd++;
    }
    bEnd--;

    String blk = json.substring(bStart, bEnd + 1);
    Step& step  = out[count];

    if (!parseOneStep(blk, step, innerOnly)) { pos = bEnd + 1; continue; }

    if (step.type == ST_IF_LICK) {
      step.ifStepStart = ifPoolUsed;
      step.ifStepCount = 0;

      int sk = blk.indexOf(F("\"steps\""));
      if (sk != -1) {
        int sb = blk.indexOf('[', sk);
        int se = -1, d2 = 0, p2 = sb;
        while (p2 < (int)blk.length()) {
          if (blk[p2] == '[') d2++;
          else if (blk[p2] == ']') { d2--; if (d2 == 0) { se = p2; break; } }
          p2++;
        }
        if (sb != -1 && se != -1) {
          uint8_t innerCount = 0;
          parseStepArray(blk, sb, se,
                         ifStepPool + ifPoolUsed,
                         innerCount,
                         min((uint8_t)MAX_IF_STEPS, (uint8_t)(IF_POOL_SIZE - ifPoolUsed)),
                         true);
          step.ifStepCount = innerCount;
          ifPoolUsed += innerCount;
        }
      }

      Serial.print(F("[PARSE] if_lick inner="));
      Serial.println(step.ifStepCount);
    }

    count++;
    pos = bEnd + 1;
  }
}

void loadProgram(const String& json) {
  setupLen = loopLen = ifPoolUsed = 0;
  setupDone = false;

  int sk = json.indexOf(F("\"setup\""));
  if (sk != -1) {
    int sb = json.indexOf('[', sk);
    int se = json.indexOf(']', sb);
    if (sb != -1 && se != -1)
      parseStepArray(json, sb, se, setupSteps, setupLen, MAX_STEPS, false);
  }

  int lk = json.indexOf(F("\"loop\""));
  if (lk != -1) {
    int lb = json.indexOf('[', lk);
    int le = json.indexOf(']', lb);
    if (lb != -1 && le != -1)
      parseStepArray(json, lb, le, loopSteps, loopLen, MAX_STEPS, false);
  }

  Serial.print(F("[OK] Loaded: "));
  Serial.print(setupLen); Serial.print(F("su "));
  Serial.print(loopLen);  Serial.print(F("lp "));
  Serial.print(ifPoolUsed); Serial.println(F("if"));
}

/* ── SERIAL COMMAND HANDLER ──────────────────────────────────────────────── */

void pause()    { isRunning = false; Serial.println(F("[STOPPED]")); }
void resume()   { isRunning = true;  Serial.println(F("[RUNNING]")); }
void getState() {
  Serial.println(isRunning ? F("[RUNNING]") : F("[STOPPED]"));
  Serial.print(F("[STEPS] su=")); Serial.print(setupLen);
  Serial.print(F(" lp="));       Serial.print(loopLen);
  Serial.print(F(" if="));       Serial.println(ifPoolUsed);
}
void resetDevice() {
  // Flush TX buffer before resetting so the host sees the message
  Serial.println(F("[RESET]"));
  Serial.flush();
  void (*r)(void) = 0; r();
}

void handleSerialCommand(const String& json) {
  String cmd = jsonStr(json, "cmd");
  if      (cmd == F("pause"))        pause();
  else if (cmd == F("resume"))       resume();
  else if (cmd == F("get_state"))    getState();
  else if (cmd == F("reset"))        resetDevice();
  else if (cmd == F("load_program")) {
    if (isRunning) { Serial.println(F("[ERR] Pause first")); }
    else           { loadProgram(json); }
  }
  else { Serial.print(F("[ERR] Unknown: ")); Serial.println(cmd); }
}

/* ── SETUP / LOOP ────────────────────────────────────────────────────────── */

void setup() {
  Serial.begin(9600);
  delay(100);
  Serial.println(F("[SETUP] NeuroHab ready"));

  while (!isRunning) { checkSerial(); }

  NH.begin();
  delay(100);
  Serial.println(F("[INIT] NH ready"));

  if (setupLen > 0) {
    Serial.println(F("[SETUP] Running..."));
    runSteps(setupSteps, setupLen);
    Serial.println(F("[SETUP] Done"));
  }
  setupDone = true;
  Serial.println(F("[LOOP] Running..."));
}

void loop() {
  checkSerial();
  if (!isRunning) {
    NH.run();  
    return;
  }
  
  runSteps(loopSteps, loopLen);
}
