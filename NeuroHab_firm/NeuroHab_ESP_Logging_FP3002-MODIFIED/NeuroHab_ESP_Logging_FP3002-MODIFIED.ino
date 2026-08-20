/*
AUTHOR: Sam Crouse
Email: scrouse2@uwyo.edu
Organization: Sun Lab Wyoming SBC
*/

// Libraries to include.
#include <LiquidCrystal_I2C.h>
#include <time.h>
#include <SdFat.h>
#include <SPI.h>
#include <RTClib.h>
#include <Wire.h>
#include <cstring>
#include <string>  // for double conversions and writing
#include <esp_timer.h>

RTC_DS1307 rtc;

// Initialize LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin definitions
#define PININ_1 5  // FED3 PULSING
#define PININ_2 4  // ARDUINO PULSING

#define SDA_PIN 17
#define SCL_PIN 16

#define SD_CS 12     // Chip Select
#define SPI_SCK 18   // Clock 18
#define SPI_MISO 19  // Master In Slave Out
#define SPI_MOSI 23  // Master Out Slave In 23

// global variables
// Read write variables.
char dirname[32];
String BNC_1_filename = "/BNC_1.csv";
String BNC_2_filename = "/BNC_2.csv";
String BNC_3_filename = "/BNC_3.csv";
String BNC_4_filename = "/BNC_4.csv";

SdFat SD;

int RP = 0;
int LP = 0;
int DISP = 0;
int RETR = 0;
int RW = 0;
int LW = 0;
int COND = 0;
int TONE = 0;
int FW = 0;
int WD = 0;
float RW_QTY = 0;
float LW_QTY = 0;
float FW_QTY = 0;
float WD_QTY = 0.02297;  // ml per WD
float TWD_QTY = 0;               // total water delivered
int lastWaterPort = -1;          // 0 for RW, 1 for LW, 2 for FW

int RWP = 0;
int LWP = 0;
int RDD = 0;
int LDD = 0;
int RIT = 0;
int LIT = 0;

// screen variables
int mousePos = 15;

// timing variables
int screenMillis = 0;  // timestamp of last time we checked
int screenTimeout = 1000;
int clearMillis = 0;
int clearTimeout = 10000;

const int BATCH_WRITE_TIMEOUT_MICROS = 60000;  // << NEEDS TO BE GREATER THAN BOX_BNC_TIMEOUT_MICROS // The time required to elapse before buffers are switched if any buffer is not empty.
const int BOX_BNC_TIMEOUT_MICROS = 40000;      // << NEEDS TO BE LESS THAN BATCH_WRITE_TIMEOUT_MICROS and GREATER THAN BNC_TIMEOUT_MICROS // The pulse count timeout for NeuroHab events. If it takes longer than this to pulse
const int BNC_TIMEOUT_MICROS = 1000;           // << NEEDS TO BE SMALL BUT GREATER THAN MIN_PULSE_TIME_MICROS // The pulse count timeout for non-NeuroHab external BNC events.
const int MIN_PULSE_TIME_MICROS = 400;         // Error check value for how long a pulse must be to be considered a valid recordable pulse.

// pulse variables
volatile uint64_t init_pulse_time = -1;
volatile uint64_t last_pulse_time = 0;

// BNC / TTL INPUT VARIABLES
// Define input pins (ensure they support interrupts)
const int pulsePins[] = { PININ_1, PININ_2, 27, 26, 25 };  // Example GPIO pins // PININ_1 and PININ_2 are one output BNC for all NeuroHAB-AI events.
const int numPins = 4;                                     // 4 provides for 3 channels of BNC IN recording, 3 for 2, 2 for 1, 1 for BOX only events.

// Incoming pulses and times are stored and managed by the following variables and buffers (2D arrays).
const int buffer_size = 90;           // << DEPENDING ON BUFFER DATA TYPES 100 WILL CRASH SYSTEM // how many pulses/times we can store before buffer overflow occurs.         Default 90 prior FED3 total event logging.
const int a_b_switch_count_max = 60;  // << NEEDS TO BE LESS THAN buffer_size - RECOMMEND 49%-80% // How many values we store for writing before triggering buffer switching. Default 60 prior FED3 total event logging.
volatile int a_b_switch_count = 0;    // Tracks buffer increments.
volatile bool a_b_switch = false;     // When false, using _A vars for storage, when true using _B for storage. This tells us which buffer is our active buffer.

// volatile uint32_t pulseCounts[numPins] = {0};
volatile uint32_t pulseCounts_A[numPins][buffer_size] = { 0 };
volatile uint32_t pulseCounts_B[numPins][buffer_size] = { 0 };

// volatile uint64_t first_pulse_times[numPins] = {0};
volatile uint64_t first_pulse_times_A[numPins][buffer_size] = { 0 };
volatile uint64_t first_pulse_times_B[numPins][buffer_size] = { 0 };

// volatile uint64_t recent_pulse_times[numPins] = {0};
volatile uint64_t recent_pulse_times_A[numPins][buffer_size] = { 0 };
volatile uint64_t recent_pulse_times_B[numPins][buffer_size] = { 0 };

// Used to track if the most recent pulse times have been updated for each pin.
volatile uint64_t MRP_LAST_1 = 0;
volatile uint64_t MRP_LAST_2 = 0;
volatile uint64_t MRP_LAST_3 = 0;
volatile uint64_t MRP_LAST_4 = 0;

// volatile uint64_t latency_stamp_1 = 0;
// volatile uint64_t latency_stamp_2 = 0;

// Define a mutex for critical sections
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// Interrupt Service Routines
// Access pulseCounts[0] for handlePulse0/1 because PININ_1 and PININ_2 share the output BNC port.

