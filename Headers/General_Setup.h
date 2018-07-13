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


void IOSetup(void);

void initClocks(void);

void RTC_setup(void);

void EnableSysTick(void);

void DisableSysTick(void);

void parrotdelay(unsigned long ulCount);

void Delay1ms(uint32_t n);


#endif /* PINSETUP_H_ */
