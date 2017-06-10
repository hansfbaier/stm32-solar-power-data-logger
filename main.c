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
#include <stdint.h>
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "GLCD.h"
#include "ugui.h"
#include "logging.h"
#include "energygraph.h"
#include "printf.h"
#include "rtc.h"
#include "hardware-config.h"
#include "usb_istr.h"
#include "usb_lib.h"
#include "usb_pwr.h"
#include "ff.h"
#include "USB/inc/usb_config.h"


void PrintFileError(FRESULT res, const char message[]);

/* Private define ------------------------------------------------------------*/
#define LOGGER_TASK_STACK_SIZE		( 150 )
#define IWDG_TASK_STACK_SIZE       ( configMINIMAL_STACK_SIZE )
#define UART_TASK_STACK_SIZE       ( configMINIMAL_STACK_SIZE )
#define DISPLAY_TASK_STACK_SIZE    ( 150 )
#define MODE_TASK_STACK_SIZE       ( configMINIMAL_STACK_SIZE )

#define IWDG_TASK_PRIORITY         ( tskIDLE_PRIORITY + 4 )
#define LOGGER_TASK_PRIORITY		( tskIDLE_PRIORITY + 3 )
#define UART_TASK_PRIORITY         ( tskIDLE_PRIORITY + 2 )
#define DISPLAY_TASK_PRIORITY      ( tskIDLE_PRIORITY + 1 )
#define MODE_TASK_PRIORITY         ( tskIDLE_PRIORITY     )

/* Private function prototypes -----------------------------------------------*/
static void prvSetupHardware(void);

void vModeTask(void *pvArg);
void vDisplayTask(void *pvArg);
void vLoggerTask(void *pvArg);
void vIwdgTask(void *pvArg);
void vUartTask(void *pvArg);

void putchar(char ch);
static void putf_serial(void * dummy, char ch);
static void putf_gui(void * dummy, char ch);

xQueueHandle impQueue     = NULL;
xQueueHandle slotQueue    = NULL;
xQueueHandle displayQueue = NULL;

int main(void)
{
    FlagStatus iwdgReset = RCC_GetFlagStatus(RCC_FLAG_IWDGRST);
    if(iwdgReset != RESET)
    {
       RCC_ClearFlag();
    }

    prvSetupHardware();
    
    if (RESET != iwdgReset) { printf("\nIWDG-RESET"); }
    
    xTaskCreate(vIwdgTask,    (signed char * ) NULL, IWDG_TASK_STACK_SIZE,    NULL, IWDG_TASK_PRIORITY,    NULL);
    xTaskCreate(vUartTask,    (signed char * ) NULL, UART_TASK_STACK_SIZE,    NULL, UART_TASK_PRIORITY,    NULL);
    xTaskCreate(vLoggerTask,  (signed char * ) NULL, LOGGER_TASK_STACK_SIZE,  NULL, LOGGER_TASK_PRIORITY,  NULL);
    xTaskCreate(vDisplayTask, (signed char * ) NULL, DISPLAY_TASK_STACK_SIZE, NULL, DISPLAY_TASK_PRIORITY, NULL);
    xTaskCreate(vModeTask,    (signed char * ) NULL, MODE_TASK_STACK_SIZE,    NULL, MODE_TASK_PRIORITY,    NULL);
    
    /* Start the scheduler. */
    vTaskStartScheduler();

    return 0;
}

extern EnergyLogger solarLogger;
extern EnergyLogger houseLogger;
extern DisplayState displayState;

void vModeTask(void *pvArg)
{
    (void)pvArg;
    
    vTaskDelay(10000 * portTICK_RATE_MS);
    
    while(1)
    {
        DisplayEvent event;
        
        vTaskDelay(4000 * portTICK_RATE_MS);
        displayState.mode = IMPORT_EXPORT;
        event = MODE_CHANGE;
        xQueueSend(displayQueue, &event, 100 * portTICK_RATE_MS);

        vTaskDelay(10000 * portTICK_RATE_MS);
        displayState.mode = ENERGY_GRAPH;
        event = MODE_CHANGE;
        xQueueSend(displayQueue, &event, 100 * portTICK_RATE_MS);
    }
}

