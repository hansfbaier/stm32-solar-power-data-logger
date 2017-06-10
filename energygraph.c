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
#include "printf.h"
#include "FreeRTOS.h"

UG_GUI gui;
DisplayState displayState;

extern EnergyLogger solarLogger;
extern EnergyLogger houseLogger;

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

static void updateImport(void)
{
    displayState.import = displayState._solarWatts < displayState._houseWatts;
    int eximWatts = displayState._solarWatts - displayState._houseWatts;
    sprintf(displayState.eximWatts, "%4dW", eximWatts);
}

void updateSolarImp(int imps, int watts, int watthours)
{
    displayState._solarWatts = watts;
    updateImport();
    sprintf(displayState.solarImps,      "%4d",   imps);
    sprintf(displayState.solarWatts,     "%4dW",  watts);
    sprintf(displayState.solarWatthours, "%4dWh", watthours);
}

void updateHouseImp(int imps, int watts, int watthours)
{
    displayState._houseWatts = watts;
    updateImport();
    sprintf(displayState.houseImps,      "%4d",   imps);
    sprintf(displayState.houseWatts,     "%4dW",  watts);
    sprintf(displayState.houseWatthours, "%4dWh", watthours);
}

void displaySolarImp(void)
{
    UG_SetForecolor(SOLAR_COLOR);
    //UG_PutString(SOLAR_X, IMPS_Y,         displayState.solarImps);
    UG_PutString(SOLAR_X, WATTS_Y,        "        ");
    UG_PutString(SOLAR_X, WATTS_Y,        displayState.solarWatts);
    UG_PutString(SOLAR_X, WATTHOURSDAY_Y, displayState.solarWatthours);
}

void displayHouseImp(void)
{
    UG_SetForecolor(HOUSE_COLOR);
    //UG_PutString(HOUSE_X, IMPS_Y,         displayState.houseImps);
    UG_PutString(HOUSE_X, WATTS_Y,        "        ");
    UG_PutString(HOUSE_X, WATTS_Y,        displayState.houseWatts);
    UG_PutString(HOUSE_X, WATTHOURSDAY_Y, displayState.houseWatthours);
}

void clearDetailsArea(void)
{
    UG_FillFrame(MAX_CONSOLE_X, 0, MAX_X, MAX_BIN_Y, C_BLACK);
}

void clearGraphArea(void)
{
    UG_FillFrame(0, MAX_BIN_Y, MAX_X, MAX_Y, C_BLACK);
}

static char buf[15];

void redrawGraphGrid()
{
    UG_SetForecolor(C_WHITE);
    UG_PutString(MAX_X/2 - 2 * 9, 0, buf);

    clearDetailsArea();
    clearGraphArea();
    
    float pixelsPerImp = ((float)Y_RANGE / (float)MAX_DISP_IMPS);
    float hundredWattsY = pixelsPerImp * 100.0f / WATT_PER_IMP_AND_BIN;
    float y = MAX_Y - hundredWattsY;
    
    int watts = 0;
    while (y > MAX_BIN_Y) {
        watts += 100;
        UG_DrawLine(30, (UG_S16)y, MAX_BIN_X, (UG_S16)y, GRID_COLOR);
        sprintf(buf, "%4d", watts);
        UG_PutString(0, (UG_S16)y-4, buf);
        y -= hundredWattsY;
    }
}

void redrawGraph()
{
    redrawGraphGrid();
    
    configASSERT(solarLogger.currentBinNo == houseLogger.currentBinNo);
    for (int i = 0; i < solarLogger.currentBinNo; i++)
    {
        plotBin(i);
    }
}

void displayExim(void)
{
    int y;
    const int yspacer = 45;
    const int xspacer = 24;
    const char *blank = "       ";
    
    UG_FontSelect(&FONT_24X40);
    UG_SetForecolor(C_WHITE);
    y = MAX_BIN_Y + yspacer/3;
    UG_PutString(xspacer * 2, y, Time_As_String());

    UG_SetForecolor(HOUSE_COLOR);
    UG_PutString(xspacer, y += yspacer, blank);
    UG_PutString(xspacer, y           , displayState.houseWatts);
    UG_SetForecolor(SOLAR_COLOR);
    UG_PutString(MAX_X/2 + xspacer, y, blank);
    UG_PutString(MAX_X/2 + xspacer, y, displayState.solarWatts);
    
    if (displayState.import) UG_SetForecolor(C_RED); else UG_SetForecolor(C_GREEN);
    UG_PutString(xspacer * 5, y += yspacer, blank);
    UG_PutString(xspacer * 5, y           , displayState.eximWatts);

    UG_SetForecolor(HOUSE_COLOR);
    UG_PutString(0, y += yspacer, blank);
    UG_PutString(0, y           , displayState.houseWatthours);
    UG_SetForecolor(SOLAR_COLOR);
    UG_PutString(MAX_X/2, y, blank);
    UG_PutString(MAX_X/2, y, displayState.solarWatthours);
    
    UG_FontSelect(&FONT_6X8);
}

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
