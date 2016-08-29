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
#include "task.h"
#include "GLCD.h"
#include "ugui.h"
#include "logging.h"
#include "energygraph.h"
#include "printf.h"
#include "rtc.h"
#include "sdio_sd.h"
#include "ff.h"

/* Private define ------------------------------------------------------------*/
#define LED_TASK_STACK_SIZE			( configMINIMAL_STACK_SIZE )
#define LCD_TASK_STACK_SIZE			( configMINIMAL_STACK_SIZE )

#define LED_TASK_PRIORITY			( tskIDLE_PRIORITY + 1 )
#define LCD_TASK_PRIORITY			( tskIDLE_PRIORITY + 3 )

/* Private function prototypes -----------------------------------------------*/
static void prvSetupHardware(void);
void EXTI_Configuration(void);
void NVIC_Configuration(void);
void GPIO_Configuration(void);
void USART_Configuration(void);
void RTC_Init(void);
void vLCDTask(void * pvArg);
void vLEDTask(void * pvArg);
void putchar(char ch);
static void putf_serial(void * dummy, char ch);
static void putf_gui(void * dummy, char ch);
FRESULT scan_files (char* path);
int SD_TotalSize(void);


FATFS fs;         /* Work area (file system object) for logical drive */
FIL fsrc;         /* file objects */   
FRESULT res;
UINT br;

char path[64]="0:";
uint8_t textFileBuffer[] = "Thank you for using HY-MiniSTM32V Development Board\r\n";   

int main(void)
{
    prvSetupHardware();
    
    //f_mkfs(0, 1, 0);
    
    if( SD_Detect() == SD_PRESENT )
    {
      printf("-- SD card detected\r\n");
    }
    else
    {
      printf("-- Please connect a SD card \r\n");
      while(SD_Detect()!=SD_PRESENT);
      printf("-- SD card connected\r\n");
    }

    f_mount(0, &fs); 

    res = f_open( &fsrc , "0:/Demo.TXT" , FA_CREATE_NEW | FA_WRITE);        

    if ( res == FR_OK )
    { 
      /* Write buffer to file */
      res = f_write(&fsrc, textFileBuffer, sizeof(textFileBuffer), &br);     
 
      printf("Demo.TXT created\r\n");
    
      /*close file */
      f_close(&fsrc);      
    }
    else if ( res == FR_EXIST )
    {
      printf("Demo.TXT exists\r\n");
    }

    scan_files(path);
    SD_TotalSize();
    
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
        //printf("%s\r\n", Time_As_String());
        /* LED1-ON */
        GPIO_SetBits(GPIOB, GPIO_Pin_0);
        vTaskDelay(1000);

        //printf("%s\r\n", Time_As_String());
        /* LED1-OFF */
        GPIO_ResetBits(GPIOB, GPIO_Pin_0);
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
    LCD_Initialization();
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
    EXTI_Configuration();
    NVIC_Configuration();
    USART_Configuration();
    init_printf(NULL, putf_serial);
    RTC_Init();
    //init_printf(NULL, putf_gui);
}

void EXTI_Configuration(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;

    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    
    EXTI_InitStructure.EXTI_Line = EXTI_Line1;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
}

void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Configure one bit for preemption priority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

  /* Enable the RTC Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
 
}

void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
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
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

}

void USART_Configuration(void)
{ 
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure; 

  RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1,ENABLE);
  /*
  *  USART1_TX -> PA9 , USART1_RX ->    PA10
  */                
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;          
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_Init(GPIOA, &GPIO_InitStructure);           

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;            
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  USART_Init(USART1, &USART_InitStructure); 
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
  USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
  USART_Cmd(USART1, ENABLE);
  
  putchar(' ');
  putchar('U');
  putchar('A');
  putchar('R');
  putchar('T');
  putchar(' ');
  putchar('O');
  putchar('K');
  putchar('\r');
  putchar('\n');
}

