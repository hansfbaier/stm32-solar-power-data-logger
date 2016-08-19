/*
 * logging.h
 *
 *  Created on: 19.08.2016
 *      Author: jack
 */

#ifndef LOGGING_H_
#define LOGGING_H_

// 1 bin = 5 min
// 1 day = 24 * 60 = 288 * 5
#define NUM_BINS 288

typedef struct 
{
    int bins[NUM_BINS];
    int currentBin;
    int currentImps;
} EnergyLogger;

void addImp(EnergyLogger *logger);
void newBin(EnergyLogger *logger);
int  getBin(EnergyLogger *logger, int binNo);
int  lastBinNo(EnergyLogger *logger);
int  getLastBin(EnergyLogger *logger);

#endif /* LOGGING_H_ */
