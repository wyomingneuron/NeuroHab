/*c:\Users\wjohnst6\Documents\Arduino\libraries\FED3\src\FED3.cpp
  Feeding experimentation device 3 (FED3)
  Classic FED3 script
  This script mimicks the classic FED3 menuing system for selecting among the following programs

  // FEDmodes:
  // 0 Free feeding
  // 1 FR1
  // 2 FR3
  // 3 FR5
  // 4 Progressive Ratio
  // 5 Extinction
  // 6 Light tracking FR1 task
  // 7 FR1 (reversed)
  // 8 PR (reversed)
  // 9 Optogenetic stimulation
  // 10 Optogenetic stimulation (reversed)
  // 11 Timed free feeding

  alexxai@wustl.edu
  December, 2020

  This project is released under the terms of the Creative Commons - Attribution - ShareAlike 3.0 license:
  human readable: https://creativecommons.org/licenses/by-sa/3.0/
  legal wording: https://creativecommons.org/licenses/by-sa/3.0/legalcode
  Copyright (c) 2020 Lex Kravitz

*/

#include <FED3.h>           //Include the FED3 library
String sketch = "Classic";  //Unique identifier text for each sketch
FED3 fed3(sketch);          //Start the FED3 object

/* ===========================================================================
   MODE -> PARADIGM
     0  New_Barbosa (NB)       6  Bellagio-48h
     1  DualPortTimed-Vegas    7  FR-Split-48h
     2  2x2Timed               8  RPR-Split-48h
     3  5x5TIMED               9  Medusa-48h
     4  ProgRatioTIMED        10  Sparrow-48h
     5  Dual_Port_Feeding     1
   =========================================================================== */

//////////////////////////
//variables for RPR tasks
int poke_num = 0;        // MODE 4 (ProgRatioTIMED) only
int pokes_required = 0;  // MODE 4 (ProgRatioTIMED) only
int left_pokes = 0;      // MODE 2 (2x2Timed) + MODE 3 (5x5TIMED)
int right_pokes = 5;     // MODE 2 (2x2Timed) + MODE 3 (5x5TIMED)
int upperbound = 11;     // MODE 4 (ProgRatioTIMED) only
///////////////////////////////////////////////////////
// Session timer variables (added for timer-based idle)
unsigned long sessionStart = 0;
const unsigned long SESSION_DURATION = 172800000;
// ^ IDLE LATCH: MODE 1 (Vegas), 2 (2x2Timed), 3 (5x5TIMED), 4 (ProgRatioTIMED),
//   9 (Medusa), 10 (Sparrow).  NOT 6 (Bellagio), 7 (FR-Split), 8 (RPR-Split).
bool isIdle = false;
bool isIdleLatch = false;
bool isProgRatio = false;  // MODE 4 (ProgRatioTIMED) only
////////////////////////////////////////////////////
// Variables for Double Poke and Double Poke Hold
float ran_range = 0;           // DEAD -- was MODE 10 (DP_Hold), replaced by Sparrow
float r1 = 1.0f;               // DEAD
float r2 = 3.0f;               // DEAD
unsigned long time_start = 0;  // DEAD
int poke_count = 0;            // DEAD

// NEW Bellagio /////////////////////////////////////////////////////////////////////////

unsigned long millis_start = 0;
const unsigned long timeout_time = 172800000;
// ^ MODE 6 (Bellagio), MODE 7 (FR-Split), MODE 8 (RPR-Split)

// NEW VEGAS FR ////////////////////////////////////////////////////////
// *** SHARED: MODE 6 (Bellagio, left port) AND MODE 7 (FR-Split, both ports) ***
int FR_init_count_right = 0;
int FR_init_limit_right = 0;  // MODE 7 (FR-Split) + MODE 11 (Bellagio-R)
int FR_init_count_left = 0;
int FR_init_limit_left = 0;  // MODE 6 (Bellagio) + MODE 7 (FR-Split)

int FR_req_right = 5;    // CHANGED 1->4. MODE 7 (FR-Split) + MODE 11 (Bellagio-R)
int FR_count_right = 0;  // MODE 7 (FR-Split) + MODE 11 (Bellagio-R)
int FR_req_left = 1;     // CHANGED 5->4. MODE 6 (Bellagio) + MODE 7 (FR-Split)
int FR_count_left = 0;   // MODE 6 (Bellagio) + MODE 7 (FR-Split)

unsigned long FR_timer_right = 0;   // MODE 7 (FR-Split)
unsigned long FR_timer_left = 0;    // MODE 6 (Bellagio) + MODE 7 (FR-Split)
int FR_timeout_ms_perpoke = 15000;  // CHANGED 5000->15000
                                    // MODE 6 (Bellagio) + MODE 7 (FR-Split)
                                    // measured median inter-poke interval 6.9s

// NEW VEGAS RPR ///////////////////////////////////////////////////////
// *** SHARED: MODE 6 (Bellagio, right port) AND MODE 8 (RPR-Split, both ports) ***
int RPR_init_count_right = 0;
int RPR_init_limit_right = 5;  // MODE 6 (Bellagio) + MODE 8 (RPR-Split)
int RPR_init_count_left = 0;
int RPR_init_limit_left = 5;  // MODE 8 (RPR-Split) only

int RPR_count_right = 0;  // MODE 6 (Bellagio) + MODE 8 (RPR-Split)
int RPR_count_left = 0;   // MODE 8 (RPR-Split) only
int RPR_req_right = 0;    // MODE 6 (Bellagio) + MODE 8 (RPR-Split)
int RPR_req_left = 0;     // MODE 8 (RPR-Split) only

int RPR_lower_bound_right = 1;  // UNCHANGED. MODE 6 (Bellagio) + MODE 8 (RPR-Split)
int RPR_upper_bound_right = 7;  // 1-7, mean 4 -- what FR4 now matches
int RPR_lower_bound_left = 1;   // MODE 8 (RPR-Split) only
int RPR_upper_bound_left = 7;   // MODE 8 (RPR-Split) only

unsigned long RPR_timer_right = 0;  // MODE 6 (Bellagio) + MODE 8 (RPR-Split)
unsigned long RPR_timer_left = 0;   // MODE 8 (RPR-Split) only
int RPR_timeout_ms = 40000;         // MODE 6 (Bellagio) + MODE 8 (RPR-Split)

// NEW MEDUSA RPR ///////////////////////////////////////////////////////
// *** MODE 9 (Medusa-48h) ONLY -- safe to edit in isolation ***
int MED_init_count_right = 0;
int MED_init_limit_right = 5;
int MED_init_count_left = 0;
int MED_init_limit_left = 5;

int MED_count_right = 0;
int MED_count_left = 0;
int MED_req_right = 0;
int MED_req_left = 0;

int MED_lower_bound_right = 1;  // CHANGED 6->1. Right = WIDE 1-7, mean 4
int MED_upper_bound_right = 7;  // CHANGED 10->7
int MED_lower_bound_left = 3;   // CHANGED 1->3. Left = NARROW 3-5, mean 4
int MED_upper_bound_left = 5;

unsigned long MED_timer_right = 0;
unsigned long MED_timer_left = 0;
int MED_timeout_ms = 40000;

// ============================================================================
//  SPARROW  ---  dual-port random progressive ratio with change-over cost
//  *** MODE 10 (Sparrow-48h) ONLY -- fully isolated ***
//
//    Left   : RPR uniform 2-5
//    Right  : 70% RPR 1-3, 30% RPR 6-9.  4 and 5 can never be drawn.
//    Window : scales linearly with the draw, 6 s per required poke
//    Change-over cost : poking one port forfeits any bout at the other and
//                       rerolls that port's requirement
//    Omission : ~10% of completions fire the cue with no pellet
// ============================================================================

// ---- init phase: FR1, one pellet, identical on both ports ----
int SPR_init_count_left = 0;
int SPR_init_limit_left = 0;
int SPR_init_count_right = 0;
int SPR_init_limit_right = 0;

// ---- current bout state ----
int SPR_count_left = 0;  // pokes so far this bout
int SPR_count_right = 0;
int SPR_req_left = 0;  // pokes required this bout
int SPR_req_right = 0;

// ---- left port: uniform draw, inclusive bounds ----
int SPR_lower_bound_left = 2;
int SPR_upper_bound_left = 5;

// ---- right port: two branches picked by probability ----
int SPR_short_pct_right = 70;  // 50 for imaging: doubles dead-zone entries
int SPR_short_lo_right = 1;
int SPR_short_hi_right = 3;
int SPR_long_lo_right = 6;
int SPR_long_hi_right = 9;

// ---- bout timers ----
unsigned long SPR_timer_left = 0;
unsigned long SPR_timer_right = 0;

// ---- bout window, scaled to the drawn requirement ----
// 6000 ms per required poke:  req 2 -> 12 s, req 3 -> 18 s, req 9 -> 54 s.
// RAISE SPR_ms_per_poke TO ~20000 DURING SHAPING so the window rarely fires.
unsigned long SPR_ms_per_poke = 6000;
unsigned long SPR_window_floor = 8000;  // req 1 is moot; this is its floor
unsigned long SPR_window_left = 0;
unsigned long SPR_window_right = 0;

// ---- omission trials ----
// Cue fires, pellet withheld. The counter rides the unused FR_Count_* columns
// so no new logging is needed. It only ever increments.
int SPR_omit_pct = 10;  // 25 for 30-min imaging sessions
int SPR_omit_count_left = 0;
int SPR_omit_count_right = 0;

// ---- redraw flags ----
// Set true wherever a bout ends. The draw itself happens once, at the top of
// the mode block, on the next pass.
bool SPR_redraw_left = true;
bool SPR_redraw_right = true;


// Barbosa Globals ///////////////////////////////////////////////////////////////////////////////
// (unchanged from the version already in your sketch -- reproduced here so
//  this file is self-contained. Nothing was added or removed.)

// ---- session ----
unsigned long BRB_session_start = 0;
const unsigned long BRB_session_ms = 259200000;  // 48 h , 72 h*, 96
bool BRB_session_over = false;

// ---- period clock ----
unsigned long BRB_wait_ms = 600000;    // 10 min dark / passive-logging gap 600000
unsigned long BRB_period_ms = 600000;  // 10 min feeding period              600000 | 5 min - 300 sec - 300000 ms 
unsigned long BRB_clock = 0;          // marks the start of the current phase
bool BRB_in_period = false;
int BRB_period_num = 0;   // periods elapsed this session
int BRB_period_init = 8;  // free pellet at onset of first N periods

// ---- trial ----
int BRB_req_left = 0;          // pokes required this trial
int BRB_count_left = 0;        // pokes so far this trial; also the phase
int BRB_lower_bound_left = 1;  // inclusive
int BRB_upper_bound_left = 4;  // inclusive
bool BRB_redraw_left = true;

int BRB_req_right = 1;          // Number of right pokes on switch required
int BRB_count_right = 0;        

// one timer, two meanings, selected by the count
unsigned long BRB_timer = 0;
unsigned long BRB_left_window_ms = 30000;   // RAISE TO ~30000 FOR SHAPING
unsigned long BRB_switch_window_ms = 20000;  // RAISE TO ~15000 FOR SHAPING

// ADDED: production (full Barbosa after TrainingComplete)
// 10 s left timeout, 10 s switch timeout
const unsigned long BRB_prod_left_window_ms   = 10000;
const unsigned long BRB_prod_switch_window_ms = 10000;

// ADDED: 10 min active every 30 min cycle → 10 min on / 20 min dark
const unsigned long BRB_prod_period_ms = 600000;   // 10 min active
const unsigned long BRB_prod_wait_ms   = 1200000;  // 20 min gap (10+20=30)

// ---- omission trials ----
int BRB_omit_pct = 0;
int BRB_omit_count = 0;

// ---- bookkeeping (rides FR_Count_Left, only ever increments) ----
int BRB_trial_count = 0;  // completed trials (pellet or omission)
int BRB_fail_count = 0;

// ===================== Barbosa training/shaping additions =====================
// Training uses 10 completed 5-min active periods at each fixed LP stage.
bool BRB_training = true;
bool BRB_training_complete = false;
int BRB_stage = 1;
const int BRB_max_stage = 3;
int BRB_train_period_num = 0;

// ---- current active-period accounting ----
int BRB_period_attempts = 0;
int BRB_period_successes = 0;
int BRB_period_failures = 0;
int BRB_period_resets = 0;
int BRB_period_rightEarly = 0;
int BRB_period_leftTimeout = 0;
int BRB_period_switchTimeout = 0;

// ---- current 10-period stage accounting ----
int BRB_stage_attempts = 0;
int BRB_stage_successes = 0;
int BRB_stage_failures = 0;
int BRB_stage_resets = 0;
int BRB_stage_rightEarly = 0;
int BRB_stage_leftTimeout = 0;
int BRB_stage_switchTimeout = 0;

// ---- stage-advancement criteria ----
// These are isolated here so the training rule can be changed without
// changing any of the existing Barbosa trial logic.
const float BRB_stage_min_success_rate = 0.50f;
const int BRB_stage_min_successes = 20; //shaping
const int BRB_stage_max_failures = 30;
const int BRB_stage_max_resets = 30;
// ============================================================================


// ------------------------------------------------------- TRIAL RESET HELPER
void BRB_resetTrial() {
  BRB_count_left = 0;
  BRB_count_right = 0;
  fed3.RPR_Count_Left = BRB_count_left;
  BRB_timer = 0;
  BRB_redraw_left = true;

  fed3.leftPokePixel(0, 10, 0, 0);  // left active
  fed3.rightPokePixel(0, 0, 0, 0);  // right inactive
}

// ------------------------------------------------------- TRAINING HELPERS
void BRB_resetStageCounters() {
  BRB_train_period_num = 0;

  BRB_stage_attempts = 0;
  BRB_stage_successes = 0;
  BRB_stage_failures = 0;
  BRB_stage_resets = 0;

  BRB_stage_rightEarly = 0;
  BRB_stage_leftTimeout = 0;
  BRB_stage_switchTimeout = 0;
}

bool BRB_stagePasses() {
  if (BRB_train_period_num < 10) return false;
  if (BRB_stage_attempts <= 0) return false;

  float BRB_stage_success_rate =
      (float)BRB_stage_successes / (float)BRB_stage_attempts;

  return (BRB_stage_successes >= BRB_stage_min_successes &&
          BRB_stage_success_rate >= BRB_stage_min_success_rate &&
          BRB_stage_failures <= BRB_stage_max_failures &&
          BRB_stage_resets <= BRB_stage_max_resets);
}

