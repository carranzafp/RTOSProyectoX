#include <stdint.h>
#include <string.h>
#include "stm32f0xx.h"
#include "app_bsp.h"
#include "app_utils.h"
#include "FreeRTOS.h"
#include "task.h"
#include "SEGGER_RTT.h"


UART_HandleTypeDef UartHandle;
WWDG_HandleTypeDef WwdgHandle;
RTC_HandleTypeDef  ClockHandle;
RTC_TimeTypeDef ClockTime;
RTC_DateTypeDef ClockDate;
RTC_AlarmTypeDef ClockAlarm;
uint8_t RxByte;

static void vInitSerial( void );
static void vInitSegger( void );
static void vInitLed( void );
static void vInitButton( void );
static void vInitClock( void );
static void reverse( char *str, int length );

/**
 * Initalize all the necesary hardware we need have on our board
 * segger RTT and systemview for sending messages and visualization
 * serial port to send msg trhou uart at 115200 bauds
 * on board led
 * on board button
*/
void vSetupHardware( void )
{
    /*init stm32cube library and set sysclok to 48MHz*/
    HAL_Init( );
    /*initialize RTT ans system view*/
    vInitSegger( );
    /*init serial port (uart2)*/
    vInitSerial( );
    /*init on board led*/
    vInitLed( );
    /*init on board button with ISR (falling edge)*/
    vInitButton( );
    /*init internal real time clock*/
    vInitClock( );
}

/**
 * send a string of characters throu the segger jlink debugger, it is neccesary
 * a Jlink client
*/
void vPrintString( char *string )
{
    vTaskSuspendAll( );
    SEGGER_RTT_printf(0, "%s\r\n", string);
    xTaskResumeAll( );
}

/**
 * send a string of characters plus a 32 bitssigned  number throu the segger jlink debugger, 
 * it is neccesary a Jlink client
*/
void vPrintStringAndNumber( char *string, int num )
{
    vTaskSuspendAll( );
    SEGGER_RTT_printf(0, "%s %d\r\n", string, num);
    xTaskResumeAll( );
}

/**
 * send a given a number of character through the serial port
*/
void vSerialSend( uint8_t *buffer, uint32_t lenght )
{
    HAL_UART_Transmit( &UartHandle, buffer, lenght, 1000u );
}

/**
 * read the character reeived from the serial port
 * NOTE: it is mandatory to call function inside the ISR vector
 * this fucntion also enable the next character reception
*/
uint8_t xSerialReadFromISR( void )
{
    uint8_t Rx = RxByte;
    HAL_UART_Receive_IT( &UartHandle, &RxByte, 1u );
    return Rx;
}

/**
 * turn off the on board led
*/
void vLedOn( void )
{
    HAL_GPIO_WritePin( LED_GPIO_PORT, LED_PIN, SET );
}

/**
 * toggle the on board led
*/
void vLedOff( void )
{
    HAL_GPIO_WritePin( LED_GPIO_PORT, LED_PIN, RESET );
}

/**
 * turn on the on board led
*/
void vLedToggle( void )
{
    HAL_GPIO_TogglePin( LED_GPIO_PORT, LED_PIN );
}

/**
 * retrun the on board button state
*/
uint8_t xReadButton( void )
{
    return HAL_GPIO_ReadPin( BTN_GPIO_PORT, BTN_PIN );
}

/**
 * Enable the button interrupt on a falling, rising or both edges
 * BTN_INT_RISING, BTN_INT_FALLING, BTN_INT_BOTH 
*/
void vSetIntButton( uint32_t intmode )
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    GPIO_InitStruct.Mode  = intmode;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Pin   = BTN_PIN;
    HAL_GPIO_Init( BTN_GPIO_PORT, &GPIO_InitStruct );

    HAL_NVIC_SetPriority( EXTI4_15_IRQn, BTN_IRQ_PRIO, 0 );
    HAL_NVIC_EnableIRQ( EXTI4_15_IRQn );
}

/**
 * set a new time of the day in 24hrs format
 */
void vSetTime( uint8_t hh, uint8_t mm, uint8_t ss )
{
    ClockTime.Hours = hh;
    ClockTime.Minutes = mm;
    ClockTime.Seconds = ss;
    HAL_RTC_SetTime( &ClockHandle, &ClockTime, RTC_FORMAT_BIN );
}

/**
 * get by reference the current time of the day in 24hrs format
 */
