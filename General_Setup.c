/*
 * General_Setup.c
 *
 *  Created on: Jul 13, 2018
 *      Author: kielev
 */


#include "Headers/General_Setup.h"

//overall function for control of program returns true for sleep or false to run again
_Bool checkControlConditions(){
    char* sendString;
    int condition = 0;
    int retry = -1;

    if (IridiumEn == 1){
        //Iridium On
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);

        pullOldFix(sendString, IRIDIUMFIXES);

        while(retry < Config.ICR && condition == 0){
            condition = sendIridiumString(sendString);
            retry++;
        }

        if(condition == 0){
            IridiumQuickRetry = true;
        } else if (condition == 1) {
            moveSentFix(IRIDIUMFIXES);
        } else if (condition == 2) {
            moveSentFix(IRIDIUMFIXES);
            updateConfigString();
        }
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
        return false;

    } else if (GPSEn == 1){
        //GPS On
        GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN0);
        FinalGPSData.HDOP = 20;

        while(GPSEn == 1){
            if(GPSGo){
                GPSParse();

                if(GPSData.HDOP < FinalGPSData.HDOP)
                    FinalGPSData = GPSData;
            }
        }

        if(FinalGPSData.HDOP < 20){
            /* write FinalGPSData to CurrentFixSaveString */
            sprintf(CurrentFixSaveString, "%d,%d,%4.4f,%c,%5.4f,%c,%1.2f"
                    , FinalGPSData.FixDate, FinalGPSData.FixTime, FinalGPSData.Lat, FinalGPSData.LatDir
                    , FinalGPSData.Lon, FinalGPSData.LonDir, FinalGPSData.HDOP);
            printf("%s\n", CurrentFixSaveString);
            save_current_fix();
        }
        GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN0);
        return false;

    } else if (VHFToggle == 1){
        GPIO_toggleOutputOnPin(GPIO_PORT_P4, GPIO_PIN7);
        VHFToggle = 0;
        return true;

    } else
        return true;
}

//update the overall set of configs from ParameterString
void updateConfigString(){

}

//update the overall set of configs from passing globals
void updateConfigGlobal(void){

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
    MAP_GPIO_interruptEdgeSelect(GPIO_PORT_P4, GPIO_PIN2, GPIO_LOW_TO_HIGH_TRANSITION);
    MAP_GPIO_interruptEdgeSelect(GPIO_PORT_P4, GPIO_PIN3, GPIO_HIGH_TO_LOW_TRANSITION);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN2|GPIO_PIN3);
    //MAP_GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN2|GPIO_PIN3);
    //MAP_Interrupt_enableInterrupt(INT_PORT4);

    //Setting up the enable pins for the GPS, VHF, and Xbee modules
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0);
    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
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
    RTC_C_TIME_EVENT_INTERRUPT | RTC_C_CLOCK_READ_READY_INTERRUPT);
    MAP_RTC_C_enableInterrupt(
    RTC_C_TIME_EVENT_INTERRUPT | RTC_C_CLOCK_READ_READY_INTERRUPT);

    /* Enable interrupts and go to sleep. */
    MAP_Interrupt_enableInterrupt(INT_RTC_C);
}

void EnableSysTick(void)
{
    //SysTick configuration - trigger at 1500000 ticks at 3MHz=0.5s
    MAP_SysTick_enableModule();
    MAP_SysTick_setPeriod(2999000);
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
