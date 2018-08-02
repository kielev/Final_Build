/*
 * General_Setup.c
 *
 *  Created on: Jul 13, 2018
 *      Author: kielev
 */


#include "Headers/General_Setup.h"

//overall function for control of program returns true for sleep or false to run again
_Bool checkControlConditions(){
    char sendString[340] = {'\0'};
    int condition = 0;
    int moreUnsent = 0;
    int retry = -1;

    BatteryLow = batteryLowCalc();

    // This checks for a magnet on the Reed switch input, indicating the device should be put in deep sleep mode
    if (GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN3) == GPIO_INPUT_PIN_HIGH) {
        puts("sleep 4\n");
        //IOSetup(); //Initializes all of the pins in the most efficient way possible to keep battery life okay.
        MAP_PCM_enableRudeMode();
        MAP_PCM_gotoLPM4(); //this is for storing it on a shelf for an extended period of time. Uses the least power
        //for modes besides 4.5.
        systemStart();
    } else if (IridiumEn == 1) {
        //Iridium On
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
        Delay1ms(4000);
        do{
            MAP_WDT_A_clearTimer();
            // Grab oldesnt unread fixes
            pullOldFix(sendString, IRIDIUMFIXES);

            // Check that the string is the appropriate length to send
            if(strlen(sendString > 20)){
                // Loop until a successful connection has been made, limited by configured number of retries
                while(retry < Config.ICR && condition == 0){
                    printf("String: %s\n", sendString);
                    condition = sendIridiumString(sendString);
                    IridiumCount++;
                    retry++;
                }

                // State machine for managing send logic
                if(condition == 0){
                    moreUnsent = 0;
                    IridiumQuickRetry = true;
                } else if (condition == 1) {
                    moreUnsent = moveSentFix(IRIDIUMFIXES);
                } else if (condition == 2) {
                    moreUnsent = 0;
                    updateConfigString();
                } else if ((condition == 3)) {
                    moreUnsent = moveSentFix(IRIDIUMFIXES);
                    updateConfigString();
                }
            } else
                moreUnsent = 0;
        } while(moreUnsent > 0); // Loop until all pending fixes have been sent

        // Disable the Iridium once finished
        IridiumEn = 0;
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
        return false;

    } else if (GPSEn == 1) {
        //GPS On
        GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN0);
        printf("GTO: %d\n", Config.GTO);
        EnableSysTick();
        FinalGPSData.HDOP = 10;

        while(GPSEn == 1){
            if(GPSGo){
                GPSGo = 0;
                MAP_WDT_A_clearTimer();
                GPSParse();

                // If timeout is extended past one minute, loop to obtain the most accurate GPS fix available
                if(GPSData.HDOP < FinalGPSData.HDOP && GPSData.HDOP > 0.0) {
                    FinalGPSData = GPSData;
                    sprintf(CurrentFixSaveString, "%0.6d,%0.6d,%09.4f,%c,%010.4f,%c,%03.2f"
                            , FinalGPSData.FixDate, FinalGPSData.FixTime, FinalGPSData.Lat, FinalGPSData.LatDir
                            , FinalGPSData.Lon, FinalGPSData.LonDir, FinalGPSData.HDOP);
                    printf("GPS: %s\n", CurrentFixSaveString);
                    // Keep this fix if the HDOP is better (lower) than the previously stored fix
                    save_current_fix();
                }
            }
        }

        // Only save the GPS fix if it is reasonably accurate (HDOP < 10)
        if(FinalGPSData.HDOP < 10){
            /* write FinalGPSData to CurrentFixSaveString */
            sprintf(CurrentFixSaveString, "%0.6d,%0.6d,%09.4f,%c,%010.4f,%c,%03.2f"
                    , FinalGPSData.FixDate, FinalGPSData.FixTime, FinalGPSData.Lat, FinalGPSData.LatDir
                    , FinalGPSData.Lon, FinalGPSData.LonDir, FinalGPSData.HDOP);
            save_current_fix();
        }
        GPSEn = 0;
        DisableSysTick();
        // Disable GPS module
        GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN0);
        return false;

    } else if (VHFToggle == 1) {
        // Switch the VHF on/off if scheduled
        GPIO_toggleOutputOnPin(GPIO_PORT_P4, GPIO_PIN7);
        VHFToggle = 0;
    }
    return true;
}

// This is a routine of tasks that should be completed on initial startup, before the main while() loop is entered
void systemStart(){
    MAP_WDT_A_holdTimer();

    IOSetup();

    initClocks();

    initIridiumUART();
    initGPSUART();

    MAP_PSS_disableHighSide();

    // Enable all SRAM bank retentions prior to going to LPM3
    SYSCTL->SRAM_BANKRET |= SYSCTL_SRAM_BANKRET_BNK7_RET;

    RTC_setup();

    memory_locator_init();

    transmission_placeholder_init();

    readout_config_params();

    store_config_params();

    /** set for time when nothing will run */
    SetTime.hours = 11;
    SetTime.minutes = 59;
    SetTime.seconds = 00;
    SetTime.dayOfmonth = 21;
    SetTime.month = 7;
    SetTime.year = 2018;

    setDateTime();
    // We don't want to automatically sleep on ISR exit, instead allowing the logic in the main loop to determine if we should
    MAP_Interrupt_disableSleepOnIsrExit();

    MAP_Interrupt_enableMaster();
    MAP_WDT_A_startTimer();

}