// BUILTIN BOX LICK EVENTS
void IRAM_ATTR handlePulse1() {
  uint64_t now_micros = esp_timer_get_time();

  const int pinNum = 0;

  if (a_b_switch == false) {
    // Get the index of the most recently set value in the array.
    int index = 0;
    for (int i = 0; i < buffer_size; i++) {
      if (pulseCounts_A[pinNum][i] == 0) {
        break;
      }

      index = i;
    }

    uint64_t most_recent_pulse = recent_pulse_times_A[pinNum][index];

    // Check if this pulse batch hasn't timed out.
    if (now_micros - most_recent_pulse < BOX_BNC_TIMEOUT_MICROS) {
      // Check the pulse was long enough to not be noise.
      if (now_micros - most_recent_pulse > MIN_PULSE_TIME_MICROS) {
        // If this is the first pulse in the batch, set the first pulse time.
        if (pulseCounts_A[pinNum][index] == 0) {
          first_pulse_times_A[pinNum][index] = now_micros;
        }

        pulseCounts_A[pinNum][index] += 1;
        recent_pulse_times_A[pinNum][index] = now_micros;
      }
    }

    // If this batch was timed out, increase the index and record the start of a new batch of pulses.
    else {
      // Check if the pulse was long enough to not be noise.
      if (now_micros - most_recent_pulse > MIN_PULSE_TIME_MICROS) {
        // ensure that the previous value had something written to it.
        if (pulseCounts_A[pinNum][index] != 0) {
          index += 1;
        }

        // If this is the first pulse in the batch, set the first pulse time.
        if (pulseCounts_A[pinNum][index] == 0) {
          first_pulse_times_A[pinNum][index] = now_micros;
        }

        pulseCounts_A[pinNum][index] += 1;
        recent_pulse_times_A[pinNum][index] = now_micros;
      }
    }
  }

  else {
    // Get the index of the most recently set value in the array.
    int index = 0;
    for (int i = 0; i < buffer_size; i++) {
      if (pulseCounts_B[pinNum][i] == 0) {
        break;
      }

      index = i;
    }

    uint64_t most_recent_pulse = recent_pulse_times_B[pinNum][index];

    // Check if this pulse batch has timed out.
    if (now_micros - most_recent_pulse < BOX_BNC_TIMEOUT_MICROS) {
      // Check the pulse was long enough to not be noise.
      if (now_micros - most_recent_pulse > MIN_PULSE_TIME_MICROS) {
        // If this is the first pulse in the batch, set the first pulse time.
        if (pulseCounts_B[pinNum][index] == 0) {
          first_pulse_times_B[pinNum][index] = now_micros;
        }

        pulseCounts_B[pinNum][index] += 1;
        recent_pulse_times_B[pinNum][index] = now_micros;
      }
    }

    // If this batch was timed out, increase the index and record the start of a new batch of pulses.
    else {
      // Check if the pulse was long enough to not be noise.
      if (now_micros - most_recent_pulse > MIN_PULSE_TIME_MICROS) {
        // ensure that the previous value had something written to it.
        if (pulseCounts_B[pinNum][index] != 0) {
          index += 1;
        }

        // If this is the first pulse in the batch, set the first pulse time.
        if (pulseCounts_B[pinNum][index] == 0) {
          first_pulse_times_B[pinNum][index] = now_micros;
        }

        pulseCounts_B[pinNum][index] += 1;
        recent_pulse_times_B[pinNum][index] = now_micros;
      }
    }
  }
}

// BNC IN PORT 3
void IRAM_ATTR handlePulse2() {
  uint64_t now_micros = esp_timer_get_time();

  const int pinNum = 1;

  if (a_b_switch == false) {
    // Get the index of the most recently set value in the array.
    int index = 0;
    for (int i = 0; i < buffer_size; i++) {
      if (pulseCounts_A[pinNum][i] == 0) {
        break;
      }

      index = i;
    }

    uint64_t most_recent_pulse = recent_pulse_times_A[pinNum][index];

    // Check if this pulse batch hasn't timed out.
    if (now_micros - most_recent_pulse < BNC_TIMEOUT_MICROS) {
      // Check the pulse was long enough to not be noise.
      if (now_micros - most_recent_pulse > MIN_PULSE_TIME_MICROS) {
        // If this is the first pulse in the batch, set the first pulse time.
        if (pulseCounts_A[pinNum][index] == 0) {
          first_pulse_times_A[pinNum][index] = now_micros;
        }

        pulseCounts_A[pinNum][index] += 1;
        recent_pulse_times_A[pinNum][index] = now_micros;
      }
    }

    // If this batch was timed out, increase the index and record the start of a new batch of pulses.
    else {
      // Check if the pulse was long enough to not be noise.
      if (now_micros - most_recent_pulse > MIN_PULSE_TIME_MICROS) {
        // ensure that the previous value had something written to it.
        if (pulseCounts_A[pinNum][index] != 0) {
          index += 1;
        }

        // If this is the first pulse in the batch, set the first pulse time.
        if (pulseCounts_A[pinNum][index] == 0) {
          first_pulse_times_A[pinNum][index] = now_micros;
        }

        pulseCounts_A[pinNum][index] += 1;
        recent_pulse_times_A[pinNum][index] = now_micros;
      }
    }
  }

  else {
    // Get the index of the most recently set value in the array.
    int index = 0;
    for (int i = 0; i < buffer_size; i++) {
      if (pulseCounts_B[pinNum][i] == 0) {
        break;
      }

      index = i;
    }

    uint64_t most_recent_pulse = recent_pulse_times_B[pinNum][index];

    // Check if this pulse batch has timed out.
    if (now_micros - most_recent_pulse < BNC_TIMEOUT_MICROS) {
      // Check the pulse was long enough to not be noise.
      if (now_micros - most_recent_pulse > MIN_PULSE_TIME_MICROS) {
        // If this is the first pulse in the batch, set the first pulse time.
        if (pulseCounts_B[pinNum][index] == 0) {
          first_pulse_times_B[pinNum][index] = now_micros;
        }

        pulseCounts_B[pinNum][index] += 1;
        recent_pulse_times_B[pinNum][index] = now_micros;
      }
    }

    // If this batch was timed out, increase the index and record the start of a new batch of pulses.
    else {
      // Check if the pulse was long enough to not be noise.
      if (now_micros - most_recent_pulse > MIN_PULSE_TIME_MICROS) {
        // ensure that the previous value had something written to it.
        if (pulseCounts_B[pinNum][index] != 0) {
          index += 1;
        }

        // If this is the first pulse in the batch, set the first pulse time.
        if (pulseCounts_B[pinNum][index] == 0) {
          first_pulse_times_B[pinNum][index] = now_micros;
        }

        pulseCounts_B[pinNum][index] += 1;
        recent_pulse_times_B[pinNum][index] = now_micros;
      }
    }
  }
}

