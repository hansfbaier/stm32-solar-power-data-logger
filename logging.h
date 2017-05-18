/*
 * logging.h
 *
 *  Created on: 19.08.2016
 *      Author: jack
 */

#ifndef LOGGING_H_
#define LOGGING_H_
#include "rtc.h"
// 1 bin = 5 min
// 1 day = 24 * 60 = 288 * 5
#define NUM_BINS (ONE_DAY / (5 * 60))
#define LOGFILENAME "0:/WattLog.csv"

typedef struct 
{
    int bins[NUM_BINS];
    int impsToday;
    int currentBinNo;
    int impTimer;
    int lastImpTimer;
} EnergyLogger;

void Init_Logging(void);
void Write_Log_Entry(void);

void addImp(EnergyLogger *logger);
void newBin(EnergyLogger *logger);
int  getBin(EnergyLogger *logger, int binNo);
int  getCurrentBin(EnergyLogger *logger);
int  getLastBinNo(EnergyLogger *logger);
int  getLastBin(EnergyLogger *logger);

#endif /* LOGGING_H_ */
