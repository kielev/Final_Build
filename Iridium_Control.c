/*
 * Iridium_Control.c
 *
 *  Created on: Jul 13, 2018
 *      Author: kielev
 */

#include <gpio.h>
#include <interrupt.h>
#include <msp432p401r.h>
#include <msp432p401r_classic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uart.h>
#include "Headers/Iridium_Control.h"

//Function to send a string of any length up to max
int sendIridiumString(char * String){
    char SBDIX[30] = {'\0'};
    char *tokString;
    int ret = 0;

    printf("\nCommand: AT\n");
    Iridium_puts("AT\r");

    // Wait until Iridium is ready with a new string
    while(IridiumGo == 0);

    // Check for an AT string
    if(!strncmp("AT",IridiumString,2))
        IridiumGo = 0;
    else{
        // If we are not getting this, an error/lockup is occurring
        printf("AT echo error %s\n", IridiumString);
        IridiumGo = 0;
    }

    // Wait again until Iridium has provided a new string
    while(IridiumGo == 0);
    if(!strncmp("OK",IridiumString,2)){
        IridiumGo = 0;
    } else {
        IridiumGo = 0;
        printf("Error AT %s\n", IridiumString);
        return 0;
    }

    // Ask the Iridium is do it's thing
    printf("\nCommand: AT&K0\n");
    Iridium_puts("AT&K0\r");
    // Wait for a response
    while(IridiumGo == 0);
    if(!strncmp("AT&K0",IridiumString,5))
        IridiumGo = 0;
    else{
        printf("AT&K0 echo error %s\n", IridiumString);
        IridiumGo = 0;
    }

    // Wait for an acknowledgment
    while(IridiumGo == 0);
    if(!strncmp("OK",IridiumString,2)){
        IridiumGo = 0;
    } else {
        IridiumGo = 0;
        printf("Error AT&K0 %s\n", IridiumString);
        return 0;
    }

    // We tell the Iridium we are ready to send a new message
    printf("\nCommand: AT+SBDWT\n");
    Iridium_puts("AT+SBDWT\r");
    while(IridiumGo == 0);
    if(!strncmp("AT+SBDWT",IridiumString,8))
        IridiumGo = 0;
    else{
        printf("AT+SBDWT echo error %s\n", IridiumString);
        IridiumGo = 0;
    }

    // Wait for the Iridium to say we can send it a message
    while(IridiumGo == 0);
    if(!strncmp("READY",IridiumString,5)){
        IridiumGo = 0;
    } else {
        IridiumGo = 0;
        printf("Error AT+SBDWT %s\n", IridiumString);
        return 0;
    }

    // Send the message itself
    printf("\nCommand: message\n");
    Iridium_puts(String);
    while(IridiumGo == 0);
    // We look for the $ character because we use it to indicate the start of a new data message
    if(!strncmp("$",IridiumString,1)){
        IridiumGo = 0;
    } else {
        IridiumGo = 0;
        printf("Error message %s\n", IridiumString);
        return 0;
    }

    // Check that it's good with our message
    while(IridiumGo == 0);
        if(!strncmp("0",IridiumString,1)){
            IridiumGo = 0;
        } else {
            IridiumGo = 0;
            printf("Error return %s\n", IridiumString);
        }

    // Kick the dog (sorry, buddy)
    MAP_WDT_A_clearTimer();

    // Tell Iridium to go ahead and send that message
    printf("\nCommand: AT+SBDIX\n");
    Iridium_puts("AT+SBDIX\r");
    while(IridiumGo == 0);
    if(!strncmp("AT",IridiumString,2))
        IridiumGo = 0;
    else if (IridiumString[0] == '\0'){
        IridiumGo = 0;
        while(IridiumGo == 0);
        IridiumGo = 0;
    }
    else{
        printf("AT+SBDIX echo error %s\n", IridiumString);
        IridiumGo = 0;
    }

    // Make sure Iridium is OK
    while(IridiumGo == 0);
    if(!strncmp("+SBDIX",IridiumString,6)){
        strcpy(SBDIX, IridiumString);
        IridiumGo = 0;
    } else {
        printf("Error +SBDIX %s\n", IridiumString);
        IridiumGo = 0;
    }

    // Check for end of the message before continuing
    while(IridiumGo == 0);
    if(!strncmp("\r",IridiumString,1)){
        IridiumGo = 0;
    } else {
        IridiumGo = 0;
        printf("Error return %s\n", IridiumString);
    }

    // Check Iridium again
    while(IridiumGo == 0);
    if(!strncmp("OK",IridiumString,2)){
        IridiumGo = 0;
    } else {
        IridiumGo = 0;
        printf("Error message %s\n", IridiumString);
    }

    // Ask Iridium for its current status (including if we have any messages waiting)
    printf("\nCommand: AT+SBDD0\n");
    Iridium_puts("AT+SBDD0\r");
    while(IridiumGo == 0);
    if(!strncmp("AT",IridiumString,2))
        IridiumGo = 0;
    else{
        printf("AT+SBDD0 echo error %s\n", IridiumString);
        IridiumGo = 0;
    }

    while(IridiumGo == 0);
    if(!strncmp("0",IridiumString,1)){
        IridiumGo = 0;
    } else {
        printf("Error AT+SBDD0 %s\n", IridiumString);
        IridiumGo = 0;
    }

    // Iridium will tell us with an error code if our message failed to send
    printf("message: %s --> ", SBDIX);
    tokString = strtok(SBDIX, ",");
    if(atoi(&tokString[8]) > 2){
        printf("send failure - %d\n", atoi(&tokString[8]));
        ret = 0;
    } else{
        printf("send success\n");
        ret = 1;
    }

    strtok(NULL, ",");
    tokString = strtok(NULL, ",");
    // Check if a message is queued, awaiting for us to retrieve it from the Iridium system
    if(atoi(tokString) == 1){
        printf("message queued\n");

        // Tell it to go ahead and give us that message
        Iridium_puts("AT+SBDRT\r");
        IridiumGo = 0;
        while(IridiumGo == 0);
        if(!strncmp("AT",IridiumString,2))
            IridiumGo = 0;
        else{
            printf("AT+SBRT echo error %s\n", IridiumString);
            IridiumGo = 0;
        }

        // Make sure it got our request and is good with it
        while(IridiumGo == 0);
        if(!strncmp("+SBDRT",IridiumString,6))
            IridiumGo = 0;
        else{
            printf("AT+SBRT error %s\n", IridiumString);
            IridiumGo = 0;
        }

        // Check for the $ character indicating the start of an inbound parameter message
        while(IridiumGo == 0);
        if(!strncmp("$",IridiumString,1)){
            strcpy(ParameterString, IridiumString);
            IridiumGo = 0;
        } else {
            printf("Message Error %s\n", IridiumString);
            IridiumGo = 0;
        }

        // Check that Iridium is still OK
        while(IridiumGo == 0);
        if(!strncmp("OK",IridiumString,2)){
            IridiumGo = 0;
        } else {
            IridiumGo = 0;
            printf("End Message Error %s\n", IridiumString);
        }

        // Store that parameter message so we can update our internal configurations
        strcat(ParameterString,"\0");
        printf("Received Message - %s\n", ParameterString);
        return (ret+2);

    } else {
        IridiumGo=0;
        printf("no message received - %d\n", atoi(tokString));
        return ret;
    }
}