void vGetTime( uint8_t *hh, uint8_t *mm, uint8_t *ss )
{
    HAL_RTC_GetTime( &ClockHandle, &ClockTime, RTC_FORMAT_BIN );
    *hh = ClockTime.Hours;
    *mm = ClockTime.Minutes;
    *ss = ClockTime.Seconds;

    /**dummy read, it is necesary for the RTCC*/
    HAL_RTC_GetDate( &ClockHandle, &ClockDate, RTC_FORMAT_BIN );  
}


/**
 * get by reference the current time of the alarm in 24hrs format
 */
void vGetAlarm( uint8_t *hh, uint8_t *mm, uint8_t *ss )
{
    /*Get the Alarm time from the intermediate buffer*/
    *hh = ClockAlarm.AlarmTime.Hours;
    *mm = ClockAlarm.AlarmTime.Minutes;
    *ss = ClockAlarm.AlarmTime.Seconds;  
}

/**
 * set a new date with format dd mm yy
 */
void vSetDate( uint8_t dd, uint8_t mm, uint8_t yy )
{
    ClockDate.Year = yy;
    ClockDate.Month = mm;
    ClockDate.Date = dd;
    HAL_RTC_SetDate( &ClockHandle, &ClockDate, RTC_FORMAT_BIN );
}

/**
 * get by reference the current date
 */
void vGetDate( uint8_t *dd, uint8_t *mm, uint8_t *yy )
{
    /**dummy read, it is necesary for the RTCC*/
    HAL_RTC_GetTime( &ClockHandle, &ClockTime, RTC_FORMAT_BIN );

    HAL_RTC_GetDate( &ClockHandle, &ClockDate, RTC_FORMAT_BIN );
    *yy = ClockDate.Year;
    *mm = ClockDate.Month;
    *dd = ClockDate.Date;  
}

/**
 * Set a new alarm, this alarm will be valid until is activate, the same
 * interrupt will be prevent form happening again, until a new alarm is set
 * The alarm config will only take into account hours, minutes and seconds 
 */
void vSetAlarm( uint8_t hh, uint8_t mm, uint8_t ss )
{
    ClockAlarm.AlarmTime.Hours = hh;
    ClockAlarm.AlarmTime.Minutes = mm;
    ClockAlarm.AlarmTime.Seconds = ss;
    HAL_RTC_SetAlarm_IT( &ClockHandle, &ClockAlarm, RTC_FORMAT_BIN );
}

void vUnsetAlarm(void)
{
    HAL_RTC_DeactivateAlarm(&ClockHandle, ClockAlarm.Alarm);
}

/**
 * intit the the watchdog with a time window of 32.1ms and 43ms
 * you need to call the reset function between the time windows to avoid a reset
*/
void vEnableWatchDog( void )
{
    WwdgHandle.Instance       = WWDG;
    WwdgHandle.Init.Prescaler = WWDG_PRESCALER_8;
    WwdgHandle.Init.Window    = 80u;
    WwdgHandle.Init.Counter   = 127u;
    WwdgHandle.Init.EWIMode  =  WWDG_EWI_DISABLE;
    HAL_WWDG_Init( &WwdgHandle );
}

/**
 * A suitable value to call this fucntion will be between 32ms - 43ms
 */
void vWatchdogReset( void )
{
    HAL_WWDG_Refresh( &WwdgHandle );
}

/**
 * intialize serial port
*/
static void vInitSerial( void )
{
    /*initialize serial port*/
    UartHandle.Instance        = UARTx;
    UartHandle.Init.BaudRate   = 115200;
    UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits   = UART_STOPBITS_1;
    UartHandle.Init.Parity     = UART_PARITY_NONE;
    UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode       = UART_MODE_TX_RX;
    UartHandle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_UART_Init( &UartHandle );

    HAL_UART_Receive_IT( &UartHandle, &RxByte, 1u );
}

/**
 * intialize segger system view and RTT
*/
static void vInitSegger( void )
{
    SEGGER_SYSVIEW_Conf( );
    SEGGER_SYSVIEW_Start( );
}

/**
 * initailize gpio A5 to control the on board led
*/
static void vInitLed( void )
{
    GPIO_InitTypeDef  GPIO_InitStruct;
    /*init led port as output*/
    LED_RCC_GPIOx_CLK_ENABLE( );

    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pin   = LED_PIN;
    HAL_GPIO_Init( LED_GPIO_PORT, &GPIO_InitStruct );
}

