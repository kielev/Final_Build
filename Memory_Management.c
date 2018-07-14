/*
 * Memory_Management.c
 *
 *  Created on: Jul 13, 2018
 *      Author: kielev
 */

#include "Headers/Memory_Management.h"

volatile _Bool MemoryFull = 0;


//Function to pull oldest unsent fixes and assemble a string
void pullOldFix(char* String, int n){

}

//Saves the current fix into memory and increments the location tracking
void save_current_fix(void)
{
    uint8_t ReadFixCount[2]; //This stores the current sector position and current sector read out from flash

    //Get current memory location
    ReadFixCount[0] = *(uint8_t*) (0x0003F000);
    ReadFixCount[1] = *(uint8_t*) (0x0003F001);

    //Compute the offset for the save address
    unsigned offset = (32 * ReadFixCount[0]) + (4096 * ReadFixCount[1]);

    //Compute the sector address
    unsigned CurSector = pow(2, ReadFixCount[1]);

    //Save fix if memory available
    if (MemoryFull == 0)
    {
        FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, CurSector); //unprotect sector
        FlashCtl_enableWordProgramming(FLASH_IMMEDIATE_WRITE_MODE); // Allow for immediate writing
        FlashCtl_programMemory(CurrentFixSaveString,
                               (void*) 0x00020000 + offset, 32); //write the data
        FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, CurSector); //protect sector
    }

    //Update the memory location tracking
    //@NOTE 5-7-2018 128 = num of fixes that can be stored in sector
    if (FixMemoryLocator[0] < 127)
    {
        FixMemoryLocator[0]++;
    }
    else
    {
        FixMemoryLocator[0] = 0;
        if (FixMemoryLocator[1] < 27) //Sectors 28, 29, 30, and 31 reserved, not for location data
        {
            FixMemoryLocator[1]++;
        }
        else
        {
            setMemoryFull(1);
        }
    }
    //Save the memory location tracking to flash
    // @NOTE 5-7-2018 Location of most recent fix in flash memory
    FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31); //unprotect sector
    FlashCtl_eraseSector(0x0003F000); //erase the sector
    FlashCtl_enableWordProgramming(FLASH_IMMEDIATE_WRITE_MODE); // Allow for immediate writing
    FlashCtl_programMemory(FixMemoryLocator, (void*) 0x0003F000, 2); //write the data
    FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31); //protect sector
}

//Reads out a single fix given a starting address, see flash address cheat sheet if needed
void readout_fix(unsigned startposition)
{
    int i = 0;
    for (i = 0; i < 32; i++)
    {
        FixRead[i] = *(uint8_t*) (i + startposition);
    }
    FixRead[32] = '\0';
}

//Reads out a complete sector given a starting address, see flash address cheat sheet if needed
void readout_sector(unsigned startposition)
{
    int i = 0;
    for (i = 0; i < 4096; i++)
    {
        SectorRead[i] = *(uint8_t*) (i + startposition);
    }
    SectorRead[4096] = '\0';
}

//Reads out all of the NEW memory locations with data, see flash address cheat sheet if needed
void readout_memory_new(void)
{
    int i = 0;
    uint8_t ReadFixCount[2]; //This stores the current sector position and current sector read out from flash
    uint8_t ReadPlaceholder[2]; //This stores the sector position and sector of the last point passed over wireless transmission succesfully

    //Get current memory location
    ReadFixCount[0] = *(uint8_t*) (0x0003F000);
    ReadFixCount[1] = *(uint8_t*) (0x0003F001);

    //Compute amount of points saved
    int max = (ReadFixCount[1] * 128) + ReadFixCount[0];

    //Get the address that the Xbee left off at last transmission
    ReadPlaceholder[0] = *(uint8_t*) (0x0003E000);
    ReadPlaceholder[1] = *(uint8_t*) (0x0003E001);

    //Compute the hexadecimal offset for the readout address
    unsigned offset = (32 * ReadPlaceholder[0]) + (4096 * ReadPlaceholder[1]);

    //Compute the integer offset for the readout address
    int offsetInt = (ReadPlaceholder[1] * 128) + ReadPlaceholder[0];

    //Grab fixes from flash and push them out through the Xbee
    for (i = 0; i < max - offsetInt; i++)
    {
        MAP_WDT_A_clearTimer();
        readout_fix(0x00020000 + offset + (i * 0x00000020));
        Xbee_puts(FixRead);
    }
}

