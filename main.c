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
    /* Start Everything  */
    systemStart();

    /* Update System Time */
    SystemTime = MAP_RTC_C_getCalendarTime();

    // ST 7-21-2018 Remove this after testing that flash memory functions correctly
    //printf("%0.2d:%0.2d:%0.2d\n", SystemTime.hours, SystemTime.minutes, SystemTime.seconds);
    //memory_test();
    //GPSEn = true;
    //IridiumEn = true;
    //GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);

    while(1)
    {
        // Checks if configurations need to be loaded based on wake up
        if(updateConfig == true){
            // Readout configuration parameters into a global string
            readout_config_params();
            // Readout VHF, GPS, battery counters into globals
            readout_battery_counters();
            // Clear the flag
            updateConfig = false;
        }

        //If VHF needs to be started
        if(VHFStartCount == 0){
            EnableSysTick();
            GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN7);
        }

        // If one minute has passed since VHF was initiated
        else if(VHFStartCount == 60){
            // Enable VHF
            GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN7);
            // Stop the SysTick timer
            DisableSysTick();
            VHFStartCount++;
        }

        // Run logic for determining which modules should be initiated
        else if(VHFStartCount >= 61 && checkControlConditions()){
            MAP_WDT_A_holdTimer();
            // Check memory
            if(isMemoryFull()){
                clearMemory();
            }

            MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0); //Puts the enable pins for periph devices high so they're off
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
            MAP_GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN0);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN0);
            disableIridiumUART(); //Disables UART channels and associated pins, trying to stay the most power efficient.
            disableGPSUART();

            // Stop the watchdog before sleeping
            MAP_PCM_enableRudeMode();
            MAP_PCM_gotoLPM3();
            // The code should re-enter here once woken up again -- start the watchdog timer and let the loop roll over to the next iteration
            MAP_WDT_A_startTimer();
            initIridiumUART();
            initGPSUART();
        }
        // Check if the PC hardwired GUI has initiated a change in configuration parameters
        if(newConfigReceivedPC())
        {
            updateConfigGlobal();
        }
        // Check if the PC hardwired GUI is requesting that the next GPS string in memory be loaded
        if(PC_READY_DATA)
        {
            readout_memory_all();
        }
        // Kick the dog
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
        // If normal battery status and GPS time has been hit, enable the GPS
        if(((SystemTime.hours % Config.GPS) == 0)  && BatteryLow == 0){
            GPSCount += Config.GTO;
            RMCSetTime = 0;
            GPSEn = 1;
        }

        // Check which week we are on and compare it to the Iridium scheduling configuration
        if((((SystemTime.hours == Config.ICT)
                && (((SystemTime.dayOfmonth-1) / 7) % Config.ITF == (Config.ITF-1)) // 1 - Every Week, 2 - 8-14,22-28, 3 - 15-21
                && (SystemTime.dayOfWeek == Config.ITD)) || IridiumQuickRetry == true) && BatteryLow == 0){
            // If an Iridium upload is called for, enable the module
            IridiumEn = 1;
            IridiumQuickRetry = false;
        }

        // If the VST on-time is scheduled to begin now
        if(SystemTime.hours == Config.VST) {
            // Turn on the VHF
            VHFToggle = 1;
            VHFStarted = 1;
            // Check if VST/VET occurs on the same or consecutive days
            if(Config.VST < Config.VET)
                VHFCount += Config.VET - Config.VST;
            else
                VHFCount += (24-Config.VST) + Config.VET;
        }
        // If the VST off-time is scheduled to end
        if(SystemTime.hours == Config.VET && VHFStarted == 1){
            // Turn off the VHF
            VHFToggle = 1;
            VHFStarted = 0;
        }
    }
}

//This is used for a second interrupt to count how long the Iridium/GPS has been on.
//The seconds, minutes, and hours of each device are incremented and the values in
//each is stored into flash as needed.  These parameters are also pulled on startup.
void SysTick_IRQHandler(void)
{
    //Counter for the total GPS on time
    if (VHFStartCount < 60){
            VHFStartCount++;
    } else if (GPSEn == 1 && IridiumEn != 1) {
        GPSSecOnCount++;
        if(GPSSecOnCount / 60 > (Config.GTO-1)){
            GPSEn = 0;
            GPSSecOnCount = 0;

            //For Full System Test Remove For Operation
            //IridiumEn = 1;
        }
    }
}

//PORT 4 ISR, this is for determining if the magnet is present or not on pin 3
//and if the USB is present or not on pin 2.
void PORT4_IRQHandler(void)
{
    //Get the status of Port 4
    uint32_t status;
    status = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P4);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P4, status);

    /*initClocks(); //Initialize clocks because we are waking up from sleep
    MAP_WDT_A_startTimer(); //Start watchdog because we are waking up from sleep
    initIridiumUART();
    initGPSUART();*/

    //If the magnet is removed
    if (status & GPIO_PIN3)
    {
        MAP_RTC_C_startClock(); //Start the RTC
        VHFStartCount = 0;
        updateConfig = true;
    }
}


/* EUSCI A1 UART ISR - Stores Iridium response data */
void EUSCIA1_IRQHandler(void)
{
    uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A1_BASE);

    MAP_UART_clearInterruptFlag(EUSCI_A1_BASE, status);

    if (status & EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG)
    {
        // Grab the most recently received character
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
}
