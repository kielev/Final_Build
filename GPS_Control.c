/*
 * GPS_Control.c
 *
 *  Created on: Jul 13, 2018
 *      Author: kielev
 */

#include "Headers/GPS_Control.h"


void GPS_puts(char *outString)
{
  unsigned int i, len;

  len = strlen(outString);

  for(i=0 ; i<len ; i++)
  {
      while((UCA2IFG & UCTXIFG) != UCTXIFG);  // wait until flag is set to indicate a new byte can be sent
      UCA2TXBUF = (uint8_t) outString[i];;  // load register with byte to send
  }
}

void initGPSUART(void)
{

    /* UART Configuration Parameter. These are the configuration parameters to
     * make the eUSCI A UART module to operate with a 115200 baud rate. These
     * values were calculated based on the instructions in the MSP432P4xx Family
     * Technical Reference Manual, section 22.3.10 p.721
     */
    const eUSCI_UART_Config uartConfig = {
    EUSCI_A_UART_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
            19,                                     // BRDIV = 39
            8,                                       // UCxBRF = 1
            0x65,                                       // UCxBRS = 0
            EUSCI_A_UART_NO_PARITY,                  // No Parity
            EUSCI_A_UART_LSB_FIRST,                  // LSB First
            EUSCI_A_UART_ONE_STOP_BIT,               // Two stop bits
            EUSCI_A_UART_MODE,                       // UART mode
            EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION  // Oversampling
            };

    /* Selecting P3.2 and P3.3 in UART mode */
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_P3,
            GPIO_PIN2 | GPIO_PIN3,
            GPIO_PRIMARY_MODULE_FUNCTION);

    /* Configuring UART Module for MicroHornet GPS*/
    MAP_UART_initModule(EUSCI_A2_BASE, &uartConfig);
    /* Enable UART module */
    MAP_UART_enableModule(EUSCI_A2_BASE);

    /* Enabling interrupts */
    MAP_UART_enableInterrupt(EUSCI_A2_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    MAP_Interrupt_enableInterrupt(INT_EUSCIA2);
    MAP_Interrupt_disableInterrupt(INT_EUSCIA0);
}

void disableGPSUART(void)
{
    MAP_UART_disableModule(EUSCI_A2_BASE);
    MAP_Interrupt_disableInterrupt(INT_EUSCIA2);

    // Reseting P3.2 and P3.3 from UART mode
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN2 | GPIO_PIN3);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN2 | GPIO_PIN3);
}
