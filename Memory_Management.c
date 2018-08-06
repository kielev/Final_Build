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

    sprintf(String,"$%.1d%.2d%.1d%.1d%.1d%.2d%.1d%.2d%.2d"
            , BatteryLow, Config.GPS, Config.GTO, Config.ITF, Config.ITD, Config.ICT, Config.ICR, Config.VST, Config.VET);


   //Get current memory location
    TransmitFixCount[0] = *(uint8_t*) (0x0003E000);
    TransmitFixCount[1] = *(uint8_t*) (0x0003E001);

   // Last place we stored a fix (TODO Check if this is right or if it's this + 1 more fix)
    ReadFixCount[0] = *(uint8_t*) (0x0003F000); // should this 3E for transmission placeholder?
    ReadFixCount[1] = *(uint8_t*) (0x0003F001);

   //Compute the offset for the save address
   unsigned offsetTransmit = (FIX_SIZE * TransmitFixCount[0]) + (4085 * TransmitFixCount[1]);
   unsigned offsetRead = (FIX_SIZE * ReadFixCount[0]) + (4085 * ReadFixCount[1]);

   int maxFixesTransmittable = ((offsetRead - offsetTransmit) / FIX_SIZE);
   //printf("Max: %d\n", maxFixesTransmittable);
   offsetTransmit += (11 * TransmitFixCount[1]);

   if(n > maxFixesTransmittable)
   {
       // don't try to read more than we have stored
       n = maxFixesTransmittable;
   }


   int i;

   // In this loop, we are running through the messages available for transmission until all have been loaded/sent
   for(i = 0; i < n; ++i)
   {
       int nextTransmit = (i + TransmitFixCount[0]) * FIX_SIZE;

       // Roll over to the next sector if we exceed its size
       if((nextTransmit + FIX_SIZE) > 4096)
       {
           TransmitFixCount[0] = 0;
           TransmitFixCount[1] += 1;
           offsetTransmit = (4096 * (TransmitFixCount[1])) - (i * FIX_SIZE);
       }
       readout_fix(0x00020000 + (i * FIX_SIZE) + offsetTransmit);
       strcat(String, FixRead);

       // Separate fixes with a colon (:) symbol
       if(i != n-1)
           strcat(String, ":");
   }
   strcat(String, "\r");
}

// TODO EK 7-18-2018 move transmission placeholder n gps location and update memory
int moveSentFix(int n){
    uint8_t ReadFixCount[2]; //This stores the current sector position and current sector read out from flash
    uint8_t TransmitFixCount[2]; // Last transmission sector position and sector

   //Get current memory location
    TransmitFixCount[0] = *(uint8_t*) (0x0003E000);
    TransmitFixCount[1] = *(uint8_t*) (0x0003E001);

   // Last place we stored a fix (TODO Check if this is right or if it's this + 1 more fix)
    ReadFixCount[0] = *(uint8_t*) (0x0003F000); // should this 3E for transmission placeholder?
    ReadFixCount[1] = *(uint8_t*) (0x0003F001);

   //Compute the offset for the save address
   unsigned offsetTransmit = (FIX_SIZE * TransmitFixCount[0]) + (4085 * TransmitFixCount[1]);
   unsigned offsetRead = (FIX_SIZE * ReadFixCount[0]) + (4085 * ReadFixCount[1]);
   int maxFixesTransmittable = ((offsetRead - offsetTransmit)/FIX_SIZE);

   if(n > maxFixesTransmittable)
   {
       // don't try to read more than we have stored
       n = maxFixesTransmittable;
   }

   int sectorRemain = SECTOR_CAPACITY - TransmitFixCount[0];
   // This assumes n won't be larger than SECTOR_SIZE
   if(n >= sectorRemain)
   {
       MemPlaceholder[0] = n - sectorRemain;
       MemPlaceholder[1] = TransmitFixCount[1] + 1;
   }
   else
   {
       MemPlaceholder[0] = TransmitFixCount[0] + n;
       MemPlaceholder[1] = TransmitFixCount[1];
   }

   transmission_placeholder_store();
   return (maxFixesTransmittable-n);
}

