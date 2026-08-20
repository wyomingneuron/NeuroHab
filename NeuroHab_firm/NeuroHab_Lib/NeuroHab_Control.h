/*
AUTHOR: Sam Crouse
Email: scrouse2@uwyo.edu
Organization: Sun Lab Wyoming SBC
*/

/* IMPORT STATEMENTS ************************************************************************************************************************/
#ifndef NH_Control_H
#define NH_Control_H

#include <Arduino.h>
#include <CapacitiveSensor.h>

/* PIN DEFINITIONS ************************************************************************************************************************/
// BNC and valve pins.
#define PINOUT_1 26  // ESP32 Pulsing
#define PINOUT_2 25  // BNC Pulsing
#define VALVE_1 49  // 47
#define VALVE_2 51
#define VALVE_3 53

// Conditioned stimulus pins.
#define R1 4
#define G1 3
#define B1 2
#define BUZ1 5

#define R2 8
#define G2 7
#define B2 6
#define BUZ2 9

#define R3 12
#define G3 11
#define B3 10  //
#define BUZ3 13

// Capacitive sensor pins.
#define S1_pin 22
#define S2_pin 23
#define S3_pin 24

// Const value definitions.
#define CAP_BUFFER_SIZE 200 //1100 // Preprocessor macro for array size

class NH_Control {
	public:
		// CONSTRUCTOR
		NH_Control() {};

		/* PUBLIC DEFINITIONS ************************************************************************************************************************/
		// Capacitive sensor initialization.
		CapacitiveSensor S1 = CapacitiveSensor(S1_pin, A0);  // 2
		CapacitiveSensor S2 = CapacitiveSensor(S2_pin, A1);  // 4
		CapacitiveSensor S3 = CapacitiveSensor(S3_pin, A2);  // 6

		// VARIABLE DEFINTIONS
		// Capacitive sensor variables.
		const int capBufSize = CAP_BUFFER_SIZE;  // 500
		int capBuffer1[CAP_BUFFER_SIZE] = { 0 };  // Used to calculate the average values for the capacitive sensors so we can detect a rising / falling edge.
		int capBuffer2[CAP_BUFFER_SIZE] = { 0 };
		int capBuffer3[CAP_BUFFER_SIZE] = { 0 };

		int CS_reinittimeout = 10000;	// How long before we reinit the capacitive sensors?
		unsigned long last_reinit = 0;  // Last initialization time.
		int CS_timeoutmillis = 1;		// How many ms before -2 is returned from a capacitive sensor due to a timeout.  4
		float triggerThreshold = 4;	// How much must the average capacitance value increase to trigger a detect.	 4
		float resetThreshold = 0.05;	// How close to the saved base must the value return to trigger a reset.
		int samples = 1;				// The number of samples each capacitive sensor takes.							 3
		
		float base_saved1 = -1;			// The saved average value of capacitive sensors used to detect falling edges.
		float base_saved2 = -1;
		float base_saved3 = -1;

		int val1 = 0;					// Current value of the capacitive sensors.
		int val2 = 0;
		int val3 = 0;

		int val1_head = 0;  			// Where the circular buffer is at in the running sum.  // We do this to reduce latencies from adding O(n) buffer shifting.
		long val1_running_sum = 0;		// Current running sum of sensor1 values.
		int val2_head = 0;
		long val2_running_sum = 0;
		int val3_head = 0;
		long val3_running_sum = 0;

		// Variables for conditioned stimulus control.
		bool canDetectPort1 = true;		// Latch bool for lickport detection.
		bool canDetectPort2 = true;
		bool canDetectPort3 = true;

		int lickportTimeout = 1;		// Timeout after lick detected before loop restarts. (MS)

		bool RW_CS_ON = false;			// Conditioned stimulus on and off. (Control bools.)
		bool LW_CS_ON = false;

		bool RW_ON = false;				// Turn water on. Automatic off after timeout. (Control bools.)
		bool LW_ON = false;
		bool FW_ON = false;

		bool RW_SIGNALED = false;		// Bools for signaling and control flow.
		bool LW_SIGNALED = false;
		bool FW_SIGNALED = false;

