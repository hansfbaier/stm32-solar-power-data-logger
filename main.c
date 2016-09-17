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

/* Private define ------------------------------------------------------------*/
#define LOGGER_TASK_STACK_SIZE		( 150 )
#define LCD_TASK_STACK_SIZE			( configMINIMAL_STACK_SIZE )

#define LOGGER_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define LCD_TASK_PRIORITY			( tskIDLE_PRIORITY + 1 )

/* Private function prototypes -----------------------------------------------*/
static void prvSetupHardware(void);
void vLoggerTask(void * pvArg);
void putchar(char ch);
static void putf_serial(void * dummy, char ch);
static void putf_gui(void * dummy, char ch);

xQueueHandle impQueue  = NULL;
xQueueHandle slotQueue = NULL;

int main(void)
{
    prvSetupHardware();
    
    Init_Logging();
    
    xTaskCreate(vLoggerTask, (signed char * ) NULL, LOGGER_TASK_STACK_SIZE, NULL, LOGGER_TASK_PRIORITY, NULL);
    /* Start the scheduler. */
    vTaskStartScheduler();

    return 0;
}

extern EnergyLogger solarLogger;
extern EnergyLogger houseLogger;

void vLoggerTask(void * pvArg)
{    
    EnergyLogger *logger;
    int seconds;
    
    impQueue  = xQueueCreate(10, sizeof(EnergyLogger *));
    slotQueue = xQueueCreate(10, sizeof(int));
    
    while (1)
    {
        uint32_t currentRtc = RTC_GetCounter();
        if (0 == currentRtc % ONE_DAY)
        {
            // clear graph
            UG_DrawFrame(0, MAX_BIN_Y, MAX_CONSOLE_X, MAX_Y, C_BLACK);
        }
        
        if (xQueueReceive(impQueue, &logger, 10))
        {
            configASSERT(NULL != logger);
            addImp(logger);
            
            char imps[10];
            int impsThisBin = getCurrentBin(logger);
            sprintf(imps, "%4d", impsThisBin);
            
#ifdef DISPLAY_WATTS
            char watts[10];
            if (logger->impTimer < logger->lastImpTimer) {
                logger->impTimer += 64000;
            }
            int millisSinceLastImp = (logger->impTimer - logger->lastImpTimer) / 2;
            int wattsUsed = 2250000 / millisSinceLastImp;
            sprintf(watts, "%4dW", wattsUsed);
#endif
            
            char watthours[10];
            int wattHoursThisBin = (1000 * impsThisBin) / 1600;
            sprintf(watthours, "%4dWh", wattHoursThisBin);
            
            char watthoursday[10];
            int wattHoursToday = (1000 * logger->impsToday) / 1600;
            sprintf(watthoursday, "%4dWh", wattHoursToday);            
            
            if (&solarLogger == logger)
            {
                GPIO_SetBits(GPIOB, GPIO_Pin_1);
                vTaskDelay(10);                
                GPIO_ResetBits(GPIOB, GPIO_Pin_1);
                
                UG_SetForecolor(SOLAR_COLOR);
                UG_PutString(SOLAR_X, IMPS_Y,         imps);
                UG_PutString(SOLAR_X, WATTS_Y,        "        ");
#ifdef DISPLAY_WATTS
                UG_PutString(SOLAR_X, WATTS_Y,        watts);
#endif
                UG_PutString(SOLAR_X, WATTHOURS_Y,    watthours);
                UG_PutString(SOLAR_X, WATTHOURSDAY_Y, watthoursday);
            }
            else if (&houseLogger == logger)
            {
                GPIO_SetBits(GPIOB, GPIO_Pin_0);
                vTaskDelay(10);     
                GPIO_ResetBits(GPIOB, GPIO_Pin_0);
                
                UG_SetForecolor(HOUSE_COLOR);
                UG_PutString(HOUSE_X, IMPS_Y,         imps);
                UG_PutString(HOUSE_X, WATTS_Y,        "        ");
#ifdef DISPLAY_WATTS
                UG_PutString(HOUSE_X, WATTS_Y,        watts);
#endif
                UG_PutString(HOUSE_X, WATTHOURS_Y,    watthours);
                UG_PutString(HOUSE_X, WATTHOURSDAY_Y, watthoursday);
            }
            
            plotBin(solarLogger.currentBinNo);
        }

        if (xQueueReceive(slotQueue, &seconds, 10))
        {            
            UG_SetForecolor(C_WHITE);
            char buf[6];
            sprintf(buf, "%5d", TIM_GetCounter(TIM2));
            UG_PutString(MAX_CONSOLE_X + 7, 0, buf);

            UG_PutString(MAX_CONSOLE_X + 56, 0, Time_As_String());
            if (0 == seconds)
            {
                configASSERT(solarLogger.currentBinNo == houseLogger.currentBinNo);
                newBin(&solarLogger);
                newBin(&houseLogger);
                configASSERT(solarLogger.currentBinNo == houseLogger.currentBinNo);
                printZeroedCounters();
                SD_TotalSize();
                
                plotBin(getLastBinNo(&solarLogger));
                Write_Log_Entry();
                
                if (0 == solarLogger.currentBinNo)
                {
                    char buf[15];
                    sprintf(buf, "Day %d", (int)(RTC_GetCounter() / ONE_DAY));
                    UG_SetForecolor(C_WHITE);
                    UG_PutString(MAX_X/2 - 2 * 9, 0, buf);
                    UG_FillFrame(0, IMPS_Y, MAX_X, MAX_Y, C_BLACK);
                }
            }
        }
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
    EXTI_Configuration();
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
    putchar(ch);
}

static void putf_gui(void *dummy, char ch)
{
    char buf[2];
    buf[0] = ch;
    buf[1] = 0;
    UG_ConsolePutString(buf);
}

static long int dummy;

#define FIVE_MINUTES (60 * 5)
void RTC_IRQHandler(void)
{
    RTC_ClearITPendingBit(RTC_IT_SEC);
    static int seconds;
    
    seconds = RTC_GetCounter() % FIVE_MINUTES;
    if (slotQueue) { xQueueSendFromISR(slotQueue, &seconds, &dummy); }   
}

void EXTI0_IRQHandler(void)
{    
    if (EXTI_GetITStatus(EXTI_Line0) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line0);
        EnergyLogger *solarLoggerPtr = &solarLogger;
        solarLogger.lastImpTimer = solarLogger.impTimer;
        solarLogger.impTimer = TIM_GetCounter(TIM2);
        if (impQueue) { xQueueSendFromISR(impQueue, &solarLoggerPtr, &dummy); }
    }
}