// BNC IN PORT 4
void IRAM_ATTR handlePulse3() {
  uint64_t now_micros = esp_timer_get_time();

  const int pinNum = 2;

  if (a_b_switch == false) {
    // Get the index of the most recently set value in the array.
    int index = 0;
    for (int i = 0; i < buffer_size; i++) {
      if (pulseCounts_A[pinNum][i] == 0) {
        break;
      }

      index = i;
    }

    uint64_t most_recent_pulse = recent_pulse_times_A[pinNum][index];

    // Check if this pulse batch hasn't timed out.
    if (now_micros - most_recent_pulse < BNC_TIMEOUT_MICROS) {
      // Check the pulse was long enough to not be noise.
      if (now_micros - most_recent_pulse > MIN_PULSE_TIME_MICROS) {
        // If this is the first pulse in the batch, set the first pulse time.
        if (pulseCounts_A[pinNum][index] == 0) {
          first_pulse_times_A[pinNum][index] = now_micros;
        }

        pulseCounts_A[pinNum][index] += 1;
        recent_pulse_times_A[pinNum][index] = now_micros;
      }
    }

    // If this batch was timed out, increase the index and record the start of a new batch of pulses.
    else {
      // Check if the pulse was long enough to not be noise.
      if (now_micros - most_recent_pulse > MIN_PULSE_TIME_MICROS) {
        // ensure that the previous value had something written to it.
        if (pulseCounts_A[pinNum][index] != 0) {
          index += 1;
        }

        // If this is the first pulse in the batch, set the first pulse time.
        if (pulseCounts_A[pinNum][index] == 0) {
          first_pulse_times_A[pinNum][index] = now_micros;
        }

        pulseCounts_A[pinNum][index] += 1;
        recent_pulse_times_A[pinNum][index] = now_micros;
      }
    }
  }

  else {
    // Get the index of the most recently set value in the array.
    int index = 0;
    for (int i = 0; i < buffer_size; i++) {
      if (pulseCounts_B[pinNum][i] == 0) {
        break;
      }

      index = i;
    }

    uint64_t most_recent_pulse = recent_pulse_times_B[pinNum][index];

    // Check if this pulse batch has timed out.
    if (now_micros - most_recent_pulse < BNC_TIMEOUT_MICROS) {
      // Check the pulse was long enough to not be noise.
      if (now_micros - most_recent_pulse > MIN_PULSE_TIME_MICROS) {
        // If this is the first pulse in the batch, set the first pulse time.
        if (pulseCounts_B[pinNum][index] == 0) {
          first_pulse_times_B[pinNum][index] = now_micros;
        }

        pulseCounts_B[pinNum][index] += 1;
        recent_pulse_times_B[pinNum][index] = now_micros;
      }
    }

    // If this batch was timed out, increase the index and record the start of a new batch of pulses.
    else {
      // Check if the pulse was long enough to not be noise.
      if (now_micros - most_recent_pulse > MIN_PULSE_TIME_MICROS) {
        // ensure that the previous value had something written to it.
        if (pulseCounts_B[pinNum][index] != 0) {
          index += 1;
        }

        // If this is the first pulse in the batch, set the first pulse time.
        if (pulseCounts_B[pinNum][index] == 0) {
          first_pulse_times_B[pinNum][index] = now_micros;
        }

        pulseCounts_B[pinNum][index] += 1;
        recent_pulse_times_B[pinNum][index] = now_micros;
      }
    }
  }
}

// BNC IN PORT 5
void IRAM_ATTR handlePulse4() {
  uint64_t now_micros = esp_timer_get_time();

  const int pinNum = 3;

  // TEMPORARY TO RECORD ONLY 1 PULSE INPUT USEFUL FOR TIMESTAMPING THE START OF EXTERNAL SYSTEMS WITH NO PULSE CONTROL
  detachInterrupt(digitalPinToInterrupt(pulsePins[4]));

  if (a_b_switch == false) {
    // Get the index of the most recently set value in the array.
    int index = 0;
    for (int i = 0; i < buffer_size; i++) {
      if (pulseCounts_A[pinNum][i] == 0) {
        break;
      }

      index = i;
    }

    uint64_t most_recent_pulse = recent_pulse_times_A[pinNum][index];

    // Check if this pulse batch hasn't timed out.
    if (now_micros - most_recent_pulse < BNC_TIMEOUT_MICROS) {
      // Check the pulse was long enough to not be noise.
      if (now_micros - most_recent_pulse > MIN_PULSE_TIME_MICROS) {
        // If this is the first pulse in the batch, set the first pulse time.
        if (pulseCounts_A[pinNum][index] == 0) {
          first_pulse_times_A[pinNum][index] = now_micros;
        }

        pulseCounts_A[pinNum][index] += 1;
        recent_pulse_times_A[pinNum][index] = now_micros;
      }
    }

    // If this batch was timed out, increase the index and record the start of a new batch of pulses.
    else {
      // Check if the pulse was long enough to not be noise.
      if (now_micros - most_recent_pulse > MIN_PULSE_TIME_MICROS) {
        // ensure that the previous value had something written to it.
        if (pulseCounts_A[pinNum][index] != 0) {
          index += 1;
        }

        // If this is the first pulse in the batch, set the first pulse time.
        if (pulseCounts_A[pinNum][index] == 0) {
          first_pulse_times_A[pinNum][index] = now_micros;
        }

        pulseCounts_A[pinNum][index] += 1;
        recent_pulse_times_A[pinNum][index] = now_micros;
      }
    }
  }

  else {
    // Get the index of the most recently set value in the array.
    int index = 0;
    for (int i = 0; i < buffer_size; i++) {
      if (pulseCounts_B[pinNum][i] == 0) {
        break;
      }

      index = i;
    }

    uint64_t most_recent_pulse = recent_pulse_times_B[pinNum][index];

    // Check if this pulse batch has timed out.
    if (now_micros - most_recent_pulse < BNC_TIMEOUT_MICROS) {
      // Check the pulse was long enough to not be noise.
      if (now_micros - most_recent_pulse > MIN_PULSE_TIME_MICROS) {
        // If this is the first pulse in the batch, set the first pulse time.
        if (pulseCounts_B[pinNum][index] == 0) {
          first_pulse_times_B[pinNum][index] = now_micros;
        }

        pulseCounts_B[pinNum][index] += 1;
        recent_pulse_times_B[pinNum][index] = now_micros;
      }
    }

    // If this batch was timed out, increase the index and record the start of a new batch of pulses.
    else {
      // Check if the pulse was long enough to not be noise.
      if (now_micros - most_recent_pulse > MIN_PULSE_TIME_MICROS) {
        // ensure that the previous value had something written to it.
        if (pulseCounts_B[pinNum][index] != 0) {
          index += 1;
        }

        // If this is the first pulse in the batch, set the first pulse time.
        if (pulseCounts_B[pinNum][index] == 0) {
          first_pulse_times_B[pinNum][index] = now_micros;
        }

        pulseCounts_B[pinNum][index] += 1;
        recent_pulse_times_B[pinNum][index] = now_micros;
      }
    }
  }
}

