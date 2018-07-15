/*
 * Structures.h
 *
 *  Created on: Jul 15, 2018
 *      Author: kielev
 */

#ifndef HEADERS_STRUCTURES_H_
#define HEADERS_STRUCTURES_H_


typedef struct _Configparameters
{
    uint8_t COM; //Command program or read (program or read)
    uint8_t GPS; //GPS sample interval
    uint8_t WTM; //Wireless transmission mode (confirmed or spew)
    uint8_t WTD; //Wireless transmission day
    uint8_t WCT; //Wireless connection start time
    uint8_t WCW; //Wireless connection window
    uint8_t VST; //VHF broadcast start time
    uint8_t VET; //VHF broadcast end time
    uint8_t DOP; //PDOP Threshold
    uint8_t GTO; //GPS timeout
} Configparameters;
static volatile Configparameters Config;

typedef struct _GPSData
{
    int FixDate;
    int FixTime;
    float Lat;
    char LatDir;
    float Lon;
    char LonDir;
    int FixQuality;
    float HDOP;
} GPSDataStruct;

//Another structure for the Xbee parameters that are obtained.
typedef struct _Xbeeparameters
{
    uint8_t Command; //Command read all or since last confirmed connection
    //1 = all data within flash, 2 = since last confirmed connection
    int NumRec; //Number of points LabView received consecutively correctly
    //during wireless download. The collar compares this with the number sent
    //from flash.  If they match the location for the next time of confirmed
    //connection is moved up.
    uint8_t CurLocReq; //Command while the device has low battery life to request
//for the current GPS location of the device. 1 = get location, 0 = no action

} Xbeeparameters;

#endif /* HEADERS_STRUCTURES_H_ */
