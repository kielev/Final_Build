/*
 * Memory_Management.c
 *
 *  Created on: Jul 13, 2018
 *      Author: kielev
 */

#include "Headers/Memory_Management.h"

volatile _Bool MemoryFull = 0;
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//Function to pull oldest unsent fixes and assemble a string
void pullOldFix(char* String, int n){
    uint8_t ReadFixCount[2]; //This stores the current sector position and current sector read out from flash
    uint8_t TransmitFixCount[2]; // Last transmission sector position and sector
    String[0] = '\0';

    String[0] = Config.GPS;
    String[1] = Config.GTO;
    String[2] = Config.ICR;
    ConfigSave[3] = Config.ICT;
    ConfigSave[4] = Config.ITD;
    ConfigSave[5] = Config.ITF;
    ConfigSave[6] = Config.VET;
    ConfigSave[7] = Config.VST;
    ConfigSave[8] = '\0';

   //Get current memory location
    TransmitFixCount[0] = *(uint8_t*) (0x0003E000);
    TransmitFixCount[1] = *(uint8_t*) (0x0003E001);

   // Last place we stored a fix (TODO Check if this is right or if it's this + 1 more fix)
    ReadFixCount[0] = *(uint8_t*) (0x0003F000); // should this 3E for transmission placeholder?
    ReadFixCount[1] = *(uint8_t*) (0x0003F001);

   //Compute the offset for the save address
   unsigned offsetTransmit = (FIX_SIZE * TransmitFixCount[0]) + (4096 * TransmitFixCount[1]);
   unsigned offsetRead = (FIX_SIZE * ReadFixCount[0]) + (4096 * ReadFixCount[1]);

   int maxFixesTransmittable = (offsetRead - offsetTransmit)/FIX_SIZE;
   if(n > maxFixesTransmittable)
   {
       // don't try to read more than we have stored
       n = maxFixesTransmittable;
   }

   int i;
   // IT WILL ALWAYS BE 7
   for(i = 0; i < n; ++i)
   {
       int nextTransmit = i * FIX_SIZE + TransmitFixCount[0];
       if(nextTransmit + FIX_SIZE > 4096)
       {
           TransmitFixCount[0] = 0;
           TransmitFixCount[1] += 1;
           offsetTransmit = 4096 * (TransmitFixCount[1]);
       }
       readout_fix(0x00020000 + (i * FIX_SIZE) + offsetTransmit);
       strcat(String, '\n');
       strcat(String, FixRead);
   }
   String[14+(FIX_SIZE+1)*n] = '\0';
}

// TODO EK 7-18-2018 move transmission placeholder n gps location and update memory
void moveSentFix(int n){
    uint8_t ReadFixCount[2]; //This stores the current sector position and current sector read out from flash
    uint8_t TransmitFixCount[2]; // Last transmission sector position and sector

   //Get current memory location
    TransmitFixCount[0] = *(uint8_t*) (0x0003E000);
    TransmitFixCount[1] = *(uint8_t*) (0x0003E001);
   // Last place we stored a fix (TODO Check if this is right or if it's this + 1 more fix)
    ReadFixCount[0] = *(uint8_t*) (0x0003F000); // should this 3E for transmission placeholder?
    ReadFixCount[1] = *(uint8_t*) (0x0003F001);
   //Compute the offset for the save address
   unsigned offsetTransmit = (FIX_SIZE * TransmitFixCount[0]) + (4096 * TransmitFixCount[1]);
   unsigned offsetRead = (FIX_SIZE * ReadFixCount[0]) + (4096 * ReadFixCount[1]);
   int maxFixesTransmittable = (offsetRead - offsetTransmit)/FIX_SIZE;



   if(n > maxFixesTransmittable)
   {
       // don't try to read more than we have stored
   }
       n = maxFixesTransmittable;

   int sectorRemain = SECTOR_CAPACITY - TransmitFixCount[0];
   // This assumes n won't be larger than SECTOR_SIZE
   if(n > sectorRemain)
   {
       MemPlaceholder[0] = n - sectorRemain;
       do
       {
           MemPlaceholder[1] = TransmitFixCount[1]++;
           MemPlaceholder[0] -= MemPlaceholder[0] > SECTOR_CAPACITY ? SECTOR_CAPACITY : 0;
       } while(MemPlaceholder[0] > SECTOR_CAPACITY);
   }
   else
   {
       MemPlaceholder[0] = TransmitFixCount[0] + n;
       MemPlaceholder[1] = TransmitFixCount[1];
   }

   transmission_placeholder_store();
}

