/****************************************Copyright (c)****************************************************
 **                                      
 **                                 http://www.powermcu.com
 **
 **--------------File Info---------------------------------------------------------------------------------
 ** File name:               main.c
 ** Descriptions:            The FreeRTOS application function
 **
 **--------------------------------------------------------------------------------------------------------
 ** Created by:              AVRman
 ** Created date:            2010-11-8
 ** Version:                 v1.0
 ** Descriptions:            The original version
 **
 **--------------------------------------------------------------------------------------------------------
 ** Modified by:             
 ** Modified date:           
 ** Version:                 
 ** Descriptions:            
 **
 *********************************************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "GLCD.h"
#include "ugui.h"
#include "logging.h"
#include "energygraph.h"

/* Private define ------------------------------------------------------------*/
#define LED_TASK_STACK_SIZE			( configMINIMAL_STACK_SIZE )
#define LCD_TASK_STACK_SIZE			( configMINIMAL_STACK_SIZE )

#define LED_TASK_PRIORITY			( tskIDLE_PRIORITY + 1 )
#define LCD_TASK_PRIORITY			( tskIDLE_PRIORITY + 3 )

/* Private function prototypes -----------------------------------------------*/
static void prvSetupHardware(void);
void GPIO_Configuration(void);
void vLCDTask(void * pvArg);
void vLEDTask(void * pvArg);

int main(void)
{
    prvSetupHardware();
    xTaskCreate(vLEDTask, (signed char * ) NULL, LED_TASK_STACK_SIZE, NULL,
            LED_TASK_PRIORITY, NULL);
    xTaskCreate(vLCDTask, (signed char * ) NULL, LCD_TASK_STACK_SIZE, NULL,
            LCD_TASK_PRIORITY, NULL);
    /* Start the scheduler. */
    vTaskStartScheduler();

    return 0;
}

void vLEDTask(void * pvArg)
{
    while (1)
    {
        /* LED1-ON LED2-OFF */
        GPIO_SetBits(GPIOB, GPIO_Pin_0);
        GPIO_ResetBits(GPIOB, GPIO_Pin_1);
        vTaskDelay(1000);

        /* LED1-OFF LED2-ON */
        GPIO_ResetBits(GPIOB, GPIO_Pin_0);
        GPIO_SetBits(GPIOB, GPIO_Pin_1);
        vTaskDelay(1000);
    }
}

void UserPixelSetFunction(UG_S16 x, UG_S16 y, UG_COLOR c)
{
    uint16_t lcdColor = RGB565CONVERT((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
    LCD_SetPoint(x, y, lcdColor);
}

extern UG_GUI gui;
extern EnergyLogger solarLogger;
extern EnergyLogger houseLogger;

void vLCDTask(void * pvArg)
{
    LCD_Initializtion();
    LCD_Clear(Black);
    UG_Init (&gui, &UserPixelSetFunction, MAX_X, MAX_Y);
    UG_SelectGUI(&gui);
    UG_SetForecolor(White);
    UG_SetBackcolor(Black);
    
    UG_FontSelect(&FONT_6X8);

    vTaskDelay(1000);
    
    UG_ConsoleSetArea(0, 0, MAX_X, MAX_BIN_Y);
    UG_ConsoleSetForecolor(C_GREEN);
    UG_ConsoleSetBackcolor(C_BLACK);
    UG_ConsolePutString("Init Done!\n");
    
    while (1)
    {
        solarLogger.currentImps = rand() % MAX_DISP_IMPS;
        houseLogger.currentImps = rand() % MAX_DISP_IMPS;
        newBin(&solarLogger);
        newBin(&houseLogger);
        plotLastBins();

        vTaskDelay(1000);
    }
}

static void prvSetupHardware(void)
{
    /* Configure HCLK clock as SysTick clock source. */
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
    GPIO_Configuration();
}

void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC,
            ENABLE);
    /**
     *	LED1 -> PB0   LED2 -> PB1
     */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /**
     *  KeyA -> PC13 , KeyB -> PB2
     */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}


#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *   where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif
