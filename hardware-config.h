/*
 * hardware-config.h
 *
 *  Created on: Aug 29, 2016
 *      Author: jack
 */

#ifndef HARDWARE_CONFIG_H_
#define HARDWARE_CONFIG_H_

void EXTI_Configuration(void);
void NVIC_Configuration(void);
void GPIO_Configuration(void);
void USART_Configuration(void);
void RTC_Init(void);

#endif /* HARDWARE_CONFIG_H_ */
