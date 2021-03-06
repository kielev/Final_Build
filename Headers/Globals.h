 /* Globals.h
 *
 *  Created on: Jul 13, 2018
 *      Author: kielev
 */
#ifndef HEADERS_GLOBALS_H_
#define HEADERS_GLOBALS_H_

/* DriverLib Includes */
#include "driverlib.h"
#include "msp432.h"
#include "rom_map.h"
#include "Headers/Structures.h"

//Status Globals
//Req is when a request for a device is on, enable is what actually turns on a pin or off. Enables are set by the StateMachine
//which handles the requests and the priorites set out
extern volatile _Bool IridiumEn;
extern volatile _Bool GPSEn;
extern volatile _Bool VHFToggle;
extern volatile _Bool VHFStarted;


//Scheduling Globals

//RTC Globals
volatile RTC_C_Calendar SystemTime; //This is for within the system, it gets updated every time the RTC interrupt happens
volatile RTC_C_Calendar SetTime; //Had to have a separate structure which got populated for updating the RTC time.  It was weird
//in that it wouldn't work with using the same structure, likely because the RTC handler was messing with it.

//UART ISR Globals
extern volatile uint8_t IridiumRXData; //These are where the characters obtained on the UART buffer for each channel are stored.
extern volatile uint8_t GPSRXData;

//PC Hardwire Globals
volatile char ParameterString[99]; //Puts the characters from the buffer into the string and increments so we can get more than one character
//at a time and get a useful message
volatile char PCString[99]; //Puts the raw string from the buffer into a new location for parsing. Gets updated when the PCdataString has a complete string.
extern volatile int PCindex; //Increments within the string and puts the characters in and increments to the next position
extern volatile int PCflag; //Used for when there has been the start character detected and the characters on the buffer are to be put into PCdataString
extern volatile int PCStringExtractGo; //When a complete string has been formed and needs to be parsed, this flag gets set.
extern volatile _Bool USBPresentFlag; //When the USB is connected, this flag is set and used within the code.
extern volatile _Bool USBPresentOns; //One shot to initialize the PC UART channel
extern volatile _Bool MagnetRemovedFlag; //Used for when the magnet has been removed and the "start up" procedure can take place.

//Power Calculation Globals
//All of these uint8_t's are for power calc's. The second counts are incremented in the SysTick ISR, and corresponding minutes and seconds thereafter.
extern volatile uint16_t VHFCount;
extern volatile uint16_t IridiumCount;
extern volatile uint16_t GPSCount;

extern volatile uint16_t VHFStartCount;
extern volatile uint8_t GPSSecOnCount;

extern volatile _Bool LongevityMode; //When the battery is on the last legs this flag gets set and doesn't allow the GPS to get data points anymore

extern volatile _Bool CurrentLocationRequestInitiated; //On the Xbee if the researcher requests for the current location of the collar, this is set
//and allows the location to be determined.
extern volatile _Bool NowGetFix; //Another bit for getting the current location of the collar
volatile char BatteryString[4]; //The value of the calculated battery percentage is stored in here and sent over the PC and Xbee connection
extern volatile _Bool updateConfig;

extern volatile uint8_t VHFSecOnCount; //On startup, this gets incremented and is a "beacon" so they know the collar is on.

//Cool structure I made to store all of the parameters obtained through the UART connection
volatile GPSDataStruct GPSData;
volatile GPSDataStruct FinalGPSData;
extern volatile Configparameters Config;
extern volatile _Bool BatteryLow;

//GPS Globals
volatile char dataString[300]; //Raw characters from the buffer are put in here
volatile char GPSString[300]; //When something needed to be parsed then it's put in this string
extern volatile int GPSindex; //Same thing as PC string
extern volatile int GPSGo;
extern volatile _Bool FixAttemptFailed;
extern volatile _Bool GPSQuickRetry;
extern volatile _Bool RMCSetTime;
//volatile char CurrentFixSaveString[300];

//Iridium Globals
volatile char IridiumString[350];
extern volatile int Iridiumindex;
extern volatile int IridiumGo;
extern volatile _Bool IridiumQuickRetry;

// PC GUI globals
extern volatile struct PC_Set_Time {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t month;
    uint8_t day;
    uint16_t year;
} pc_set_time;

extern volatile struct PC_GPS_Settings {
    uint8_t num_hours;
    uint8_t timeout;
} pc_gps_settings;
extern volatile struct PC_Sat_Settings {
    uint8_t upload_day;
    uint8_t hour_connect;
    uint8_t am_pm;
    uint8_t frequency;
    uint8_t retries;
} pc_sat_settings;
extern volatile struct PC_VHF_Settings {
    uint8_t start_hour;
    uint8_t start_am_pm;
    uint8_t end_hour;
    uint8_t end_am_pm;
} pc_vhf_settings;
extern volatile int LED_STATE;
extern volatile int LED_CHANGE;
extern volatile int ALL_DATA_SENT;
extern volatile int NEW_DATA_READY;
extern volatile int PC_READY_DATA;
extern volatile int SET_TIME_PRESSED;
extern volatile int SET_GPS_PRESSED;
extern volatile int SET_SAT_PRESSED;
extern volatile int SET_VHF_PRESSED;
extern volatile float READ_DATA_PROGRESS;
#endif /* HEADERS_GLOBALS_H_ */
