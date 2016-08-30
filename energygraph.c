/*
 * energygraph.c
 *
 *  Created on: 19.08.2016
 *      Author: jack
 */

#include "GLCD.h"
#include "ugui.h"
#include "energygraph.h"
#include "logging.h"

UG_GUI gui;

extern EnergyLogger solarLogger;
extern EnergyLogger houseLogger;

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

void printZeroedCounters()
{
    char blank[] = "0000";
    UG_SetForecolor(SOLAR_COLOR);
    UG_PutString(SOLAR_X, IMPS_Y, blank);
    UG_SetForecolor(HOUSE_COLOR);
    UG_PutString(HOUSE_X, IMPS_Y, blank);
}

static void plotLastBin(EnergyLogger *logger)
{
    int binX = lastBinNo(logger);
    
    int impsToDisplay = MIN(getLastBin(logger), MAX_DISP_IMPS);
    int yLength = (impsToDisplay * Y_RANGE) / MAX_DISP_IMPS;
    int binY = BOTTOM - yLength;
    
    char buffer[16];
    
    UG_DrawLine(binX, BOTTOM, binX, binY, gui.fore_color);
}

void plotLastBins()
{
    int solarVal = getLastBin(&solarLogger);
    int houseVal = getLastBin(&houseLogger);
    
    if (solarVal > houseVal)
    {
        UG_SetForecolor(SOLAR_COLOR);
        plotLastBin(&solarLogger);
        UG_SetForecolor(HOUSE_COLOR);
        plotLastBin(&houseLogger);
    }
    else if (solarVal < houseVal)
    {
        UG_SetForecolor(HOUSE_COLOR);
        plotLastBin(&houseLogger);
        UG_SetForecolor(SOLAR_COLOR);
        plotLastBin(&solarLogger);
    }
    else
    {
        UG_SetForecolor(C_WHITE);
        plotLastBin(&solarLogger);
    }
}