/**
 * intit the gpio C13 to read the on board push button
*/
static void vInitButton( void )
{
    GPIO_InitTypeDef  GPIO_InitStruct;
    /*init led port as output*/
    BTN_RCC_GPIOx_CLK_ENABLE( );

    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Pin   = BTN_PIN;
    HAL_GPIO_Init( BTN_GPIO_PORT, &GPIO_InitStruct );
}

/**
 * intit the internal real time clock 
*/
static void vInitClock( void )
{
    ClockHandle.Instance             = RTC;
    ClockHandle.Init.HourFormat      = RTC_HOURFORMAT_24;
    ClockHandle.Init.AsynchPrediv    = 0x7F;
    ClockHandle.Init.SynchPrediv     = 0x00FF;
    ClockHandle.Init.OutPut          = RTC_OUTPUT_DISABLE;
    HAL_RTC_Init( &ClockHandle );

    ClockTime.Hours = 03;
    ClockTime.Minutes = 40;
    ClockTime.Seconds = 45;
    HAL_RTC_SetTime(&ClockHandle, &ClockTime, RTC_FORMAT_BIN);

    ClockDate.WeekDay = RTC_WEEKDAY_MONDAY;
    ClockDate.Date = 07;
    ClockDate.Month = 9;
    ClockDate.Year = 20;
    HAL_RTC_SetDate(&ClockHandle, &ClockDate, RTC_FORMAT_BIN);


    ClockAlarm.Alarm = RTC_ALARM_A;
    ClockAlarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;
    ClockAlarm.AlarmTime.TimeFormat = RTC_HOURFORMAT_24;
}

char* xItoa( int num, char* str, int base )  
{
    int rem, i = 0; 
    int negative = 0u; 
  
    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if( num == 0 ) 
    { 
        str[ i++ ] = '0'; 
        str[ i ] = '\0'; 
        return str; 
    } 
  
    // In standard itoa(), negative numbers are handled only with  
    // base 10. Otherwise numbers are considered unsigned. 
    if( ( num < 0 ) && ( base == 10 ) ) 
    { 
        negative = 1u; 
        num *= -1; 
    } 
  
    // Process individual digits 
    while( num != 0 ) 
    { 
        rem = num % base; 
        //str[ i++ ] = ( rem > 9 ) ? ( rem-10 ) + 'a' : rem + '0'; 
        if( rem > 9 )
        {
            str[ i++ ] = ( rem - 10 ) + 'a';
        }
        else
        {
            str[ i++ ] = rem + '0';
        }
        
        num = num / base; 
    } 
  
    // If number is negative, append '-' 
    if( negative )
    {
        str[ i++ ] = '-';
    } 
  
    str[ i ] = '\0'; // Append string terminator 
  
    // Reverse the string 
    reverse( str, i ); 
  
    return str; 
}


static void reverse( char *str, int length ) 
{ 
    int start = 0; 
    int end = length - 1;
    char temp;

    while( start < end ) 
    { 
        temp = str[ start ];
        str[ start ] = str[ end ];
        str[ end ] = temp;
        start++; 
        end--; 
    } 
}

uint32_t ulRandomNumber( uint32_t min, uint32_t max )
{
	uint32_t r;

	vTaskSuspendAll();
	{
		r = min + ( rand() % ( max - min ) );
	}
	xTaskResumeAll();

	return r;
}

/**
 * strtok_r emulation to be safe thrade compatible with rtos, this is not a full implementation
 * the function does not make a parameter validation. To keep tokenisation over the same string
 * is mandatory to call the function with a NULL parameter
 * str: string to tokenaize
 * delim: character to use as token, this is not a string is a single character
 * save: pointer to save where the string is being tokenise
 * retrun the adreess where the chunk beging, NULL is no token is found 
*/
char *xStrTok( char *str, const char delim, char** save )
{
    char *ptr;
    
    /*if this is not the first time, use save pointer*/
    if( str == NULL )
    {
        str = *save;
    }
    else
    {
        /*if this is the first time save pointer should be null*/
        *save = NULL;
    }
    
    /*look for the character in the string*/
    ptr = strchr( str, delim );
    if( ptr != NULL )
    {
        /*found a character*/
        *ptr = '\0';
        *save = ++ptr;
    }
    else if( *save == NULL )
    {
        /*if not probably the character is not in the entire string*/
        str = NULL;
    }
    
    return str;
}
