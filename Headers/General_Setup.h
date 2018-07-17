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


#ifndef PINSETUP_H_
#define PINSETUP_H_

//TODO EK 7-16-2018 overall function for control of program returns true for sleep or false to run again
_Bool checkControlConditions(void);

//TODO EK 7-14-2018 update the overall set of configs from a string
void updateConfigString(char* String);

//TODO EK 7-14-2018 update the overall set of configs from passing globals
void updateConfigGlobal(void);

void IOSetup(void);

void initClocks(void);

void RTC_setup(void);

void EnableSysTick(void);

void DisableSysTick(void);

void parrotdelay(unsigned long ulCount);

void Delay1ms(uint32_t n);


#endif /* PINSETUP_H_ */