//Reads out the last known GPS location from flash
void readout_last_known_location(void)
{
    MAP_WDT_A_clearTimer();
    int i = 0;
    uint8_t ReadFixCount[2]; //This stores the current sector position and current sector read out from flash

    //Get current memory location
    ReadFixCount[0] = *(uint8_t*) (0x0003F000);
    ReadFixCount[1] = *(uint8_t*) (0x0003F001);

    //Compute amount of points saved
    int max = (ReadFixCount[1] * 128) + ReadFixCount[0];

    //Grab fixes from flash and push them out through the Xbee
    for (i = max - 1; i > 0; i--)
    {
        readout_fix(0x00020000 + (i * 0x00000020));
        if (FixRead[17] != '*')
        {
            Xbee_puts(FixRead);
            i = 0; //fail safe :)
            break;
        }

    }
}

//Reads out all of the memory locations with data
void readout_memory_all(void)
{
    int i_1 = 0;
    uint8_t ReadFixCount[2]; //This stores the current sector position and current sector read out from flash

    //Get current memory location
    ReadFixCount[0] = *(uint8_t*) (0x0003F000);
    ReadFixCount[1] = *(uint8_t*) (0x0003F001);

    //Compute amount of points saved
    int max = (ReadFixCount[1] * 128) + ReadFixCount[0];

    //Grab fixes from flash and push them out through the Xbee or PC
    for (i_1 = 0; i_1 < max; i_1++)
    {
        MAP_WDT_A_clearTimer();
        readout_fix(0x00020000 + (i_1 * 0x00000020));


         //PC_puts(FixRead);

    }
}

//Resets the memory location tracking
void reset_memory_locator(void)
{
    FixMemoryLocator[0] = 0;
    FixMemoryLocator[1] = 0;
    FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31); //unprotect sector
    FlashCtl_eraseSector(0x0003F000); //erase the sector
    FlashCtl_enableWordProgramming(FLASH_IMMEDIATE_WRITE_MODE); // Allow for immediate writing
    FlashCtl_programMemory(FixMemoryLocator, (void*) 0x0003F000, 2); //write the data
    FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31); //protect sector
}

//Executes a mass erase of the flash
void flash_mass_erase(void)
{
    int i = 0;
    int sector_array[32] = { FLASH_SECTOR0, FLASH_SECTOR1, FLASH_SECTOR2,
    FLASH_SECTOR3,
                             FLASH_SECTOR4, FLASH_SECTOR5,
                             FLASH_SECTOR6,
                             FLASH_SECTOR7, FLASH_SECTOR8, FLASH_SECTOR9,
                             FLASH_SECTOR10,
                             FLASH_SECTOR11,
                             FLASH_SECTOR12,
                             FLASH_SECTOR13, FLASH_SECTOR14, FLASH_SECTOR15,
                             FLASH_SECTOR16,
                             FLASH_SECTOR17,
                             FLASH_SECTOR18,
                             FLASH_SECTOR19, FLASH_SECTOR20, FLASH_SECTOR21,
                             FLASH_SECTOR22,
                             FLASH_SECTOR23,
                             FLASH_SECTOR24,
                             FLASH_SECTOR25, FLASH_SECTOR26, FLASH_SECTOR27,
                             FLASH_SECTOR28,
                             FLASH_SECTOR29,
                             FLASH_SECTOR30,
                             FLASH_SECTOR31 };

    for (i = 0; i < 32; i++)
    {
        WDT_A_clearTimer();
        FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1,
                                 sector_array[i]);
    }
    FlashCtl_initiateMassErase();
    for (i = 0; i < 32; i++)
    {
        WDT_A_clearTimer();
        FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, sector_array[i]);
    }
}

//Stores the memory location of the next wireless transmission start point
void transmission_placeholder_store(void)
{
    //Save the last successful transmission memory location to flash
    FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR30); //unprotect sector
    FlashCtl_eraseSector(0x0003E000); //erase the sector
    FlashCtl_enableWordProgramming(FLASH_IMMEDIATE_WRITE_MODE); // Allow for immediate writing
    FlashCtl_programMemory(MemPlaceholder, (void*) 0x0003E000, 2); //write the data
    FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR30); //protect sector
}

//Resets the the placeholder for the next wireless transmission
void transmission_placeholder_reset(void)
{
    MemPlaceholder[0] = 0;
    MemPlaceholder[1] = 0;
    FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR30); //unprotect sector
    FlashCtl_eraseSector(0x0003E000); //erase the sector
    FlashCtl_enableWordProgramming(FLASH_IMMEDIATE_WRITE_MODE); // Allow for immediate writing
    FlashCtl_programMemory(MemPlaceholder, (void*) 0x0003E000, 2); //write the data
    FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR30); //protect sector
}

_Bool isMemoryFull(void){
    return MemoryFull;
}

void setMemoryFull(_Bool Status){
    MemoryFull = Status;
}


