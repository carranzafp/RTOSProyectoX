#include "stm32f0xx.h"
#include <stdint.h>
#include "app_bsp.h"
#include "FreeRTOS.h"
#include "task.h"

extern RTC_HandleTypeDef  ClockHandle;
extern UART_HandleTypeDef UartHandle;

/**------------------------------------------------------------------------------------------------
Brief.- NMI isr vector
-------------------------------------------------------------------------------------------------*/
void NMI_Handler( void )
{

}

/**------------------------------------------------------------------------------------------------
Brief.- HardFault isr vector
-------------------------------------------------------------------------------------------------*/
void HardFault_Handler( void )
{
    while(1u);
}

/**------------------------------------------------------------------------------------------------
Brief.- SVC isr vector, this vector is defined inside port.c as part of FreeRTOS 
-------------------------------------------------------------------------------------------------*/
/*void SVC_Handler( void )
{

}*/

/**------------------------------------------------------------------------------------------------
Brief.- PendSV isr vector, this vector is defined inside port.c as part of FreeRTOS
-------------------------------------------------------------------------------------------------*/
/*void PendSV_Handler( void )
{

}*/

/**------------------------------------------------------------------------------------------------
Brief.- Systick ISR wich calls the rtos ticj and also the hal library tick
-------------------------------------------------------------------------------------------------*/
void SysTick_Handler( void )
{
    /* query if the os is running */
    if( xTaskGetSchedulerState( ) != taskSCHEDULER_NOT_STARTED )
    {
        /* service the os tick interrupt */
        xPortSysTickHandler( );
    }
    /* increment the internal tick for the hal drivers */
    HAL_IncTick( );
}

/**------------------------------------------------------------------------------------------------
Brief.- ISR vector for the on board button
-------------------------------------------------------------------------------------------------*/
void EXTI4_15_IRQHandler(void)
{
    /*this fucntion is defined in main.c with the wrap name vInterruptButton*/
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_13 );
}

/**------------------------------------------------------------------------------------------------
Brief.- ISR vector for the internal clock
-------------------------------------------------------------------------------------------------*/
void RTC_IRQHandler(void)
{
    HAL_RTC_AlarmIRQHandler( &ClockHandle );
}

/**------------------------------------------------------------------------------------------------
Brief.- ISR vector for uart
-------------------------------------------------------------------------------------------------*/
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler( &UartHandle );
}
