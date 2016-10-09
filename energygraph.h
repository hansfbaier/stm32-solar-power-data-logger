/*
 * energygraph.h
 *
 *  Created on: 19.08.2016
 *      Author: jack
 */

#ifndef ENERGYGRAPH_H_
#define ENERGYGRAPH_H_

#define SOLAR_COLOR C_YELLOW
#define HOUSE_COLOR C_BLUE

#define BOTTOM MAX_Y
#define MAX_CONSOLE_X (MAX_X - 112)
#define SOLAR_X       (MAX_CONSOLE_X + 56)
#define HOUSE_X       (MAX_CONSOLE_X)

#define MAX_BIN_Y MAX_Y/8
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

#endif /* ENERGYGRAPH_H_ */
