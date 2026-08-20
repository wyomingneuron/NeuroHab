/*
AUTHOR: Sam Crouse
Email: scrouse2@uwyo.edu
Organization: Sun Lab Wyoming SBC
*/

// Libraries to include.
#include <LiquidCrystal_I2C.h>  // version 2.0.0
#include <time.h>
#include <RTClib.h>
#include <Wire.h>

// Pin definitions
#define SDA_PIN 17
#define SCL_PIN 16

RTC_DS1307 rtc;

// Initialize LCD with ESP32 I2C Connection
LiquidCrystal_I2C lcd(0x27, 16, 2);

// LCD Display
uint8_t blankChar[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};
uint8_t heart[8] = { B00000, B00000, B01010, B11111, B11111, B01110, B00100, B00000 };
uint8_t mouse_head[8] = { B00000, B00000, B00000, B00000, B00100, B01111, B11111, B00101 };
uint8_t mouse_tail[8] = { B00000, B00000, B00000, B00000, B00000, B10010, B01101, B00000};

// screen variables
int mousePos = 15;

// Set the time manually (year, month, day, hour, minute, second)
void setManualTime() {
  struct tm timeinfo;
  timeinfo.tm_year = 2025 - 1900; // Year since 1900
  timeinfo.tm_mon = 5; // June (0-based: 0=Jan, 5=June)
  timeinfo.tm_mday = 17; // Day of month
  timeinfo.tm_hour = 11; // Hour (24-hour format)
  timeinfo.tm_min = 37; // Minute
  timeinfo.tm_sec = 0; // Second
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
  else month = 1; // Default to January if parsing fails

  // Parse time
  int hour, minute, second;
  sscanf(compile_time, "%d:%d:%d", &hour, &minute, &second);

  // Set DS1307 time
  rtc.adjust(DateTime(year, month, day, hour, minute, second));

  // Set system time
  struct tm timeinfo;
  timeinfo.tm_year = year - 1900; // Years since 1900
  timeinfo.tm_mon = month - 1;    // 0-based for system time
  timeinfo.tm_mday = day;
  timeinfo.tm_hour = hour;
  timeinfo.tm_min = minute;
  timeinfo.tm_sec = second;
  timeinfo.tm_isdst = -1; // Auto-detect DST

  time_t t = mktime(&timeinfo);
  struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
  settimeofday(&tv, NULL);
}


void drawScreen() {
  // Get current time from RTC
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  // Format time as HH:MM:SS
  char timeStr[9];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

  // Format date as MM/DD/YY
  char dateStr[9];
  strftime(dateStr, sizeof(dateStr), "%m/%d/%y", &timeinfo);

  // Write time.
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(dateStr);
  lcd.setCursor(0, 1);
  lcd.print(timeStr);

  // draw hearts
  lcd.setCursor(0, 0);
  lcd.setCursor(15, 0);
  lcd.write((uint8_t)0);

  // draw mouse
  lcd.setCursor(11, 1);
  lcd.print("     ");
  lcd.setCursor(mousePos, 1);
  lcd.write((uint8_t)2);
  lcd.setCursor(mousePos-1, 1);
  lcd.write((uint8_t)1);
  mousePos -= 1;
  if (mousePos == 8) {
    mousePos = 15;
  }
}

void setup() {
  Serial.begin(4800);
  
  // Initialize LCD
  Serial.println("Initializing LCD...");
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.begin(16, 2);
  lcd.backlight();

  Serial.println("Initializing RTC...");
  Wire.begin(15, 16);
  if (!rtc.begin()) {
    Serial.println("FAILED");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.println("RTC FAILED          ");

    while (true);
  }

  // Set as compile time
  setTimeFromCompile();
  delay(100);

  // Overwrite all 8 custom character slots with blank pattern
  for (uint8_t i = 0; i < 8; i++) {
    lcd.createChar(i, blankChar);
  }

  // Create custom characters
  lcd.createChar(0, heart);
  lcd.createChar(1, mouse_head);
  lcd.createChar(2, mouse_tail);
}

void loop() {
  drawScreen();
  delay(1000);
}