void BRB_logTrainingPeriodSummary() {
  int BRB_period_rate_pct = 0;
  if (BRB_period_attempts > 0) {
    BRB_period_rate_pct =
        (100 * BRB_period_successes) / BRB_period_attempts;
  }

  String BRB_event = "TrainP";
  BRB_event += String(BRB_train_period_num);
  BRB_event += "_S";
  BRB_event += String(BRB_stage);
  BRB_event += "_A";
  BRB_event += String(BRB_period_attempts);
  BRB_event += "_Pel";
  BRB_event += String(BRB_period_successes);
  BRB_event += "_Rate";
  BRB_event += String(BRB_period_rate_pct);
  BRB_event += "_F";
  BRB_event += String(BRB_period_failures);
  BRB_event += "_R";
  BRB_event += String(BRB_period_resets);
  BRB_event += "_RE";
  BRB_event += String(BRB_period_rightEarly);
  BRB_event += "_LT";
  BRB_event += String(BRB_period_leftTimeout);
  BRB_event += "_ST";
  BRB_event += String(BRB_period_switchTimeout);

  fed3.setEvent(BRB_event);
  fed3.logSpecialEvent();
}

void BRB_logTrainingStageSummary(bool BRB_passed) {
  int BRB_stage_rate_pct = 0;
  if (BRB_stage_attempts > 0) {
    BRB_stage_rate_pct =
        (100 * BRB_stage_successes) / BRB_stage_attempts;
  }

  String BRB_event = "TrainStage";
  BRB_event += String(BRB_stage);
  BRB_event += BRB_passed ? "_Pass" : "_Repeat";
  BRB_event += "_P";
  BRB_event += String(BRB_train_period_num);
  BRB_event += "_A";
  BRB_event += String(BRB_stage_attempts);
  BRB_event += "_Pel";
  BRB_event += String(BRB_stage_successes);
  BRB_event += "_Rate";
  BRB_event += String(BRB_stage_rate_pct);
  BRB_event += "_F";
  BRB_event += String(BRB_stage_failures);
  BRB_event += "_R";
  BRB_event += String(BRB_stage_resets);
  BRB_event += "_RE";
  BRB_event += String(BRB_stage_rightEarly);
  BRB_event += "_LT";
  BRB_event += String(BRB_stage_leftTimeout);
  BRB_event += "_ST";
  BRB_event += String(BRB_stage_switchTimeout);

  fed3.setEvent(BRB_event);
  fed3.logSpecialEvent();
}

// ------------------------------------------------------- LCD helper
void BRB_updateScreen() {
  if (BRB_training) {
    fed3.sessiontype = "BRB-S";
    fed3.sessiontype += String(BRB_stage);
    fed3.sessiontype += "  ";          // pad to 8 chars for clean overwrite
  } else {
    fed3.sessiontype = "Barbosa ";     // or "Barbosa-48h" if you prefer
  }
  fed3.UpdateDisplay();
}
// Barbosa Globals End ////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// =================================================================================================
//  NEW BARBOSA  (Mode 0)  ---  two cued counting rules, late light, variable delay
//  *** MODE 0 ONLY. Every name that starts with NB_ belongs to Mode 0 and is used nowhere else.
//      Mode 0 reads no other mode's globals, so nothing in this block can change Modes 1-11. ***
//
//  WHAT THE MOUSE DOES (one attempt)
//    1. A rule is drawn (A, B, or C). Its sound plays once and the left port lights.
//    2. The mouse pokes LEFT exactly N times (Rule A: N = 1, Rule C: N = 2, Rule B: N = 3).
//       Nothing changes at N. Rule C (N = 2) is a training bridge only (Stage 2).
//    3. The mouse stops. After EVERY left poke the FED waits a delay D (2.5-3.5 s, new each attempt):
//         D runs out on exactly N pokes        -> GO tone, left light off, right light on.
//         D runs out on fewer than N pokes     -> UNDER-COUNT error (the mouse stopped short).
//       (Early shaping steps in Stages 2-3 shorten the go delay and lengthen the pause; see 4b.)
//         a left poke that makes the count N+1 -> OVER-COUNT error, at once (also during the delay).
//    4. Right poke within the switch window    -> reward tone + pellet, then a new rule is drawn.
//         right poke before GO                 -> RIGHT-EARLY error.
//         no right poke within the window      -> SWITCH-TIMEOUT error.
//    5. Any error: no pellet, both ports dark for the error timeout (30 s in training, 5 s in New
//       Barbosa; pokes logged, not counted), then the SAME
//       rule and N again with no cue: a "correction" attempt.
//
//  TRAINING (runs until Stage 4 is passed; no time cap). Each stage has its own sound.
//    Stage 1: Rule A only (N1, rising chirp).
//    Stage 2: Rule C only (N2, warble)         <- bridge: one more poke than N1, its own sound so the
//                                                 Rule B sound only ever means 3.
//    Stage 3: Rule B only (N3, flutter).
//    Stage 4: A or B (N1 or N3), 50/50 after every pellet. Rule C is not used again.
//    SHAPING STEPS (Stages 2 and 3, section 4b), 4 steps each. Two things start easy and tighten:
//      go delay  (after the Nth poke):          0.5 s, 0.5 s, 1.5 s, D
//      pause allowed before UNDER-COUNT:        10 s,  5 s,   D,     D
//    Step 4 is the real task. Each step passes with the same rule as a stage; passing step 4 passes
//    the stage. Screen: NB-S2.1 ... NB-S2.4, NB-S3.1 ... NB-S3.4.
//    ERROR TIMEOUT: 30 s in all training stages (1-4), 5 s in New Barbosa (section 4).
//    Pass rule, checked when each window closes: the last two blocks of 30 SCORED attempts are each
//    >= 60% rewarded. Stages 1-3 score every attempt. Stage 4 needs BOTH rules to pass and scores
//    only the first attempt after each cue (correction attempts never count).
//  NEW BARBOSA (after training): same trial; 10 min window / 20 min dark for 72 h, then idle.
//    An imaging session = one window.
//
//  LOG COLUMNS (same CSV as every mode, no library change)
//    FR                 = delay D (ms) of the current attempt; on a "Go" row, the delay actually
//                         used before the go tone (= D, or the step's go delay in a go-delay step)
//    Block_Pellet_Count = rule of the current attempt: 1 = A, 2 = B, 3 = C (0 = none: dark, free pellet)
//    RPR_Left           = N for the current rule draw
//    RPR_Count_Left     = left pokes so far in this attempt (a "Left" row already includes that poke;
//                         note Mode 5 logs the count BEFORE the poke, this mode logs it AFTER)
//    RPR_Right          = 1 if this attempt counts toward the pass rule, 0 if it is a correction
//    FR_Count_Left      = stage: 1, 2, 3, 4 = training, 5 = New Barbosa
//    FR_Count_Right     = window number since power-on
//    RPR_Count_Right    = go-delay step (1, 2, 3 ...) in Stages 2-3 with steps on; 0 otherwise
//  EVENTS written by this mode
//    NB_Boot_... (settings in force), PeriodStart, RuleA / RuleB / RuleC (rule drawn, cue plays), Go,
//    OverCount, UnderCount, RightEarly, SwitchTimeout, LeftTimeout (shaping option only), Retry
//    (ports re-lit after an error), Block_..., Win..._summary, StageCheck_..., StepAdvance_S#.#,
//    StageAdvance_S#,
//    TrainingComplete, PeriodEnd, Idle, CueCheck_... (test mode only).
//    The library itself writes Left, Right and Pellet rows (and its own with-pellet rows).
//
//  POWER: sleep is switched off in this mode (as in Mode 5) so the clock and the port lights run
//  continuously. Run on USB power.
// =================================================================================================


// ---------------------------------------------------------------------------- 1. BENCH TEST
// HOW TO BENCH-TEST (no animal, a finger in the ports):
//   1. Set NB_TEST_MODE = true (and NB_SERIAL_DEBUG = true if a computer is attached). Flash, pick Mode 0.
//   2. At power-on you hear Rule A (rising chirp), Rule C (warble), Rule B (flutter), the go
//      tone and the reward tone, 1 s apart.
//   3. After 15 s dark the first 1 min window opens: free pellet, the stage's cue, left port lit
//      (Stage 1: Rule A, poke once; Stage 2: Rule C, poke twice; Stage 3: Rule B, poke 3 times).
//      Poke left N times and stay out: 2.5-3.5 s later the go tone plays and the right port lights.
//      Poke right: pellet, then the next cue. Poke left N+1 times: OverCount, both ports dark for the
//      error timeout (30 s in training, 5 s in New Barbosa), Retry.
//   4. Two passing blocks of 3 attempts, then the window closing, advance the stage
//      (StageCheck_S1_Pass, StageAdvance_S2). Stage 2 needs 2 left pokes (warble), Stage 3
//      needs 3 (flutter), Stage 4 mixes A and B.
//   5. To start at any stage, set NB_START_STAGE (2, 3, 4, or 5 = New Barbosa; in test mode it
//      idles after 15 min) and, for Stages 2-3, NB_START_STEP. Change any timing in section 1b.
//      Shaping steps (section 4b): step 1 = go 0.5 s after the Nth poke, a pause up to 10 s
//      between pokes allowed; step 2 = 0.5 s / 5 s; step 3 = 1.5 s / D; step 4 = D / D (the real
//      task). The screen shows the step (e.g. tNB-S2.1).
//   6. Set NB_TEST_MODE = false before any animal runs. Test files show "t" in Session_type.
//
// NB_TEST_MODE = true swaps in the short timings in section 1b so every stage can be checked by hand
// in minutes. The screen and the Session_type column then start with "t" (e.g. "tNB-S1  "), so a
// test file can never be mistaken for animal data. NEVER run an animal with this on.
const bool NB_TEST_MODE = true;

// Stage the FED starts in at power-on: 1 = N1, 2 = N2, 3 = N3, 4 = N1/N3 mix (training),
// 5 = New Barbosa (training done).
// Use it to test any stage directly, or to put a mouse back where it was after a reboot
// (training progress is held in memory and is lost if the FED loses power). Keep 1 for a new mouse.
const int NB_START_STAGE = 2;

// Go-delay step the FED starts in when NB_START_STAGE is 2 or 3 and steps are on there (section 4b):
// 1 = first step. Ignored in any other stage (they start at step 1 when the mouse reaches them).
const int NB_START_STEP = 1;

// Prints every state change to the Serial Monitor (115200 baud) while a computer is attached by USB.
// It is skipped when no computer is listening, so it is safe to leave on. Most useful with test mode.
const bool NB_SERIAL_DEBUG = false;

// Test mode only: at power-on, play Rule A, Rule C, Rule B cues, Go tone and reward tone, 1 s apart,
// each with a CueCheck_ row. Use it with a sound meter at the port to set levels and pitches.
const bool NB_TEST_CUE_CHECK = true;

// ---------------------------------------------------------------------------- 1b. TEST TIMINGS
// Used ONLY when NB_TEST_MODE = true. The real values (sections 3 and 4) are in the comments.
// Trial timings are kept real by default so the trial can be tested by hand at true speed;
// shorten them here if you want faster checks.
const unsigned long NB_TEST_WINDOW_MS        = 60000;    // feeding window        (real 600000 = 10 min)
const unsigned long NB_TEST_DARK_TRAIN_MS    = 60000;    // dark gap, training    (real 600000 = 10 min)
const unsigned long NB_TEST_DARK_PROD_MS     = 20000;    // dark gap, New Barbosa (real 1200000 = 20 min)
const unsigned long NB_TEST_SESSION_MS       = 900000;   // New Barbosa run       (real 259200000 = 72 h)
const int           NB_TEST_FREE_PELLET_WINS = 1;        // free-pellet windows   (real 8)
const int           NB_TEST_BLOCK_SIZE       = 3;        // scored attempts/block (real 30)
const unsigned long NB_TEST_DELAY_MIN_MS     = 2500;     // delay D, shortest     (real 2500)
const unsigned long NB_TEST_DELAY_MAX_MS     = 3500;     // delay D, longest      (real 3500)
const unsigned long NB_TEST_SWITCH_TRAIN_MS  = 20000;    // switch window, train  (real 20000)
const unsigned long NB_TEST_SWITCH_PROD_MS   = 10000;    // switch window, NB     (real 10000)
const unsigned long NB_TEST_ERROR_TIMEOUT_MS = 5000;     // error timeout, NB     (real 5000)
const unsigned long NB_TEST_ERROR_TRAIN_MS   = 30000;    // error timeout, train  (real 30000)


// ---------------------------------------------------------------------------- 2. RULES
// N for each rule. One value = fixed N (the design: A = 1, C = 2, B = 3). Two or more values = N is
// drawn from the list each time that rule is drawn (N random within the rule), e.g. {3, 4}.
// Rule A: Stage 1, Stage 4, New Barbosa.   Rule B: Stage 3, Stage 4, New Barbosa.
// Rule C: Stage 2 only (the N2 bridge between N1 and N3).
const int NB_N_LIST_A[] = {1};
const int NB_N_LIST_B[] = {3};
const int NB_N_LIST_C[] = {2};
const int NB_N_LEN_A = sizeof(NB_N_LIST_A) / sizeof(NB_N_LIST_A[0]);  // list lengths, automatic
const int NB_N_LEN_B = sizeof(NB_N_LIST_B) / sizeof(NB_N_LIST_B[0]);
const int NB_N_LEN_C = sizeof(NB_N_LIST_C) / sizeof(NB_N_LIST_C[0]);

// Stage 4 and New Barbosa: chance (percent) that the next rule is B (N3); otherwise A (N1).
// 50 = 50/50. Repeats allowed. Keep it between 1 and 99: Stage 4 can only be passed when both
// rules come up.
const int NB_RULE_B_PCT = 50;

// Stage 4 and New Barbosa: longest allowed run of the same rule. 0 = no cap (pure random draws).
// e.g. 3 = after A, A, A the next rule is forced to B.
const int NB_MAX_SAME_RULE = 0;


// ---------------------------------------------------------------------------- 3. SESSION
const unsigned long NB_WINDOW_MS        = 600000;     // 10 min feeding window (training and New Barbosa)
const unsigned long NB_DARK_TRAIN_MS    = 600000;     // 10 min dark between training windows (as Mode 5)
const unsigned long NB_DARK_PROD_MS     = 1200000;    // 20 min dark between New Barbosa windows (30 min cycle)
const unsigned long NB_SESSION_MS       = 259200000;  // New Barbosa runs 72 h after training, then idles
const int           NB_FREE_PELLET_WINS = 8;          // training windows 1-8 open with a free pellet


