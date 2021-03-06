/*
 * energygraph.h
 *
 *  Created on: 19.08.2016
 *      Author: jack
 */

#ifndef ENERGYGRAPH_H_
#define ENERGYGRAPH_H_

#include "logging.h"
#include "stdbool.h"

#define SOLAR_COLOR C_YELLOW
#define HOUSE_COLOR C_BLUE
#define GRID_COLOR C_CHOCOLATE

#define WATT_HOURS_PER_IMP (1.0f)
#define WATT_PER_IMP_AND_BIN (7.5f)
#define BOTTOM MAX_Y
#define DAY_X (MAX_X/2 - 2*8)
#define MAX_CONSOLE_X (MAX_X - 112)
#define SOLAR_X       (MAX_CONSOLE_X + 56)
#define HOUSE_X       (MAX_CONSOLE_X)
#define HARDFAULT_Y (MAX_Y/3)

#define MAX_BIN_Y MAX_Y/8
#define MAX_BIN_X NUM_BINS
#define CONSOLE_START_Y     (9)
#define CONSOLE_END_Y       (MAX_BIN_Y)
#define IMPS_Y         (18)
#define WATTHOURSDAY_Y (9)
#define Y_RANGE (MAX_Y - MAX_BIN_Y)

#define MAX_DISP_IMPS 160

void plotBin(int binNo);
void printZeroedCounters(void);
void clearGraphArea(void);
void clearDetailsArea(void);
void redrawGraphGrid(void);
void redrawGraph(void);

void displayExim(void);
void eximDisplayTime(void);
void eximDisplayHouse(void);
void eximDisplaySolar(void);
void eximDisplayExim(void);

typedef enum 
{
    ENERGY_GRAPH,
    IMPORT_EXPORT,
} DisplayMode;

typedef enum
{
    NEW_DAY,
    NEW_BIN,
    SOLAR_IMP,
    HOUSE_IMP,
    MODE_CHANGE,
    UPDATE_DISPLAY,
} DisplayEvent;

typedef struct 
{
    DisplayMode mode;
    int _solarWatts;
    int _houseWatts;

    char solarImps [10];
    char solarWatts[10];
    char houseImps [10];
    char houseWatts[10];
    
    bool import;
    char eximWatts [10];

    char solarWatthours[10];
    char houseWatthours[10];
    
} DisplayState;

void updateSolarImp(int imps, int watts, int watthours);
void updateHouseImp(int imps, int watts, int watthours);
void displaySolarImp();
void displayHouseImp();

#endif /* ENERGYGRAPH_H_ */