//Store the configuration parameters to flash
int store_config_params(void)
{
    int result = 0;

    //Declare save array
    uint8_t ConfigSave[9] = { 0 };
    ConfigSave[0] = Config.GPS;
    ConfigSave[1] = Config.GTO;
    ConfigSave[2] = Config.ICR;
    ConfigSave[3] = Config.ICT;
    ConfigSave[4] = Config.ITD;
    ConfigSave[5] = Config.ITF;
    ConfigSave[6] = Config.VET;
    ConfigSave[7] = Config.VST;
    ConfigSave[8] = '\0';

    //Save the config params to flash
    FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR28); //unprotect sector
    FlashCtl_eraseSector(0x0003C000); //erase the sector
    FlashCtl_enableWordProgramming(FLASH_IMMEDIATE_WRITE_MODE); // Allow for immediate writing
    result = FlashCtl_programMemory(ConfigSave, (void*) 0x0003C000, 9); //write the data
    FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR28); //protect sector

    return result;
}

//Readout the configuration parameters from flash
void readout_config_params(void)
{
    uint8_t FlashCheck[8]; //Store what's in flash here

    //See if flash has been initialized
    FlashCheck[0] = *(uint8_t*) (0x0003C000);
    FlashCheck[1] = *(uint8_t*) (0x0003C001);
    FlashCheck[2] = *(uint8_t*) (0x0003C002);
    FlashCheck[3] = *(uint8_t*) (0x0003C003);
    FlashCheck[4] = *(uint8_t*) (0x0003C004);
    FlashCheck[5] = *(uint8_t*) (0x0003C005);
    FlashCheck[6] = *(uint8_t*) (0x0003C006);
    FlashCheck[7] = *(uint8_t*) (0x0003C007);

    //Fill structure from flash if initialized
    if (FlashCheck[0] != 255)
    {
        Config.GPS = *(uint8_t*) (0x0003C000);
    }
    if (FlashCheck[1] != 255)
    {
        Config.GTO = *(uint8_t*) (0x0003C001);
    }
    if (FlashCheck[2] != 255)
    {
        Config.ICR = *(uint8_t*) (0x0003C002);
    }
    if (FlashCheck[3] != 255)
    {
        Config.ICT = *(uint8_t*) (0x0003C003);
    }
    if (FlashCheck[4] != 255)
    {
        Config.ITD = *(uint8_t*) (0x0003C004);
    }
    if (FlashCheck[5] != 255)
    {
        Config.ITF = *(uint8_t*) (0x0003C005);
    }
    if (FlashCheck[6] != 255)
    {
        Config.VET = *(uint8_t*) (0x0003C006);
    }
    if (FlashCheck[7] != 255)
    {
        Config.VST = *(uint8_t*) (0x0003C007);
    }
}

//Saves the current fix into memory and increments the location tracking
void save_current_fix(void)
{
    uint8_t ReadFixCount[2]; //This stores the current sector position and current sector read out from flash

    //Get current memory location
    ReadFixCount[0] = *(uint8_t*) (0x0003F000);
    ReadFixCount[1] = *(uint8_t*) (0x0003F001);

    //Compute the offset for the save address
    unsigned offset = (FIX_SIZE * ReadFixCount[0]) + (4096 * ReadFixCount[1]);

    //Compute the sector address
    unsigned CurSector = pow(2, ReadFixCount[1]);

    //Save fix if memory available
    if (MemoryFull == 0)
    {
        FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, CurSector); //unprotect sector
        FlashCtl_enableWordProgramming(FLASH_IMMEDIATE_WRITE_MODE); // Allow for immediate writing
        FlashCtl_programMemory(CurrentFixSaveString,
                               (void*) 0x00020000 + offset, FIX_SIZE); //write the data
        FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, CurSector); //protect sector
    }

    //Update the memory location tracking
    //@NOTE 5-7-2018 128 = num of fixes that can be stored in sector
    if (ReadFixCount[0] < SECTOR_CAPACITY)
    {
        ReadFixCount[0]++;
    }
    else
    {
        ReadFixCount[0] = 0;
        if (ReadFixCount[1] < 27) //Sectors 28, 29, 30, and 31 reserved, not for location data
        {
            ReadFixCount[1]++;
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
    FlashCtl_programMemory(ReadFixCount, (void*) 0x0003F000, 2); //write the data
    FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31); //protect sector
}