// ---------------------------------------------------------------------------- 4. TRIAL
const unsigned long NB_DELAY_MIN_MS     = 2500;   // delay D: uniform between these (inclusive),
const unsigned long NB_DELAY_MAX_MS     = 3500;   //   a new draw for every attempt, logged in FR
const unsigned long NB_SWITCH_TRAIN_MS  = 20000;  // time allowed for the right poke after GO, training
const unsigned long NB_SWITCH_PROD_MS   = 10000;  // same, New Barbosa
const unsigned long NB_ERROR_TIMEOUT_MS = 5000;   // both ports dark after any error, New Barbosa
// Training stages 1-4: a longer timeout so errors cost food. The mouse earns all its food here and
// has more windows than it uses, so a 5 s timeout costs it almost nothing; 30 s cuts what it can
// earn at low accuracy (about 97 pellets/day at 26% correct, his S2 data) while >= 45% correct
// still feeds it fully. Weigh daily; set back to 5000 if body weight falls below 85% of baseline.
const unsigned long NB_ERROR_TIMEOUT_TRAIN_MS = 30000;

// true (the design): stopping short is an error. When D runs out with fewer than N pokes the attempt
// ends as UNDER-COUNT. This closes the "poke, wait for GO, poke again" strategy.
// false (shaping only): stopping short does nothing and the mouse may keep poking; an attempt left
// open longer than NB_LEFT_SAFETY_MS from its first poke then ends as LeftTimeout.
const bool          NB_UNDERCOUNT_IS_ERROR = true;
const unsigned long NB_LEFT_SAFETY_MS      = 30000;  // used only when NB_UNDERCOUNT_IS_ERROR = false

// ---------------------------------------------------------------------------- 4b. SHAPING STEPS
// Training only, Stages 2 (N2) and 3 (N3). Two parts of the trial start easy and tighten step by
// step to the real task:
//   GO DELAY: in the design the go tone comes D (2.5-3.5 s) after the last poke, so the poke that
//     makes the count right is rewarded only seconds later, and learning falls off steeply with that
//     delay. In early steps a correct count gets the go tone sooner.
//   PAUSE ALLOWED: in the design a pause of D with fewer than N pokes ends the attempt (UnderCount).
//     In S2 the mouse mostly poked once and paused longer than D; in early steps a longer pause is
//     allowed, so a slower next poke still counts and is rewarded.
//   - A poke past N before the go tone is still OverCount; pokes after the go tone are ignored.
//   - Stage 1, Stage 4 and New Barbosa never use steps: go and pause are always D.
// Each step passes with the section 5 pass rule; passing a step starts the next one with fresh
// blocks (StepAdvance_S2.2 ...), passing the last step passes the stage.
const bool NB_GO_FADE_S2 = true;   // steps in Stage 2 (N2)
const bool NB_GO_FADE_S3 = true;   // steps in Stage 3 (N3)
// One column per step, ms. 0 = the full delay D (the design). Both lists must be the same length;
// add or remove steps freely, and keep the last step 0 / 0 so the stage ends on the real task.
// Real go time is about 0.16 s later than the "Left" row (the library logs a poke when the nose
// leaves and the SD write takes that long).
//                                     step:  1      2      3     4
const unsigned long NB_GO_STEP_MS[]    = {  500,   500,  1500,    0};  // go tone after the Nth poke
const unsigned long NB_PAUSE_STEP_MS[] = {10000,  5000,     0,    0};  // pause allowed before UnderCount
const int NB_GO_STEPS = sizeof(NB_GO_STEP_MS) / sizeof(NB_GO_STEP_MS[0]);  // number of steps, automatic
static_assert(sizeof(NB_GO_STEP_MS) == sizeof(NB_PAUSE_STEP_MS), "NB_GO_STEP_MS and NB_PAUSE_STEP_MS must have the same number of steps");

// Rule cue replays (both off in the design: the cue plays once, when the rule is drawn).
const bool NB_CUE_REPLAY_FIRST_POKE  = false;  // also play it at the first left poke of every attempt
const bool NB_CUE_REPLAY_AFTER_ERROR = false;  // also play it when the ports re-light after an error


// ---------------------------------------------------------------------------- 5. PASS RULE
const int NB_BLOCK_SIZE = 30;  // scored attempts per block (per rule)
const int NB_PASS_PCT   = 60;  // a block passes at >= this % rewarded; a stage needs its last 2 blocks to pass


// ---------------------------------------------------------------------------- 6. SOUNDS AND LIGHTS
// tone() has no volume control: keep cues short, damp the buzzer, and measure the level at the port.
// Set the pitches from that measurement. About 0.5 s of sound per attempt in total.
// Rule A cue, rising chirp: NB_CUE_A_STEPS tones stepping from START to END Hz, NB_CUE_A_STEP_MS each
// (6 x 25 ms = 150 ms).
const int NB_CUE_A_START_HZ = 3000;
const int NB_CUE_A_END_HZ   = 7000;
const int NB_CUE_A_STEPS    = 6;
const int NB_CUE_A_STEP_MS  = 25;
// Rule C cue (Stage 2 only), warble: two pitches far apart, alternating LOW, HIGH, LOW, HIGH ...
// NB_CUE_C_CYCLES times, NB_CUE_C_STEP_MS each (4 x (20 + 20) ms = 160 ms). It jumps back and forth
// instead of gliding one way (Rule A chirp) or repeating one pitch (Rule B flutter), so the three
// rule cues differ in pattern, not only in pitch. Change the pitches/timing here after a bench listen.
const int NB_CUE_C_LOW_HZ   = 2500;
const int NB_CUE_C_HIGH_HZ  = 8000;
const int NB_CUE_C_CYCLES   = 4;
const int NB_CUE_C_STEP_MS  = 20;
// Rule B cue, fast flutter: NB_CUE_B_PULSES pulses of NB_CUE_B_HZ, ON ms on then OFF ms silent
// (8 x 19 ms = 152 ms, about 53 pulses per second: too fast to count, so the cue does not spell out N).
const int NB_CUE_B_HZ       = 5000;
const int NB_CUE_B_PULSES   = 8;
const int NB_CUE_B_ON_MS    = 9;
const int NB_CUE_B_OFF_MS   = 10;
// Go tone: short and steady, at a pitch unlike the 4 kHz reward tone.
const int NB_GO_HZ          = 6000;
const int NB_GO_MS          = 100;
// Reward cue (with house light), window open/close tone, and port light level: same as Mode 5.
const int NB_REWARD_MS      = 200;
const int NB_REWARD_HZ      = 4000;
const int NB_WINDOW_TONE_HZ = 4000;
const int NB_WINDOW_TONE_MS = 1200;
const int NB_PORT_GREEN     = 10;   // green level of a lit port (Mode 5 uses 10)


// ---------------------------------------------------------------------------- STATE (do not edit)
// Phase of the current attempt. The phase decides what a poke means and what the lights show.
const int NB_DARK    = 0;  // between windows: lights off, pokes logged only
const int NB_READY   = 1;  // window open, attempt available: left lit, waiting for the first left poke
const int NB_COUNT   = 2;  // 1+ left pokes: left lit, counting; D runs from the latest left poke
const int NB_GO      = 3;  // D ran out on exactly N: right lit, switch window running
const int NB_PENALTY = 4;  // error timeout: both ports dark, pokes logged only
const int NB_IDLE    = 5;  // New Barbosa run over: lights off, nothing more happens
int NB_phase = NB_DARK;

// Error codes; NB_fail() turns each into its event name.
const int NB_ERR_OVER   = 1;  // OverCount
const int NB_ERR_UNDER  = 2;  // UnderCount
const int NB_ERR_EARLY  = 3;  // RightEarly
const int NB_ERR_SWITCH = 4;  // SwitchTimeout
const int NB_ERR_LEFT   = 5;  // LeftTimeout (shaping option only)

// Stages. Training is Stages 1 to NB_STAGE_MIX; NB_STAGE_NB is New Barbosa.
const int NB_STAGE_MIX = 4;           // last training stage: A/B mixed
const int NB_STAGE_NB  = 5;           // New Barbosa (after training)

// Rule codes (also the Block_Pellet_Count column). A and B keep their earlier codes.
const int NB_RULE_A = 1;              // N1
const int NB_RULE_B = 2;              // N3
const int NB_RULE_C = 3;              // N2, Stage 2 only

// Stage and clocks.
bool NB_training = true;              // false once Stage 4 is passed (or NB_START_STAGE = 5)
int NB_stage = 1;                     // 1-4 = training, 5 = New Barbosa
int NB_step = 1;                      // go-delay step within Stage 2 or 3 (section 4b); 1 elsewhere
bool NB_in_window = false;            // true while a feeding window is open
int NB_window_num = 0;                // windows opened since power-on
unsigned long NB_clock = 0;           // start of the current window or dark gap
unsigned long NB_session_start = 0;   // start of the 72 h New Barbosa run
unsigned long NB_paint_t = 0;         // last time the port lights were re-asserted

// Current rule draw (kept through correction attempts).
int NB_rule = 0;                      // 1 = A, 2 = B, 0 = none
int NB_N = 0;                         // left pokes required by this rule draw
int NB_last_rule = 0;                 // previous rule, for the run cap
int NB_same_run = 0;                  // how many draws in a row NB_last_rule has come up

// Current attempt.
unsigned long NB_D = 0;               // delay for this attempt (ms)
int NB_count = 0;                     // left pokes this attempt
bool NB_scored = false;               // does this attempt count toward the pass rule?
unsigned long NB_t_first = 0;         // time of the first left poke of this attempt
unsigned long NB_t_last = 0;          // time of the latest left poke (D runs from here)
unsigned long NB_t_go = 0;            // time GO played
unsigned long NB_t_penalty = 0;       // time the error timeout started

// Pass-rule blocks, one set per rule: index 1 = A, 2 = B, 3 = C (index 0 unused).
int NB_blk_n[4]         = {0, 0, 0, 0};                  // scored attempts in the block being filled
int NB_blk_hits[4]      = {0, 0, 0, 0};                  // rewarded among them
int NB_blk_done[4]      = {0, 0, 0, 0};                  // blocks completed this stage
bool NB_blk_last_ok[4]  = {false, false, false, false};  // did the most recent block pass?
bool NB_blk_prev_ok[4]  = {false, false, false, false};  // did the block before it pass?

// Per-window counts, written to the CSV as one summary row when the window closes.
int NB_w_att = 0, NB_w_pel = 0, NB_w_sc = 0, NB_w_schit = 0;
int NB_w_oc = 0, NB_w_uc = 0, NB_w_re = 0, NB_w_st = 0, NB_w_lt = 0;


// ---------------------------------------------------------------------------- ACTIVE TIMINGS
// Every timing is read through these, so NB_TEST_MODE switches all of them in one place.
// Dark gap and switch window also depend on training vs New Barbosa.
unsigned long NB_windowMs()   { return NB_TEST_MODE ? NB_TEST_WINDOW_MS : NB_WINDOW_MS; }
unsigned long NB_sessionMs()  { return NB_TEST_MODE ? NB_TEST_SESSION_MS : NB_SESSION_MS; }
unsigned long NB_errorMs() {
  if (NB_TEST_MODE) return NB_training ? NB_TEST_ERROR_TRAIN_MS : NB_TEST_ERROR_TIMEOUT_MS;
  return NB_training ? NB_ERROR_TIMEOUT_TRAIN_MS : NB_ERROR_TIMEOUT_MS;
}
int NB_freeWindows()          { return NB_TEST_MODE ? NB_TEST_FREE_PELLET_WINS : NB_FREE_PELLET_WINS; }
int NB_blockSize()            { return NB_TEST_MODE ? NB_TEST_BLOCK_SIZE : NB_BLOCK_SIZE; }

unsigned long NB_darkMs() {
  if (NB_TEST_MODE) return NB_training ? NB_TEST_DARK_TRAIN_MS : NB_TEST_DARK_PROD_MS;
  return NB_training ? NB_DARK_TRAIN_MS : NB_DARK_PROD_MS;
}

unsigned long NB_switchMs() {
  if (NB_TEST_MODE) return NB_training ? NB_TEST_SWITCH_TRAIN_MS : NB_TEST_SWITCH_PROD_MS;
  return NB_training ? NB_SWITCH_TRAIN_MS : NB_SWITCH_PROD_MS;
}

// Draws a new delay D, uniform and inclusive. Survives the two bounds being entered backwards.
unsigned long NB_drawDelay() {
  unsigned long lo = NB_TEST_MODE ? NB_TEST_DELAY_MIN_MS : NB_DELAY_MIN_MS;
  unsigned long hi = NB_TEST_MODE ? NB_TEST_DELAY_MAX_MS : NB_DELAY_MAX_MS;
  if (hi < lo) { unsigned long t = lo; lo = hi; hi = t; }
  return (unsigned long)random((long)lo, (long)hi + 1);  // random() excludes its upper bound
}

// True when the current stage runs go-delay steps (training Stage 2 or 3 with its switch on).
bool NB_fadeOn() {
  if (!NB_training || NB_GO_STEPS < 1) return false;
  if (NB_stage == 2) return NB_GO_FADE_S2;
  if (NB_stage == 3) return NB_GO_FADE_S3;
  return false;
}

// Delay before the go tone once the count is exactly N: the step's go delay, or D when steps are
// off or the step's value is 0.
unsigned long NB_goDelay() {
  if (!NB_fadeOn()) return NB_D;
  int i = constrain(NB_step, 1, NB_GO_STEPS) - 1;
  if (NB_GO_STEP_MS[i] == 0) return NB_D;
  return NB_GO_STEP_MS[i];
}

// Pause allowed with fewer than N pokes before the attempt ends as UnderCount: the step's value, or
// D when steps are off or the step's value is 0.
unsigned long NB_pauseMs() {
  if (!NB_fadeOn()) return NB_D;
  int i = constrain(NB_step, 1, NB_GO_STEPS) - 1;
  if (NB_PAUSE_STEP_MS[i] == 0) return NB_D;
  return NB_PAUSE_STEP_MS[i];
}