void clearMemory(void){
    int i = 0, loc=0;
    uint8_t Save[4096];

    int sector_array[32] = { FLASH_SECTOR0, FLASH_SECTOR1, FLASH_SECTOR2,
                             FLASH_SECTOR3, FLASH_SECTOR4, FLASH_SECTOR5,
                             FLASH_SECTOR6, FLASH_SECTOR7, FLASH_SECTOR8,
                             FLASH_SECTOR9, FLASH_SECTOR10,FLASH_SECTOR11,
                             FLASH_SECTOR12,FLASH_SECTOR13,FLASH_SECTOR14,
                             FLASH_SECTOR15,FLASH_SECTOR16,FLASH_SECTOR17,
                             FLASH_SECTOR18,FLASH_SECTOR19,FLASH_SECTOR20,
                             FLASH_SECTOR21,FLASH_SECTOR22,FLASH_SECTOR23,
                             FLASH_SECTOR24,FLASH_SECTOR25,FLASH_SECTOR26,
                             FLASH_SECTOR27};

    uint8_t ReadFixCount[2]; //This stores the current sector position and current sector read out from flash
    uint8_t TransmitFixCount[2]; // Last transmission sector position and sector

    //Get current memory location
    TransmitFixCount[0] = *(uint8_t*) (0x0003E000);
    TransmitFixCount[1] = *(uint8_t*) (0x0003E001);

    // Last place we stored a fix
    ReadFixCount[0] = *(uint8_t*) (0x0003F000);
    ReadFixCount[1] = *(uint8_t*) (0x0003F001);

    // Roll over the count at the start of a new sector
    if(TransmitFixCount[1] == 0){
        TransmitFixCount[1] = 2;
        TransmitFixCount[0] = 0;
    }

    // Open the appropriate locations up for modification
    for(i = 0;i < TransmitFixCount[1];i++){
        FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, sector_array[i]);
    }

    FlashCtl_initiateMassErase();

    for(i = TransmitFixCount[1];i < 28;i++){
        FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, sector_array[i]);
    }

    for(i=TransmitFixCount[1];i <= ReadFixCount[1]; i++){
        MAP_WDT_A_clearTimer();
        for(loc = 0;loc < 4096;loc++){
            Save[loc] = *(uint8_t*)(0x00020000 + loc + (i * 4096));
        }
        //printf("Moving sector %d to %d\n", i, i-TransmitFixCount[1]);
        FlashCtl_enableWordProgramming(FLASH_IMMEDIATE_WRITE_MODE);
        FlashCtl_programMemory(Save, (void*)(0x00020000 + ((i-TransmitFixCount[1])*4096)), 4096);
        FlashCtl_eraseSector((void*)(0x00020000 + (i*4096)));
        //printf("Moved sector %d to %d\n", i, i-TransmitFixCount[1]);
    }

    for(i = TransmitFixCount[1];i < 28;i++){
        FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, sector_array[i]);
    }
    MAP_WDT_A_clearTimer();

    FixMemoryLocator[1] = ReadFixCount[1] - (TransmitFixCount[1]-1);
    FixMemoryLocator[0] = 0;
    if(TransmitFixCount[0] == 95){
        MemPlaceholder[0] = 0;
    } else
        MemPlaceholder[0] = TransmitFixCount[0];
    MemPlaceholder[1] = 0;

    FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR30); //unprotect sector
    FlashCtl_eraseSector(0x0003E000); //erase the sector
    FlashCtl_enableWordProgramming(FLASH_IMMEDIATE_WRITE_MODE); // Allow for immediate writing
    FlashCtl_programMemory(MemPlaceholder, (void*) 0x0003E000, 2); //write the data
    FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR30); //protect sector

    FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31); //unprotect sector
    FlashCtl_eraseSector(0x0003F000); //erase the sector
    FlashCtl_enableWordProgramming(FLASH_IMMEDIATE_WRITE_MODE); // Allow for immediate writing
    FlashCtl_programMemory(FixMemoryLocator, (void*) 0x0003F000, 2); //write the data
    FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31); //protect sector
    MemoryFull = 0;
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

//Store the battery life counters to flash
void store_battery_counters(void)
{
    //Declare save array
    uint8_t BatSave[5];
    BatSave[0] = IridiumCount;
    BatSave[1] = GPSCount;
    BatSave[2] = VHFCount;
    BatSave[3] = BatteryLow;
    BatSave[4] = '\0';

    //Save the battery counters to flash
    FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR29); //unprotect sector
    FlashCtl_eraseSector(0x0003D000); //erase the sector
    FlashCtl_enableWordProgramming(FLASH_IMMEDIATE_WRITE_MODE); // Allow for immediate writing
    FlashCtl_programMemory(BatSave, (void*) 0x0003D000, 4); //write the data
    FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR29); //protect sector
}