// Array of ISR function pointers
void (*isrFunctions[])() = { handlePulse1, handlePulse1, handlePulse2, handlePulse3, handlePulse4 };

// LCD Display
uint8_t blankChar[8] = { B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000 };
uint8_t heart[8] = { B00000, B00000, B01010, B11111, B11111, B01110, B00100, B00000 };
uint8_t mouse_head[8] = { B00000, B00000, B00000, B00000, B00100, B01111, B11111, B00101 };
uint8_t mouse_tail[8] = { B00000, B00000, B00000, B00000, B00000, B10010, B01101, B00000 };

// Set the time manually (year, month, day, hour, minute, second)
void setManualTime() {
  struct tm timeinfo;
  timeinfo.tm_year = 2025 - 1900;  // Year since 1900
  timeinfo.tm_mon = 5;             // June (0-based: 0=Jan, 5=June)
  timeinfo.tm_mday = 17;           // Day of month
  timeinfo.tm_hour = 11;           // Hour (24-hour format)
  timeinfo.tm_min = 37;            // Minute
  timeinfo.tm_sec = 0;             // Second
  time_t t = mktime(&timeinfo);
  struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
  settimeofday(&tv, NULL);
}

// Function to parse __DATE__ and __TIME__ and set RTC
void setTimeFromCompile() {
  // Get compile date and time (e.g., "Jun 17 2025" and "11:42:37")
  const char* compile_date = __DATE__;
  const char* compile_time = __TIME__;

  // Parse date
  char monthStr[4];
  int day, year;
  sscanf(compile_date, "%3s %d %d", monthStr, &day, &year);

  // Convert month string to number (1-based for RTClib)
  int month;
  if (strcmp(monthStr, "Jan") == 0) month = 1;
  else if (strcmp(monthStr, "Feb") == 0) month = 2;
  else if (strcmp(monthStr, "Mar") == 0) month = 3;
  else if (strcmp(monthStr, "Apr") == 0) month = 4;
  else if (strcmp(monthStr, "May") == 0) month = 5;
  else if (strcmp(monthStr, "Jun") == 0) month = 6;
  else if (strcmp(monthStr, "Jul") == 0) month = 7;
  else if (strcmp(monthStr, "Aug") == 0) month = 8;
  else if (strcmp(monthStr, "Sep") == 0) month = 9;
  else if (strcmp(monthStr, "Oct") == 0) month = 10;
  else if (strcmp(monthStr, "Nov") == 0) month = 11;
  else if (strcmp(monthStr, "Dec") == 0) month = 12;
  else month = 1;  // Default to January if parsing fails

  // Parse time
  int hour, minute, second;
  sscanf(compile_time, "%d:%d:%d", &hour, &minute, &second);

  // Set DS1307 time
  rtc.adjust(DateTime(year, month, day, hour, minute, second));

  // Set system time
  struct tm timeinfo;
  timeinfo.tm_year = year - 1900;  // Years since 1900
  timeinfo.tm_mon = month - 1;     // 0-based for system time
  timeinfo.tm_mday = day;
  timeinfo.tm_hour = hour;
  timeinfo.tm_min = minute;
  timeinfo.tm_sec = second;
  timeinfo.tm_isdst = -1;  // Auto-detect DST

  time_t t = mktime(&timeinfo);
  struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
  settimeofday(&tv, NULL);
}

// Function to set system time from DS1307
void setTimeFromRTC() {
  if (rtc.begin()) {
    if (rtc.isrunning()) {
      DateTime now = rtc.now();
      struct tm timeinfo;
      timeinfo.tm_year = now.year() - 1900;  // Years since 1900
      timeinfo.tm_mon = now.month() - 1;     // 0-based for system time
      timeinfo.tm_mday = now.day();
      timeinfo.tm_hour = now.hour();
      timeinfo.tm_min = now.minute();
      timeinfo.tm_sec = now.second();
      timeinfo.tm_isdst = -1;  // Auto-detect DST

      time_t t = mktime(&timeinfo);
      struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
      settimeofday(&tv, NULL);
      Serial.println("System time set from DS1307");
    } else {
      Serial.println("DS1307 is not running, setting from compile time");
      setTimeFromCompile();
    }
  } else {
    Serial.println("Couldn't find DS1307, setting from compile time");
    setTimeFromCompile();
  }
}

void dateTime(uint16_t* date, uint16_t* time) {
  // Callback function - called automatically when creating/closing files
  DateTime now = rtc.now();  // Get fresh time from RTC

  // Pack into FAT16 format (year-1980, since FAT starts at 1980)
  *date = FAT_DATE(now.year(), now.month(), now.day());
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}