// ---------------------------------------------------------------------------- HELPERS
// Serial trace for bench testing. Skipped unless NB_SERIAL_DEBUG is on AND a computer is listening,
// so it can never stall the FED.
void NB_debug(String msg) {
  if (!NB_SERIAL_DEBUG) return;
  if (!Serial) return;
  Serial.print(millis());
  Serial.print(" ms  NB  ");
  Serial.println(msg);
}

// Copies the current attempt into the CSV columns, so every row the library or this mode writes
// carries them (see LOG COLUMNS above).
void NB_setColumns() {
  fed3.FR = (int)NB_D;
  fed3.BlockPelletCount = NB_rule;
  fed3.RPR_Left = NB_N;
  fed3.RPR_Count_Left = NB_count;
  fed3.RPR_Right = NB_scored ? 1 : 0;
  fed3.RPR_Count_Right = NB_fadeOn() ? NB_step : 0;
  fed3.FR_Count_Left = NB_stage;
  fed3.FR_Count_Right = NB_window_num;
}

// Writes one event row with the current columns.
void NB_log(String ev) {
  NB_setColumns();
  fed3.setEvent(ev);
  fed3.logSpecialEvent();
  NB_debug(ev);
}

// Rule letter for event names.
String NB_letter(int rule) {
  if (rule == NB_RULE_A) return "A";
  if (rule == NB_RULE_B) return "B";
  if (rule == NB_RULE_C) return "C";
  return "0";
}

// "1" or "3-4": an N list written into the boot row.
String NB_listStr(const int* list, int len) {
  String s = "";
  for (int i = 0; i < len; i++) {
    if (i > 0) s += "-";
    s += String(list[i]);
  }
  return s;
}

// Stage as text: "1", "2.1" (Stage 2, go-delay step 1), "3.3", "4", "5".
String NB_stageLabel() {
  String s = String(NB_stage);
  if (NB_fadeOn()) {
    s += ".";
    s += String(NB_step);
  }
  return s;
}

// Screen and Session_type text, padded to 8 characters like Mode 5 ("NB-S1", "NB-S2.1", ...,
// "NewBRB"). Test mode adds a leading "t" ("tNB-S2.1" is exactly 8).
void NB_updateScreen() {
  String s = NB_TEST_MODE ? "t" : "";
  if (NB_stage <= NB_STAGE_MIX) {
    s += "NB-S";
    s += NB_stageLabel();
  } else {
    s += "NewBRB";
  }
  while (s.length() < 8) s += " ";
  fed3.sessiontype = s;
  fed3.UpdateDisplay();
}

// Chirp: equal frequency steps from startHz to endHz (rising if end > start, falling if not).
void NB_playChirp(int startHz, int endHz, int steps, int stepMs) {
  for (int i = 0; i < steps; i++) {
    int hz = startHz;
    if (steps > 1) hz += (endHz - startHz) * i / (steps - 1);
    fed3.Tone(hz, stepMs);
    delay(stepMs);
  }
}

// Plays a rule cue. Blocks for about 150 ms; pokes made meanwhile are kept by the library and
// handled on the next pass.
void NB_playRuleCue(int rule) {
  if (rule == NB_RULE_A) {
    // Rule A: rising chirp.
    NB_playChirp(NB_CUE_A_START_HZ, NB_CUE_A_END_HZ, NB_CUE_A_STEPS, NB_CUE_A_STEP_MS);
  } else if (rule == NB_RULE_C) {
    // Rule C: warble, low and high pitch alternating.
    for (int i = 0; i < NB_CUE_C_CYCLES; i++) {
      fed3.Tone(NB_CUE_C_LOW_HZ, NB_CUE_C_STEP_MS);
      delay(NB_CUE_C_STEP_MS);
      fed3.Tone(NB_CUE_C_HIGH_HZ, NB_CUE_C_STEP_MS);
      delay(NB_CUE_C_STEP_MS);
    }
  } else if (rule == NB_RULE_B) {
    // Rule B: fast flutter, short pulses separated by short silences.
    for (int i = 0; i < NB_CUE_B_PULSES; i++) {
      fed3.Tone(NB_CUE_B_HZ, NB_CUE_B_ON_MS);
      delay(NB_CUE_B_ON_MS + NB_CUE_B_OFF_MS);
    }
  }
}

// Sets the port lights for the current phase, house light off. Same method as Mode 5: house pixels
// 0-7 are cleared in the buffer only, then each port call sends the whole strip at once.
// Called every 200 ms (the library's cue and feed calls can leave the house light on) and
// immediately whenever the phase changes.
void NB_paintPorts() {
  NB_paint_t = millis();
  for (uint16_t i = 0; i < 8; i++) fed3.strip.setPixelColor(i, 0, 0, 0, 0);
  if (NB_phase == NB_READY || NB_phase == NB_COUNT) {
    fed3.leftPokePixel(0, NB_PORT_GREEN, 0, 0);   // left lit: count here
    fed3.rightPokePixel(0, 0, 0, 0);
  } else if (NB_phase == NB_GO) {
    fed3.leftPokePixel(0, 0, 0, 0);
    fed3.rightPokePixel(0, NB_PORT_GREEN, 0, 0);  // right lit: go here
  } else {
    fed3.leftPokePixel(0, 0, 0, 0);               // penalty, dark gap, idle: both dark
    fed3.rightPokePixel(0, 0, 0, 0);
  }
}

// Clears the pass-rule blocks. Called at power-on and whenever the stage changes, so every stage
// is judged only on its own attempts.
void NB_resetBlocks() {
  for (int r = 0; r < 4; r++) {
    NB_blk_n[r] = 0;
    NB_blk_hits[r] = 0;
    NB_blk_done[r] = 0;
    NB_blk_last_ok[r] = false;
    NB_blk_prev_ok[r] = false;
  }
}

// True when the last two completed blocks of this rule both passed.
bool NB_rulePassed(int rule) {
  return NB_blk_done[rule] >= 2 && NB_blk_last_ok[rule] && NB_blk_prev_ok[rule];
}

// Books the attempt that just ended. Every attempt counts in the window summary; only scored
// attempts go into the pass-rule block of their rule. A full block is judged, logged, and cleared.
void NB_scoreAttempt(bool rewarded) {
  NB_w_att += 1;
  if (!NB_scored) return;                  // correction attempt (Stage 4 / New Barbosa): not scored
  NB_w_sc += 1;
  if (rewarded) NB_w_schit += 1;

  int r = NB_rule;
  if (r < NB_RULE_A || r > NB_RULE_C) return;
  NB_blk_n[r] += 1;
  if (rewarded) NB_blk_hits[r] += 1;

  if (NB_blk_n[r] >= NB_blockSize()) {
    bool ok = (NB_blk_hits[r] * 100 >= NB_PASS_PCT * NB_blk_n[r]);
    NB_blk_prev_ok[r] = NB_blk_last_ok[r];
    NB_blk_last_ok[r] = ok;
    NB_blk_done[r] += 1;

    String ev = "Block_";
    ev += NB_letter(r);
    ev += String(NB_blk_done[r]);
    ev += "_";
    ev += String(NB_blk_hits[r]);
    ev += "of";
    ev += String(NB_blk_n[r]);
    ev += ok ? "_Pass" : "_Fail";
    NB_log(ev);

    NB_blk_n[r] = 0;
    NB_blk_hits[r] = 0;
  }
}

// Draws the next rule and its N. Stage 1 is always A (N1), Stage 2 always C (N2), Stage 3 always
// B (N3); Stage 4 and New Barbosa draw B with NB_RULE_B_PCT percent, otherwise A, with an optional
// cap on runs of the same rule.
void NB_drawRule() {
  int next;
  if (NB_stage == 1) {
    next = NB_RULE_A;
  } else if (NB_stage == 2) {
    next = NB_RULE_C;
  } else if (NB_stage == 3) {
    next = NB_RULE_B;
  } else {
    next = (random(100) < NB_RULE_B_PCT) ? NB_RULE_B : NB_RULE_A;
    if (NB_MAX_SAME_RULE > 0 && next == NB_last_rule && NB_same_run >= NB_MAX_SAME_RULE) {
      next = (next == NB_RULE_A) ? NB_RULE_B : NB_RULE_A;   // run cap reached: switch rule
    }
  }
  NB_same_run = (next == NB_last_rule) ? NB_same_run + 1 : 1;
  NB_last_rule = next;
  NB_rule = next;

  if (next == NB_RULE_A)      NB_N = NB_N_LIST_A[random(NB_N_LEN_A)];
  else if (next == NB_RULE_C) NB_N = NB_N_LIST_C[random(NB_N_LEN_C)];
  else                        NB_N = NB_N_LIST_B[random(NB_N_LEN_B)];
  if (NB_N < 1) NB_N = 1;                  // guard against a 0 in a list
}

// Opens a new attempt on the current rule: fresh D, count 0, left port available.
void NB_armAttempt(bool scored) {
  NB_D = NB_drawDelay();
  NB_count = 0;
  NB_scored = scored;
  NB_phase = NB_READY;
  NB_setColumns();
}

// New rule: draw it, open its first attempt (always scored), log RuleA/RuleB, light the left
// port, play the cue once. The row is written just before the cue starts.
void NB_newRule() {
  NB_drawRule();
  NB_armAttempt(true);
  NB_log("Rule" + NB_letter(NB_rule));
  NB_paintPorts();
  NB_playRuleCue(NB_rule);
}

// Ends the attempt as an error: log it (the row shows the count it ended on), book it, then
// both ports dark for the error timeout.
void NB_fail(int code) {
  String ev = "Error";
  if (code == NB_ERR_OVER)   { ev = "OverCount";     NB_w_oc += 1; }
  if (code == NB_ERR_UNDER)  { ev = "UnderCount";    NB_w_uc += 1; }
  if (code == NB_ERR_EARLY)  { ev = "RightEarly";    NB_w_re += 1; }
  if (code == NB_ERR_SWITCH) { ev = "SwitchTimeout"; NB_w_st += 1; }
  if (code == NB_ERR_LEFT)   { ev = "LeftTimeout";   NB_w_lt += 1; }
  NB_log(ev);
  NB_scoreAttempt(false);

  NB_phase = NB_PENALTY;
  NB_t_penalty = millis();
  NB_count = 0;
  NB_setColumns();
  NB_paintPorts();                         // both ports dark now
}

// Error timeout over: same rule and N again with a fresh D. Single-rule Stages 1-3 score it; in
// Stage 4 and New Barbosa it is a correction attempt and does not count.
void NB_retry() {
  NB_armAttempt(NB_stage < NB_STAGE_MIX);
  NB_log("Retry");
  NB_paintPorts();
  if (NB_CUE_REPLAY_AFTER_ERROR) NB_playRuleCue(NB_rule);
}

// Go delay ran out on exactly N: go tone (does not block), left off and right on at once, then the
// row. The Go row's FR holds the delay actually used (D, or the step's go delay); FR goes back to D
// for every later row of the attempt.
void NB_go() {
  NB_phase = NB_GO;
  NB_t_go = millis();
  fed3.Tone(NB_GO_HZ, NB_GO_MS);
  NB_paintPorts();
  NB_setColumns();
  fed3.FR = (int)NB_goDelay();
  fed3.setEvent("Go");
  fed3.logSpecialEvent();
  NB_debug("Go");
  fed3.FR = (int)NB_D;
}

// Right poke after GO: Mode 5's reward cue and pellet, then the next rule. The library's Feed()
// returns only once the pellet is taken and writes the Pellet row itself. If the window ran out
// while the pellet sat in the well, no new rule is drawn: no attempt is open and the next pass
// closes the window.
void NB_reward() {
  fed3.ConditionedStimulus(NB_REWARD_MS, NB_REWARD_HZ, 'G', true, false);
  fed3.Feed(false, 'G');
  NB_w_pel += 1;
  NB_scoreAttempt(true);
  if (millis() - NB_clock > NB_windowMs()) {
    NB_phase = NB_DARK;
    NB_paintPorts();
    return;
  }
  NB_newRule();
}

// One summary row per window, e.g. Win12_S4_Att14_Pel10_Sc9_ScHit7_OC1_UC2_RE1_ST0_LT0
// (attempts, pellets, scored attempts, rewarded scored attempts, then each error type).
void NB_logWindowSummary() {
  String ev = "Win";
  ev += String(NB_window_num);
  ev += "_S";     ev += NB_stageLabel();
  ev += "_Att";   ev += String(NB_w_att);
  ev += "_Pel";   ev += String(NB_w_pel);
  ev += "_Sc";    ev += String(NB_w_sc);
  ev += "_ScHit"; ev += String(NB_w_schit);
  ev += "_OC";    ev += String(NB_w_oc);
  ev += "_UC";    ev += String(NB_w_uc);
  ev += "_RE";    ev += String(NB_w_re);
  ev += "_ST";    ev += String(NB_w_st);
  ev += "_LT";    ev += String(NB_w_lt);
  NB_log(ev);
}

// Training only, at each window close: does the stage (or go-delay step) pass? Stage 1 needs Rule A,
// Stage 2 Rule C, Stage 3 Rule B, Stage 4 both A and B. In a stage with go-delay steps, passing a
// step that is not the last moves to the next step; passing the last step passes the stage.
// Passing moves on from the next window.
void NB_checkStage() {
  bool passA = NB_rulePassed(NB_RULE_A);
  bool passB = NB_rulePassed(NB_RULE_B);
  bool passC = NB_rulePassed(NB_RULE_C);
  bool pass = false;
  if (NB_stage == 1) pass = passA;
  if (NB_stage == 2) pass = passC;
  if (NB_stage == 3) pass = passB;
  if (NB_stage == NB_STAGE_MIX) pass = passA && passB;

  String ev = "StageCheck_S";
  ev += NB_stageLabel();
  ev += pass ? "_Pass" : "_Continue";
  NB_log(ev);
  if (!pass) return;                       // stage continues; its blocks keep rolling

  if (NB_fadeOn() && NB_step < NB_GO_STEPS) {
    NB_step += 1;                          // next go-delay step, same stage, fresh blocks
    NB_resetBlocks();
    NB_log("StepAdvance_S" + NB_stageLabel());
    NB_updateScreen();
    return;
  }

  NB_last_rule = 0;                        // the run cap starts fresh in the new stage
  NB_same_run = 0;
  NB_step = 1;                             // a new stage starts at its first go-delay step
  if (NB_stage < NB_STAGE_MIX) {
    NB_stage += 1;
    NB_resetBlocks();
    NB_log("StageAdvance_S" + String(NB_stage));
  } else {
    NB_training = false;                   // switches dark gap and switch window to New Barbosa
    NB_stage = NB_STAGE_NB;
    NB_resetBlocks();
    NB_session_start = millis();           // the 72 h run starts now
    NB_log("TrainingComplete");
  }
  NB_updateScreen();
}

