#include "stm32f0xx.h"
#include <stdint.h>
#include "app_bsp.h"

void HAL_MspInit( void )
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;

    /* Select HSI48 Oscillator as PLL source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI48;
    RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
    HAL_RCC_OscConfig( &RCC_OscInitStruct );
    
    /* Select PLL as system clock source and configure the HCLK and PCLK1 clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig( &RCC_ClkInitStruct, FLASH_LATENCY_1 );

    __HAL_RCC_SYSCFG_CLK_ENABLE( );
}

void HAL_RTC_MspInit( RTC_HandleTypeDef *hrtc )
{
    RCC_OscInitTypeDef        RCC_OscInitStruct;
    RCC_PeriphCLKInitTypeDef  RCC_PeriphClkStruct;

    __HAL_RCC_PWR_CLK_ENABLE( );
    HAL_PWR_EnableBkUpAccess( );
    
    /*##-2- Configue LSE/LSI as RTC clock soucre ###############################*/
    RCC_OscInitStruct.OscillatorType =  RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    RCC_OscInitStruct.LSIState = RCC_LSI_OFF;
    HAL_RCC_OscConfig( &RCC_OscInitStruct );
    
    RCC_PeriphClkStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    RCC_PeriphClkStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    HAL_RCCEx_PeriphCLKConfig( &RCC_PeriphClkStruct );

    __HAL_RCC_RTC_ENABLE( );

    HAL_NVIC_SetPriority( RTC_IRQn, RTCC_IRQ_PRIO, 0u );
    HAL_NVIC_EnableIRQ( RTC_IRQn );
}

void HAL_UART_MspInit( UART_HandleTypeDef *huart )
{  
    GPIO_InitTypeDef  GPIO_InitStruct;
  
    /*##-1- Enable peripherals and GPIO Clocks #################################*/
    /* Enable GPIO TX/RX clock */
    UARTx_RCC_GPIOx_CLK_ENABLE();
    /* Enable USARTx clock */
    UARTx_RCC_USARTx_CLK_ENABLE(); 
  
    /*##-2- Configure peripheral GPIO ##########################################*/  
    /* UART TX GPIO pin configuration  */
    GPIO_InitStruct.Pin       = UARTx_TX_PIN | UARTx_RX_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = UARTx_GPIO_AF;
    HAL_GPIO_Init( UARTx_GPIO_PORT, &GPIO_InitStruct );

    HAL_NVIC_SetPriority( USART2_IRQn, UART_IRQ_PRIO, 0u );
    HAL_NVIC_EnableIRQ( USART2_IRQn );
}

void HAL_WWDG_MspInit( WWDG_HandleTypeDef *hwwdg )
{
    __HAL_RCC_WWDG_CLK_ENABLE( );
}