//update the overall set of configs from ParameterString
void updateConfigString(){
    if(strlen(ParameterString) == 15){
        //BatteryLow = ParameterString[1] - '0';
        // Configurations are pulled from flash memory and stored there to maintain them through power cycles and LPM4
        Config.GPS = (ParameterString[2]-'0') * 10 + (ParameterString[3]-'0');
        Config.GTO = (ParameterString[4]-'0');
        Config.ITF = (ParameterString[5]-'0');
        Config.ITD = (ParameterString[6]-'0');
        Config.ICT = (ParameterString[7]-'0') * 10 + (ParameterString[8]-'0');
        Config.ICR = (ParameterString[9]-'0');
        Config.VST = (ParameterString[10]-'0') * 10 + (ParameterString[11]-'0');
        Config.VET = (ParameterString[12]-'0') * 10 + (ParameterString[13]-'0');
        store_config_params();
    }
}

// Used to determine if the battery is nearing end-of-life
_Bool batteryLowCalc(){ //need to update values
    int calc = (VHFUAH * VHFCount) + (IRIDIUMUAT * IridiumCount) + (GPSUAM * GPSCount);
    return ((BATTERYVALUE-calc)/(BATTERYVALUE/100) < BATTERYPERCENT );
}

// Updates the internal RTC based on the most recently stored time configuration
void setDateTime()
{
    int DOW1, DOW2;
    // First make sure day of week is calculated correctly
    DOW1 = ((SetTime.month + 9) % 12);
    DOW2 = (2000 + SetTime.year - (DOW1 / 10));
    SetTime.dayOfWeek = ((365 * DOW2 + (DOW2 / 4) - (DOW2 / 100) + (DOW2 / 400)
            + ((DOW1 * 306 + 5) / 10) + SetTime.dayOfmonth + 2) % 7);

    // Now, update the RTC using the values we have in the SetTime structure
    MAP_RTC_C_holdClock();
    MAP_RTC_C_initCalendar(&SetTime, RTC_C_FORMAT_BINARY);
    MAP_RTC_C_startClock();
}

//update the overall set of configs from passing globals
void updateConfigGlobal(void){
    if(SET_GPS_PRESSED)
    {
        SET_GPS_PRESSED = 0;
        // Map indices from PC GUI drop-down menu to number of hours in the GPS interval
        int gps_intervals[] = {1, 2, 3, 4, 6, 8, 12, 24};
        // Safety check that selected index is in bounds
        if(pc_gps_settings.num_hours >= 0 && pc_gps_settings.num_hours < 8)
        {
            // Update GPS interval
            Config.GPS = gps_intervals[pc_gps_settings.num_hours];
        }

        // Update GPS timeout
        Config.GTO = pc_gps_settings.timeout;
    }

    if(SET_SAT_PRESSED)
    {
        SET_SAT_PRESSED = 0;
        // Satellite upload day
        Config.ITD = pc_sat_settings.upload_day;

        // Satellite upload frequency
        Config.ITF = pc_sat_settings.frequency;

        // Satellite connection time (GUI is in 12-hr format while internally we use 24)
        Config.ICT = convert12to24(pc_sat_settings.hour_connect,  pc_sat_settings.am_pm);

        // Satellite allowed retries
        Config.ICR = pc_sat_settings.retries;
    }

    if(SET_VHF_PRESSED)
    {
        SET_VHF_PRESSED = 0;
        // VHF start time
        Config.VST = convert12to24(pc_vhf_settings.start_hour, pc_vhf_settings.start_am_pm);

        // VHF end time
        Config.VET = convert12to24(pc_vhf_settings.end_hour, pc_vhf_settings.end_am_pm);
    }

    if(SET_TIME_PRESSED)
    {
        SET_TIME_PRESSED = 0;
        // System time
        SetTime.month = pc_set_time.month;
        SetTime.year = pc_set_time.year - 2000;
        SetTime.dayOfmonth = pc_set_time.day;
        SetTime.hours = pc_set_time.hour;
        SetTime.minutes = pc_set_time.minute;
        SetTime.seconds = pc_set_time.second;
        setDateTime();
    }
    store_config_params();
}

_Bool newConfigReceivedPC()
{
    // Used to determine if the hardwired GUI has initiated a configuration change
    return (SET_GPS_PRESSED || SET_VHF_PRESSED || SET_TIME_PRESSED || SET_SAT_PRESSED);
}

// Converts 12-hour time (AM/PM) to 24 hour time
int convert12to24(int hour, _Bool pm)
{
    int result = 0; // Return value (hour in 24 format)
    if(pm)  // PM
    {
        result = 12;
        if(hour != 12)
        {
            result += hour;
        }
    }
    else  // AM
    {
        result = (hour == 12) ? 0 : hour;
    }
    return result;
}

