/*
  FED3_block_program.ino
  AUTHOR: Sam Crouse  |  scrouse2@uwyo.edu  |  Sun Lab Wyoming SBC

  Receives JSON programs from the NeuroHab Desktop over USB serial (9600 baud)
  and executes them step-by-step, mirroring the NeuroHab-MEGA block program
  architecture but using FED3 hardware (Adafruit Feather M0).

  ── PROTOCOL (identical to NeuroHab-MEGA) ────────────────────────────────
    {"cmd":"pause"}
    {"cmd":"resume"}
    {"cmd":"get_state"}
    {"cmd":"reset"}
    {"cmd":"load_program",
     "name":"My Program",
     "setup":[<step>, ...],
     "loop":[<step>, ...]}

  load_program must be sent while paused. After loading, send resume.

  ── STEP TYPES ───────────────────────────────────────────────────────────
  {"type":"feed"}
    Calls fed3.Feed() — dispenses one pellet.

  {"type":"conditioned_stimulus"}
    Calls fed3.ConditionedStimulus('G') — tone + lights cue.

  {"type":"pixel_on",  "pixel":"left"|"right"|"all", "r":5, "g":5, "b":5}
    Turns on NeoPixel(s) inside the nosepoke(s).

  {"type":"pixel_off", "pixel":"left"|"right"|"all"}
    Turns off NeoPixel(s).

  {"type":"wait",    "ms":500}
    Blocking delay.

  {"type":"timeout", "s":5}
    Equivalent to fed3.Timeout(s).

  {"type":"wait_left_poke",  "timeout_ms":0}
    Blocks until fed3.Left is true (nosepoke detected), then logs it.
    timeout_ms=0 means wait forever.

  {"type":"wait_right_poke", "timeout_ms":0}
    Blocks until fed3.Right is true, then logs it.

  {"type":"if_left_poke",  "steps":[...]}
    Non-blocking check. If fed3.Left right now → log poke + run inner steps.

  {"type":"if_right_poke", "steps":[...]}
    Non-blocking check. If fed3.Right right now → log poke + run inner steps.

  {"type":"set_fr", "val":3}
    Sets fed3.FR (fixed ratio requirement).

  {"type":"increment_poke_count"}
    Increments the internal poke counter (poke_num++).

  {"type":"reset_poke_count"}
    Resets the internal poke counter to 0.

  {"type":"log_msg", "msg":"checkpoint"}
    Prints [LOG] message to serial.

  ── SETUP / LOOP ARCHITECTURE ────────────────────────────────────────────
  Because the Feather M0 disconnects from USB on a software reset, we avoid
  issuing resets mid-session. Instead, send & run works as:
    1. pause  → host sends {"cmd":"pause"}
    2. load   → host sends {"cmd":"load_program", ...}
    3. resume → host sends {"cmd":"resume"}

  A "fake setup" is implemented inside loop(): the first time loop() runs
  after a resume with a fresh program, ranSetup is false, so the setup steps
  execute once before loop steps begin repeating. ranSetup is reset to false
  each time load_program is received, so the next resume will re-run setup.

  ── MEMORY BUDGET ────────────────────────────────────────────────────────
  Feather M0 (SAMD21) has 32 KB RAM — much more headroom than ATmega2560.
  sizeof(Step) ≈ 28 bytes.
    Outer arrays:  MAX_STEPS(30) × 2 × 28 B =  1680 B
    if pool:       IF_POOL_SIZE(20) × 28 B   =   560 B
  Leaves ~30 KB for FED3 library + stack.
*/

#include <FED3.h>

String sketch = "BlockProg";
FED3 fed3(sketch);

/* ── CONSTANTS ───────────────────────────────────────────────────────────── */
#define MAX_STEPS    30
#define MAX_IF_STEPS 10
#define IF_POOL_SIZE 20

/* ── STEP STRUCT ─────────────────────────────────────────────────────────── */
enum StepType : uint8_t {
  ST_FEED = 0,
  ST_COND_STIM,
  ST_PIXEL_ON,
  ST_PIXEL_OFF,
  ST_WAIT,
  ST_TIMEOUT,
  ST_WAIT_LEFT_POKE,
  ST_WAIT_RIGHT_POKE,
  ST_IF_LEFT_POKE,
  ST_IF_RIGHT_POKE,
  ST_SET_FR,
  ST_INCREMENT_POKE,
  ST_RESET_POKE,
  ST_LOG_MSG,
  ST_UNKNOWN
};