//Readout the battery life counters into the proper variables
void readout_battery_counters(void)
{
    uint8_t ClearThemAll = 0;

    uint8_t FlashCheck[8]; //Store what's in flash here

    //See if flash has been initialized
    FlashCheck[0] = *(uint8_t*) (0x0003D000);
    FlashCheck[1] = *(uint8_t*) (0x0003D001);
    FlashCheck[2] = *(uint8_t*) (0x0003D002);
    FlashCheck[3] = *(uint8_t*) (0x0003D003);

    //Fill structure from flash if initialized
    if (FlashCheck[0] != 255)
    {
        IridiumCount = *(uint8_t*) (0x0003D000);
        ClearThemAll++;
    }

    if (FlashCheck[1] != 255)
    {
        GPSCount = *(uint8_t*) (0x0003D001);
        ClearThemAll++;
    }
    if (FlashCheck[2] != 255)
    {
        VHFCount = *(uint8_t*) (0x0003D002);
        ClearThemAll++;
    }
    if (FlashCheck[3] != 255)
    {
        BatteryLow = *(uint8_t*) (0x0003D003);
        ClearThemAll++;
    }

    //Clear out the values if not initialized and store
    if (ClearThemAll < 4)
    {
        IridiumCount = 0;
        GPSCount = 0;
        VHFCount = 0;
        BatteryLow = 0;

        store_battery_counters();
    }
}

//Saves the current fix into memory and increments the location tracking
void save_current_fix(void)
{
    uint8_t ReadFixCount[2]; //This stores the current sector position and current sector read out from flash
    unsigned offset;

    //Get current memory location
    ReadFixCount[0] = *(uint8_t*) (0x0003F000);
    ReadFixCount[1] = *(uint8_t*) (0x0003F001);


    //Compute the offset for the save address
    if(FIX_SIZE * (ReadFixCount[0] + 1) < 4096)
        offset = (FIX_SIZE * ReadFixCount[0]) + (4096 * ReadFixCount[1]);
    else
        offset = (4096 * (ReadFixCount[1]+1)); //should never run


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
    if ((ReadFixCount[0] + 1) < SECTOR_CAPACITY)
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

// Generates random GPS fixes and fills the memory. Used to test the functioning of memory -- not necessary to use at deployment
void memory_test()
{
    srand(time(0));
    char testStr[100] = {'\0'};
    char sendString[340] = {'\0'};

    int i,x;
    for(i = 0; i < 6; ++i)
    {
        if(isMemoryFull()){
            printf("memory clearing");
            clearMemory();
        }

        sprintf(testStr, "$GPGGA,%0.2d%0.2d%0.2d,%08.4f,N,%09.4f,W,1,%.2d,%.2f,%.1f,M,%.1f,M,,,*%3.2d"
                , SystemTime.hours, SystemTime.minutes, SystemTime.seconds, rand()%9999 + (float)rand()/RAND_MAX
                , rand()%9999 + (float)rand()/RAND_MAX, rand() % 31, rand()%9+ (float)rand()/RAND_MAX
                , rand()%1000 + (float)rand()/RAND_MAX, rand()%1000 + (float)rand()/RAND_MAX, rand()%98 + 1);
        //printf("TestString: %s\n", testStr);
        strcpy(GPSString, testStr);
        GPSParse();
        sprintf(CurrentFixSaveString, "%0.6d,%0.6d,%09.4f,%c,%010.4f,%c,%3.2f"
                            , GPSData.FixDate, GPSData.FixTime, GPSData.Lat, GPSData.LatDir
                            , GPSData.Lon, GPSData.LonDir, GPSData.HDOP);
                    //printf("%s\n", CurrentFixSaveString);
        save_current_fix();

        sprintf(CurrentFixSaveString, "");

        MAP_WDT_A_clearTimer();

    }

    printf("end of memory test\n");
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
        while(!PC_READY_DATA)
        {
            NEW_DATA_READY = 0;
            NEW_DATA_READY = 1;
        }
        NEW_DATA_READY = 0;
        readout_fix(0x00020000 + (i_1 * FIX_SIZE) + (11 * ((i_1 - 1)/ (SECTOR_CAPACITY))));
        PC_READY_DATA = 0;
        NEW_DATA_READY = 1;
        //PC_puts(FixRead);
        READ_DATA_PROGRESS = ((float)i_1)/((float)max);
    }
    ALL_DATA_SENT = 1;
    NEW_DATA_READY = 0;
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