void writeHeader(String file_name) {
  // Serial.println("Writing header to file...");
  File file = SD.open(file_name, FILE_WRITE);

  file.println("date,time,millis,event,pulseCount,rightCount,leftCount,dispensedCount,retrievedCount,rightWaterCount,leftWaterCount,LEDCount,toneCount,frontWaterCount,waterDispensedCount,rightWD_QTY,leftWD_QTY,frontWD_QTY,totalWD_QTY,rightWithPelletCount,leftWithPelletCount,rightDuringDispenseCount,leftDuringDispenseCount,rightinTimeoutCount,leftinTimeoutCount");

  file.close();
  // Serial.println("Complete.");
}

void writeEntry(String file_name, uint64_t timestamp, const char* event, int pulseCount = 0) {
  // Serial.println("Writing entry to file...");
  File file = SD.open(file_name, O_WRITE | O_APPEND);  //FILE_APPEND);

  // Get current time from RTC
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  // Format time as HH:MM:SS
  char timeStr[9];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

  // Format date as MM/DD/YY
  char dateStr[9];
  strftime(dateStr, sizeof(dateStr), "%m/%d/%y", &timeinfo);

  file.print(dateStr);
  file.print(",");
  file.print(timeStr);
  file.print(",");
  file.print(timestamp * 0.001, 2);
  file.print(",");
  file.print(event);
  file.print(",");
  file.print(pulseCount);
  file.print(",");
  file.print(RP);
  file.print(",");
  file.print(LP);
  file.print(",");
  file.print(DISP);
  file.print(",");
  file.print(RETR);
  file.print(",");
  file.print(RW);
  file.print(",");
  file.print(LW);
  file.print(",");
  file.print(COND);
  file.print(",");
  file.print(TONE);
  file.print(",");
  file.print(FW);
  file.print(",");
  file.print(WD);
  file.print(",");
  file.print(RW_QTY);
  file.print(",");
  file.print(LW_QTY);
  file.print(",");
  file.print(FW_QTY);
  file.print(",");
  file.print(TWD_QTY);
  file.print(",");
  file.print(RWP);
  file.print(",");
  file.print(LWP);
  file.print(",");
  file.print(RDD);
  file.print(",");
  file.print(LDD);
  file.print(",");
  file.print(RIT);
  file.print(",");
  file.print(LIT);
  file.println(",");

  file.close();
  // Serial.println("Complete.");
}


void writeEntryBatch(String file_name, uint64_t timestamps[buffer_size], const char* event, uint32_t pulseCounts[buffer_size]) {
  // Serial.println("Writing entry to file...");
  File file = SD.open(file_name, O_WRITE | O_APPEND);  //FILE_APPEND);

  // Get current time from RTC
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  // Format time as HH:MM:SS
  char timeStr[9];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

  // Format date as MM/DD/YY
  char dateStr[9];
  strftime(dateStr, sizeof(dateStr), "%m/%d/%y", &timeinfo);

  for (int i = 0; i < buffer_size; i++) {
    if (pulseCounts[i] == 0) {
      break;
    }

    file.print(dateStr);
    file.print(",");
    file.print(timeStr);
    file.print(",");
    file.print(timestamps[i] * 0.001, 2);
    file.print(",");
    file.print(event);
    file.print(",");
    file.print(pulseCounts[i]);
    file.print(",");
    file.print(RP);
    file.print(",");
    file.print(LP);
    file.print(",");
    file.print(DISP);
    file.print(",");
    file.print(RETR);
    file.print(",");
    file.print(RW);
    file.print(",");
    file.print(LW);
    file.print(",");
    file.print(COND);
    file.print(",");
    file.print(TONE);
    file.print(",");
    file.print(FW);
    file.print(",");
    file.print(WD);
    file.print(",");
    file.print(std::to_string(RW_QTY).c_str());
    file.print(",");
    file.print(std::to_string(LW_QTY).c_str());
    file.print(",");
    file.print(std::to_string(FW_QTY).c_str());
    file.print(",");
    file.print(std::to_string(TWD_QTY).c_str());
    file.print(",");
    file.print(RWP);
    file.print(",");
    file.print(LWP);
    file.print(",");
    file.print(RDD);
    file.print(",");
    file.print(LDD);
    file.print(",");
    file.print(RIT);
    file.print(",");
    file.print(LIT);
    file.println(",");
  }

  file.close();
  // Serial.println("Complete.");
}

void writeEntryBatch(String file_name, uint64_t timestamps[buffer_size], const char* events[buffer_size], uint32_t pulseCounts[buffer_size]) {
  // Serial.println("Writing entry to file...");
  File file = SD.open(file_name, O_WRITE | O_APPEND);  //FILE_APPEND);

  // Get current time from RTC
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  // Format time as HH:MM:SS
  char timeStr[9];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

  // Format date as MM/DD/YY
  char dateStr[9];
  strftime(dateStr, sizeof(dateStr), "%m/%d/%y", &timeinfo);

  // note there is a bug when recording with micros that can happen when: an event gets recorded exactly before rollover happens.
  // event -> ISR -> timestamp recorded -> ISR returns -> micros_rollover_count incremented -> event logged ~71.6 minutes in the future.
  // solution to this bug is to save the micros_rollover_count in the ISR, pass that micros_rollover_count here, and calculate with that, however that would require me create a new buffer for the ISR to save micros_rollover_count.
  // UPDATE MICROS RECORDING IN OTHER LOGGING!
  // The most practical solution is to change buffers to be uint64_t instead of unsigned long which uses more ram but requires no work arounds and introduces no bugs. the above bug is somewhat misleading as I don't believe the current logic will even recognize the new event.

  for (int i = 0; i < buffer_size; i++) {
    if (pulseCounts[i] == 0) {
      break;
    }

    file.print(dateStr);
    file.print(",");
    file.print(timeStr);
    file.print(",");
    file.print(timestamps[i] * 0.001, 2);
    file.print(",");
    file.print(events[i]);
    file.print(",");
    file.print(pulseCounts[i]);
    file.print(",");
    file.print(RP);
    file.print(",");
    file.print(LP);
    file.print(",");
    file.print(DISP);
    file.print(",");
    file.print(RETR);
    file.print(",");
    file.print(RW);
    file.print(",");
    file.print(LW);
    file.print(",");
    file.print(COND);
    file.print(",");
    file.print(TONE);
    file.print(",");
    file.print(FW);
    file.print(",");
    file.print(WD);
    file.print(",");
    file.print(std::to_string(RW_QTY).c_str());
    file.print(",");
    file.print(std::to_string(LW_QTY).c_str());
    file.print(",");
    file.print(std::to_string(FW_QTY).c_str());
    file.print(",");
    file.print(std::to_string(TWD_QTY).c_str());
    file.print(",");
    file.print(RWP);
    file.print(",");
    file.print(LWP);
    file.print(",");
    file.print(RDD);
    file.print(",");
    file.print(LDD);
    file.print(",");
    file.print(RIT);
    file.print(",");
    file.print(LIT);
    file.println(",");
  }

  file.close();
  // Serial.println("Complete.");
}


