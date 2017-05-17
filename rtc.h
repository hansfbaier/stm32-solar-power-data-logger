/*
 * rtc.h
 *
 *  Created on: Aug 27, 2016
 *      Author: jack
 */

#ifndef RTC_H_
#define RTC_H_
#include <stdint.h>

#define ONE_DAY (24 * 60 * 60)

uint32_t Time_Regulate(void);
void Time_Adjust(void);
char *Time_As_String();

#define TODAY ((int)(RTC_GetCounter() / ONE_DAY))

#endif /* RTC_H_ */