//Reads out a single fix given a starting address, see flash address cheat sheet if needed
void readout_fix(unsigned startposition)
{
    int i = 0;
    for (i = 0; i < FIX_SIZE; i++)
    {
        //printf("%c\n",*(uint8_t*) (i + startposition));
        FixRead[i] = *(uint8_t*) (i + startposition);
    }
    FixRead[FIX_SIZE] = '\0';
    //printf("%s\n", FixRead);
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

void memory_test()
{
    srand(time(0));
    char testStr[100] = {'\0'};

    int i;
    for(i = 0; i < 2; ++i)
    {
        sprintf(testStr, "$GPGGA,%.2d%.2d%.2d,%08.4f,N,%09.4f,W,1,%.2d,%.2f,%.1f,M,%.1f,M,,,*%.2d"
                , SystemTime.hours, SystemTime.minutes, SystemTime.seconds, rand()%9999 + (float)rand()/RAND_MAX
                , rand()%9999 + (float)rand()/RAND_MAX, rand() % 31, rand()%9+ (float)rand()/RAND_MAX
                , rand()%1000 + (float)rand()/RAND_MAX, rand()%1000 + (float)rand()/RAND_MAX, rand()%98 + 1);
        //printf("TestString: %s\n", testStr);
        strcpy(GPSString, testStr);
        GPSParse();
        sprintf(CurrentFixSaveString, "%.6d,%.6d,%09.4f,%c,%010.4f,%c,%.2f"
                            , GPSData.FixDate, GPSData.FixTime, GPSData.Lat, GPSData.LatDir
                            , GPSData.Lon, GPSData.LonDir, GPSData.HDOP);
                    //printf("%s\n", CurrentFixSaveString);
        save_current_fix();
    }

    pullOldFix(sendString, 7);

    printf("String: %s\n", sendString);
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
    int max = (ReadFixCount[1] * SECTOR_CAPACITY) + ReadFixCount[0];

    //Get the address that the Xbee left off at last transmission
    ReadPlaceholder[0] = *(uint8_t*) (0x0003E000);
    ReadPlaceholder[1] = *(uint8_t*) (0x0003E001);

    //Compute the hexadecimal offset for the readout address
    unsigned offset = (FIX_SIZE * ReadPlaceholder[0]) + (4096 * ReadPlaceholder[1]);

    //Compute the integer offset for the readout address
    int offsetInt = (ReadPlaceholder[1] * SECTOR_CAPACITY) + ReadPlaceholder[0];

    //Grab fixes from flash and push them out through the Xbee
    for (i = 0; i < max - offsetInt; i++)
    {
        MAP_WDT_A_clearTimer();
        readout_fix(0x00020000 + offset + (i * 0x00000020));
        //Xbee_puts(FixRead);
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
    int max = (ReadFixCount[1] * SECTOR_CAPACITY) + ReadFixCount[0];

    //Grab fixes from flash and push them out through the Xbee
    for (i = max - 1; i > 0; i--)
    {
        readout_fix(0x00020000 + (i * 0x00000020));
        if (FixRead[17] != '*')
        {
            //Xbee_puts(FixRead);
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
    int max = (ReadFixCount[1] * SECTOR_CAPACITY) + ReadFixCount[0];

    //Grab fixes from flash and push them out through the Xbee or PC
    for (i_1 = 0; i_1 < max; i_1++)
    {
        MAP_WDT_A_clearTimer();
        while(!PC_READY_DATA);
        NEW_DATA_READY = 0;
        readout_fix(0x00020000 + (i_1 * 0x00000020));
        NEW_DATA_READY = 1;
        //PC_puts(FixRead);
    }
    ALL_DATA_SENT = 1;
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

//Initialize the memory location tracking
void memory_locator_init(void)
{
    uint8_t FlashCheck[2]; //Store what's in flash here

    //See if flash has been initialized
    FlashCheck[0] = *(uint8_t*) (0x0003F000);
    FlashCheck[1] = *(uint8_t*) (0x0003F001);

    //If flash hasn't been initialized, set to 0
    if (FlashCheck[0] == 255 && FlashCheck[1] == 255)
    {
        FixMemoryLocator[0] = 0;
        FixMemoryLocator[1] = 0;
        FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31); //unprotect sector
        FlashCtl_eraseSector(0x0003F000); //erase the sector
        FlashCtl_enableWordProgramming(FLASH_IMMEDIATE_WRITE_MODE); // Allow for immediate writing
        FlashCtl_programMemory(FixMemoryLocator, (void*) 0x0003F000, 2); //write the data
        FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31); //protect sector
    }
    //If initialized already, readout values
    else
    {
        FixMemoryLocator[0] = *(uint8_t*) (0x0003F000);
        FixMemoryLocator[1] = *(uint8_t*) (0x0003F001);
    }
}

//Initialize the placeholder for the next wireless transmission
void transmission_placeholder_init(void)
{
    uint8_t FlashCheck[2]; //Store what's in flash here

    //See if flash has been initialized
    FlashCheck[0] = *(uint8_t*) (0x0003E000);
    FlashCheck[1] = *(uint8_t*) (0x0003E001);

    //If flash hasn't been initialized, set to 0
    if (FlashCheck[0] == 255 && FlashCheck[1] == 255)
    {
        MemPlaceholder[0] = 0;
        MemPlaceholder[1] = 0;
        FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR30); //unprotect sector
        FlashCtl_eraseSector(0x0003E000); //erase the sector
        FlashCtl_enableWordProgramming(FLASH_IMMEDIATE_WRITE_MODE); // Allow for immediate writing
        FlashCtl_programMemory(MemPlaceholder, (void*) 0x0003E000, 2); //write the data
        FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR30); //protect sector
    }
    //If initialized already, readout values
    else
    {
        MemPlaceholder[0] = *(uint8_t*) (0x0003E000);
        MemPlaceholder[1] = *(uint8_t*) (0x0003E001);
    }
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
