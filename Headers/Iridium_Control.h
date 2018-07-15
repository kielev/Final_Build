/*
 * Iridium_Control.h
 *
 *  Created on: Jul 13, 2018
 *      Author: kielev
 */

#ifndef HEADERS_IRIDIUM_CONTROL_H_
#define HEADERS_IRIDIUM_CONTROL_H_

/* DriverLib Includes */
#include "driverlib.h"
#include "msp432.h"
#include "rom_map.h"
#include "Headers/General_Setup.h"
#include "Headers/Globals.h"

//TODO EK 7-14-2018 Function to send a string of any length up to max
void sendIridiumString(char* String);

//TODO EK 7-14-2018 Function to receive an Iridium String
void receiveIridiumString(char* String);

void initIridiumUART(void);

void disableIridiumUART(void);

void Iridium_puts(char *outString);


#endif /* HEADERS_IRIDIUM_CONTROL_H_ */