void Iridium_puts(char *outString)
{
  unsigned int i, len;

  len = strlen(outString);

  for(i=0 ; i<len ; i++)
  {
      while((UCA1IFG & UCTXIFG) != UCTXIFG);  // wait until flag is set to indicate a new byte can be sent
      UCA1TXBUF = (uint8_t) outString[i];  // load register with byte to send
  }
}

void initIridiumUART(void)
{

    /* UART Configuration Parameter. These are the configuration parameters to
     * make the eUSCI A UART module to operate with a 115200 baud rate. These
     * values were calculated based on the instructions in the MSP432P4xx Family
     * Technical Reference Manual, section 22.3.10 p.721
     */
    const eUSCI_UART_Config uartConfig = {
    EUSCI_A_UART_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
            9,                                     // BRDIV = 39
            12,                                       // UCxBRF = 1
            0x0,                                       // UCxBRS = 0
            EUSCI_A_UART_NO_PARITY,                  // No Parity
            EUSCI_A_UART_LSB_FIRST,                  // LSB First
            EUSCI_A_UART_ONE_STOP_BIT,               // Two stop bits
            EUSCI_A_UART_MODE,                       // UART mode
            EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION  // Oversampling
            };

    // set up EUSCI0 in UART mode
    /* Selecting P1.2 and P1.3 in UART mode */
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_P2,
            GPIO_PIN2 | GPIO_PIN3,
            GPIO_PRIMARY_MODULE_FUNCTION);

    /* Configuring UART Module for communication with Iridium*/
    MAP_UART_initModule(EUSCI_A1_BASE, &uartConfig);
    /* Enable UART module */
    MAP_UART_enableModule(EUSCI_A1_BASE);

    /* Enabling interrupts */
    MAP_UART_enableInterrupt(EUSCI_A1_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    MAP_Interrupt_enableInterrupt(INT_EUSCIA1);
    //MAP_Interrupt_disableInterrupt(INT_EUSCIA2);
}

void disableIridiumUART(void)
{
    MAP_UART_disableModule(EUSCI_A0_BASE);
    MAP_Interrupt_disableInterrupt(INT_EUSCIA0);

    // Reseting P1.2 and P1.3 from UART mode
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN2 | GPIO_PIN3);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN2 | GPIO_PIN3);
}

