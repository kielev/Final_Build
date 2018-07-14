/*
 * Memeory_Management.h
 *
 *  Created on: Jul 13, 2018
 *      Author: kielev
 */

#include <stdint.h>
#include <math.h>
#include "rom_map.h"
#include "driverlib.h"
#include "msp432.h"

//Where Flash Bank 1 begins
#define START_LOC 0x00020000

//Flash Globals
static volatile char CurrentFixSaveString[33]; //This is the string that is constructed that has the time, date, lat and long
//in it that came from the GPS when a fix was to be obtained. When there isn't a fix, it's populated properly before getting
//written to flash.
static volatile char FixRead[33]; //Stores the 32 bytes from flash that has a fix in it, it's 33 length for the end character
static volatile char SectorRead[4097]; //Same, but this one can store an entire sector.
static volatile uint8_t FixMemoryLocator[2]; //FixMemoryLocator[0] stores position in the current sector, range 0-124. //??
//FixMemoryLocator[1] stores the current sector, range 0-31.
static volatile uint8_t MemPlaceholder[2]; //MemPlaceholder[0] stores position in the readout sector, range 0-128. //??
//MemPlaceholder[1] stores the readout sector, range 0-31

#ifndef HEADERS_MEMORY_MANAGEMENT_H_
#define HEADERS_MEMORY_MANAGEMENT_H_


//TODO EK 7-14-2018 Function to pull oldest unsent fixes and assemble a string
void pullOldFix(char* String, int n);

//Saves the current fix into memory and increments the location tracking
void save_current_fix(void);

//Reads out a single fix given a starting address, see flash address cheat sheet if needed
void readout_fix(unsigned startposition);

//Reads out a complete sector given a starting address, see flash address cheat sheet if needed
void readout_sector(unsigned startposition);

//Reads out all of the NEW memory locations with data, see flash address cheat sheet if needed
void readout_memory_new(void);

//Reads out the last known GPS location from flash
void readout_last_known_location(void);

//Reads out all of the memory locations with data
void readout_memory_all(void);

//Resets the memory location tracking
void reset_memory_locator(void);

//Executes a mass erase of the flash
void flash_mass_erase(void);

//Stores the memory location of the next wireless transmission start point
void transmission_placeholder_store(void);

//Resets the the placeholder for the next wireless transmission
void transmission_placeholder_reset(void);

_Bool isMemoryFull(void);

void setMemoryFull(_Bool Status);

#endif /* HEADERS_MEMORY_MANAGEMENT_H_ */
