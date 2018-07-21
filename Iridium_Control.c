/*
 * Iridium_Control.c
 *
 *  Created on: Jul 13, 2018
 *      Author: kielev
 */

#include "Headers/Iridium_Control.h"


//Function to send a string of any length up to max
int sendIridiumString(char * String){
    char* IMessage;
    int x = 0;

    Iridium_puts("AT\r");
    while(strncmp("OK",IridiumString,2) != 0 && x < 100){
        Delay1ms(1);
        x++;
    }
    if(x >= 100){
        return 0;
    }
    x = 0;

    Iridium_puts("AT&K0\r");
    strcpy(IridiumString, "NO");
    while(strncmp("OK",IridiumString,2) != 0 && x < 100){
        Delay1ms(1);
        x++;
    }
    if(x >= 100){
        return 0;
    }
    x = 0;

    sprintf(IMessage,"AT+SBDWT=%s\r", String);
    Iridium_puts(IMessage);
    strcpy(IridiumString, "NO");
    while(strncmp("OK",IridiumString,2) != 0 && strncmp("ERROR",IridiumString,5) != 0 && x < 100){
        Delay1ms(1);
        x++;
    }
    if(x >= 100){
        return 0;
    }
    x = 0;

    Iridium_puts("AT+SBDIX\r");
    strcpy(IridiumString, "NO");
    while(strncmp("+SBDIX",IridiumString,6) != 0 && x < 2000){
        Delay1ms(1);
        x++;
    }
    if(x >= 2000 ||strlen(IridiumString) < 7 || strncmp(&IridiumString[8],"2",1) > 0){
        return 0;
    }
    x = 0;

    if(!strncmp(&IridiumString[14],"1,",2)){
        Iridium_puts("AT+SBDRT\r");
        strcpy(IridiumString, "NO");
        while(strncmp("+SBDRT",IridiumString,6) != 0 && x < 2000){
            Delay1ms(1);
            x++;
        }
        if(x >= 2000){
            return 1;
        }
        x=0;
        while(strncmp("$",IridiumString,1) != 0 && x < 2000){
            Delay1ms(1);
            x++;
        }
        if(x >= 2000){
            return 1;
        }
        strcpy(ParameterString, IridiumString);
        return 2;
    }
    return 1;
}


void Iridium_puts(char *outString)
{
  unsigned int i, len;

  len = strlen(outString);

  for(i=0 ; i<len ; i++)
  {
      while((UCA0IFG & UCTXIFG) != UCTXIFG);  // wait until flag is set to indicate a new byte can be sent
      UCA0TXBUF = (uint8_t) outString[i];;  // load register with byte to send
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
            39,                                     // BRDIV = 39
            1,                                       // UCxBRF = 1
            0,                                       // UCxBRS = 0
            EUSCI_A_UART_NO_PARITY,                  // No Parity
            EUSCI_A_UART_LSB_FIRST,                  // LSB First
            EUSCI_A_UART_TWO_STOP_BITS,               // Two stop bits
            EUSCI_A_UART_MODE,                       // UART mode
            EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION  // Oversampling
            };

    // set up EUSCI0 in UART mode
    /* Selecting P1.2 and P1.3 in UART mode */
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_P1,
            GPIO_PIN2 | GPIO_PIN3,
            GPIO_PRIMARY_MODULE_FUNCTION);

    /* Configuring UART Module for communication with PC*/
    MAP_UART_initModule(EUSCI_A0_BASE, &uartConfig);
    /* Enable UART module */
    MAP_UART_enableModule(EUSCI_A0_BASE);

    /* Enabling interrupts */
    MAP_UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    MAP_Interrupt_enableInterrupt(INT_EUSCIA0);
    MAP_Interrupt_disableInterrupt(INT_EUSCIA2);
}

void disableIridiumUART(void)
{
    MAP_UART_disableModule(EUSCI_A0_BASE);
    MAP_Interrupt_disableInterrupt(INT_EUSCIA0);

    // Reseting P1.2 and P1.3 from UART mode
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN2 | GPIO_PIN3);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN2 | GPIO_PIN3);
}