void RTC_Init(void)
{
    if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
    {
      /* Backup data register value is not correct or not yet programmed (when
         the first time the program is executed) */
    
      printf("\r\n\n RTC not yet configured....");
    
      /* RTC Configuration */
      RTC_Configuration();
    
      printf("\r\n RTC configured....");
    
      /* Adjust time by values entered by the user on the hyperterminal */
      Time_Adjust();
    
      BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
    }
    else
    {
      /* Check if the Power On Reset flag is set */
      if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)
      {
        printf("\r\n\n Power On Reset");
      }
      /* Check if the Pin Reset flag is set */
      else if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET)
      {
        printf("\r\n\n External Reset");
      }
    
      printf("\r\n No need to configure RTC");
      /* Wait for RTC registers synchronization */
      RTC_WaitForSynchro();
    
      /* Enable the RTC Second */
      RTC_ITConfig(RTC_IT_SEC, ENABLE);
      /* Wait until last write operation on RTC registers has finished */
      RTC_WaitForLastTask();
    }
}

void RTC_Configuration(void)
{
  /* Enable PWR and BKP clocks */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

  /* Allow access to BKP Domain */
  PWR_BackupAccessCmd(ENABLE);

  /* Reset Backup Domain */
  BKP_DeInit();

  /* Enable LSE */
  RCC_LSEConfig(RCC_LSE_ON);
  /* Wait till LSE is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
  {}

  /* Select LSE as RTC Clock Source */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

  /* Enable RTC Clock */
  RCC_RTCCLKCmd(ENABLE);

  /* Wait for RTC registers synchronization */
  RTC_WaitForSynchro();

  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();

  /* Enable the RTC Second */
  //RTC_ITConfig(RTC_IT_SEC, ENABLE);

  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();

  /* Set RTC prescaler: set RTC period to 1sec */
  RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */

  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();
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

FRESULT scan_files (char* path)
{
    FILINFO fno;
    DIR dir;
    int i;
    char *fn;
#if _USE_LFN
    static char lfn[_MAX_LFN * (_DF1S ? 2 : 1) + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif

    res = f_opendir(&dir, path);
    if (res == FR_OK) {
        i = strlen(path);
        for (;;) {
            res = f_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0) break;
            if (fno.fname[0] == '.') continue;
#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif
            printf("%s/%s \r\n", path, fn);
        }
    }

    return res;
}

int SD_TotalSize(void)
{
    FATFS *fs;
    DWORD fre_clust;        

    res = f_getfree("0:", &fre_clust, &fs); 
    if ( res==FR_OK ) 
    {
      /* Print free space in unit of MB (assuming 512 bytes/sector) */
      printf("\r\n%d MB total drive space.\r\n"
           "%d MB available.\r\n",
           ( (fs->n_fatent - 2) * fs->csize ) / 2 /1024 , (fre_clust * fs->csize) / 2 /1024 );
        
      return ENABLE;
    }
    else 
      return DISABLE;   
}


void SDIO_IRQHandler(void)
{
  SD_ProcessIRQSrc();
}

//#define FIVE_MINUTES (60 * 5)
#define FIVE_MINUTES 3
void RTC_IRQHandler(void)
{
    static int seconds = 0;
    ++seconds;
    if (FIVE_MINUTES == seconds)
    {
        newBin(&solarLogger);
        newBin(&houseLogger);
        seconds = 0;
    }
    
    RTC_ClearITPendingBit(RTC_IT_SEC);
    UG_PutString(MAX_X - 56, 0, Time_As_String());
}

void EXTI0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line0) != RESET)
    {
        addImp(&solarLogger);
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}

void EXTI1_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line1) != RESET)
    {
        addImp(&houseLogger);
        EXTI_ClearITPendingBit(EXTI_Line1);
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

    __asm("BKPT #0\n") ; // Break into the debugger

    /* When the following line is hit, the variables contain the register values. */
    for( ;; );
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
