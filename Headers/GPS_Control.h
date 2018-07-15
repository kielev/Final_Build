/*
 * GPS_Control.h
 *
 *  Created on: Jul 13, 2018
 *      Author: kielev
 */

/* DriverLib Includes */
#include "driverlib.h"
#include "msp432.h"
#include "rom_map.h"
#include "Headers/Structures.h"
#include <string.h>
#include <stdlib.h>

#ifndef HEADERS_GPS_CONTROL_H_
#define HEADERS_GPS_CONTROL_H_

void GPSParse(char *String, GPSDataStruct *GPSData);

void GPS_puts(char *outString);

void initGPSUART(void);

void disableGPSUART(void);

#endif /* HEADERS_GPS_CONTROL_H_ */
