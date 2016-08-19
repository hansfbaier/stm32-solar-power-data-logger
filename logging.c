/*
 * logging.c
 *
 *  Created on: 19.08.2016
 *      Author: jack
 */

#include "logging.h"

EnergyLogger solarLogger;
EnergyLogger houseLogger;

void addImp(EnergyLogger *logger)
{
    ++logger->currentImps;
}

void newBin(EnergyLogger *logger)
{
    logger->bins[logger->currentBin] = logger->currentImps;
    logger->currentImps = 0;
    logger->currentBin = (logger->currentBin + 1) % NUM_BINS;
}

int getBin(EnergyLogger *logger, int binNo)
{
    return logger->bins[binNo];
}

int lastBinNo(EnergyLogger *logger)
{
    return logger->currentBin - 1;
}

int getLastBin(EnergyLogger *logger)
{
    return getBin(logger, lastBinNo(logger));
}

