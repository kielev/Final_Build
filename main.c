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
    MAP_WDT_A_holdTimer();

    IOSetup();

    initClocks();

    RTC_setup();

    MAP_WDT_A_startTimer();

    while(1)
    {
        if(checkControlConditions()){
            MAP_PCM_enableRudeMode();
            MAP_PCM_gotoLPM3();
        }
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
        if((SystemTime.hours % Config.GPS) == 0){
            GPSEn = 1;
            EnableSysTick();
        }

        if(!(SystemTime.hours % Config.ICT)
                && (((SystemTime.dayOfmonth-1) / 7) % Config.ITF == (Config.ITF-1)) // 1 - Every Week, 2 - 8-14,22-28, 3 - 15-21
                && (SystemTime.dayOfWeek == Config.ITD)){
            IridiumEn = 1;
        }

        if((SystemTime.hours % Config.VST) == 0 || (SystemTime.hours % Config.VET) == 0)
            VHFToggle = 1;
    }
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
        if(GPSSecOnCount / 60 > Config.GTO){
            GPSEn = 0;
            GPSSecOnCount = 0;
            DisableSysTick();
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

    initClocks(); //Initialize clocks because we are waking up from sleep
    MAP_WDT_A_startTimer(); //Start watchdog because we are waking up from sleep

    //If the magnet is removed
    if (status & GPIO_PIN3)
    {
        MAP_RTC_C_startClock(); //Start the RTC

        //Check for the UBS present
        if (MAP_GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN2) == 1)
        {
            //initPCUART(); //Initialize the PC UART
            USBPresentFlag = 1; //USB present
            GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
            GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN0);
            GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN7);
        }
        else
        {
            MagnetRemovedFlag = 1; //USB is not present, magnet is removed.
            //VHFStartUpCount = 0; //Clear the timer for the VHF beacon that occurs on system start up.
            EnableSysTick();
        }
    }

    if (status & GPIO_PIN2)
    {
        //initPCUART();
        MAP_RTC_C_startClock();
        USBPresentFlag = 1;
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
        GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN0);
        GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN7);
    }
}

// EUSCI A0 UART ISR - Captures what's input from the serial emulator and puts it in a buffer for the MCU to deal with.

void EUSCIA0_IRQHandler(void)
{
    uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A0_BASE);

    MAP_UART_clearInterruptFlag(EUSCI_A0_BASE, status);
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