void vDisplayTask(void * pvArg)
{   
    (void)pvArg;
    static char buf[16];
    
    displayQueue = xQueueCreate(10, sizeof(DisplayEvent));
    
    while(1)
    {
        DisplayEvent event;
        
        if (xQueueReceive(displayQueue, &event, 100 * portTICK_RATE_MS))
        {
            if (SOLAR_IMP == event)
            {
                displaySolarImp();

                if (ENERGY_GRAPH == displayState.mode) plotBin(solarLogger.currentBinNo);
                else
                {
                    eximDisplaySolar();
                    eximDisplayExim();
                }
            } 
            else if (HOUSE_IMP == event)
            {
                displayHouseImp();
                if (ENERGY_GRAPH == displayState.mode) plotBin(houseLogger.currentBinNo);
                else
                {
                    eximDisplayHouse();
                    eximDisplayExim();
                }
            }     
            else if (NEW_DAY == event)
            {
                displayState.mode = ENERGY_GRAPH;
                redrawGraphGrid();
                sprintf(buf, "Day %d", (int)(RTC_GetCounter() / ONE_DAY));
            }
            else if (NEW_BIN == event)
            {
                if (ENERGY_GRAPH == displayState.mode) plotBin(getLastBinNo(&solarLogger));
                printZeroedCounters();
                SD_TotalSize();
            }
            else if (MODE_CHANGE == event)
            {
                switch (displayState.mode)
                {
                    case ENERGY_GRAPH:
                        redrawGraph();
                        displaySolarImp();
                        displayHouseImp();
                        break;
                        
                    case IMPORT_EXPORT:
                        clearGraphArea();
                        displayExim();
                        break;
                }
            }
            else if (UPDATE_DISPLAY == event)
            {
                UG_SetForecolor(C_WHITE);
                sprintf(buf, "%5d", TIM_GetCounter(TIM2));

                UG_PutString(MAX_CONSOLE_X + 7, 0,  buf);
                UG_PutString(MAX_CONSOLE_X + 56, 0, Time_As_String());

                displaySolarImp();
                displayHouseImp();
                
                if (false && IMPORT_EXPORT == displayState.mode)
                {
                    displayExim();
                }
            }
        }
    }
}

void vLoggerTask(void * pvArg)
{   
    (void)pvArg;
    EnergyLogger *logger;
    int seconds;

    redrawGraphGrid();
    
    Init_Logging();
    
    impQueue  = xQueueCreate(10, sizeof(EnergyLogger *));
    slotQueue = xQueueCreate(10, sizeof(int));
    
    EXTI_Configuration();
    IWDG_Configuration();
    
    while (1)
    {
        DisplayEvent displayEvent;
        
        uint32_t currentRtc = RTC_GetCounter();
        if (0 == currentRtc % ONE_DAY)
        {
            displayEvent = NEW_DAY;
            xQueueSend(displayQueue, &displayEvent, 10 * portTICK_RATE_MS);
        }
        
        if (xQueueReceive(impQueue, &logger, 1 * portTICK_RATE_MS))
        {
            configASSERT(NULL != logger);
            addImp(logger);
            
            int impsThisBin        = getCurrentBin(logger);
            int millisSinceLastImp = (logger->impTimer - logger->lastImpTimer) / 2;
            int wattsUsed          = 2250000 / millisSinceLastImp;
            int wattHoursToday     = (1000 * logger->impsToday) / 1600;
            
            if (&solarLogger == logger)
            {
                GPIO_SetBits(GPIOB, GPIO_Pin_1);
                vTaskDelay(10);                
                GPIO_ResetBits(GPIOB, GPIO_Pin_1);
                
                updateSolarImp(impsThisBin, wattsUsed, wattHoursToday);
                displayEvent = SOLAR_IMP;
                xQueueSend(displayQueue, &displayEvent, 1 * portTICK_RATE_MS);
            }
            else if (&houseLogger == logger)
            {
                GPIO_SetBits(GPIOB, GPIO_Pin_0);
                vTaskDelay(10);     
                GPIO_ResetBits(GPIOB, GPIO_Pin_0);
                
                updateHouseImp(impsThisBin, wattsUsed, wattHoursToday);    
                displayEvent = HOUSE_IMP;
                xQueueSend(displayQueue, &displayEvent, 1 * portTICK_RATE_MS);
            }            
        }

        if (xQueueReceive(slotQueue, &seconds, 1 * portTICK_RATE_MS))
        {                        
            displayEvent = UPDATE_DISPLAY;
            xQueueSend(displayQueue, &displayEvent, 1 * portTICK_RATE_MS);
            
            if (0 == seconds)
            {
                configASSERT(solarLogger.currentBinNo == houseLogger.currentBinNo);
                newBin(&solarLogger);
                newBin(&houseLogger);
                configASSERT(solarLogger.currentBinNo == houseLogger.currentBinNo);
                
                Write_Log_Entry();
                displayEvent = NEW_BIN;
                xQueueSend(displayQueue, &displayEvent, 10 * portTICK_RATE_MS);
                
                if (0 == solarLogger.currentBinNo)
                {
                    displayEvent = NEW_DAY;
                    xQueueSend(displayQueue, &displayEvent, 10 * portTICK_RATE_MS);
                }
            }
        }
    }
}