void EXTI1_IRQHandler(void)
{    
    if (EXTI_GetITStatus(EXTI_Line1) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line1);
        int impTimer = TIM_GetCounter(TIM2);
        // debounce broken house meter output
        if (impTimer - houseLogger.impTimer > 200)
        {
            EnergyLogger *houseLoggerPtr = &houseLogger;
            houseLogger.lastImpTimer = houseLogger.impTimer;
            houseLogger.impTimer = impTimer;
            if (impQueue) { xQueueSendFromISR(impQueue, &houseLoggerPtr, &dummy); }
        }
    }
}

void __attribute((__naked__)) HardFault_Handler( void )
{
    __asm volatile
    (
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
}

void TIM2_IRQHandler(void)
{
    if ( TIM_GetITStatus(TIM2 , TIM_IT_Update) != RESET ) 
    {
       TIM_ClearITPendingBit(TIM2 , TIM_FLAG_Update);
    }   
}

void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
    /* These are volatile to try and prevent the compiler/linker optimising them
    away as the variables never actually get used.  If the debugger won't show the
    values of the variables, make them global my moving their declaration outside
    of this function. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
    volatile uint32_t r0;
    volatile uint32_t r1;
    volatile uint32_t r2;
    volatile uint32_t r3;
    volatile uint32_t r12;
    volatile uint32_t lr; /* Link register. */
    volatile uint32_t pc; /* Program counter. */
    volatile uint32_t psr;/* Program status register. */
    volatile unsigned long _CFSR ;
    volatile unsigned long _HFSR ;
    volatile unsigned long _DFSR ;
    volatile unsigned long _AFSR ;
    volatile unsigned long _BFAR ;
    volatile unsigned long _MMAR ;
    volatile char forced;
    volatile char vectTbl;
#pragma GCC diagnostic pop

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    // Configurable Fault Status Register
    // Consists of MMSR, BFSR and UFSR
    _CFSR = (*((volatile unsigned long *)(0xE000ED28))) ;

    // Hard Fault Status Register
    _HFSR = (*((volatile unsigned long *)(0xE000ED2C))) ;
    forced = (_HFSR & (1 << 30)) > 0;
    vectTbl = (_HFSR & (1 << 1)) > 0;

    // Debug Fault Status Register
    _DFSR = (*((volatile unsigned long *)(0xE000ED30))) ;

    // Auxiliary Fault Status Register
    _AFSR = (*((volatile unsigned long *)(0xE000ED3C))) ;

    // Read the Fault Address Registers. These may not contain valid values.
    // Check BFARVALID/MMARVALID to see if they are valid values
    // MemManage Fault Address Register
    _MMAR = (*((volatile unsigned long *)(0xE000ED34))) ;
    // Bus Fault Address Register
    _BFAR = (*((volatile unsigned long *)(0xE000ED38))) ;
    
    GPIO_SetBits(GPIOB, GPIO_Pin_1);
    UG_ConsoleSetForecolor(C_RED);
    register int sp asm ("sp");
    char buf[32];
    UG_ConsolePutString("HardFault\n");
    sprintf(buf, "pc: %x sp: %x\n", pc, sp);
    UG_ConsolePutString(buf);
    
    __asm("BKPT #0\n") ; // Break into the debugger

    /* When the following line is hit, the variables contain the register values. */
    for( ;; )
    {              
    }
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
