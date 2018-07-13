/*
 * Iridium_Control.h
 *
 *  Created on: Jul 13, 2018
 *      Author: kielev
 */

/* DriverLib Includes */
#include "driverlib.h"
#include "msp432.h"
#include "rom_map.h"

#ifndef HEADERS_IRIDIUM_CONTROL_H_
#define HEADERS_IRIDIUM_CONTROL_H_


void initIridiumUART(void);

void disableIridiumUART(void);

void Iridium_puts(char *outString);


#endif /* HEADERS_IRIDIUM_CONTROL_H_ */