void drawScreen() {
  screenMillis = millis();

  // Get current time from RTC
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  // Format time as HH:MM:SS
  char timeStr[9];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

  // Format date as MM/DD/YY
  char dateStr[9];
  strftime(dateStr, sizeof(dateStr), "%m/%d/%y", &timeinfo);

  lcd.setCursor(9, 0);
  lcd.print("P");

  lcd.setCursor(0, 0);
  lcd.print(dateStr);
  lcd.setCursor(0, 1);
  lcd.print(timeStr);

  // draw hearts
  lcd.setCursor(15, 0);
  lcd.write((uint8_t)0);

  // draw mouse
  lcd.setCursor(11, 1);
  lcd.print("     ");
  lcd.setCursor(mousePos, 1);
  lcd.write((uint8_t)2);
  lcd.setCursor(mousePos - 1, 1);
  lcd.write((uint8_t)1);
  mousePos -= 1;
  if (mousePos == 11) {
    mousePos = 15;
  }
}

void pulsePin(int pin, int count, int delay_ms) {
  /*
    Pulses the given pin, count times, with a delay_ms before and after the pulse.
  */
  for (int i = 0; i < count; i++) {
    digitalWrite(pin, HIGH);
    delay(delay_ms);
    digitalWrite(pin, LOW);
    delay(delay_ms);
  }
}

