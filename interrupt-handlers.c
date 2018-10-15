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
#include "usb_istr.h"
#include "usb_lib.h"
#include "usb_pwr.h"

extern xQueueHandle impQueue;
extern xQueueHandle slotQueue;
extern EnergyLogger solarLogger;
extern EnergyLogger houseLogger;
extern char *uart_buf;

#define DEBOUNCE_MS 500

#define FIVE_MINUTES (60 * 5)
void RTC_IRQHandler(void)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    
    RTC_ClearITPendingBit(RTC_IT_SEC);
    static int seconds;
    
    seconds = RTC_GetCounter() % FIVE_MINUTES;
    if (slotQueue) { xQueueSendFromISR(slotQueue, &seconds, &xHigherPriorityTaskWoken); }
    
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

static volatile unsigned long timer_value;

static unsigned long get_timer()
{
    return timer_value + TIM_GetCounter(TIM2);
}

void EXTI0_IRQHandler(void)
{    
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    
    if (EXTI_GetITStatus(EXTI_Line0) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line0);
        unsigned long impTimer = get_timer();

        // debounce house meter output
        if (impTimer - solarLogger.impTimer > DEBOUNCE_MS)
        {
            EnergyLogger *solarLoggerPtr = &solarLogger;
            solarLogger.lastImpTimer = solarLogger.impTimer;
	    solarLogger.impTimer = impTimer;
            if (impQueue) { xQueueSendFromISR(impQueue, &solarLoggerPtr, &xHigherPriorityTaskWoken); }
        }
    }
    
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

void EXTI1_IRQHandler(void)
{    
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    if (EXTI_GetITStatus(EXTI_Line1) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line1);
        unsigned long impTimer = get_timer();
      
        // debounce house meter output
        if (impTimer - houseLogger.impTimer > DEBOUNCE_MS)
        {
            EnergyLogger *houseLoggerPtr = &houseLogger;
            houseLogger.lastImpTimer = houseLogger.impTimer;
            houseLogger.impTimer = impTimer;
            if (impQueue) { xQueueSendFromISR(impQueue, &houseLoggerPtr, &xHigherPriorityTaskWoken); }
        }
    }
    
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

void TIM2_IRQHandler(void)
{
    if ( TIM_GetITStatus(TIM2 , TIM_IT_Update) != RESET ) 
    {
       timer_value += 64000;
       TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
    }   
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
  USB_Istr();
}

void USB_HP_CAN1_TX_IRQHandler(void)
{
  CTR_HP();
}

static const char *whichFault = "HardFault\n";

void __attribute((__naked__))  BusFault_Handler(void)
{
    whichFault = "BusFault\n";
    __asm volatile  ( " b HardFault_Handler\n" );
}

void __attribute((__naked__))  UsageFault_Handler(void)
{
    whichFault = "UsageFault\n";
    __asm volatile  ( " b HardFault_Handler\n" );
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
    
    GPIO_SetBits(GPIOB, GPIO_Pin_1);
    UG_SetForecolor(C_RED);
    register int sp asm ("sp");
    UG_PutString(0, HARDFAULT_Y, whichFault);
    sprintf(uart_buf, "pc: %x sp: %x lr: %x", pc, sp, lr);
    UG_PutString(0, HARDFAULT_Y + 9, uart_buf);
    sprintf(uart_buf, "r0: %x r1: %x r2: %x", r0, r1, r2);
    UG_PutString(0, HARDFAULT_Y + 18, uart_buf);
    sprintf(uart_buf, "r3: %x forced: %d", r3, forced);
    UG_PutString(0, HARDFAULT_Y + 27, uart_buf);
    
    __asm("BKPT #0\n") ; // Break into the debugger

    /* When the following line is hit, the variables contain the register values. */
    for( ;; )
    {              
    }
}