// pixel target bitmask
#define PIX_LEFT  0x01
#define PIX_RIGHT 0x02
#define PIX_ALL   0x03

struct Step {
  StepType type;
  uint8_t  pixel;       // PIX_LEFT / PIX_RIGHT / PIX_ALL
  uint32_t ms;          // wait ms, timeout_ms
  uint32_t s;           // timeout seconds
  int      fr_val;      // for set_fr
  uint8_t  r, g, b;    // pixel color
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
bool isRunning = false;
bool ranSetup  = false;   // true once setup steps have run for the current program
int  poke_num  = 0;       // internal poke counter (for FR tracking by user programs)

/* ── NON-BLOCKING SERIAL BUFFER ──────────────────────────────────────────── */
void handleSerialCommand(const String& json);

#define SERIAL_BUF_SIZE 2048
static char  serialBuf[SERIAL_BUF_SIZE];
static int   serialBufLen   = 0;
static bool  serialOverflow = false;

void checkSerial() {
  if (!Serial.available()) return;
  delay(20);  // let rest of message arrive at 9600 baud
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (serialOverflow) {
        Serial.println(F("[ERR] Serial overflow — command dropped"));
        serialOverflow = false;
      } else if (serialBufLen > 0) {
        serialBuf[serialBufLen] = '\0';
        handleSerialCommand(String(serialBuf));
      }
      serialBufLen   = 0;
      serialOverflow = false;
      continue;
    }
    if (serialOverflow) continue;
    if (serialBufLen >= SERIAL_BUF_SIZE - 1) {
      serialOverflow = true;
      serialBufLen   = 0;
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
  String v = jsonStr(json, key); return v.length() ? v.toFloat() : def;
}
long jsonLong(const String& json, const char* key, long def = 0) {
  String v = jsonStr(json, key); return v.length() ? v.toInt() : def;
}

uint8_t parsePixel(const String& s) {
  if (s == F("left"))  return PIX_LEFT;
  if (s == F("right")) return PIX_RIGHT;
  return PIX_ALL;
}

StepType parseType(const String& t) {
  if (t == F("feed"))                  return ST_FEED;
  if (t == F("conditioned_stimulus"))  return ST_COND_STIM;
  if (t == F("pixel_on"))              return ST_PIXEL_ON;
  if (t == F("pixel_off"))             return ST_PIXEL_OFF;
  if (t == F("wait"))                  return ST_WAIT;
  if (t == F("timeout"))               return ST_TIMEOUT;
  if (t == F("wait_left_poke"))        return ST_WAIT_LEFT_POKE;
  if (t == F("wait_right_poke"))       return ST_WAIT_RIGHT_POKE;
  if (t == F("if_left_poke"))          return ST_IF_LEFT_POKE;
  if (t == F("if_right_poke"))         return ST_IF_RIGHT_POKE;
  if (t == F("set_fr"))                return ST_SET_FR;
  if (t == F("increment_poke_count"))  return ST_INCREMENT_POKE;
  if (t == F("reset_poke_count"))      return ST_RESET_POKE;
  if (t == F("log_msg"))               return ST_LOG_MSG;
  return ST_UNKNOWN;
}

/* ── STEP EXECUTOR ───────────────────────────────────────────────────────── */
void execStep(const Step& s);  // forward declaration

void execStep(const Step& s) {
  switch (s.type) {

    case ST_FEED:
      fed3.Feed();
      checkSerial();
      return;

    case ST_COND_STIM:
      fed3.ConditionedStimulus('G');
      checkSerial();
      return;

    case ST_PIXEL_ON:
      // FED3.1 nosepoke pixels — if you have FED3 (no nosepoke LEDs),
      // swap these for fed3.leftPixel / fed3.rightPixel calls.
      if (s.pixel & PIX_LEFT)  fed3.leftPokePixel(s.r, s.g, s.b, 0);
      if (s.pixel & PIX_RIGHT) fed3.rightPokePixel(s.r, s.g, s.b, 0);
      return;

    case ST_PIXEL_OFF:
      if (s.pixel & PIX_LEFT)  fed3.leftPokePixel(0, 0, 0, 0);
      if (s.pixel & PIX_RIGHT) fed3.rightPokePixel(0, 0, 0, 0);
      return;

    case ST_WAIT:
      delay(s.ms);
      checkSerial();
      return;

    case ST_TIMEOUT:
      fed3.Timeout((int)s.s);
      checkSerial();
      return;

    case ST_WAIT_LEFT_POKE: {
      uint32_t start = millis();
      while (true) {
        fed3.run();
        checkSerial();
        if (!isRunning) return;
        if (fed3.Left) {
          fed3.logLeftPoke();
          return;
        }
        if (s.ms > 0 && millis() - start >= s.ms) return;
      }
    }

    case ST_WAIT_RIGHT_POKE: {
      uint32_t start = millis();
      while (true) {
        fed3.run();
        checkSerial();
        if (!isRunning) return;
        if (fed3.Right) {
          fed3.logRightPoke();
          return;
        }
        if (s.ms > 0 && millis() - start >= s.ms) return;
      }
    }

    case ST_IF_LEFT_POKE:
      fed3.run();
      if (fed3.Left) {
        fed3.logLeftPoke();
        for (uint8_t i = 0; i < s.ifStepCount; i++) {
          if (!isRunning) return;
          checkSerial();
          execStep(ifStepPool[s.ifStepStart + i]);
        }
      }
      return;

    case ST_IF_RIGHT_POKE:
      fed3.run();
      if (fed3.Right) {
        fed3.logRightPoke();
        for (uint8_t i = 0; i < s.ifStepCount; i++) {
          if (!isRunning) return;
          checkSerial();
          execStep(ifStepPool[s.ifStepStart + i]);
        }
      }
      return;

    case ST_SET_FR:
      fed3.FR = s.fr_val;
      return;

    case ST_INCREMENT_POKE:
      poke_num++;
      return;

    case ST_RESET_POKE:
      poke_num = 0;
      return;

    case ST_LOG_MSG:
      Serial.print(F("[LOG] "));
      Serial.println(s.msg);
      return;

    default: return;
  }
}

/* ── STEP VECTOR EXECUTION ───────────────────────────────────────────────── */
void runSteps(Step* steps, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    if (!isRunning) return;
    fed3.run();
    checkSerial();
    execStep(steps[i]);
  }
}