// Opens a feeding window: Mode 5's onset signal (1.2 s green flash on both ports and the house
// light, 4 kHz tone), a free pellet in the first training windows, then the first rule.
void NB_openWindow() {
  NB_in_window = true;
  NB_clock = millis();
  NB_window_num += 1;

  // Seed once, at the first window (as Mode 5): run() has kept fed3.unixtime current from the RTC,
  // and the device number keeps FEDs started in the same second apart. No random() is used before.
  if (NB_window_num == 1) randomSeed(fed3.unixtime ^ ((unsigned long)fed3.FED << 16));

  NB_w_att = 0; NB_w_pel = 0; NB_w_sc = 0; NB_w_schit = 0;
  NB_w_oc = 0; NB_w_uc = 0; NB_w_re = 0; NB_w_st = 0; NB_w_lt = 0;

  fed3.pixelsOn(0, 10, 0, 0);
  fed3.leftPokePixel(0, 10, 0, 0);
  fed3.rightPokePixel(0, 10, 0, 0);
  NB_log("PeriodStart");
  fed3.Tone(NB_WINDOW_TONE_HZ, NB_WINDOW_TONE_MS);
  delay(NB_WINDOW_TONE_MS);
  fed3.pixelsOff();

  if (NB_training && NB_window_num <= NB_freeWindows()) {
    fed3.ConditionedStimulus(NB_REWARD_MS, NB_REWARD_HZ, 'G', true, false);
    fed3.Feed(false, 'G');
    NB_clock = millis();                   // full window counted from the free pellet's retrieval
  }

  // Pokes made during the onset flash came before any rule was cued: log them now, before the first
  // attempt opens, so they cannot count toward it. (After a free pellet, Feed() has cleared them.)
  if (fed3.Left) fed3.logLeftPoke();
  if (fed3.Right) fed3.logRightPoke();

  NB_newRule();
}

// Closes a feeding window: any open attempt is dropped (not scored), lights off, PeriodEnd, the
// window summary, the stage check in training, then Mode 5's closing tone.
void NB_closeWindow() {
  NB_in_window = false;
  NB_clock = millis();
  NB_phase = NB_DARK;
  NB_rule = 0;
  NB_N = 0;
  NB_D = 0;
  NB_count = 0;
  NB_scored = false;

  fed3.leftPokePixel(0, 0, 0, 0);
  fed3.rightPokePixel(0, 0, 0, 0);
  fed3.pixelsOff();
  NB_log("PeriodEnd");
  NB_logWindowSummary();
  if (NB_training) NB_checkStage();
  fed3.Tone(NB_WINDOW_TONE_HZ, NB_WINDOW_TONE_MS);
}

// End of the 72 h New Barbosa run: lights off, one Idle row, then nothing (as Mode 5).
void NB_goIdle() {
  NB_phase = NB_IDLE;
  fed3.leftPokePixel(0, 0, 0, 0);
  fed3.rightPokePixel(0, 0, 0, 0);
  fed3.pixelsOff();
  NB_log("Idle");
}

// Test mode: every New Barbosa sound once, 1 s apart, for setting levels at the port.
void NB_cueCheck() {
  NB_log("CueCheck_RuleA");
  NB_playRuleCue(NB_RULE_A);
  delay(1000);
  NB_log("CueCheck_RuleC");
  NB_playRuleCue(NB_RULE_C);
  delay(1000);
  NB_log("CueCheck_RuleB");
  NB_playRuleCue(NB_RULE_B);
  delay(1000);
  NB_log("CueCheck_Go");
  fed3.Tone(NB_GO_HZ, NB_GO_MS);
  delay(1000);
  NB_log("CueCheck_Reward");
  fed3.Tone(NB_REWARD_HZ, NB_REWARD_MS);
  delay(1000);
}

// Runs once, on the first pass through Mode 0: sleep off, starting stage, clean columns, screen,
// a boot row recording the settings in force, and the cue check in test mode. The first window
// opens after one dark gap, as in Mode 5.
void NB_boot() {
  fed3.disableSleep();
  if (NB_SERIAL_DEBUG) Serial.begin(115200);

  NB_stage = constrain(NB_START_STAGE, 1, NB_STAGE_NB);
  NB_training = (NB_stage <= NB_STAGE_MIX);
  NB_step = NB_fadeOn() ? constrain(NB_START_STEP, 1, NB_GO_STEPS) : 1;
  NB_resetBlocks();
  NB_phase = NB_DARK;
  NB_clock = millis();
  NB_session_start = millis();             // used only when starting directly in New Barbosa
  NB_updateScreen();

  String ev = "NB_Boot_S";
  ev += NB_stageLabel();
  ev += "_NA";   ev += NB_listStr(NB_N_LIST_A, NB_N_LEN_A);
  ev += "_NC";   ev += NB_listStr(NB_N_LIST_C, NB_N_LEN_C);
  ev += "_NB";   ev += NB_listStr(NB_N_LIST_B, NB_N_LEN_B);
  ev += "_PB";   ev += String(NB_RULE_B_PCT);
  ev += "_D";    ev += String(NB_TEST_MODE ? NB_TEST_DELAY_MIN_MS : NB_DELAY_MIN_MS);
  ev += "to";    ev += String(NB_TEST_MODE ? NB_TEST_DELAY_MAX_MS : NB_DELAY_MAX_MS);
  ev += "_Err";  ev += String(NB_errorMs());
  ev += "_Blk";  ev += String(NB_blockSize());
  ev += "x";     ev += String(NB_PASS_PCT);
  ev += "_UC";   ev += String(NB_UNDERCOUNT_IS_ERROR ? 1 : 0);
  ev += "_Rp";   ev += String(NB_CUE_REPLAY_FIRST_POKE ? 1 : 0);
  ev += String(NB_CUE_REPLAY_AFTER_ERROR ? 1 : 0);
  ev += "_Test"; ev += String(NB_TEST_MODE ? 1 : 0);
  ev += "_GF";   ev += String(NB_GO_FADE_S2 ? 1 : 0);
  ev += String(NB_GO_FADE_S3 ? 1 : 0);
  for (int i = 0; i < NB_GO_STEPS; i++) {
    ev += (i == 0) ? "_" : "-";
    ev += String(NB_GO_STEP_MS[i]);
  }
  ev += "_PS";
  for (int i = 0; i < NB_GO_STEPS; i++) {
    if (i > 0) ev += "-";
    ev += String(NB_PAUSE_STEP_MS[i]);
  }
  NB_log(ev);

  if (NB_TEST_MODE && NB_TEST_CUE_CHECK) NB_cueCheck();
  NB_paintPorts();                         // both dark until the first window
}
// New Barbosa Globals End /////////////////////////////////////////////////////////////////////////

