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
    if (watthours / 1000 > 10)
        sprintf(displayState.solarWatthours, "%5d ", watthours);
    else
        sprintf(displayState.solarWatthours, "%4dWh", watthours);
}

void updateHouseImp(int imps, int watts, int watthours)
{
    displayState._houseWatts = watts;
    updateImport();
    sprintf(displayState.houseImps,      "%4d",   imps);
    sprintf(displayState.houseWatts,     "%4dW",  watts);
    if (watthours / 1000 > 10)
        sprintf(displayState.houseWatthours, "%5d ", watthours);
    else
        sprintf(displayState.houseWatthours, "%4dWh", watthours);
}

void displaySolarImp(void)
{
    UG_SetForecolor(SOLAR_COLOR);
    UG_PutString(SOLAR_X, IMPS_Y,         displayState.solarImps);
    UG_PutString(SOLAR_X, WATTHOURSDAY_Y, displayState.solarWatthours);
}

void displayHouseImp(void)
{
    UG_SetForecolor(HOUSE_COLOR);
    UG_PutString(HOUSE_X, IMPS_Y,         displayState.houseImps);
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

#define EXIM_TIME_Y   (MAX_BIN_Y + yspacer/3)
#define EXIM_WATTS_Y  (EXIM_TIME_Y + yspacer)
#define EXIM_EXIM_Y   (EXIM_WATTS_Y + yspacer)
#define EXIM_WH_Y     (EXIM_EXIM_Y + yspacer)

const int yspacer = 45;
const int xspacer = 24;
const char *exim_blank = "      ";

void eximDisplayTime(void)
{
    UG_FontSelect(&FONT_24X40);
    UG_SetForecolor(C_WHITE);
    UG_PutString(xspacer * 2, EXIM_TIME_Y, Time_As_String());
    UG_FontSelect(&FONT_6X8);
}

void eximDisplayHouse(void)
{
    UG_FontSelect(&FONT_24X40);
    UG_SetForecolor(HOUSE_COLOR);
    UG_PutString(xspacer, EXIM_WATTS_Y, exim_blank);
    UG_PutString(xspacer, EXIM_WATTS_Y, displayState.houseWatts);
    UG_PutString(0, EXIM_WH_Y, exim_blank);
    UG_PutString(0, EXIM_WH_Y, displayState.houseWatthours);
    UG_FontSelect(&FONT_6X8);
}

void eximDisplaySolar(void)
{
    UG_FontSelect(&FONT_24X40);
    UG_SetForecolor(SOLAR_COLOR);
    UG_PutString(MAX_X/2 + xspacer, EXIM_WATTS_Y, exim_blank);
    UG_PutString(MAX_X/2 + xspacer, EXIM_WATTS_Y, displayState.solarWatts);
    UG_PutString(MAX_X/2, EXIM_WH_Y, exim_blank);
    UG_PutString(MAX_X/2, EXIM_WH_Y, displayState.solarWatthours);
    UG_FontSelect(&FONT_6X8);
}

void eximDisplayExim(void)
{
    UG_FontSelect(&FONT_24X40);
    if (displayState.import) UG_SetForecolor(C_RED); else UG_SetForecolor(C_GREEN);
    UG_PutString(xspacer * 4, EXIM_EXIM_Y, exim_blank);
    UG_PutString(xspacer * 4, EXIM_EXIM_Y, displayState.eximWatts);
    UG_FontSelect(&FONT_6X8);
}

void displayExim(void)
{
    eximDisplayTime();
    eximDisplayHouse();
    eximDisplaySolar();
    eximDisplayExim();
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
