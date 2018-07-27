/*
 * Globals.c
 *
 *  Created on: Jul 15, 2018
 *      Author: Sam
 */

#include "Headers/Globals.h"


//Status Globals
//Req is when a request for a device is on, enable is what actually turns on a pin or off. Enables are set by the StateMachine
//which handles the requests and the priorites set out
volatile _Bool IridiumEn = 0;
volatile _Bool GPSEn = 0;
volatile _Bool VHFToggle = 0;


//Scheduling Globals

//UART ISR Globals
volatile uint8_t IridiumRXData = 0; //These are where the characters obtained on the UART buffer for each channel are stored.
volatile uint8_t GPSRXData = 0;

//PC Hardwire Globals
volatile int PCindex = 0; //Increments within the string and puts the characters in and increments to the next position
volatile int PCflag = 0; //Used for when there has been the start character detected and the characters on the buffer are to be put into PCdataString
volatile int PCStringExtractGo = 0; //When a complete string has been formed and needs to be parsed, this flag gets set.
volatile _Bool USBPresentFlag = 0; //When the USB is connected, this flag is set and used within the code.
volatile _Bool USBPresentOns = 1; //One shot to initialize the PC UART channel
volatile _Bool MagnetRemovedFlag = 0; //Used for when the magnet has been removed and the "start up" procedure can take place.

//Power Calculation Globals
//All of these uint8_t's are for power calc's. The second counts are incremented in the SysTick ISR, and corresponding minutes and seconds thereafter.
volatile uint16_t VHFCount = 0;
volatile uint16_t IridiumCount = 0;
volatile uint16_t GPSCount = 0;

volatile uint8_t GPSSecOnCount = 0;
volatile _Bool LongevityMode = 0; //When the battery is on the last legs this flag gets set and doesn't allow the GPS to get data points anymore
volatile _Bool CurrentLocationRequestInitiated = 0; //On the Xbee if the researcher requests for the current location of the collar, this is set
//and allows the location to be determined.
volatile _Bool NowGetFix = 0; //Another bit for getting the current location of the collar
volatile _Bool updateConfig = false;

volatile uint8_t VHFSecOnCount = 0; //On startup, this gets incremented and is a "beacon" so they know the collar is on.


volatile Configparameters Config = {4,1,1,0,0,1,9,12};
volatile _Bool BatteryLow = 0;

//GPS Globals
volatile int GPSindex = 0; //Same thing as PC string
volatile int GPSGo = 0;
volatile _Bool FixAttemptFailed = 0;
volatile _Bool GPSQuickRetry = false;
volatile _Bool RMCSetTime;

//Iridium Globals
volatile int Iridiumindex = 0;
volatile int IridiumGo = 0;
volatile _Bool IridiumQuickRetry = false;

//PC GUI Globals
volatile struct PC_Set_Time pc_set_time = {0,0,0,0,0,0};
volatile struct PC_GPS_Settings pc_gps_settings = {0,0};
volatile struct PC_Sat_Settings pc_sat_settings = {0,0,0,0,0};
volatile struct PC_VHF_Settings pc_vhf_settings = {0,0,0,0};
int volatile LED_STATE = 0;
int volatile LED_CHANGE = 0;
volatile int NEW_DATA_READY = 0;
volatile int PC_READY_DATA = 0;
volatile int ALL_DATA_SENT = 1;
volatile int SET_TIME_PRESSED = 0;
volatile int SET_GPS_PRESSED = 0;
volatile int SET_SAT_PRESSED = 0;
volatile int SET_VHF_PRESSED = 0;
volatile float READ_DATA_PROGRESS = 0;
