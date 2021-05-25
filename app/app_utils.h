#ifndef _UTILS_H_
#define _UTILS_H_

#define BTN_INT_RISING          GPIO_MODE_IT_RISING
#define BTN_INT_FALLING         GPIO_MODE_IT_FALLING
#define BTN_INT_BOTH            GPIO_MODE_IT_RISING_FALLING

#define vPrintf                         SEGGER_SYSVIEW_PrintfHost
#define intENABLE_INTERRUPT()           HAL_NVIC_SetPriority(TSC_IRQn,2,0); HAL_NVIC_EnableIRQ(TSC_IRQn)
#define intTRIGGER_INTERRUPT()          HAL_NVIC_SetPendingIRQ(TSC_IRQn)
#define intCLEAR_INTERRUPT()            HAL_NVIC_ClearPendingIRQ(TSC_IRQn)
#define vInterruptHandler               TSC_IRQHandler
#define vInterruptButton                HAL_GPIO_EXTI_Callback
#define vInterruptAlarm                 HAL_RTC_AlarmAEventCallback
#define vInterruptSerialRx              HAL_UART_RxCpltCallback
#define vDelay                          HAL_Delay

extern void vSetupHardware( void );
extern void vPrintString( char *string );
extern void vPrintStringAndNumber( char *string, int num );
extern void vSerialSend( uint8_t *buffer, uint32_t lenght );
extern uint8_t xSerialReadFromISR( void );
extern void vLedOn( void );
extern void vLedOff( void );
extern void vLedToggle( void );
extern uint8_t xReadButton( void );
extern void vSetIntButton( uint32_t intmode );
extern void vSetTime( uint8_t hh, uint8_t mm, uint8_t ss );
extern void vGetTime( uint8_t *hh, uint8_t *mm, uint8_t *ss );
extern void vSetDate( uint8_t dd, uint8_t mm, uint8_t yy );
extern void vGetDate( uint8_t *dd, uint8_t *mm, uint8_t *yy );
extern void vSetAlarm( uint8_t hh, uint8_t mm, uint8_t ss );
extern void vGetAlarm( uint8_t *hh, uint8_t *mm, uint8_t *ss );
extern void vUnsetAlarm(void);
extern void vEnableWatchDog( void );
extern void vWatchdogReset( void );
extern char* xItoa( int num, char* str, int base );
extern uint32_t ulRandomNumber( uint32_t min, uint32_t max );
extern char *xStrTok( char *str, const char delim, char** save );
#endif