void checkForPulses() {
  // Buffer to store counts for atomic reading
  uint32_t localCounts[numPins];
  uint64_t local_first_pulse_times[numPins];

  uint32_t local_pulseCounts[numPins][buffer_size];
  uint64_t local_first_pulse_times_2D[numPins][buffer_size];

  bool local_a_b_switch = false;
  bool a_b_count_met = false;

  // Enter critical section to safely read pulse counts
  portENTER_CRITICAL(&mux);
  uint64_t now_micros = esp_timer_get_time();

  // switch the pulse batching to the other buffer so that we can safely log without missing pulses.
  local_a_b_switch = a_b_switch;

  // get the most recent pulse times for each bnc port.
  uint64_t MRP_1 = 0;
  uint64_t MRP_2 = 0;
  uint64_t MRP_3 = 0;
  uint64_t MRP_4 = 0;

  if (local_a_b_switch == false) {
    for (int i = 0; i < buffer_size; i++) {
      if (recent_pulse_times_A[0][i] == 0) {
        break;
      }
      MRP_1 = recent_pulse_times_A[0][i];
    }

    for (int i = 0; i < buffer_size; i++) {
      if (recent_pulse_times_A[1][i] == 0) {
        break;
      }
      MRP_2 = recent_pulse_times_A[1][i];
    }

    for (int i = 0; i < buffer_size; i++) {
      if (recent_pulse_times_A[2][i] == 0) {
        break;
      }
      MRP_3 = recent_pulse_times_A[2][i];
    }

    for (int i = 0; i < buffer_size; i++) {
      if (recent_pulse_times_A[3][i] == 0) {
        break;
      }
      MRP_4 = recent_pulse_times_A[3][i];
    }
  }

  else {
    for (int i = 0; i < buffer_size; i++) {
      if (recent_pulse_times_B[0][i] == 0) {
        break;
      }
      MRP_1 = recent_pulse_times_B[0][i];
    }

    for (int i = 0; i < buffer_size; i++) {
      if (recent_pulse_times_B[1][i] == 0) {
        break;
      }
      MRP_2 = recent_pulse_times_B[1][i];
    }

    for (int i = 0; i < buffer_size; i++) {
      if (recent_pulse_times_B[2][i] == 0) {
        break;
      }
      MRP_3 = recent_pulse_times_B[2][i];
    }

    for (int i = 0; i < buffer_size; i++) {
      if (recent_pulse_times_B[3][i] == 0) {
        break;
      }
      MRP_4 = recent_pulse_times_B[3][i];
    }
  }

  if (MRP_1 == 0) { MRP_1 = now_micros; }
  if (MRP_2 == 0) { MRP_2 = now_micros; }
  if (MRP_3 == 0) { MRP_3 = now_micros; }
  if (MRP_4 == 0) { MRP_4 = now_micros; }

  // if the most recent pulse is not 0 and also greater than the timeout, increment the a_b_switch_count
  if (MRP_LAST_1 != MRP_1 && now_micros - MRP_1 > BOX_BNC_TIMEOUT_MICROS) {
    a_b_switch_count += 1;
    MRP_LAST_1 = MRP_1;
  }

  if (MRP_LAST_2 != MRP_2 && now_micros - MRP_2 > BNC_TIMEOUT_MICROS) {
    a_b_switch_count += 1;
    MRP_LAST_2 = MRP_2;
  }

  if (MRP_LAST_3 != MRP_3 && now_micros - MRP_3 > BNC_TIMEOUT_MICROS) {
    a_b_switch_count += 1;
    MRP_LAST_3 = MRP_3;
  }

  if (MRP_LAST_4 != MRP_4 && now_micros - MRP_4 > BNC_TIMEOUT_MICROS) {
    a_b_switch_count += 1;
    MRP_LAST_4 = MRP_4;
  }

  // get the most recent pulse based on all pulses
  uint64_t most_recent_pulse = 0;
  if (local_a_b_switch == false) {
    for (int i = 0; i < numPins; i++) {
      for (int j = 0; j < buffer_size; j++) {
        if (recent_pulse_times_A[i][j] == 0) {
          break;
        } else if (recent_pulse_times_A[i][j] > most_recent_pulse) {  // set the most recent pulse to the latest one.
          most_recent_pulse = recent_pulse_times_A[i][j];
        }
      }
    }
  } else {
    for (int i = 0; i < numPins; i++) {
      for (int j = 0; j < buffer_size; j++) {
        if (recent_pulse_times_B[i][j] == 0) {
          break;
        } else if (recent_pulse_times_B[i][j] > most_recent_pulse) {  // set the most recent pulse to the latest one.
          most_recent_pulse = recent_pulse_times_B[i][j];
        }
      }
    }
  }

  // copy all arrays to local and reset the old buffers
  if ((a_b_switch_count >= a_b_switch_count_max) || (now_micros - most_recent_pulse > BATCH_WRITE_TIMEOUT_MICROS && a_b_switch_count > 0)) {
    a_b_switch = !a_b_switch;

    if (local_a_b_switch == false) {
      // memcpy the arrays to the local arrays for later logging. we will then free them.
      std::memcpy(local_pulseCounts, static_cast<void*>(const_cast<uint32_t(*)[buffer_size]>(pulseCounts_A)), sizeof(uint32_t) * numPins * buffer_size);
      std::memcpy(local_first_pulse_times_2D, static_cast<void*>(const_cast<uint64_t(*)[buffer_size]>(first_pulse_times_A)), sizeof(uint64_t) * numPins * buffer_size);

      // free the old arrays
      std::memset(const_cast<uint32_t(*)[buffer_size]>(pulseCounts_A), 0, sizeof(uint32_t) * numPins * buffer_size);
      std::memset(const_cast<uint64_t(*)[buffer_size]>(first_pulse_times_A), 0, sizeof(uint64_t) * numPins * buffer_size);
      std::memset(const_cast<uint64_t(*)[buffer_size]>(recent_pulse_times_A), 0, sizeof(uint64_t) * numPins * buffer_size);
    }

    else {
      // memcpy the arrays to the local arrays for later logging. we will then free them.
      std::memcpy(local_pulseCounts, static_cast<void*>(const_cast<uint32_t(*)[buffer_size]>(pulseCounts_B)), sizeof(uint32_t) * numPins * buffer_size);
      std::memcpy(local_first_pulse_times_2D, static_cast<void*>(const_cast<uint64_t(*)[buffer_size]>(first_pulse_times_B)), sizeof(uint64_t) * numPins * buffer_size);

      // free the old arrays
      std::memset(const_cast<uint32_t(*)[buffer_size]>(pulseCounts_B), 0, sizeof(uint32_t) * numPins * buffer_size);
      std::memset(const_cast<uint64_t(*)[buffer_size]>(first_pulse_times_B), 0, sizeof(uint64_t) * numPins * buffer_size);
      std::memset(const_cast<uint64_t(*)[buffer_size]>(recent_pulse_times_B), 0, sizeof(uint64_t) * numPins * buffer_size);
    }

    a_b_count_met = true;
    a_b_switch_count = 0;
  }
  portEXIT_CRITICAL(&mux);

  // Logging
  if (a_b_count_met) {
    String printStr = "";
    int printCounts = -1;
    if (local_pulseCounts[0][0] != 0) {  // if nothing to write don't write
      const char* events[buffer_size] = { "" };

      for (int i = 0; i < buffer_size; i++) {
        if (local_pulseCounts[0][i] == 0) {
          break;
        }

        if (local_pulseCounts[0][i] == 1) {
          printStr = "RP  ";
          printCounts = 1;
          RP += 1;
          events[i] = "Right";
        }

        else if (local_pulseCounts[0][i] == 2) {
          printStr = "LP  ";
          printCounts = 2;
          LP += 1;
          events[i] = "Left";
        }

        else if (local_pulseCounts[0][i] == 3) {
          printStr = "DISP";
          printCounts = 3;
          DISP += 1;
          events[i] = "Dispensed";
        }

        else if (local_pulseCounts[0][i] == 4) {
          printStr = "RETR";
          printCounts = 4;
          RETR += 1;
          events[i] = "Retrieved";
        }

        else if (local_pulseCounts[0][i] == 5) {
          printStr = "RW  ";
          printCounts = 5;
          lastWaterPort = 0;
          RW += 1;
          events[i] = "RightWater";
        }

        else if (local_pulseCounts[0][i] == 6) {
          printStr = "LW  ";
          printCounts = 6;
          lastWaterPort = 1;
          LW += 1;
          events[i] = "LeftWater";
        }

        else if (local_pulseCounts[0][i] == 7) {
          printStr = "LED ";
          printCounts = 7;
          COND += 1;
          events[i] = "LED";
        }

        else if (local_pulseCounts[0][i] == 8) {
          printStr = "TONE";
          printCounts = 8;
          TONE += 1;
          events[i] = "TONE";
        }

        else if (local_pulseCounts[0][i] == 9) {
          printStr = "FW  ";
          printCounts = 9;
          lastWaterPort = 2;
          FW += 1;
          events[i] = "FrontWater";
        }

        else if (local_pulseCounts[0][i] == 10) {
          printStr = "WD  ";
          printCounts = 10;
          WD += 1;
          TWD_QTY += WD_QTY;
          if (lastWaterPort == 0) {
            RW_QTY += WD_QTY;
          }
          if (lastWaterPort == 1) {
            LW_QTY += WD_QTY;
          }
          if (lastWaterPort == 2) {
            FW_QTY += WD_QTY;
          }
          events[i] = "WaterDispensed";
        }

        else if (local_pulseCounts[0][i] == 11) {
          printStr = "RWP ";
          printCounts = 11;
          RWP += 1;
          events[i] = "RightWithPellet";
        }

        else if (local_pulseCounts[0][i] == 12) {
          printStr = "LWP ";
          printCounts = 12;
          LWP += 1;
          events[i] = "LeftWithPellet";
        }

        else if (local_pulseCounts[0][i] == 13) {
          printStr = "RDD ";
          printCounts = 13;
          RDD += 1;
          events[i] = "RightDuringDispense";
        }

        else if (local_pulseCounts[0][i] == 14) {
          printStr = "LDD ";
          printCounts = 14;
          LDD += 1;
          events[i] = "LeftDuringDispense";
        }

        else if (local_pulseCounts[0][i] == 15) {
          printStr = "RIT ";
          printCounts = 15;
          RIT += 1;
          events[i] = "RightinTimeOut";
        }

        else if (local_pulseCounts[0][i] == 16) {
          printStr = "LIT ";
          printCounts = 16;
          LIT += 1;
          events[i] = "LeftinTimeOut";
        }
      }

      writeEntryBatch(BNC_1_filename, local_first_pulse_times_2D[0], events, local_pulseCounts[0]);
      lcd.setCursor(11, 0);
      lcd.print(printStr);
      lcd.setCursor(9, 1);
      lcd.print("  ");
      lcd.setCursor(9, 1);
      lcd.print(printCounts);
    }

    if (local_pulseCounts[1][0] != 0) {  // if nothing to write don't write
      writeEntryBatch(BNC_2_filename, local_first_pulse_times_2D[1], "BNC1", local_pulseCounts[1]);
    }

    if (local_pulseCounts[2][0] != 0) {  // if nothing to write don't write
      writeEntryBatch(BNC_3_filename, local_first_pulse_times_2D[2], "BNC2", local_pulseCounts[2]);
    }

    if (local_pulseCounts[3][0] != 0) {  // if nothing to write don't write
      writeEntryBatch(BNC_4_filename, local_first_pulse_times_2D[3], "BNC3", local_pulseCounts[3]);
    }
  }
}