		bool RW_CS_SIGNALED = false;
		bool LW_CS_SIGNALED = false;

		float COND_STIM_DUR = 250;		// Conditioned stimulus duration, buzzer volume, and led brightness.
		float LED_BRIGHT = 0.50;
		float BUZ_VOL = 100;

		unsigned long BUZ_TIMER = 0;	// Timestamp variables.
		unsigned long LED_TIMER = 0;
		unsigned long LICK_TIMER = 0;

		int R = 0;						// The color applied to the LEDS.
		int G = 255;
		int B = 0;

		// Data variables.
		float WD_QTY = 0.02297;	// ml per WD  // DIRECTLY FROM NeuroHAB-AI_Logging.ino
		float RW_delivered = 0;
		float LW_delivered = 0;
		float FW_delivered = 0;

		// Encoding, pulsed to the logger.
		const int RW = 5;
		const int LW = 6;
		const int LED = 7;
		const int TONE = 8;
		const int FW = 9;
		const int WD = 10;								// Encoding for water dispensed.
		const int PULSE_SEPAR_DELAY_MS = 60;			// The delay after pulsing to prevent overlap in pulses within the logger.
		float lickTime = 130 - PULSE_SEPAR_DELAY_MS;	// How long the lickport valve is open before closing. Pulse delay to account for pulse time while valve is open.

		const int pulse_delay_micros = 500;				// Time between pulses.

		unsigned long initialization_start = 0;			// Start timestamp to keep track of initialization time.
		unsigned long initialization_delay = 1000;		// How many milliseconds to let buffers fill and system settle before starting.

		unsigned long latency_stamp1 = 0;	// Used for benchmarking latencies.
		unsigned long latency_stamp2 = 0;


		/* FUNCTION DEFINITIONS ************************************************************************************************************************/
		void begin(float lick_threshold);
		void begin() { begin(triggerThreshold); }  // Overload for default triggerThreshold value.
		void setColor(float r_color, int r_pin, float g_color, int g_pin, float b_color, int b_pin, float brightness = 1);
		void pulsePin(int pin, int count, int delay_ms);
		void dualPulsePin_HIGH(int pin1, int pin2, int count, int delay_micros);
		void dualPulsePin_LOW(int pin1, int pin2, int count, int delay_micros);
		void insertAtRight(int* buffer, int size, int newValue);
		void insertAtRight(int* buffer, int size, int newValue, long& runningSum, int& head);
		void printBuffer(int* buffer, int size);
		float calculateAverage(int* buffer, int size);
		float calculateAverageExclude(int* buffer, int size, int exclude_n);
		float runningAverage(long running_sum, int size);
		void condStim_ON_LED(bool rightW = false, bool leftW = false, bool frontW = false, float R_col = 0, float G_col = 0, float B_col = 0);
		void condStim_ON_BUZ(bool rightW = false, bool leftW = false, bool frontW = false, int volume = 0);
		void condStim_OFF_LED(bool rightW = false, bool leftW = false, bool frontW = false);
		void condStim_OFF_BUZ(bool rightW = false, bool leftW = false, bool frontW = false);
		void log_event(const char* event);
		void set_LED_color(int r, int g, int b);
		void set_LED_brightness(float brightness);
		void set_BUZ_volume(int volume);
		
		bool RW_lick();
		void RW_control();
		void RW_dispense();
		void RW_LED_ON();
		void RW_LED_OFF();
		void RW_BUZ_ON();
		void RW_BUZ_OFF();

		bool LW_lick();
		void LW_control();
		void LW_dispense();
		void LW_LED_ON();
		void LW_LED_OFF();
		void LW_BUZ_ON();
		void LW_BUZ_OFF();

		bool FW_lick();
		void FW_control();
		void FW_dispense();
		void FW_LED_ON();
		void FW_LED_OFF();
		void FW_BUZ_ON();
		void FW_BUZ_OFF();
		void run();

	private:
		/* PRIVATE DEFINITIONS ************************************************************************************************************************/
		int placeHolder = 0;
};


#endif
