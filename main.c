/* DriverLib Includes */
#include "driverlib.h"

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>

#include "Headers/Iridium_Control.h"
#include "Headers/Globals.h"
#include "Headers/General_Setup.h"
#include "Headers/GPS_Control.h"
#include "Headers/Memory_Management.h"

int main(void)
{


    /* Stop Watchdog  */
    systemStart();

    SystemTime = MAP_RTC_C_getCalendarTime();

    printf("%0.2d:%0.2d:%0.2d\n", SystemTime.hours, SystemTime.minutes, SystemTime.seconds);

    // ST 7-21-2018 Remove this after testing that flash memory functions correctly
    //memory_test();
    //GPSEn = true;
    //IridiumEn = true;
    //return 1;

    while(1)
    {
        puts("looping\n");
        if(updateConfig == true){
            readout_config_params();
            readout_battery_counters();
            updateConfig = false;
        }
        if(checkControlConditions()){
            puts("Sleeping\n");
            MAP_WDT_A_holdTimer();
            MAP_PCM_enableRudeMode();
            MAP_PCM_gotoLPM3();
            MAP_WDT_A_startTimer();
        }
        if(newConfigReceivedPC())
        {
            updateConfigGlobal();
        }
        if(PC_READY_DATA)
        {
            readout_memory_all();
        }
        MAP_WDT_A_clearTimer();
    }
}




// RTC ISR
void RTC_C_IRQHandler(void)
{
    uint32_t status;

    status = MAP_RTC_C_getEnabledInterruptStatus();
    MAP_RTC_C_clearInterruptFlag(status);
    SystemTime = MAP_RTC_C_getCalendarTime(); //Update Time

    if (status & RTC_C_TIME_EVENT_INTERRUPT)
    {
        if(((SystemTime.hours % Config.GPS) == 0)  && BatteryLow == 0){
            GPSCount += Config.GTO;
            RMCSetTime = 0;
            GPSEn = 1;
        }

        if(((!(SystemTime.hours % Config.ICT)
                && (((SystemTime.dayOfmonth-1) / 7) % Config.ITF == (Config.ITF-1)) // 1 - Every Week, 2 - 8-14,22-28, 3 - 15-21
                && (SystemTime.dayOfWeek == Config.ITD)) || IridiumQuickRetry == true) && BatteryLow == 0){
            IridiumEn = 1;
            IridiumQuickRetry = false;
        }

        if((SystemTime.hours % Config.VST) == 0) {
            VHFToggle = 1;
            if(Config.VST < Config.VET)
                VHFCount += Config.VET - Config.VST;
            else
                VHFCount += (24-Config.VST) + Config.VET;
        }
        if((SystemTime.hours % Config.VET) == 0){
            VHFToggle = 1;
        }
    }
    //puts("RTC\n");
}

//This is used for a second interrupt to count how long the Iridium/GPS has been on.
//The seconds, minutes, and hours of each device are incremented and the values in
//each is stored into flash as needed.  These parameters are also pulled on startup.
void SysTick_IRQHandler(void)
{
    //Counter for the total GPS on time
    if (GPSEn == 1)
    {
        GPSSecOnCount++;
        if(GPSSecOnCount / 60 > (Config.GTO-1)){
            GPSEn = 0;
            GPSSecOnCount = 0;

            //For Full System Test Remove For Operation
            //IridiumEn = 1;
        }
    }

    if (VHFStartUp){

    }
    //puts("SysTick\n");
}

//PORT 4 ISR, this is for determining if the magnet is present or not on pin 3
//and if the USB is present or not on pin 2.
void PORT4_IRQHandler(void)
{
    //Get the status of Port 4
    uint32_t status;
    status = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P4);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P4, status);

    initClocks(); //Initialize clocks because we are waking up from sleep
    MAP_WDT_A_startTimer(); //Start watchdog because we are waking up from sleep

    //If the magnet is removed
    if (status & GPIO_PIN3)
    {
        MAP_RTC_C_startClock(); //Start the RTC
        updateConfig = true;
    }
    //puts("Port4\n");
}


/* EUSCI A1 UART ISR - Stores Iridium response data */
void EUSCIA1_IRQHandler(void)
{
    uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A1_BASE);

    MAP_UART_clearInterruptFlag(EUSCI_A1_BASE, status);

    if (status & EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG)
    {
        IridiumRXData = MAP_UART_receiveData(EUSCI_A1_BASE);
        //printf("%c", RXData);
        switch (IridiumRXData)
        {
        case '\n': //looks for a new line character which is the end of the AT string
            IridiumString[Iridiumindex] = '\0'; //puts a NULL at the end of the string again because strncpy doesn't
            Iridiumindex = 0;
            IridiumGo = 1;
            break;

        default:
            IridiumString[Iridiumindex] = IridiumRXData; //puts what was in the buffer into an index in dataString
            Iridiumindex++; //increments the index so the next character received in the buffer can be stored
            //into dataString
            break;
        }
    }
    //puts("A1\n");
}


/* EUSCI A2 UART ISR - Echoes data back to PC host (from GPS to monitor on serial monitor) */
void EUSCIA2_IRQHandler(void)
{
    uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A2_BASE);

    MAP_UART_clearInterruptFlag(EUSCI_A2_BASE, status);

    if (status & EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG)
    {
        GPSRXData = MAP_UART_receiveData(EUSCI_A2_BASE);
        //printf("%c", RXData);
        switch (GPSRXData)
        {
        case '\n': //looks for a new line character which is the end of the NMEA string
            GPSString[GPSindex] = '\0'; //puts a NULL at the end of the string again because strncpy doesn't
            GPSindex = 0;
            GPSGo = 1;
            break;

        default:
            GPSString[GPSindex] = GPSRXData; //puts what was in the buffer into an index in dataString
            GPSindex++; //increments the index so the next character received in the buffer can be stored
            //into dataString
            break;
        }
    }
    ///puts("A2\n");
}
