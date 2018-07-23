/*
 * Memeory_Management.h
 *
 *  Created on: Jul 13, 2018
 *      Author: kielev
 */
#ifndef HEADERS_MEMORY_MANAGEMENT_H_
#define HEADERS_MEMORY_MANAGEMENT_H_

#include <stdint.h>
#include <math.h>
#include "rom_map.h"
#include "driverlib.h"
#include "msp432.h"
#include "Headers/Globals.h"

//Where Flash Bank 1 begins
#define START_LOC 0x00020000
#define FIX_SIZE 43
#define SECTOR_CAPACITY 95 // Number of fixes that can be stored in a sector

//Flash Globals
volatile char CurrentFixSaveString[FIX_SIZE + 1]; //This is the string that is constructed that has the time, date, lat, long, and quality
//in it that came from the GPS when a fix was to be obtained. When there isn't a fix, it's populated properly before getting
//written to flash.
volatile char FixRead[FIX_SIZE + 1]; //Stores the 45  bytes from flash that has a fix in it, it's 46 length for the end character
volatile char SectorRead[4097]; //Same, but this one can store an entire sector.
volatile uint8_t FixMemoryLocator[2]; //FixMemoryLocator[0] stores position in the current sector, range 0-90. //??
//FixMemoryLocator[1] stores the current sector, range 0-44.
volatile uint8_t MemPlaceholder[2]; //MemPlaceholder[0] stores position in the readout sector, range 0-128. //??not sure
//MemPlaceholder[1] stores the readout sector, range 0-44

extern volatile _Bool MemoryFull;

//TODO EK 7-14-2018 Function to pull oldest unsent fixes and assemble a string
void pullOldFix(char* String, int n);

int store_config_params(void);

void readout_config_params(void);

// TODO EK 7-18-2018 move transmission placeholder n gps location and update memory
void moveSentFix(int n);

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

// Writes a bunch of random GPS fixes to memory
void memory_test();

//Resets the memory location tracking
void reset_memory_locator(void);

void memory_locator_init(void);

void transmission_placeholder_init(void);

//Executes a mass erase of the flash
void flash_mass_erase(void);

//Stores the memory location of the next wireless transmission start point
void transmission_placeholder_store(void);

//Resets the the placeholder for the next wireless transmission
void transmission_placeholder_reset(void);

_Bool isMemoryFull(void);

void setMemoryFull(_Bool Status);

#endif /* HEADERS_MEMORY_MANAGEMENT_H_ */
