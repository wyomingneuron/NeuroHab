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
     0  Free_feed              6  Bellagio-48h
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
int FR_init_limit_right = 5;  // MODE 7 (FR-Split) + MODE 11 (Bellagio-R)
int FR_init_count_left = 0;
int FR_init_limit_left = 5;  // MODE 6 (Bellagio) + MODE 7 (FR-Split)

int FR_req_right = 1;    // CHANGED 1->4. MODE 7 (FR-Split) + MODE 11 (Bellagio-R)
int FR_count_right = 0;  // MODE 7 (FR-Split) + MODE 11 (Bellagio-R)
int FR_req_left = 5;     // CHANGED 5->4. MODE 6 (Bellagio) + MODE 7 (FR-Split)
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
const unsigned long BRB_session_ms = 172800000;  // 48 h
bool BRB_session_over = false;

// ---- period clock ----
unsigned long BRB_wait_ms = 1800000;    // 30 min dark / passive-logging gap 1800000
unsigned long BRB_period_ms = 180000;  // 180 s feeding period              180000
unsigned long BRB_clock = 0;          // marks the start of the current phase
bool BRB_in_period = false;
int BRB_period_num = 0;   // periods elapsed this session
int BRB_period_init = 3;  // free pellet at onset of first N periods

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
unsigned long BRB_left_window_ms = 10000;   // RAISE TO ~30000 FOR SHAPING
unsigned long BRB_switch_window_ms = 5000;  // RAISE TO ~15000 FOR SHAPING

// ---- omission trials ----
int BRB_omit_pct = 10;
int BRB_omit_count = 0;

// ---- bookkeeping (rides FR_Count_Left, only ever increments) ----
int BRB_trial_count = 0;  // completed trials (pellet or omission)
int BRB_fail_count = 0;


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
// Barbosa Globals End ////////////////////////////////////////////////////////////////////////////

float randomFloat(float min, float max) {  // DEAD -- only MODE 10 (DP_Hold) called this
  return min + (float)random() / (float)RAND_MAX * (max - min);
}

/////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

  ran_range = randomFloat(r1, r2) * 1000;
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
    fed3.logdata();
  }

  // If in idle mode, skip all processing (no logging, no delivery)
  if (isIdle) {
    fed3.run();
    return;  // Skip the rest of the loop
  }
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //                                                                     Mode 0: Free feeding
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (fed3.FEDmode == 0) {
    fed3.sessiontype = "Free_feed";  //The text in "sessiontype" will appear on the screen and in the logfile
    fed3.DisplayPokes = false;       //Turn off poke indicators for free feeding mode
    fed3.Feed();
    fed3.Timeout(5);  //5s timeout
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
    if (fed3.PelletCount < 5) {
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
    fed3.sessiontype = "Barbosa-48h";
    fed3.DisplayPokes = true;
    fed3.DisplayTimed = false;

    // Static local, not a global. Throttles the pixel re-assert below.
    // Set it to 0 anywhere the lights need to change NOW.
    static unsigned long BRB_paint = 0;

    // ------------------------------------------------------ 48 h session stop
    if (millis() - BRB_session_start > BRB_session_ms) {
      if (!BRB_session_over) {
        BRB_session_over = true;
        fed3.leftPokePixel(0, 0, 0, 0);
        fed3.rightPokePixel(0, 0, 0, 0);
        fed3.pixelsOff();
        fed3.setEvent("Idle");
        fed3.logdata();
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

        // green on both ports and the bottom strip, marks period onset
        fed3.pixelsOn(0, 10, 0, 0);
        fed3.leftPokePixel(0, 10, 0, 0);
        fed3.rightPokePixel(0, 10, 0, 0);

        fed3.setEvent("PeriodStart");
        fed3.logdata();
        fed3.Tone(4000, 1200);  // non-blocking, tone() returns immediately
        delay(1200);

        // shaping: free pellet at onset of the first BRB_period_init periods
        if (BRB_period_num <= BRB_period_init) {
          fed3.ConditionedStimulus(200, 4000, 'G', true, false);
          fed3.Feed(false, 'G');
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
      fed3.logdata();
      fed3.Tone(4000, 1200);
      fed3.run();
      return;
    }

    // ------------------------------------------------------------- paint
    // The library clears the WHOLE strip and drops MOTOR_ENABLE inside
    // pixelsOff(), which ConditionedStimulus() and Feed() both call. That is
    // why the house light only flashes. Nothing here can be a one-shot: the
    // lights have to be re-asserted, so every cue is followed by a repaint.
    // Phase is read off the count, same as everywhere else in this mode.
    if (millis() - BRB_paint > 200) {
      BRB_paint = millis();
      fed3.pixelsOn(0, 10, 0, 0);  // house light, strip pixels 0-7

      if (BRB_count_left > 0 && BRB_count_left >= BRB_req_left) {
        fed3.leftPokePixel(0, 0, 0, 0);  // change-over armed
        fed3.rightPokePixel(0, 10, 0, 0);
      } else {
        fed3.leftPokePixel(0, 10, 0, 0);  // left is the active port
        fed3.rightPokePixel(0, 0, 0, 0);
      }
    }

    // ------------------------------------------------------------------ draw
    if (BRB_redraw_left) {
      BRB_req_left = random(BRB_lower_bound_left, BRB_upper_bound_left + 1);
      fed3.RPR_Left = BRB_req_left;
      BRB_redraw_left = false;
      fed3.setEvent("Redraw");
      fed3.logdata();
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
        fed3.setEvent(BRB_armed ? "SwitchTimeout" : "LeftTimeout");
        fed3.logdata();
        BRB_resetTrial();
        BRB_paint = 0;
      }
    }

    // ------------------------------------------------------------- LEFT PORT
    if (fed3.Left) {
      fed3.logLeftPoke();

      BRB_count_left += 1;
      fed3.RPR_Count_Left = BRB_count_left;

      if (BRB_count_left == 1) BRB_timer = millis();  // bout-level timer

      if (BRB_count_left == BRB_req_left) {
        // requirement met, exactly once
        fed3.ConditionedStimulus(200, 4000, 'G', true, false);  // left-success tone, NO pellet
        BRB_timer = millis();           // re-stamp: now the 5 s window
        BRB_paint = 0;                  // left dark, right green, house back on
      }
      // BRB_count_left > BRB_req_left: surplus pokes during the change-over
      // window. Logged and counted, no cue, no penalty, window not extended.
    }

    // ------------------------------------------------------------ RIGHT PORT
    if (fed3.Right) {
      fed3.logRightPoke();

      if (BRB_count_left < BRB_req_left) {
        // switched early: trial forfeited, requirement rerolled
        BRB_fail_count += 1;
        fed3.setEvent("RightEarly");
        fed3.logdata();
        BRB_resetTrial();

      } else if (BRB_count_right >= (BRB_req_right-1)) {  // minus 1 because we update post check
        // single right poke inside the change-over window completes the trial
        BRB_trial_count += 1;
        fed3.FR_Count_Left = BRB_trial_count;

        fed3.ConditionedStimulus(200, 4000, 'G', true, false);  // cue always fires
        if (random(100) >= BRB_omit_pct) {
          fed3.Feed(false, 'G');
        } else {
          BRB_omit_count += 1;  // omission trial
          fed3.FR_Count_Right = BRB_omit_count;
          fed3.setEvent("Omission");
          fed3.logdata();
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