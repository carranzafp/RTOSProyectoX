#ifndef _BSP_H_
#define _BSP_H_

/* Definition for USARTx Pins */
#define UARTx                           USART2
#define UARTx_TX_PIN                    GPIO_PIN_2
#define UARTx_RX_PIN                    GPIO_PIN_3
#define UARTx_GPIO_PORT                 GPIOA
#define UARTx_GPIO_AF                   GPIO_AF1_USART2
#define UARTx_RCC_GPIOx_CLK_ENABLE      __HAL_RCC_GPIOA_CLK_ENABLE
#define UARTx_RCC_USARTx_CLK_ENABLE     __HAL_RCC_USART2_CLK_ENABLE

/* Definition for USARTx clock resources */
#define LED_PIN                         GPIO_PIN_5
#define LED_GPIO_PORT                   GPIOA
#define LED_RCC_GPIOx_CLK_ENABLE        __HAL_RCC_GPIOA_CLK_ENABLE

#define BTN_PIN                         GPIO_PIN_13
#define BTN_GPIO_PORT                   GPIOC
#define BTN_RCC_GPIOx_CLK_ENABLE        __HAL_RCC_GPIOC_CLK_ENABLE

/*button input interrupt priority*/
#define BTN_IRQ_PRIO                    2u
/*internal clock interrupt priority*/
#define RTCC_IRQ_PRIO                   2u
/*serial port interrupt priority*/
#define UART_IRQ_PRIO                   2u

#endif