void vIwdgTask(void *pvArg)
{
    (void)pvArg;
    while (1)
    {
        IWDG_ReloadCounter();
        vTaskDelay(2000 * portTICK_RATE_MS);
    }
}

#define BUFSIZE 512
char uart_buf[BUFSIZE];

void vUartTask(void *pvArg)
{
    (void)pvArg;
    static FIL logfile;
    static FRESULT res;
    
    unsigned int bytes_read = 0;
    
    while (1)
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET) vTaskDelay(100);
        uint8_t command = USART_ReceiveData(USART1) & 0xFF;
        if ('S' == command)
        {
            sprintf(uart_buf, "%d,%d\n", solarLogger.impsToday, houseLogger.impsToday);

            for (int i = 0; 0 != uart_buf[i]; i++)
            {
                putf_serial(NULL, uart_buf[i]);
            }
        }
        else if ('L' == command)
        {
            res = f_open(&logfile, LOGFILENAME, FA_OPEN_EXISTING | FA_READ);
            if (FR_OK != res)
            {
                PrintFileError(res, "open log for total read");
                goto bye;
            }

            while (!f_eof(&logfile))
            {
                res = f_read(&logfile, uart_buf, BUFSIZE, &bytes_read);
                if (FR_OK != res)
                {
                    PrintFileError(res, "during total read");
                    goto bye;
                }
                
                for (unsigned int i=0; i < bytes_read; i++)
                {
                    putf_serial(NULL, uart_buf[i]);
                }
            }
            
            bye:
            putf_serial(NULL, 'E');
            putf_serial(NULL, 'N');
            putf_serial(NULL, 'D');
            putf_serial(NULL, '\n');
            f_close(&logfile);
        }
        else if ('R' == command)
        {
            NVIC_SystemReset();
        }
        else if ('C' == command)
        {
            init_printf(NULL, putf_serial);
            Time_Adjust();
            init_printf(NULL, putf_gui);
            setCurrentBinFromRtc();
        }
        
        vTaskDelay(100 * portTICK_RATE_MS);
    }
}

void UserPixelSetFunction(UG_S16 x, UG_S16 y, UG_COLOR c)
{
    uint16_t lcdColor = RGB565CONVERT((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
    LCD_SetPoint(x, y, lcdColor);
}

extern UG_GUI gui;

static void prvSetupHardware(void)
{
    /* Configure HCLK clock as SysTick clock source. */
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
    GPIO_Configuration();
    NVIC_Configuration();
    USART_Configuration();
    init_printf(NULL, putf_serial);
    RTC_Init();
    TIM_Configuration();
    
    LCD_Initialization();
    LCD_Clear(Black);
    UG_Init (&gui, &UserPixelSetFunction, MAX_X, MAX_Y);
    UG_SelectGUI(&gui);
    UG_SetForecolor(White);
    UG_SetBackcolor(Black);
    
    UG_FontSelect(&FONT_6X8);
    
    UG_ConsoleSetArea(0, CONSOLE_START_Y, MAX_CONSOLE_X, CONSOLE_END_Y);
    UG_ConsoleSetForecolor(C_GREEN);
    UG_ConsoleSetBackcolor(C_BLACK);
    init_printf(NULL, putf_gui);
    
    printf("initializing USB...\n");
    Set_System();
    Set_USBClock();
    USB_Interrupts_Config();
    USB_Init();
}

void putchar(char ch)
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART */
  USART_SendData(USART1, (uint8_t) ch);

  /* Loop until the end of transmission */
  while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
  {}
}

static void putf_serial(void *dummy, char ch)
{
    (void)dummy;
    putchar(ch);
}

static void putf_gui(void *dummy, char ch)
{
    (void)dummy;
    char buffer[2];
    buffer[0] = ch;
    buffer[1] = 0;
    UG_ConsolePutString(buffer);
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
