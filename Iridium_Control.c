/*
 * Iridium_Control.c
 *
 *  Created on: Jul 13, 2018
 *      Author: kielev
 */

#include "Headers/Iridium_Control.h"


int IridiumConfigure(void){
    Iridium_puts("AT&K0\r");
    while(strncmp("OK",IridiumString,2) != 0);
    IridiumString[0] = '\0';

    Iridium_puts("AT+SBDMTA=1\r");
    while(strncmp("OK",IridiumString,2) != 0);
    IridiumString[0] = '\0';

    Iridium_puts("AT&W0\r");
    while(strncmp("OK",IridiumString,2) != 0);
    IridiumString[0] = '\0';

    Iridium_puts("AT&Y0\r");
    while(strncmp("OK",IridiumString,2) != 0);
    IridiumString[0] = '\0';

    printf("Iridium Configure Finished\n");
}


//Function to send a string of any length up to max
int sendIridiumString(char * String){
    char SBDIX[30] = {'\0'};
    char *tokString;
    int x = 0;

    /*Iridium_puts("AT+CIER=1,0,1\r");
    while(strncmp("OK",IridiumString,2) != 0 && strncmp("ERROR",IridiumString,5) != 0);
    IridiumString[0] = '\0';
    if(!strncmp("ERROR",IridiumString,5)){
            printf("ERROR\n");
            return 0;
    }

    while(strncmp("+CIEV: 1,1",IridiumString,10) != 0 && x < 1000){
        printf("CIEV: %s\n", IridiumString);
        Delay1ms(1);
        x++;
    }
    IridiumString[0] = '\0';

    if(x >= 1000){
        return 0;
    }*/

    printf("Message: %s\n", String);

    Iridium_puts("AT+SBDWT\r");
    while(strncmp("READY",IridiumString,5) != 0);

    Iridium_puts(String);
    while(strncmp("OK",IridiumString,2) != 0 && strncmp("0",IridiumString,1) != 0); //

    MAP_WDT_A_clearTimer();

    printf("SBDIX\n");
    Iridium_puts("AT+SBDIX\r");

    while(strncmp("+SBDIX",IridiumString,6) != 0);
    printf("SBDIX: %s\n", IridiumString);
    strcpy(SBDIX, IridiumString);
    tokString = strtok(SBDIX, ",");

    if(atoi(&tokString[7]) > 2 || strlen(SBDIX) < 10){
        return 0;
    }

    strtok(NULL, ",");
    tokString = strtok(NULL, ",");
    if(!strcmp(tokString,"1")){
        printf("Message Recieved\n");
        Iridium_puts("AT+SBDRT\r");

        while(strncmp("+SBDRT",IridiumString,6) != 0);

        while(strncmp("$",IridiumString,1) != 0); //checks for parameter string only

        strcpy(ParameterString, IridiumString); //copy to Parameter string to be stored
        return 2;
    }
    printf("Message Sent\n");

    return 1;
}


void Iridium_puts(char *outString)
{
  unsigned int i, len;

  len = strlen(outString);

  for(i=0 ; i<len ; i++)
  {
      while((UCA1IFG & UCTXIFG) != UCTXIFG);  // wait until flag is set to indicate a new byte can be sent
      UCA1TXBUF = (uint8_t) outString[i];;  // load register with byte to send
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

