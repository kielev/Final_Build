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
    uint8_t GPS; //GPS sample interval
    uint8_t GTO; //GPS timeout
    uint8_t GFQ; //GPS Fix Quality
    uint8_t ITF; //Iridium Transmission Frequency (1-weekly, 2-bimonthly, 3 - monthly)
    uint8_t ITD; //Iridium transmission day(s) bit encoded
    uint8_t ICT; //Iridium connection start time
    uint8_t ICW; //Iridium connection window
    uint8_t VST; //VHF broadcast start time
    uint8_t VET; //VHF broadcast end time
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

#endif /* HEADERS_STRUCTURES_H_ */
