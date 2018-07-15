/*
 * Globals.h
 *
 *  Created on: Jul 13, 2018
 *      Author: kielev
 */

/* DriverLib Includes */
#include "driverlib.h"
#include "msp432.h"
#include "rom_map.h"
#include "Headers/Structures.h"

#ifndef HEADERS_GLOBALS_H_
#define HEADERS_GLOBALS_H_


//Status Globals
//Req is when a request for a device is on, enable is what actually turns on a pin or off. Enables are set by the StateMachine
//which handles the requests and the priorites set out
volatile _Bool IridiumReq = 0;
volatile _Bool IridiumEn = 0;
volatile _Bool GPSReq = 0;
volatile _Bool GPSEn = 0;
volatile _Bool VHFReq = 0;
volatile _Bool VHFEn = 0;

//Hour or Minute RTC interrupt flags
volatile _Bool HourInt = 1; //Basically when we have nothing that is time critical to the minute, we want to wake up less
//when Xbee is on, we are in the minute interrupt and want to turn it on and off accordingly. It is too much work to wake up
//with the SysTick every second or so and create another counter.  This uses the low frequency external oscillator.
volatile _Bool MinInt = 0;

//Scheduling Globals

//RTC Globals
static volatile RTC_C_Calendar SystemTime; //This is for within the system, it gets updated every time the RTC interrupt happens
static volatile RTC_C_Calendar SetTime; //Had to have a separate structure which got populated for updating the RTC time.  It was weird
//in that it wouldn't work with using the same structure, likely because the RTC handler was messing with it.

//UART ISR Globals
volatile uint8_t IridiumRXData = 0; //These are where the characters obtained on the UART buffer for each channel are stored.
volatile uint8_t GPSRXData = 0;

//PC Hardwire Globals
volatile char PCdataString[99]; //Puts the characters from the buffer into the string and increments so we can get more than one character
//at a time and get a useful message
volatile char PCString[99]; //Puts the raw string from the buffer into a new location for parsing. Gets updated when the PCdataString has a complete string.
volatile int PCindex = 0; //Increments within the string and puts the characters in and increments to the next position
volatile int PCflag = 0; //Used for when there has been the start character detected and the characters on the buffer are to be put into PCdataString
volatile int PCStringExtractGo = 0; //When a complete string has been formed and needs to be parsed, this flag gets set.
volatile _Bool USBPresentFlag = 0; //When the USB is connected, this flag is set and used within the code.
volatile _Bool USBPresentOns = 1; //One shot to initialize the PC UART channel
volatile _Bool MagnetRemovedFlag = 0; //Used for when the magnet has been removed and the "start up" procedure can take place.

//Power Calculation Globals
//All of these uint8_t's are for power calc's. The second counts are incremented in the SysTick ISR, and corresponding minutes and seconds thereafter.
volatile uint8_t XbeeSecCount;
volatile uint8_t IridiumSecOnCount;
volatile uint8_t GPSSecOnCount = 0;
volatile _Bool LongevityMode = 0; //When the battery is on the last legs this flag gets set and doesn't allow the GPS to get data points anymore
volatile _Bool CurrentLocationRequestInitiated = 0; //On the Xbee if the researcher requests for the current location of the collar, this is set
//and allows the location to be determined.
volatile _Bool NowGetFix = 0; //Another bit for getting the current location of the collar
volatile char BatteryString[4]; //The value of the calculated battery percentage is stored in here and sent over the PC and Xbee connection

volatile uint8_t VHFSecOnCount = 0; //On startup, this gets incremented and is a "beacon" so they know the collar is on.

//Cool structure I made to store all of the parameters obtained through the UART connection
static volatile GPSDataStruct GPSData;

//GPS Globals
volatile char dataString[300]; //Raw characters from the buffer are put in here
volatile char GPSString[300]; //When something needed to be parsed then it's put in this string
volatile int GPSindex = 0; //Same thing as PC string
volatile int GPSGo = 0;
volatile _Bool FixAttemptFailed = 0;

//Iridium Globals
volatile char IridiumString[300];
volatile int Iridiumindex = 0;
volatile int IridiumGo = 0;

#endif /* HEADERS_GLOBALS_H_ */
