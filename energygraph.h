/*
 * energygraph.h
 *
 *  Created on: 19.08.2016
 *      Author: jack
 */

#ifndef ENERGYGRAPH_H_
#define ENERGYGRAPH_H_

#include "logging.h"

#define SOLAR_COLOR C_YELLOW
#define HOUSE_COLOR C_BLUE
#define GRID_COLOR C_CHOCOLATE

#define WATT_HOURS_PER_IMP (0.625f)
#define WATT_PER_IMP_AND_BIN (7.5f)
#define BOTTOM MAX_Y
#define MAX_CONSOLE_X (MAX_X - 112)
#define SOLAR_X       (MAX_CONSOLE_X + 56)
#define HOUSE_X       (MAX_CONSOLE_X)
#define HARDFAULT_Y (MAX_Y/3)

#define MAX_BIN_Y MAX_Y/8
#define MAX_BIN_X NUM_BINS
#define CONSOLE_START_Y     (9)
#define CONSOLE_END_Y       (MAX_BIN_Y)
#define IMPS_Y         (27)
#define WATTS_Y        (36)
#define WATTHOURS_Y    (18)
#define WATTHOURSDAY_Y (9)
#define Y_RANGE (MAX_Y - MAX_BIN_Y)

#define MAX_DISP_IMPS 160

void plotBin(int binNo);
void printZeroedCounters();
void clearGraph();

#endif /* ENERGYGRAPH_H_ */