void IOSetup(void)
{
    //This came from here https://e2e.ti.com/support/microcontrollers/msp430/f/166/t/18698

    //Goal is to have nothing floating, because it burns extra power.  All of the IO is set to
    //be inputs with pull down resistors on each unless configured otherwise which happens later
    //with other function calls.

    /* Configure I/O to minimize power consumption before going to sleep */
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, PIN_ALL8);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, PIN_ALL8);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, PIN_ALL8);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P4, PIN_ALL8);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, PIN_ALL8);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P6, PIN_ALL8);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P7, PIN_ALL8);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, PIN_ALL8);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P9, PIN_ALL8);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P10, PIN_ALL8);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_PJ, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, PIN_ALL8);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, PIN_ALL8);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P3, PIN_ALL8);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P4, PIN_ALL8);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P5, PIN_ALL8);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P6, (PIN_ALL8 & ~GPIO_PIN7));
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P7, PIN_ALL8);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P8, PIN_ALL8);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P9, PIN_ALL8);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P10, PIN_ALL8);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_PJ, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3);

    //Configuring LFXTOUT and LFXTIN for XTAL operation
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_PJ,
            GPIO_PIN0 | GPIO_PIN1,
            GPIO_PRIMARY_MODULE_FUNCTION);
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(
            GPIO_PORT_PJ, GPIO_PIN3 | GPIO_PIN4, GPIO_PRIMARY_MODULE_FUNCTION);

    // Configure P4.2 & P4.3 as inputs
    MAP_GPIO_setAsInputPin(GPIO_PORT_P4, GPIO_PIN2);
    MAP_GPIO_setAsInputPin(GPIO_PORT_P4, GPIO_PIN3);
    // Enable P4.2 & P4.3 interrupts - falling edge for 4.2, rising edge for 4.3
    //MAP_GPIO_interruptEdgeSelect(GPIO_PORT_P4, GPIO_PIN2, GPIO_LOW_TO_HIGH_TRANSITION);
    MAP_GPIO_interruptEdgeSelect(GPIO_PORT_P4, GPIO_PIN3, GPIO_HIGH_TO_LOW_TRANSITION);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN3);
    MAP_GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN3);
    MAP_Interrupt_enableInterrupt(INT_PORT4);

    //Setting up the enable pins for the GPS, VHF, and Iridium modules
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN0);
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN0);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN7);
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN7);
}

void initClocks(void)
{
    //Halting WDT
    MAP_WDT_A_holdTimer();

    CS_setExternalClockSourceFrequency(32768, 48000000);
    MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);
    MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
    MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);
    CS_startHFXT(false);
    //Starting HFXT
    MAP_CS_initClockSignal(CS_MCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_16);
    //Starting LFXT
    MAP_CS_startLFXT(CS_LFXT_DRIVE3);
    MAP_CS_initClockSignal(CS_BCLK, CS_LFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);

    // Configuring WDT to timeout after 128M iterations of SMCLK, at 3MHz,
    // this will roughly equal 42 seconds
    MAP_SysCtl_setWDTTimeoutResetType(SYSCTL_SOFT_RESET);
    MAP_WDT_A_initWatchdogTimer(WDT_A_CLOCKSOURCE_SMCLK,
    WDT_A_CLOCKITERATIONS_128M);
}

void RTC_setup(void)
{
    /* Specify an interrupt to assert every hour */
//    MAP_RTC_C_setCalendarEvent(RTC_C_CALENDAREVENT_MINUTECHANGE);
    MAP_RTC_C_setCalendarEvent(RTC_C_CALENDAREVENT_HOURCHANGE);

    /* Enable interrupt for RTC Ready Status, which asserts when the RTC
     * Calendar registers are ready to read.
     * Also, enable interrupts for the Calendar alarm and Calendar event. */

    //delete the read_ready_interrupt once this is ready to be buttoned up more
    MAP_RTC_C_clearInterruptFlag(
    RTC_C_TIME_EVENT_INTERRUPT );
    MAP_RTC_C_enableInterrupt(
    RTC_C_TIME_EVENT_INTERRUPT);

    /* Enable interrupts and go to sleep. */
    MAP_Interrupt_enableInterrupt(INT_RTC_C);
}


void EnableSysTick(void)
{
    //SysTick configuration - trigger at 1500000 ticks at 3MHz=0.5s
    MAP_SysTick_enableModule();
    MAP_SysTick_setPeriod(2990000);
    MAP_SysTick_enableInterrupt();
}

void DisableSysTick(void)
{
    MAP_SysTick_disableModule();
    MAP_SysTick_disableInterrupt();
}

//How a second delay is created without having to use ACLK or another source besides what I know. Delay1ms is included in this
void parrotdelay(unsigned long ulCount)
{
    __asm ( "pdloop:  subs    r0, #1\n"
            "    bne    pdloop\n");
}

void Delay1ms(uint32_t n) //just burns CPU as it decrements this counter. It was scaled to 3MHz from what we had in another project.
{
    while (n)
    {
        parrotdelay(369);    // 1 msec, tuned at 48 MHz is 5901, 3MHz set to 369
        n--;
    }
}
