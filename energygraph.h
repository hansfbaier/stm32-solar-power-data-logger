/*
 * energygraph.h
 *
 *  Created on: 19.08.2016
 *      Author: jack
 */

#ifndef ENERGYGRAPH_H_
#define ENERGYGRAPH_H_


#define BOTTOM MAX_Y
#define MAX_BIN_Y MAX_Y/8
#define MAX_CONSOLE_X (MAX_X - 56)
#define Y_RANGE (MAX_Y - MAX_BIN_Y)

#define MAX_DISP_IMPS 1600

void plotLastBins();

#endif /* ENERGYGRAPH_H_ */