/* ── PARSER ──────────────────────────────────────────────────────────────── */
bool parseOneStep(const String& blk, Step& step, bool innerOnly) {
  memset(&step, 0, sizeof(Step));
  step.type = parseType(jsonStr(blk, "type"));
  if (step.type == ST_UNKNOWN) return false;
  if (innerOnly && (step.type == ST_IF_LEFT_POKE || step.type == ST_IF_RIGHT_POKE)) {
    Serial.println(F("[WARN] Nested if_lick skipped"));
    return false;
  }

  switch (step.type) {
    case ST_PIXEL_ON:
      step.pixel = parsePixel(jsonStr(blk, "pixel"));
      step.r = (uint8_t)jsonLong(blk, "r", 5);
      step.g = (uint8_t)jsonLong(blk, "g", 5);
      step.b = (uint8_t)jsonLong(blk, "b", 5);
      break;
    case ST_PIXEL_OFF:
      step.pixel = parsePixel(jsonStr(blk, "pixel"));
      break;
    case ST_WAIT:
      step.ms = (uint32_t)jsonLong(blk, "ms", 500);
      break;
    case ST_TIMEOUT:
      step.s = (uint32_t)jsonLong(blk, "s", 5);
      break;
    case ST_WAIT_LEFT_POKE:
    case ST_WAIT_RIGHT_POKE:
      step.ms = (uint32_t)jsonLong(blk, "timeout_ms", 0);
      break;
    case ST_SET_FR:
      step.fr_val = (int)jsonLong(blk, "val", 1);
      break;
    case ST_LOG_MSG: {
      String m = jsonStr(blk, "msg");
      m.toCharArray(step.msg, sizeof(step.msg));
      break;
    }
    case ST_IF_LEFT_POKE:
    case ST_IF_RIGHT_POKE:
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

    String blk  = json.substring(bStart, bEnd + 1);
    Step&  step = out[count];

    if (!parseOneStep(blk, step, innerOnly)) { pos = bEnd + 1; continue; }

    // Parse inner steps array for container blocks
    if (step.type == ST_IF_LEFT_POKE || step.type == ST_IF_RIGHT_POKE) {
      step.ifStepStart = ifPoolUsed;
      step.ifStepCount = 0;

      int sk = blk.indexOf(F("\"steps\""));
      if (sk != -1) {
        int sb = blk.indexOf('[', sk);
        int se = -1, d2 = 0, p2 = sb;
        while (p2 < (int)blk.length()) {
          if      (blk[p2] == '[') d2++;
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
      Serial.print(F("[PARSE] if_poke inner="));
      Serial.println(step.ifStepCount);
    }

    count++;
    pos = bEnd + 1;
  }
}

void loadProgram(const String& json) {
  setupLen = loopLen = ifPoolUsed = 0;
  ranSetup  = false;   // reset so setup steps re-run on next resume
  poke_num  = 0;

  // Use depth-tracking bracket scan so nested arrays (e.g. if_left_poke "steps":[...])
  // don't fool indexOf(']') into stopping at the wrong closing bracket.
  auto findArrayEnd = [](const String& s, int openBracket) -> int {
    int depth = 0, p = openBracket;
    while (p < (int)s.length()) {
      if      (s[p] == '[') depth++;
      else if (s[p] == ']') { depth--; if (depth == 0) return p; }
      p++;
    }
    return -1;
  };

  int sk = json.indexOf(F("\"setup\""));
  if (sk != -1) {
    int sb = json.indexOf('[', sk);
    int se = (sb != -1) ? findArrayEnd(json, sb) : -1;
    if (sb != -1 && se != -1)
      parseStepArray(json, sb, se, setupSteps, setupLen, MAX_STEPS, false);
  }

  int lk = json.indexOf(F("\"loop\""));
  if (lk != -1) {
    int lb = json.indexOf('[', lk);
    int le = (lb != -1) ? findArrayEnd(json, lb) : -1;
    if (lb != -1 && le != -1)
      parseStepArray(json, lb, le, loopSteps, loopLen, MAX_STEPS, false);
  }

  Serial.print(F("[OK] Loaded: "));
  Serial.print(setupLen);  Serial.print(F("su "));
  Serial.print(loopLen);   Serial.print(F("lp "));
  Serial.print(ifPoolUsed); Serial.println(F("if"));
}

/* ── SERIAL COMMAND HANDLER ──────────────────────────────────────────────── */
void pause()    { isRunning = false; Serial.println(F("[PAUSE RECEIVED]")); }
void resume()   { isRunning = true;  Serial.println(F("[START RECEIVED]")); }
void getState() {
  Serial.println(isRunning ? F("[RUNNING]") : F("[STOPPED]"));
  Serial.print(F("[STEPS] su=")); Serial.print(setupLen);
  Serial.print(F(" lp="));       Serial.print(loopLen);
  Serial.print(F(" if="));       Serial.println(ifPoolUsed);
}
void resetDevice() {
  Serial.println(F("[RESET]"));
  Serial.flush();
  NVIC_SystemReset();   // SAMD21 software reset (Feather M0)
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

/* ── SETUP ───────────────────────────────────────────────────────────────── */
void setup() {
  Serial.begin(9600);
  delay(200);
  fed3.begin();
  Serial.println(F("[BOOT] FED3 block program ready"));
  Serial.println(F("[STOPPED]"));
}

/* ── LOOP ────────────────────────────────────────────────────────────────── */
void loop() {
  checkSerial();
  fed3.run();

  if (!isRunning) return;

  // Fake setup: run setup steps once per program load before looping
  if (!ranSetup) {
    if (setupLen > 0) {
      Serial.println(F("[SETUP] Running..."));
      runSteps(setupSteps, setupLen);
      Serial.println(F("[SETUP] Done"));
    }
    ranSetup = true;
    Serial.println(F("[LOOP] Running..."));
  }

  runSteps(loopSteps, loopLen);
}
