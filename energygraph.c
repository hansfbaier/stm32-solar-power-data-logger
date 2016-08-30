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
    char blank[] = "   0";
    UG_SetForecolor(SOLAR_COLOR);
    UG_PutString(SOLAR_X, IMPS_Y, blank);
    UG_SetForecolor(HOUSE_COLOR);
    UG_PutString(HOUSE_X, IMPS_Y, blank);
}

static void plotBinBar(EnergyLogger *logger, int binNo)
{
    int binX = binNo;
    
    int impsToDisplay = MIN(getBin(logger, binNo), MAX_DISP_IMPS);
    int yLength = (impsToDisplay * Y_RANGE) / MAX_DISP_IMPS;
    int binY = BOTTOM - yLength;
    
    char buffer[16];
    
    UG_DrawLine(binX, BOTTOM, binX, binY, gui.fore_color);
}

void plotBin(int binNo)
{
    int solarVal = getBin(&solarLogger, binNo);
    int houseVal = getBin(&houseLogger, binNo);
    
    if (solarVal > houseVal)
    {
        UG_SetForecolor(SOLAR_COLOR);
        plotBinBar(&solarLogger, binNo);
        UG_SetForecolor(HOUSE_COLOR);
        plotBinBar(&houseLogger, binNo);
    }
    else if (solarVal < houseVal)
    {
        UG_SetForecolor(HOUSE_COLOR);
        plotBinBar(&houseLogger, binNo);
        UG_SetForecolor(SOLAR_COLOR);
        plotBinBar(&solarLogger, binNo);
    }
    else
    {
        UG_SetForecolor(C_WHITE);
        plotBinBar(&solarLogger, binNo);
    }    
}
