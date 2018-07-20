/*
 * PinSetup.h
 *
 *  Created on: Jun 25, 2018
 *      Author: kielev
 */

#include "driverlib.h"

/* Standard Includes */
#include <stdio.h>
#include <string.h>

#include "Headers/Iridium_Control.h"
#include "Headers/GPS_Control.h"
#include "Headers/Memory_Management.h"


#ifndef PINSETUP_H_
#define PINSETUP_H_

#define IRIDIUMFIXES 7


_Bool checkControlConditions(void);

//TODO EK 7-14-2018 update the overall set of configs from a string
void updateConfigString();

//TODO EK 7-14-2018 update the overall set of configs from passing globals
void updateConfigGlobal(void);

void IOSetup(void);

void initClocks(void);

void RTC_setup(void);

_Bool newConfigReceivedPC();

void setDateTime();

// ST 7-19-2018 A helper function for converting an AM/PM hour to the 24-hour clock
int convert12to24(int hour, _Bool pm);

void EnableSysTick(void);

void DisableSysTick(void);

void parrotdelay(unsigned long ulCount);

void Delay1ms(uint32_t n);


#endif /* PINSETUP_H_ */