void lcdManualClear() {
  lcd.setCursor(0, 0);
  lcd.println("                ");
  lcd.setCursor(0, 1);
  lcd.println("                ");
}

void setup() {
  Serial.begin(9600);

  // Initialize LCD
  Serial.println("Initializing LCD...");
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.begin(16, 2);
  lcd.backlight();

  Serial.println("Initializing RTC...");
  if (!rtc.begin()) {
    Serial.println("FAILED");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.println("RTC FAILED          ");

    while (true)
      ;
  }

  // Get time from rtc.
  setTimeFromRTC();
  delay(100);

  // Overwrite all 8 custom character slots with blank pattern
  for (uint8_t i = 0; i < 8; i++) {
    lcd.createChar(i, blankChar);
  }

  // Create custom characters
  lcd.createChar(0, heart);
  lcd.createChar(1, mouse_head);
  lcd.createChar(2, mouse_tail);

  Serial.println("Initializing SD Card...");

  // Initialize SPI with custom pins
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SD_SCK_MHZ(15))) {
    Serial.println("FAILED");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.println("SD CARD ERROR    ");

    SPI.end();
    SD.end();
    while (true)
      ;
  }

  SdFile::dateTimeCallback(dateTime);  // Initialize the datetime callback for modifying the file timestamp.

  Serial.println("SUCCESS");

  // Generate unique filename
  char tmpname[32];  // Fixed-size buffer for temporary filename
  int count = 1;
  strcpy(tmpname, "/NH_1");  // Initial filename

  while (SD.exists(tmpname)) {
    strcpy(tmpname, "/NH_");    // Reset to base name
    char countstr[12];          // Buffer for number (max 32-bit int + null)
    itoa(count, countstr, 10);  // Convert count to char*

    // Concatenate base, number, and extension
    if (strlen(tmpname) + strlen(countstr) >= sizeof(tmpname)) {
      Serial.println("Error: Buffer too small for filename");
      return;
    }
    strcat(tmpname, countstr);
    // strcat(tmpname, ".csv");
    count++;
  }

  // Copy to global filename
  strcpy(dirname, tmpname);

  Serial.print("Working in: ");
  Serial.println(dirname);

  SD.mkdir(dirname);

  String tmp_dirname = dirname;
  BNC_1_filename = tmp_dirname + BNC_1_filename;
  BNC_2_filename = tmp_dirname + BNC_2_filename;
  BNC_3_filename = tmp_dirname + BNC_3_filename;
  BNC_4_filename = tmp_dirname + BNC_4_filename;
  Serial.println(BNC_1_filename);
  Serial.println(BNC_2_filename);
  Serial.println(BNC_3_filename);
  Serial.println(BNC_4_filename);

  writeHeader(BNC_1_filename);
  writeHeader(BNC_2_filename);
  writeHeader(BNC_3_filename);
  writeHeader(BNC_4_filename);

  // Write the file data to the LCD screen.
  lcdManualClear();
  lcd.setCursor(0, 0);
  lcd.println("WORKING IN:     ");
  lcd.setCursor(0, 1);
  lcd.print(dirname);
  lcd.println("                ");
  delay(2500);

  Serial.println("STARTED");
  lcdManualClear();
  lcd.setCursor(0, 0);


  // Attach Interupts to NeuroHab pins.
  if (numPins >= 1) {
    pinMode(pulsePins[0], INPUT_PULLUP);  // Use INPUT_PULLUP to avoid floating pins
    pinMode(pulsePins[1], INPUT_PULLUP);  // Use INPUT_PULLUP to avoid floating pins
    attachInterrupt(digitalPinToInterrupt(pulsePins[0]), isrFunctions[0], FALLING);
    attachInterrupt(digitalPinToInterrupt(pulsePins[1]), isrFunctions[1], FALLING);
  }

  if (numPins >= 2) {
    pinMode(pulsePins[2], INPUT_PULLUP);  // Use INPUT_PULLUP to avoid floating pins
    attachInterrupt(digitalPinToInterrupt(pulsePins[2]), isrFunctions[2], FALLING);
  }

  if (numPins >= 3) {
    pinMode(pulsePins[3], INPUT_PULLUP);  // Use INPUT_PULLUP to avoid floating pins
    attachInterrupt(digitalPinToInterrupt(pulsePins[3]), isrFunctions[3], FALLING);
  }

  if (numPins >= 4) {
    pinMode(pulsePins[4], INPUT_PULLUP);  // Use INPUT_PULLUP to avoid floating pins
    attachInterrupt(digitalPinToInterrupt(pulsePins[4]), isrFunctions[4], RISING);
  }
}

void loop() {
  if (millis() - screenMillis >= screenTimeout) {
    drawScreen();
  }

  checkForPulses();
}
