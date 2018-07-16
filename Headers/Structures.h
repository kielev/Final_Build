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
    uint8_t GPS; //GPS sample interval (0-23 hours)
    uint8_t GTO; //GPS timeout (1-10 minutes)
    uint8_t ITF; //Iridium Transmission Frequency (1-weekly, 2-biweekly, 3 - monthly)
    uint8_t ITD; //Iridium transmission day (Sunday 0)
    uint8_t ICT; //Iridium connection start time (0-23 hour)
    uint8_t ICR; //Iridium connection retry (0-3)
    uint8_t VST; //VHF broadcast start time (0-23 hour)
    uint8_t VET; //VHF broadcast end time (0-23 hour)
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