void setup() {
  randomSeed(analogRead(0));
  pokes_required = random(1, upperbound);  // Initialize random seed using an unconnected analog pin for better randomness  (inclusive, exlusive)
  fed3.ClassicFED3 = true;
  fed3.begin();  //Setup the FED3 hardware

  // BARBOSA SETUP ///////////////////////////////////////////////////////////
  BRB_session_start = millis();
  BRB_clock = millis();
  BRB_req_left = random(BRB_lower_bound_left, BRB_upper_bound_left + 1);
  // BARBOSA SETUP ///////////////////////////////////////////////////////////

  fed3.activePoke = 1;      // initialize active poke to left
  sessionStart = millis();  // Initialize session start time (added)                                      //Setup the FED3 hardware

  // Setup FED3.
  fed3.FR = FR_req_right;  // FR is the same for both.
  fed3.FR_Count_Right = FR_count_right;
  fed3.FR_Count_Left = FR_count_left;

  // Setup random.
  RPR_req_right = random(RPR_lower_bound_right, RPR_upper_bound_right + 1);  // Initialize random seed using an unconnected analog pin for better randomness
  RPR_req_left = random(RPR_lower_bound_left, RPR_upper_bound_left + 1);
  MED_req_right = random(MED_lower_bound_right, MED_upper_bound_right + 1);  // Initialize random seed using an unconnected analog pin for better randomness
  MED_req_left = random(MED_lower_bound_left, MED_upper_bound_left + 1);

  fed3.RPR_Right = RPR_req_right;
  fed3.RPR_Left = RPR_req_left;
  fed3.RPR_Count_Right = RPR_count_right;
  fed3.RPR_Count_Left = RPR_count_left;

  millis_start = millis();

  // ADDED: Mode 5 only — sleep off (millis = wall clock; port lights stay powered)
  // Run Mode 5 on USB (~3–4 day battery if on battery with sleep off).
  if (fed3.FEDmode == 5) {
    fed3.disableSleep();
    // CHANGED: randomSeed moved into Mode 5 (first PeriodStart, RTC-based)
    fed3.FR = 0;                     // ADDED: clean log columns
    fed3.RPR_Left = 0;
    fed3.RPR_Right = BRB_req_right;
  }  
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  // fed3.t1 = micros();      // REMOVE IN PRODUCTION
  // /*
  if ((millis() - sessionStart > SESSION_DURATION) && (fed3.FEDmode == 1 || fed3.FEDmode == 2 || fed3.FEDmode == 3 || fed3.FEDmode == 4 || fed3.FEDmode == 9 || fed3.FEDmode == 10)) {
    isIdle = true;
  }

  if (isIdle && !isIdleLatch) {
    isIdleLatch = true;

    fed3.setEvent("Idle");
    fed3.logSpecialEvent();
  }

  // If in idle mode, skip all processing (no logging, no delivery)
  if (isIdle) {
    fed3.run();
    return;  // Skip the rest of the loop
  }
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                     Mode 0: New Barbosa
  //   Replaces Free_feed (which was: fed3.Feed(); fed3.Timeout(5);). All settings and helpers are in
  //   the NEW BARBOSA globals above setup(). Self-contained: nothing outside this block and the NB_
  //   globals is read or written.
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (fed3.FEDmode == 0) {
    fed3.DisplayPokes = true;
    fed3.DisplayTimed = false;

    // ------------------------------------------------------------------ one-shot start-up
    // Sleep off, starting stage, clean log columns, boot row, cue check in test mode.
    static bool NB_boot_done = false;
    if (!NB_boot_done) {
      NB_boot_done = true;
      NB_boot();
    }

    // ------------------------------------------------------------------ 72 h stop
    // Applies only after training (training has no time cap). Once idle, nothing else runs.
    if (NB_phase == NB_IDLE) {
      fed3.run();
      return;
    }
    if (!NB_training && (millis() - NB_session_start > NB_sessionMs())) {
      NB_goIdle();
      fed3.run();
      return;
    }

    // ------------------------------------------------------------------ window clock
    // Dark gap -> window -> dark gap. While dark, pokes are logged and nothing else happens.
    if (!NB_in_window) {
      if (millis() - NB_clock > NB_darkMs()) {
        NB_openWindow();                     // onset signal, free pellet (early training), first rule
      } else {
        if (fed3.Left) fed3.logLeftPoke();
        if (fed3.Right) fed3.logRightPoke();
        fed3.run();
        return;
      }
    } else if (millis() - NB_clock > NB_windowMs()) {
      NB_closeWindow();                      // drops any open attempt, summary, stage check
      fed3.run();
      return;
    }

    // ------------------------------------------------------------------ LEFT PORT
    // Pokes are handled before the timers: a poke waiting here happened before "now", so it must be
    // judged against the delay that was running when the mouse made it.
    if (fed3.Left) {
      // Counts only while an attempt is available or counting. During GO and the error timeout a
      // left poke is logged and nothing else.
      bool NB_counts = (NB_phase == NB_READY || NB_phase == NB_COUNT);
      if (NB_counts) {
        NB_count += 1;                       // count first, so the Left row includes this poke
        fed3.RPR_Count_Left = NB_count;
      }
      fed3.logLeftPoke();                    // library: waits for the nose to leave, writes "Left"

      if (fed3.Left) {
        // The library skipped this poke: right after Feed() it still flags a pellet in the well
        // until fed3.run() clears that flag. Undo the count; the poke stays pending and is logged
        // and counted (once) on the next pass.
        if (NB_counts) {
          NB_count -= 1;
          fed3.RPR_Count_Left = NB_count;
        }
      } else if (NB_counts) {
        NB_t_last = millis();                // D restarts from every left poke
        if (NB_count == 1) {                 // first poke: the attempt starts
          NB_phase = NB_COUNT;
          NB_t_first = NB_t_last;
          if (NB_SERIAL_DEBUG) NB_debug("attempt start, rule " + NB_letter(NB_rule) + " N=" + String(NB_N) + " D=" + String(NB_D));
          if (NB_CUE_REPLAY_FIRST_POKE) NB_playRuleCue(NB_rule);
        }
        if (NB_count > NB_N) {
          NB_fail(NB_ERR_OVER);              // one poke too many, including during the delay
        }
      }
    }

    // ------------------------------------------------------------------ RIGHT PORT
    if (fed3.Right) {
      fed3.logRightPoke();
      if (!fed3.Right) {                     // logged by the library (see the pellet note above)
        if (NB_phase == NB_COUNT) {
          NB_fail(NB_ERR_EARLY);             // went right before the go tone
        } else if (NB_phase == NB_GO) {
          NB_reward();                       // the only rewarded path; next rule drawn inside
        }
        // NB_READY (no attempt open) and NB_PENALTY: logged only
      }
    }

    // ------------------------------------------------------------------ timers
    unsigned long NB_now = millis();
    if (NB_phase == NB_COUNT) {
      if (NB_count == NB_N) {
        // Exactly N: go once the mouse has stopped for the go delay (D in the design; shorter in a
        // go-delay step of Stages 2-3, section 4b). A poke before then is OverCount (LEFT PORT above).
        if (NB_now - NB_t_last >= NB_goDelay()) NB_go();
      } else if (NB_now - NB_t_last >= NB_pauseMs()) {
        // Fewer than N and the mouse has stopped for the allowed pause (D in the design; longer in
        // early shaping steps of Stages 2-3, section 4b).
        if (NB_UNDERCOUNT_IS_ERROR) {
          NB_fail(NB_ERR_UNDER);             // stopped short
        } else if (NB_now - NB_t_first > NB_LEFT_SAFETY_MS) {
          NB_fail(NB_ERR_LEFT);              // shaping option only: attempt open too long
        }
      }
    } else if (NB_phase == NB_GO && (NB_now - NB_t_go > NB_switchMs())) {
      NB_fail(NB_ERR_SWITCH);                // no right poke in time
    } else if (NB_phase == NB_PENALTY && (NB_now - NB_t_penalty >= NB_errorMs())) {
      NB_retry();                            // ports re-light, same rule, correction attempt
    }

    // ------------------------------------------------------------------ lights
    // Re-assert every 200 ms: the library's cue and feed calls can leave the house light on.
    if (millis() - NB_paint_t > 200) NB_paintPorts();

    fed3.run();
    return;  // don't run twice
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                     Modes 1: Dual Port Timed Changed CS Variables for Vegas
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  if (fed3.FEDmode == 1) {
    fed3.sessiontype = "DualPortTimed-Vegas";

    if (fed3.Left) {
      fed3.logLeftPoke();
      fed3.ConditionedStimulus('B');
      fed3.Feed('B');
    }

    if (fed3.Right) {
      fed3.logRightPoke();
      fed3.ConditionedStimulus('Y');
      fed3.Feed('Y');
    }
  }
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                     Mode 2: 2x2 timed
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (fed3.FEDmode == 2) {
    fed3.sessiontype = "2x2Timed";
    fed3.DisplayPokes = true;
    fed3.DisplayTimed = false;

    int feedDelay = 1;
    int maxPokes = 2;

    // Log every left poke, even if the port isn't active
    if (fed3.Left) {
      fed3.logLeftPoke();

      if (fed3.activePoke == 0) {
        fed3.ConditionedStimulus('B', false);
      }

      if (left_pokes < maxPokes) {
        left_pokes += 1;
        fed3.ConditionedStimulus('G');
        fed3.Timeout(feedDelay);
        fed3.Feed();
        if (left_pokes >= maxPokes) {
          right_pokes = 0;
          fed3.activePoke = 0;  // set the active poke to be right
        }
      }
    }

    // Log every right poke, even if the port isn't active
    if (fed3.Right) {
      fed3.logRightPoke();

      if (fed3.activePoke == 1) {
        fed3.ConditionedStimulus('B', false);
      }

      if (right_pokes < maxPokes) {
        right_pokes += 1;
        fed3.ConditionedStimulus('G');
        fed3.Timeout(feedDelay);
        fed3.Feed();
        if (right_pokes >= maxPokes) {
          left_pokes = 0;
          fed3.activePoke = 1;  // set the active poke to be left
        }
      }
    }
  }
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                     Mode 3: 5x5 Timed
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (fed3.FEDmode == 3) {
    fed3.sessiontype = "5x5TIMED";
    fed3.DisplayPokes = true;
    fed3.DisplayTimed = false;

    int feedDelay = 1;
    int maxPokes = 5;

    // Log every left poke, even if the port isn't active
    if (fed3.Left) {
      fed3.logLeftPoke();

      if (fed3.activePoke == 0) {
        fed3.ConditionedStimulus('B', false);
      }

      if (left_pokes < maxPokes) {
        left_pokes += 1;
        fed3.ConditionedStimulus('G');
        fed3.Timeout(feedDelay);
        fed3.Feed();
        if (left_pokes >= maxPokes) {
          right_pokes = 0;
          fed3.activePoke = 0;  // set the active poke to be right
        }
      }
    }

    // Log every right poke, even if the port isn't active
    if (fed3.Right) {
      fed3.logRightPoke();

      if (fed3.activePoke == 1) {
        fed3.ConditionedStimulus('B', false);
      }

      if (right_pokes < maxPokes) {
        right_pokes += 1;
        fed3.ConditionedStimulus('G');
        fed3.Timeout(feedDelay);
        fed3.Feed();
        if (right_pokes >= maxPokes) {
          left_pokes = 0;
          fed3.activePoke = 1;  // set the active poke to be left
        }
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                     Mode 4: Random Progressive Ratio Timed with Switch to RPR
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (fed3.FEDmode == 4) {
    if (fed3.PelletCount < 2) {
      fed3.sessiontype = "Dual_Port_Feeding";

      if (fed3.Left) {
        fed3.logLeftPoke();
        fed3.ConditionedStimulus('G');
        fed3.Feed();
      }

      if (fed3.Right) {
        fed3.logRightPoke();
        fed3.ConditionedStimulus('G');
        fed3.Feed();
      }
    } else {
      if (!isProgRatio) {
        isProgRatio = true;
        sessionStart = millis();
      }

      fed3.sessiontype = "ProgRatioTIMED";  // The text in "sessiontype" will appear on the screen and in the logfile
      if (fed3.Left) {                      // If left poke is triggered and pellet is not in the well
        fed3.FR = pokes_required;
        fed3.logLeftPoke();                        // Log left poke
        poke_num++;                                // Increment poke counter
        if (poke_num >= pokes_required) {          // Check if required pokes are achieved
          fed3.ConditionedStimulus('G', true);     // Deliver conditioned stimulus (tone and lights)
          fed3.Feed();                             // Deliver pellet
          poke_num = 0;                            // Reset poke counter
          pokes_required = random(1, upperbound);  // Set new random poke requirement (1–5) for next trial
        }
      }
      if (fed3.Right) {  // If right poke is triggered and pellet is not in the well
        fed3.FR = pokes_required;
        fed3.logRightPoke();                       // Log right poke
        poke_num++;                                // Increment poke counter
        if (poke_num >= pokes_required) {          // Check if required pokes are achieved
          fed3.ConditionedStimulus('G', true);     // Deliver conditioned stimulus (tone and lights)
          fed3.Feed();                             // Deliver pellet
          poke_num = 0;                            // Reset poke counter
          pokes_required = random(1, upperbound);  // Set new random poke requirement (1–5) for next trial
        }
      }
    }
  }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                     Mode 5: Barbosa 48h
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 if (fed3.FEDmode == 5) {
  fed3.DisplayPokes = true;
  fed3.DisplayTimed = false;

  BRB_updateScreen();   // keeps BRB-S1 / S2 / S3 / Barbosa correct every loop
 
    // Show training stage (or "Barbosa") on LCD once on entry
    static bool BRB_screen_init = false;
    if (!BRB_screen_init) {
      BRB_updateScreen();
      BRB_screen_init = true;
    }

    // Static local, not a global. Throttles the pixel re-assert below.
    // Set it to 0 anywhere the lights need to change NOW.
    static unsigned long BRB_paint = 0;

 // ------------------------------------------------------ 72 h session stop
    // ADDED: cap only after TrainingComplete (training has no time cap)
    if (!BRB_training && (millis() - BRB_session_start > BRB_session_ms)) {
      if (!BRB_session_over) {
        BRB_session_over = true;
        fed3.leftPokePixel(0, 0, 0, 0);
        fed3.rightPokePixel(0, 0, 0, 0);
        fed3.pixelsOff();
        fed3.setEvent("Idle");
        fed3.logSpecialEvent();
      }
      fed3.run();
      return;
    }

    // ----------------------------------------------------------- period clock
    // Single !BRB_in_period entry point: either the wait has expired and the
    // period opens, or we are still between periods and only log pokes.
    if (!BRB_in_period) {
      if (millis() - BRB_clock > BRB_wait_ms) {
        BRB_in_period = true;
        BRB_clock = millis();
        BRB_period_num += 1;

        // CHANGED: seed moved here from setup(). millis() at the end of setup()
        // is just how long setup() took, nearly the same every boot. Seeded once,
        // at the first PeriodStart: run() has been refreshing fed3.unixtime from
        // the RTC for the whole pre-period, and every Mode 5 random() call comes
        // after this point. Device number mixed in so FEDs booted in the same
        // second still diverge. (Relies on run() setting fed3.unixtime; stock does.)
        if (BRB_period_num == 1) {
          randomSeed(fed3.unixtime ^ ((unsigned long)fed3.FED << 16));
        }

        // TRAINING ADDITION: count each completed 5-min active period
        // independently from BRB_period_num, which remains the existing
        // Barbosa session period counter.
        if (BRB_training) {
          BRB_train_period_num += 1;

          BRB_period_attempts = 0;
          BRB_period_successes = 0;
          BRB_period_failures = 0;
          BRB_period_resets = 0;

          BRB_period_rightEarly = 0;
          BRB_period_leftTimeout = 0;
          BRB_period_switchTimeout = 0;

          BRB_req_left = BRB_stage;
          BRB_redraw_left = false;
          fed3.RPR_Left = BRB_req_left;

          BRB_updateScreen();   // keep LCD in sync with current stage
        }

        // green on both ports and the bottom strip, marks period onset
        fed3.pixelsOn(0, 10, 0, 0);
        fed3.leftPokePixel(0, 10, 0, 0);
        fed3.rightPokePixel(0, 10, 0, 0);

        fed3.setEvent("PeriodStart");
        fed3.logSpecialEvent();
        fed3.Tone(4000, 1200);  // non-blocking, tone() returns immediately
        delay(1200);
        fed3.pixelsOff();

        // shaping: free pellet at onset of the first BRB_period_init periods
       if (BRB_period_num <= BRB_period_init) {
          fed3.ConditionedStimulus(200, 4000, 'G', true, false);
          fed3.Feed(false, 'G');
          // ADDED: full active period from pellet retrieval
          BRB_clock = millis();
        }
        BRB_resetTrial();  // left active, right dark, fresh draw
        BRB_paint = 0;     // repaint immediately

      } else {
        // still dark between periods: passive logging only
        if (fed3.Left) fed3.logLeftPoke();
        if (fed3.Right) fed3.logRightPoke();
        fed3.run();
        return;
      }

    } else if (millis() - BRB_clock > BRB_period_ms) {
      BRB_in_period = false;
      BRB_clock = millis();

      fed3.leftPokePixel(0, 0, 0, 0);
      fed3.rightPokePixel(0, 0, 0, 0);
      fed3.pixelsOff();

      fed3.setEvent("PeriodEnd");
      fed3.logSpecialEvent();

      // TRAINING ADDITION: every active period is preserved as a summary,
      // and stage advancement is evaluated only after 10 completed periods.
      if (BRB_training) {
        BRB_logTrainingPeriodSummary();

        if (BRB_train_period_num >= 10) {
          bool BRB_passed = BRB_stagePasses();
          BRB_logTrainingStageSummary(BRB_passed);

          if (BRB_passed) {
            if (BRB_stage < BRB_max_stage) {
              BRB_stage += 1;
              BRB_resetStageCounters();
              BRB_req_left = BRB_stage;
              BRB_redraw_left = false;

              BRB_updateScreen();   // show new stage on LCD
              fed3.setEvent("TrainingStageAdvance");
              fed3.logSpecialEvent();
                        } else {
              // Final training stage is complete. The next active period is
              // the final randomized Barbosa task.
              BRB_training = false;
              BRB_training_complete = true;

              // Start the full Barbosa session clock only after all three
              // training stages have been completed.
              BRB_session_start = millis();

              // ADDED: strict production windows + 10 min active / 20 min dark
              BRB_left_window_ms   = BRB_prod_left_window_ms;    // 10 s
              BRB_switch_window_ms = BRB_prod_switch_window_ms;  // 10 s
              BRB_period_ms        = BRB_prod_period_ms;         // 10 min on
              BRB_wait_ms          = BRB_prod_wait_ms;           // 20 min off

              BRB_updateScreen();   // switch LCD to "Barbosa "
              fed3.setEvent("TrainingComplete");
              fed3.logSpecialEvent();

              // Keep final Barbosa bookkeeping separate from shaping.
              BRB_trial_count = 0;
              BRB_fail_count = 0;
              BRB_omit_count = 0;
              fed3.FR_Count_Left = 0;
              fed3.FR_Count_Right = 0;
            }
          } else {
            // Repeat the same LP requirement for another 10-period block.
            BRB_resetStageCounters();

            BRB_req_left = BRB_stage;
            BRB_redraw_left = false;

            BRB_updateScreen();   // re-assert same stage on LCD
            fed3.setEvent("TrainingStageRepeat");
            fed3.logSpecialEvent();
          }
        }
      }

      fed3.Tone(4000, 1200);
      fed3.run();
      return;
    }

    // ------------------------------------------------------------- paint
    // The library clears the WHOLE strip and drops MOTOR_ENABLE inside
    // pixelsOff(), which ConditionedStimulus() and Feed() both call. That is | PI
    // why the house light only flashes. Nothing here can be a one-shot: the
    // lights have to be re-asserted, so every cue is followed by a repaint.
    // Phase is read off the count, same as everywhere else in this mode.
     // ------------------------------------------------------------- paint
    // Force strip (house) dark every cycle, then only the active port. | PII
    // CS/Feed turn the strip on; without clearing here it would stay on.
    // CHANGED: clear strip pixels 0-7 in the buffer only (no show, no power
    // cut). The port-pixel calls below each show() the full 10-pixel frame, so
    // strip-off and port state go out together. pixelsOff() here blanked the
    // port cue and cut pixel power for a few ms every 200 ms.
    // (fed3.strip is public in stock FED3.h.)
    if (millis() - BRB_paint > 200) {
      BRB_paint = millis();
      for (uint16_t i = 0; i < 8; i++) fed3.strip.setPixelColor(i, 0, 0, 0, 0);  // house off, buffer only

      if (BRB_count_left > 0 && BRB_count_left >= BRB_req_left) {
        fed3.leftPokePixel(0, 0, 0, 0);
        fed3.rightPokePixel(0, 10, 0, 0);  // switch armed
      } else {
        fed3.leftPokePixel(0, 10, 0, 0);  // left active
        fed3.rightPokePixel(0, 0, 0, 0);
      }
    }

    // ------------------------------------------------------ TRAINING draw
    // During shaping, the LP requirement is fixed by BRB_stage. This block
    // clears the redraw latch before the existing random-draw code runs, so
    // the original Barbosa randomization remains untouched for the final task.
    if (BRB_training && BRB_redraw_left) {
      BRB_req_left = BRB_stage;
      fed3.RPR_Left = BRB_req_left;
      BRB_redraw_left = false;

      String BRB_event = "TrainingDraw_S";
      BRB_event += String(BRB_stage);
      fed3.setEvent(BRB_event);
      fed3.logSpecialEvent();
    }

    // ------------------------------------------------------------------ draw
    if (BRB_redraw_left) {
      BRB_req_left = random(BRB_lower_bound_left, BRB_upper_bound_left + 1);
      fed3.RPR_Left = BRB_req_left;
      BRB_redraw_left = false;
      fed3.setEvent("Redraw");
      fed3.logSpecialEvent();
    }

    // ---------------------------------------------------------- window check
    // count == 0 means nothing is running, so this is also the latch: the
    // reset zeroes the count and the branch cannot re-enter. Both timeouts
    // do the same thing, so they are one branch with the window and the
    // label picked off the phase.
    if (BRB_count_left > 0) {
      bool BRB_armed = (BRB_count_left >= BRB_req_left);
      unsigned long BRB_window = BRB_armed ? BRB_switch_window_ms : BRB_left_window_ms;

      if ((millis() - BRB_timer) > BRB_window) {
        BRB_fail_count += 1;

        // TRAINING ADDITION: every timeout terminates the current attempt.
        if (BRB_training) {
          BRB_period_failures += 1;
          BRB_period_resets += 1;
          BRB_stage_failures += 1;
          BRB_stage_resets += 1;

          if (BRB_armed) {
            BRB_period_switchTimeout += 1;
            BRB_stage_switchTimeout += 1;
          } else {
            BRB_period_leftTimeout += 1;
            BRB_stage_leftTimeout += 1;
          }
        }

        fed3.setEvent(BRB_armed ? "SwitchTimeout" : "LeftTimeout");
        fed3.logSpecialEvent();
        BRB_resetTrial();
        BRB_paint = 0;
      }
    }

    // ------------------------------------------------------------- LEFT PORT
    if (fed3.Left) {
      fed3.logLeftPoke();

      // TRAINING ADDITION: an attempt begins with the first left-port poke.
      if (BRB_training && BRB_count_left == 0) {
        BRB_period_attempts += 1;
        BRB_stage_attempts += 1;
      }

      BRB_count_left += 1;
      fed3.RPR_Count_Left = BRB_count_left;

      if (BRB_count_left == 1) BRB_timer = millis();  // bout-level timer

      if (BRB_count_left == BRB_req_left) {
        // requirement met, exactly once
        //fed3.ConditionedStimulus(200, 4000, 'G', true, false);  // left-success tone, NO pellet - as of 9/28 this is commented out for training purposes; it may be ineffective. 
        BRB_timer = millis();         // BRB_switch_window_ms
        BRB_paint = 0;                  // left dark, right green, house back on
      }
      // BRB_count_left > BRB_req_left: surplus pokes during the change-over
      // window. Logged and counted, no cue, no penalty, window not extended.
    }

    // ------------------------------------------------------------ RIGHT PORT
    if (fed3.Right) {
      fed3.logRightPoke();

      // ADDED: no trial open — log only (no failure / reset / payout)
      if (BRB_count_left == 0) {

      } else if (BRB_count_left < BRB_req_left) {
        // switched early: trial forfeited, requirement rerolled
        BRB_fail_count += 1;

        // TRAINING ADDITION: RightEarly is a failed/reset sequence.
        if (BRB_training) {
          BRB_period_failures += 1;
          BRB_period_resets += 1;
          BRB_stage_failures += 1;
          BRB_stage_resets += 1;
          BRB_period_rightEarly += 1;
          BRB_stage_rightEarly += 1;
        }

        fed3.setEvent("RightEarly");
        fed3.logSpecialEvent();
        BRB_resetTrial();

      } else if (BRB_count_right >= (BRB_req_right-1)) {  // minus 1 because we update post check
        // single right poke inside the change-over window completes the trial
        BRB_trial_count += 1;
        fed3.FR_Count_Left = BRB_trial_count;

        fed3.ConditionedStimulus(200, 4000, 'G', true, false);  // cue always fires
        if (random(100) >= BRB_omit_pct) {
          fed3.Feed(false, 'G');

          // TRAINING ADDITION: a pellet is the definition of success.
          if (BRB_training) {
            BRB_period_successes += 1;
            BRB_stage_successes += 1;
          }
        } else {
          BRB_omit_count += 1;  // omission trial

          // TRAINING ADDITION: no pellet means the attempt did not succeed.
          if (BRB_training) {
            BRB_fail_count += 1;
            BRB_period_failures += 1;
            BRB_period_resets += 1;
            BRB_stage_failures += 1;
            BRB_stage_resets += 1;
          }

          fed3.FR_Count_Right = BRB_omit_count;
          fed3.setEvent("Omission");
          fed3.logSpecialEvent();
          delay(1000);
        }

        BRB_resetTrial();
      }

      else {
        BRB_count_right += 1;
      }

      BRB_paint = 0;
    }

    fed3.run();
    return;  // don't run twice
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                     Mode 6: Bellagio 48 HOURS
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (fed3.FEDmode == 6) {
    fed3.sessiontype = "Bellagio-48h";
    fed3.DisplayPokes = true;
    fed3.DisplayTimed = false;

    if (millis() - millis_start > timeout_time) {
      fed3.run();
      return;
    }

    // Two Sections Left : FR and Right : RPR
    if (FR_init_count_left < FR_init_limit_left || (millis() - FR_timer_left < FR_timeout_ms_perpoke)) {
      fed3.leftPokePixel(0, 0, 10, 0);
    } else if (FR_count_left > 0 && millis() - FR_timer_left > FR_timeout_ms_perpoke) {
      FR_count_left = 0;
      fed3.RPR_Left = RPR_req_left;
      fed3.FR_Count_Left = FR_count_left;
      FR_timer_left = 0;
      fed3.leftPokePixel(0, 0, 0, 0);
    }

    if (RPR_init_count_right < RPR_init_limit_right || (millis() - RPR_timer_right < RPR_timeout_ms)) {
      fed3.RPR_Right = RPR_req_right;
      fed3.rightPokePixel(10, 10, 0, 0);
    } else if (RPR_count_right > 0 && millis() - RPR_timer_right > RPR_timeout_ms) {
      RPR_count_right = 0;
      RPR_req_right = random(RPR_lower_bound_right, RPR_upper_bound_right + 1);
      fed3.RPR_Right = RPR_req_right;
      fed3.RPR_Count_Right = RPR_count_right;
      RPR_timer_right = 0;
      fed3.rightPokePixel(0, 0, 0, 0);
    }

    // FR : left port, fixed ratio, 1 pellet
    if (fed3.Left) {
      fed3.logLeftPoke();
      if (FR_init_count_left < FR_init_limit_left) {
        // Initial feeding.
        fed3.ConditionedStimulus('B');
        fed3.Feed('B');

        FR_init_count_left += 1;
      } else {
        // Paradigm feeding.
        FR_count_left += 1;
        fed3.FR_Count_Left = FR_count_left;

        // Turn on the left poke light and start the timer.
        fed3.leftPokePixel(0, 0, 10, 0);
        FR_timer_left = millis();

        if (FR_count_left >= FR_req_left) {
          fed3.ConditionedStimulus('B');
          fed3.Feed('B');

          FR_count_left = 0;
          fed3.FR_Count_Left = FR_count_left;

          FR_timer_left = 0;
        }
      }
    }

    // RPR : right port, random ratio, 1 pellet
    if (fed3.Right) {
      fed3.logRightPoke();
      if (RPR_init_count_right < RPR_init_limit_right) {
        // Initial feeding.
        fed3.ConditionedStimulus('Y');
        fed3.Feed('Y');

        RPR_init_count_right += 1;
      } else {
        // Paradigm feeding.
        RPR_count_right += 1;
        fed3.RPR_Count_Right = RPR_count_right;

        // Turn on the right poke light and start the timer.
        fed3.rightPokePixel(10, 10, 0, 0);
        RPR_timer_right = millis();

        if (RPR_count_right >= RPR_req_right) {
          fed3.ConditionedStimulus('Y');
          fed3.Feed('Y');

          RPR_count_right = 0;
          fed3.RPR_Count_Right = RPR_count_right;
          RPR_req_right = random(RPR_lower_bound_right, RPR_upper_bound_right + 1);
          fed3.RPR_Right = RPR_req_right;

          RPR_timer_right = 0;
        }
      }
    }

    fed3.run();
    return;  // don't run twice
  }
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                    Mode 7: FR Split 48 HOURS
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (fed3.FEDmode == 7) {
    fed3.sessiontype = "FR-Split-48h";
    fed3.DisplayPokes = true;
    fed3.DisplayTimed = false;


    if (millis() - millis_start > timeout_time) {
      fed3.run();
      return;
    }

    if (FR_init_count_left < FR_init_limit_left || (millis() - FR_timer_left < FR_timeout_ms_perpoke)) {
      fed3.leftPokePixel(0, 0, 10, 0);
    } else if (millis() - FR_timer_left > FR_timeout_ms_perpoke) {
      FR_count_left = 0;
      fed3.FR_Count_Left = FR_count_left;
      fed3.leftPokePixel(0, 0, 0, 0);
    }

    // Two Sections Left : FR and Right : FR
    if (FR_init_count_right < FR_init_limit_right || (millis() - FR_timer_right < FR_timeout_ms_perpoke)) {
      fed3.rightPokePixel(10, 10, 0, 0);
    } else if (millis() - FR_timer_right > FR_timeout_ms_perpoke) {
      FR_count_right = 0;
      fed3.FR_Count_Right = FR_count_right;
      fed3.rightPokePixel(0, 0, 0, 0);
    }

    // FR3 : First 10 pokes = 10 pellets.
    if (fed3.Left) {
      fed3.logLeftPoke();
      if (FR_init_count_left < FR_init_limit_left) {
        // Initial feeding.
        fed3.ConditionedStimulus('B');
        fed3.Feed('B');

        FR_init_count_left += 1;
      } else {
        // Paradigm feeding.
        FR_count_left += 1;
        fed3.FR_Count_Left = FR_count_left;

        // Turn on the left poke light and start the timer.
        fed3.leftPokePixel(0, 0, 10, 0);
        FR_timer_left = millis();

        if (FR_count_left >= FR_req_left) {
          fed3.ConditionedStimulus('B');
          fed3.Feed('B');

          FR_count_left = 0;
          fed3.FR_Count_Left = FR_count_left;

          FR_timer_left = 0;
        }
      }
    }

    // FR3 : First 10 pokes = 10 pellets.
    if (fed3.Right) {
      fed3.logRightPoke();
      if (FR_init_count_right < FR_init_limit_right) {
        // Initial feeding.
        fed3.ConditionedStimulus('Y');
        fed3.Feed('Y');

        FR_init_count_right += 1;
      } else {
        // Paradigm feeding.
        FR_count_right += 1;
        fed3.FR_Count_Right = FR_count_right;

        // Turn on the right poke light and start the timer.
        fed3.rightPokePixel(10, 10, 0, 0);
        FR_timer_right = millis();

        if (FR_count_right >= FR_req_right) {
          fed3.ConditionedStimulus('Y');
          fed3.Feed('Y');

          FR_count_right = 0;
          fed3.FR_Count_Right = FR_count_right;

          FR_timer_right = 0;
        }
      }
    }

    fed3.run();
    return;  // don't run twice
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                     Mode 8: RPR Split 48 HOURS
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (fed3.FEDmode == 8) {
    fed3.sessiontype = "RPR-Split-48h";
    fed3.DisplayPokes = true;
    fed3.DisplayTimed = false;


    if (millis() - millis_start > timeout_time) {
      fed3.run();
      return;
    }

    // Two Sections Left : RPR and Right : RPR
    if (RPR_init_count_left < RPR_init_limit_left || (millis() - RPR_timer_left < RPR_timeout_ms)) {
      fed3.RPR_Left = RPR_req_left;
      fed3.leftPokePixel(10, 10, 0, 0);
    } else if (millis() - RPR_timer_left > RPR_timeout_ms) {
      RPR_count_left = 0;
      fed3.RPR_Left = RPR_req_left;
      fed3.RPR_Count_Left = RPR_count_left;
      fed3.leftPokePixel(0, 0, 0, 0);
    }

    if (RPR_init_count_right < RPR_init_limit_right || (millis() - RPR_timer_right < RPR_timeout_ms)) {
      fed3.RPR_Right = RPR_req_right;
      fed3.rightPokePixel(10, 10, 0, 0);
    } else if (millis() - RPR_timer_right > RPR_timeout_ms) {
      RPR_count_right = 0;
      fed3.RPR_Right = RPR_req_right;
      fed3.RPR_Count_Right = RPR_count_right;
      fed3.rightPokePixel(0, 0, 0, 0);
    }

    // RPR : First 10 pokes = 2 pellets.
    if (fed3.Left) {
      fed3.logLeftPoke();
      if (RPR_init_count_left < RPR_init_limit_left) {
        // Initial feeding.
        for (int i = 0; i < 4; i++) {
          fed3.ConditionedStimulus('Y');
          delay(100);
        }

        fed3.Feed('Y');
        fed3.run();
        delay(2000);
        fed3.ConditionedStimulus('Y');
        delay(100);
        fed3.Feed('Y');
        delay(100);

        RPR_init_count_left += 1;
      } else {
        // Paradigm feeding.
        RPR_count_left += 1;
        fed3.RPR_Count_Left = RPR_count_left;

        // Turn on the left poke light and start the timer.
        fed3.leftPokePixel(10, 10, 0, 0);
        RPR_timer_left = millis();

        if (RPR_count_left >= RPR_req_left) {
          for (int i = 0; i < 4; i++) {
            fed3.ConditionedStimulus('Y');
            delay(100);
          }

          fed3.Feed('Y');
          fed3.run();
          delay(2000);
          fed3.ConditionedStimulus('Y');
          delay(100);
          fed3.Feed('Y');
          delay(100);

          RPR_count_left = 0;
          fed3.RPR_Count_Left = RPR_count_left;
          RPR_req_left = random(RPR_lower_bound_left, RPR_upper_bound_left + 1);
          fed3.RPR_Left = RPR_req_left;

          RPR_timer_left = 0;
        }
      }
    }


    // RPR : First 10 pokes = 2 pellets.
    if (fed3.Right) {
      fed3.logRightPoke();
      if (RPR_init_count_right < RPR_init_limit_right) {
        // Initial feeding.
        for (int i = 0; i < 4; i++) {
          fed3.ConditionedStimulus('Y');
          delay(100);
        }

        fed3.Feed('Y');
        fed3.run();
        delay(2000);
        fed3.ConditionedStimulus('Y');
        delay(100);
        fed3.Feed('Y');
        delay(100);

        RPR_init_count_right += 1;
      } else {
        // Paradigm feeding.
        RPR_count_right += 1;
        fed3.RPR_Count_Right = RPR_count_right;

        // Turn on the right poke light and start the timer.
        fed3.rightPokePixel(10, 10, 0, 0);
        RPR_timer_right = millis();

        if (RPR_count_right >= RPR_req_right) {
          for (int i = 0; i < 4; i++) {
            fed3.ConditionedStimulus('Y');
            delay(100);
          }

          fed3.Feed('Y');
          fed3.run();
          delay(2000);
          fed3.ConditionedStimulus('Y');
          delay(100);
          fed3.Feed('Y');
          delay(100);

          RPR_count_right = 0;
          fed3.RPR_Count_Right = RPR_count_right;
          RPR_req_right = random(RPR_lower_bound_right, RPR_upper_bound_right + 1);
          fed3.RPR_Right = RPR_req_right;

          RPR_timer_right = 0;
        }
      }
    }

    fed3.run();
    return;  // don't run twice
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                     Mode 9: Medusa 48h
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (fed3.FEDmode == 9) {
    fed3.sessiontype = "Medusa-48h";
    fed3.DisplayPokes = true;
    fed3.DisplayTimed = false;


    if (millis() - millis_start > timeout_time) {
      fed3.run();
      return;
    }

    // Two Sections Left : RPR and Right : RPR
    if (MED_init_count_left < MED_init_limit_left || (millis() - MED_timer_left < MED_timeout_ms)) {
      fed3.RPR_Left = MED_req_left;
      fed3.leftPokePixel(0, 0, 10, 0);
    } else if (MED_count_left > 0 && millis() - MED_timer_left > MED_timeout_ms) {
      MED_count_left = 0;
      MED_req_left = random(MED_lower_bound_left, MED_upper_bound_left + 1);
      fed3.RPR_Left = MED_req_left;
      fed3.RPR_Count_Left = MED_count_left;
      MED_timer_left = 0;
      fed3.leftPokePixel(0, 0, 0, 0);
    }

    if (MED_init_count_right < MED_init_limit_right || (millis() - MED_timer_right < MED_timeout_ms)) {
      fed3.RPR_Right = MED_req_right;
      fed3.rightPokePixel(10, 10, 0, 0);
    } else if (MED_count_right > 0 && millis() - MED_timer_right > MED_timeout_ms) {
      MED_count_right = 0;
      MED_req_right = random(MED_lower_bound_right, MED_upper_bound_right + 1);
      fed3.RPR_Right = MED_req_right;
      fed3.RPR_Count_Right = MED_count_right;
      MED_timer_right = 0;
      fed3.rightPokePixel(0, 0, 0, 0);
    }

    // RPR : left port, narrow draw, 1 pellet
    if (fed3.Left) {
      fed3.logLeftPoke();
      if (MED_init_count_left < MED_init_limit_left) {
        // Initial feeding.
        fed3.ConditionedStimulus('B');
        fed3.Feed('B');

        MED_init_count_left += 1;
      } else {
        // Paradigm feeding.
        MED_count_left += 1;
        fed3.RPR_Count_Left = MED_count_left;

        // Turn on the left poke light and start the timer.
        fed3.leftPokePixel(0, 0, 10, 0);
        MED_timer_left = millis();

        if (MED_count_left >= MED_req_left) {
          fed3.ConditionedStimulus('B');
          fed3.Feed('B');

          MED_count_left = 0;
          fed3.RPR_Count_Left = MED_count_left;
          MED_req_left = random(MED_lower_bound_left, MED_upper_bound_left + 1);
          fed3.RPR_Left = MED_req_left;

          MED_timer_left = 0;
        }
      }
    }


    // RPR : right port, wide draw, 1 pellet
    if (fed3.Right) {
      fed3.logRightPoke();
      if (MED_init_count_right < MED_init_limit_right) {
        // Initial feeding.
        fed3.ConditionedStimulus('Y');
        fed3.Feed('Y');

        MED_init_count_right += 1;
      } else {
        // Paradigm feeding.
        MED_count_right += 1;
        fed3.RPR_Count_Right = MED_count_right;

        // Turn on the right poke light and start the timer.
        fed3.rightPokePixel(10, 10, 0, 0);
        MED_timer_right = millis();

        if (MED_count_right >= MED_req_right) {
          fed3.ConditionedStimulus('Y');
          fed3.Feed('Y');

          MED_count_right = 0;
          fed3.RPR_Count_Right = MED_count_right;
          MED_req_right = random(MED_lower_bound_right, MED_upper_bound_right + 1);
          fed3.RPR_Right = MED_req_right;

          MED_timer_right = 0;
        }
      }
    }

    fed3.run();
    return;  // don't run twice
  }
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                     Mode 10: Sparrow 48h
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (fed3.FEDmode == 10) {
    fed3.sessiontype = "Sparrow-48h";
    fed3.DisplayPokes = true;
    fed3.DisplayTimed = false;

    // ---- session timeout (disabled) ----
    // if (millis() - millis_start > timeout_time) {
    //   fed3.run();
    //   return;
    // }

    // ------------------------------------------------------------------ draws
    if (SPR_redraw_left) {
      SPR_req_left = random(SPR_lower_bound_left, SPR_upper_bound_left + 1);  // random() excludes the upper bound
      SPR_window_left = (unsigned long)SPR_req_left * SPR_ms_per_poke;
      if (SPR_window_left < SPR_window_floor) SPR_window_left = SPR_window_floor;
      fed3.RPR_Left = SPR_req_left;
      SPR_redraw_left = false;
    }

    if (SPR_redraw_right) {
      if (random(100) < SPR_short_pct_right) {
        SPR_req_right = random(SPR_short_lo_right, SPR_short_hi_right + 1);
      } else {
        SPR_req_right = random(SPR_long_lo_right, SPR_long_hi_right + 1);
      }
      SPR_window_right = (unsigned long)SPR_req_right * SPR_ms_per_poke;
      if (SPR_window_right < SPR_window_floor) SPR_window_right = SPR_window_floor;
      fed3.RPR_Right = SPR_req_right;
      SPR_redraw_right = false;
    }

    // ------------------------------------------- LEFT: window, forfeit, light
    // the (SPR_count_left > 0) test is the latch: the branch zeroes it, so it
    // cannot re-enter and fire repeatedly
    if (SPR_init_count_left < SPR_init_limit_left) {
      fed3.leftPokePixel(0, 0, 10, 0);
    } else if (SPR_count_left > 0 && (millis() - SPR_timer_left) > SPR_window_left) {
      SPR_count_left = 0;
      fed3.RPR_Count_Left = SPR_count_left;
      SPR_timer_left = 0;
      SPR_redraw_left = true;
      fed3.leftPokePixel(0, 0, 0, 0);
    } else if (SPR_count_left > 0) {
      fed3.leftPokePixel(0, 0, 10, 0);
    } else {
      fed3.leftPokePixel(0, 0, 0, 0);
    }

    // ------------------------------------------ RIGHT: window, forfeit, light
    if (SPR_init_count_right < SPR_init_limit_right) {
      fed3.rightPokePixel(10, 10, 0, 0);
    } else if (SPR_count_right > 0 && (millis() - SPR_timer_right) > SPR_window_right) {
      SPR_count_right = 0;
      fed3.RPR_Count_Right = SPR_count_right;
      SPR_timer_right = 0;
      SPR_redraw_right = true;
      fed3.rightPokePixel(0, 0, 0, 0);
    } else if (SPR_count_right > 0) {
      fed3.rightPokePixel(10, 10, 0, 0);
    } else {
      fed3.rightPokePixel(0, 0, 0, 0);
    }

    // ----------------------------------------------------------- LEFT PORT
    if (fed3.Left) {
      fed3.logLeftPoke();

      // CHANGE-OVER COST: poking left kills any right bout in progress and
      // rerolls the right requirement. Inert during init and when idle.
      if (SPR_count_right > 0) {
        SPR_count_right = 0;
        fed3.RPR_Count_Right = SPR_count_right;
        SPR_timer_right = 0;
        SPR_redraw_right = true;
        fed3.rightPokePixel(0, 0, 0, 0);
      }

      if (SPR_init_count_left < SPR_init_limit_left) {
        // Initial feeding. FR1, one pellet.
        fed3.ConditionedStimulus('B');
        fed3.Feed('B');

        SPR_init_count_left += 1;

      } else {
        // Paradigm feeding.
        SPR_count_left += 1;
        fed3.RPR_Count_Left = SPR_count_left;
        fed3.leftPokePixel(0, 0, 10, 0);

        if (SPR_count_left == 1) SPR_timer_left = millis();  // bout-level timer

        if (SPR_count_left >= SPR_req_left) {
          fed3.ConditionedStimulus('B');  // cue always fires
          if (random(100) >= SPR_omit_pct) {
            fed3.Feed('B');
          } else {
            SPR_omit_count_left += 1;  // omission trial
            fed3.FR_Count_Left = SPR_omit_count_left;
            delay(1000);
          }

          SPR_count_left = 0;
          fed3.RPR_Count_Left = SPR_count_left;
          SPR_timer_left = 0;
          SPR_redraw_left = true;
          fed3.leftPokePixel(0, 0, 0, 0);
        }
      }
    }

    // ---------------------------------------------------------- RIGHT PORT
    if (fed3.Right) {
      fed3.logRightPoke();

      // CHANGE-OVER COST: poking right kills any left bout in progress and
      // rerolls the left requirement.
      if (SPR_count_left > 0) {
        SPR_count_left = 0;
        fed3.RPR_Count_Left = SPR_count_left;
        SPR_timer_left = 0;
        SPR_redraw_left = true;
        fed3.leftPokePixel(0, 0, 0, 0);
      }

      if (SPR_init_count_right < SPR_init_limit_right) {
        // Initial feeding. FR1, one pellet, cue matched to the left port.
        fed3.ConditionedStimulus('B');
        fed3.Feed('B');

        SPR_init_count_right += 1;

      } else {
        // Paradigm feeding.
        SPR_count_right += 1;
        fed3.RPR_Count_Right = SPR_count_right;
        fed3.rightPokePixel(10, 10, 0, 0);

        if (SPR_count_right == 1) SPR_timer_right = millis();

        if (SPR_count_right >= SPR_req_right) {
          fed3.ConditionedStimulus('B');
          if (random(100) >= SPR_omit_pct) {
            fed3.Feed('B');
          } else {
            SPR_omit_count_right += 1;
            fed3.FR_Count_Right = SPR_omit_count_right;
            delay(1000);
          }

          SPR_count_right = 0;
          fed3.RPR_Count_Right = SPR_count_right;
          SPR_timer_right = 0;
          SPR_redraw_right = true;
          fed3.rightPokePixel(0, 0, 0, 0);
        }
      }
    }

    fed3.run();
    return;  // don't run twice
  }
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //                                              Mode 11: Bellagio 48 HOURS -- PORT-SWAPPED (FR on RIGHT)
  //   Mirror of mode 6. LED colour stays with the physical port: left blue, right yellow.
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (fed3.FEDmode == 11) {
    fed3.sessiontype = "Bellagio-48h-R";
    fed3.DisplayPokes = true;
    fed3.DisplayTimed = false;

    if (millis() - millis_start > timeout_time) {
      fed3.run();
      return;
    }

    // Two Sections Left : RPR and Right : FR
    if (RPR_init_count_left < RPR_init_limit_left || (millis() - RPR_timer_left < RPR_timeout_ms)) {
      fed3.RPR_Left = RPR_req_left;
      fed3.leftPokePixel(0, 0, 10, 0);
    } else if (RPR_count_left > 0 && millis() - RPR_timer_left > RPR_timeout_ms) {
      RPR_count_left = 0;
      RPR_req_left = random(RPR_lower_bound_left, RPR_upper_bound_left + 1);
      fed3.RPR_Left = RPR_req_left;
      fed3.RPR_Count_Left = RPR_count_left;
      RPR_timer_left = 0;
      fed3.leftPokePixel(0, 0, 0, 0);
    }

    if (FR_init_count_right < FR_init_limit_right || (millis() - FR_timer_right < FR_timeout_ms_perpoke)) {
      fed3.rightPokePixel(10, 10, 0, 0);
    } else if (FR_count_right > 0 && millis() - FR_timer_right > FR_timeout_ms_perpoke) {
      FR_count_right = 0;
      fed3.FR_Count_Right = FR_count_right;
      FR_timer_right = 0;
      fed3.rightPokePixel(0, 0, 0, 0);
    }

    // RPR : left port, random ratio, 1 pellet
    if (fed3.Left) {
      fed3.logLeftPoke();
      if (RPR_init_count_left < RPR_init_limit_left) {
        // Initial feeding.
        fed3.ConditionedStimulus('B');
        fed3.Feed('B');

        RPR_init_count_left += 1;
      } else {
        // Paradigm feeding.
        RPR_count_left += 1;
        fed3.RPR_Count_Left = RPR_count_left;

        // Turn on the left poke light and start the timer.
        fed3.leftPokePixel(0, 0, 10, 0);
        RPR_timer_left = millis();

        if (RPR_count_left >= RPR_req_left) {
          fed3.ConditionedStimulus('B');
          fed3.Feed('B');

          RPR_count_left = 0;
          fed3.RPR_Count_Left = RPR_count_left;
          RPR_req_left = random(RPR_lower_bound_left, RPR_upper_bound_left + 1);
          fed3.RPR_Left = RPR_req_left;

          RPR_timer_left = 0;
        }
      }
    }

    // FR : right port, fixed ratio, 1 pellet
    if (fed3.Right) {
      fed3.logRightPoke();
      if (FR_init_count_right < FR_init_limit_right) {
        // Initial feeding.
        fed3.ConditionedStimulus('Y');
        fed3.Feed('Y');

        FR_init_count_right += 1;
      } else {
        // Paradigm feeding.
        FR_count_right += 1;
        fed3.FR_Count_Right = FR_count_right;

        // Turn on the right poke light and start the timer.
        fed3.rightPokePixel(10, 10, 0, 0);
        FR_timer_right = millis();

        if (FR_count_right >= FR_req_right) {
          fed3.ConditionedStimulus('Y');
          fed3.Feed('Y');

          FR_count_right = 0;
          fed3.FR_Count_Right = FR_count_right;

          FR_timer_right = 0;
        }
      }
    }

    fed3.run();
    return;  // don't run twice
  }
  fed3.run();
}