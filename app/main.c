/** Template RTOS V2.0 */
#include "stm32f0xx.h"
#include "app_utils.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stdio.h"
#include "time.h"
#include "string.h"
#include "timers.h"

#define MAX_CMD_SIZE    (30u)
#define MAX_SERIALMSG_LEN (50u) 
#define APPMAJOR (1u)
#define APPMINOR (1u)
#define TPOLL_RATE_MS           (20u)
#define TSINGLE_2_LONG_MS       (400u)  /*Time to consider long click*/
#define TSINGLE_2_DOUBLE_MS     (160u)  /*Time to wait for 2nd click after first*/

static void HBTask(void *pvParameters);
static void TimeDispTask(void *pvParameters);
static void CmdProcessorTask(void *pvParameters);
static void TaskButton( void *parameters );
static void TaskSerial( void *parameters );
static void TmrDisableAlarmCb( TimerHandle_t pxTimer );   /*Timer callback*/

typedef enum
{
    B_IDLE,
    B_SINGLE,
    B_LONG,
    B_WAITDOUBLE,
    B_WAITDEPRESS,
    B_WAITDEPRESSFLONG
} t_btnstate;

/*Support functions*/
static void cbSingleClick(void);
static void cbDoubleClick(void);
static void cbLongClick(void);
static void cbLongRelease(void);
static void getdatetime(struct tm* info);
static BaseType_t ValidateTime(char *data, uint8_t *hour, uint8_t *min, uint8_t *sec);
static BaseType_t ValidateDate(char *data, uint16_t *year, uint8_t *month, uint8_t *day);

/*Handles*/
TimerHandle_t TmrDisableAlarm;
TaskHandle_t TDHandle;
QueueHandle_t CmdQHandle;
QueueHandle_t serQueue;

/*Global Vars*/
static uint8_t bSetTime=0;
static uint8_t bSetDate=0;
static uint8_t bSetAlarm=0;
static uint8_t AlarmActive=0;
static uint8_t BtnHeld=0;
static uint16_t HBRate=200;

int main( void )
{

    vSetupHardware(); /*inicia la opcion de semihosting*/

    SEGGER_RTT_printf(0, "Hola, soy tu amigo el reloj\npor favor configura una\nhora y fecha para empezar\na funcionar\r\n");
    /*Crear Tarea Heartbeat*/
    xTaskCreate(HBTask,             "HBTask",       128, NULL, 1,NULL);        
    /*Tarea del Time Display*/
    xTaskCreate(TimeDispTask,       "TimeDispTask", 200, NULL, 2,&TDHandle);   
    xTaskCreate(TaskButton,         "TaskButton",   128, NULL, 2u, NULL);        
    xTaskCreate(CmdProcessorTask,   "CmdProcTask",  300, NULL, 1,NULL); 
    /*Task to handle Serial Print*/    
    xTaskCreate(TaskSerial,         "TaskSerial",   200u, NULL, 1u, NULL );   
    /*Timer to disable Alarm*/
    TmrDisableAlarm = xTimerCreate("DisAlarm",60000/portTICK_PERIOD_MS,pdFALSE,NULL,TmrDisableAlarmCb);
    /*Command Queue*/
    CmdQHandle = xQueueCreate(2,MAX_CMD_SIZE);
    /*Serial Queue*/
    serQueue = xQueueCreate(1,MAX_SERIALMSG_LEN);

    xQueueSend(serQueue,"AT>\r\n",10);    

    vTaskStartScheduler(); /*ejecutamos el kernel*/
}

static void HBTask( void *pvParameters )
{
    for(;;)
    {        
        vTaskDelay( HBRate/portTICK_PERIOD_MS ); /*retardo aleatorio*/
        vLedToggle();
    }
}

static void TimeDispTask( void *pvParameters )
{

    struct tm info;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    char datetimestr[21]="DUMMYDATE";    
    for(;;)
    {        
    
        if(AlarmActive==1)
        {
            SEGGER_RTT_printf(0,"ALARM!!!\r\n");
        }
        else if(BtnHeld==1)
        {
            if(bSetAlarm==1)
            {
                /*Read alarm time*/
                vGetAlarm( &hour, &min, &sec );
                SEGGER_RTT_printf(0, "A: %02d:%02d:%02d\r\n",hour,min,sec);
            }
            else
            {
                SEGGER_RTT_printf(0,"NO ALARM SET\r\n");
            }
        }
        else
        {
            /*Enable Time Display Task*/
            if((bSetDate==1) && (bSetTime==1))
            {
                /*Button not being held, display regular date time*/
                getdatetime(&info);
                strftime(datetimestr,21,"%H:%M:%S %d/%b/%Y", &info);
                /*Print time and date in the specified format*/
                
                SEGGER_RTT_printf(0,"%s %c\r\n", datetimestr,(bSetAlarm?'A':' '));
            }            
        }

        vTaskDelay( 1000/portTICK_PERIOD_MS ); /*retardo aleatorio*/        
    }
}


static void getdatetime(struct tm* info)
{    
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t day;
    uint8_t mon;
    uint8_t year;
    /*Read current time and date and fill into the parameter passed structure*/
    vGetTime( &hour, &min, &sec);
    vGetDate( &day, &mon, &year);    
    info->tm_hour = hour;
    info->tm_min = min;
    info->tm_sec = sec;
    /*Fix the year*/
    info->tm_year = 100u + year;
    info->tm_mday = day;
    info->tm_mon = mon;
}

static void TmrDisableAlarmCb( TimerHandle_t pxTimer )   /*Timer callback*/
{
    AlarmActive=0;
}

/**------------------------------------------------------------------------------------------------
Brief.- general puorpse ISR vector, in reality this is the TSC interrupt vector but for didacting 
        pourpose is renamed
-------------------------------------------------------------------------------------------------*/
void vInterruptHandler( void )
{

}

/**------------------------------------------------------------------------------------------------
Brief.- general pourpse ISR callback, this is not an ISR vector, is just the callback placed in the
        real vector which is the HAL_GPIO_EXTI_IRQHandler. it is renamed for didactic pourpose
-------------------------------------------------------------------------------------------------*/
void vInterruptButton( uint16_t GPIO_Pin )
{

}

/**------------------------------------------------------------------------------------------------
Brief.- Clock alarm ISR callback, this is not an ISR vector, is just the callback placed in the
        real vector which is the RTC_IRQHandler. it is renamed for didactic pourpose
        This fucntion is called everytime the clock time match the alarm value
-------------------------------------------------------------------------------------------------*/
void vInterruptAlarm( RTC_HandleTypeDef *hrtc )
{
    BaseType_t HPTWoken = pdFALSE;    
    bSetAlarm=0;
    vUnsetAlarm();  /*Clear alarm from the clock*/
    AlarmActive=1;  /*Set alarm as active*/
    (void)xTimerStartFromISR(TmrDisableAlarm,&HPTWoken);
    portEND_SWITCHING_ISR(HPTWoken);  /*Switch context if xtaskWoken=pdTrue*/
}

/**------------------------------------------------------------------------------------------------
Brief.- Uart reception ISR callback, this is not an ISR vector, is just the callback placed in the
        real vector which is the RTC_IRQHandler. it is renamed for didactic pourpose
        This function is called every time a single byte is received.
        To access the received byte just call the function xSerialReceiveFromISR()
-------------------------------------------------------------------------------------------------*/
void vInterruptSerialRx( UART_HandleTypeDef *huart )
{
    static char buf[MAX_CMD_SIZE];
    static uint8_t msgidx=0;
    char rx;    
    BaseType_t HPTWoken = pdFALSE;
    rx = xSerialReadFromISR();
    buf[msgidx++] = rx;
    if((rx=='\r') || (msgidx>(MAX_CMD_SIZE-1)))
    {
        /*Ensure the buffer has null terminator*/
        buf[msgidx-1]='\0';
        /*Full command has arrived invoke the processor*/
        xQueueSendFromISR(CmdQHandle,buf,&HPTWoken);
        msgidx=0;                
    }   
    portEND_SWITCHING_ISR(HPTWoken);  /*Switch context if xtaskWoken=pdTrue*/     
    intCLEAR_INTERRUPT();   /*Limpiar bandera de interrupcion*/
}


static void CmdProcessorTask( void *pvParameters )
{

    struct tm info;
    char buffer[MAX_CMD_SIZE];
    char *str;
    char *save;
    char datetimestr[MAX_SERIALMSG_LEN];
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint16_t lHBRate=0;

    for(;;)
    {   
        /*Wait for a command to arrive*/
        if(xQueueReceive(CmdQHandle,buffer,portMAX_DELAY)==pdTRUE)
        {
            SEGGER_RTT_printf(0, "CmdRec: %s\r\n",buffer);
            /*Check if the command has the pattern CMD=xxx*/
            str = xStrTok(buffer,'=',&save);
            if(str!=NULL)
            {
                /*Command found*/
                if(strncmp(str,"TIME",4u)==0)
                {            
                    /*TIME=hh,mm,ss\r Command*/
                    /*retrieve the data part*/
                    str = xStrTok(NULL,'=',&save);
                    if(ValidateTime(str,&hour,&min,&sec)==pdTRUE)
                    {
                        /*Set time*/
                        vSetTime( hour, min, sec );
                        bSetTime=1;                        
                        xQueueSend(serQueue,"OK\r\n",10);
                    }
                    else
                    {
                        /*No valid command*/                        
                        xQueueSend(serQueue,"ERROR (TIME=hh,mm,ss)\r\n",10);
                    }
                }
                else if(strncmp(str,"DATE",4u)==0)
                {
                    /*Date command*/
                    /*retrieve the data part*/
                    str = xStrTok(NULL,'=',&save);
                    if(ValidateDate(str,&year,&month,&day)==pdTRUE)
                    {
                        /*Set date*/
                        vSetDate( day, month, (uint8_t)(year % 2000) );                        
                        bSetDate=1;
                        xQueueSend(serQueue,"OK\r\n",10);
                    }
                    else
                    {
                        /*No valid command*/
                        xQueueSend(serQueue,"ERROR (DATE=dd,mm,yyyy)\r\n",10);          
                    }
                }
                else if(strncmp(str,"ALARM",5u)==0)
                {
                    /*Time command*/
                    /*retrieve the data part*/
                    str = xStrTok(NULL,'=',&save);
                    if(ValidateTime(str,&hour,&min,&sec)==pdTRUE)
                    {
                        /*Set Alarm Time*/
                        vSetAlarm( hour, min, sec );
                        bSetAlarm=1;
                        xQueueSend(serQueue,"OK\r\n",10);
                    }
                    else
                    {
                        /*No valid command*/
                        xQueueSend(serQueue,"ERROR (ALARM=hh,mm,ss)\r\n",10);            
                    }
                }
                else if(strncmp(str,"HEARTBEAT",9u)==0)
                {
                    /*Heartbeat Rate command*/
                    /*retrieve the data part*/
                    str = xStrTok(NULL,'=',&save);
                    lHBRate = atoi(str);
                    if(lHBRate>=10 && HBRate<=5000)
                    {
                        HBRate=lHBRate;
                        xQueueSend(serQueue,"OK\r\n",10);
                    }
                    else
                    {                        
                        xQueueSend(serQueue,"ERROR (RANGE:10-5000\r\n",10);
                    }
                }
                else
                {
                    /*No valid command*/
                    xQueueSend(serQueue,"ERROR (INVALID CMD)\r\n",10);       
                }
            }
            else
            {
                if(strncmp(buffer,"INFO",4u)==0)
                {
                    /*Info command*/
                    /*Printf Info to Serial*/
                    xQueueSend(serQueue,"OK\r\n",10);
                    xQueueSend(serQueue,"Francisco's Clock. Bla Bla inc.\r\n",10);
                    sprintf(datetimestr, "VER:%02d.%02d\r\n",APPMAJOR,APPMINOR);
                    xQueueSend(serQueue,datetimestr,10);
                    xQueueSend(serQueue, "MACHINE: STM32F072RB\r\n",10);
                    vGetTime( &hour, &min, &sec);
                    sprintf(datetimestr, "TIME: %02d:%02d:%02d\r\n",hour,min,sec);
                    xQueueSend(serQueue,datetimestr,10);
                    /*Get the Actual Date*/
                    getdatetime(&info);
                    strftime(datetimestr,sizeof(datetimestr),"%d/%b/%Y", &info);                    
                    xQueueSend(serQueue,"DATE: ",10);
                    xQueueSend(serQueue,datetimestr,10);
                    xQueueSend(serQueue,"\r\n",10);
                    if(bSetAlarm)
                    {
                        vGetAlarm( &hour, &min, &sec );
                        sprintf(datetimestr, "ALARM: %02d:%02d:%02d\r\n",hour,min,sec);
                        xQueueSend(serQueue,datetimestr,10);
                    }
                    sprintf(datetimestr, "BLINK RATE: %d\r\n",HBRate);
                    xQueueSend(serQueue,datetimestr,10);
                }
                else
                {
                    /*No valid command*/
                    xQueueSend(serQueue,"ERROR (INVALID CMD)\r\n",10);       
                }
            }
        }
    }
}


static BaseType_t ValidateTime(char *data, uint8_t *hour, uint8_t *min, uint8_t *sec)
{
    BaseType_t rv=pdFALSE;

    char *save;
    /*Parse Hours*/
    if(data!=NULL)
    {
        data = xStrTok(data,',',&save);
        if(data!=NULL)
        {
            *hour=atoi(data);
            if(*hour>=0 && *hour<=23)
            {
                /*Parse minutes*/
                data = xStrTok(NULL,',',&save);
                if(data!=NULL)
                {
                    *min=atoi(data);
                    if(*min>=0 && *min<=59)
                    {
                        /*Parse seconds*/
                        data = xStrTok(NULL,',',&save);
                        if(data!=NULL)
                        {
                            *sec=atoi(data);
                            if(*sec>=0 && *sec<=59)
                            {
                                rv=pdTRUE;
                            }        
                        }
                    }            
                }
            }
        }
    }
    return rv;
}

static BaseType_t ValidateDate(char *data, uint16_t *year, uint8_t *month, uint8_t *day)
{
    BaseType_t rv=pdFALSE;
    char *save;
    /*Parse Hours*/
    data = xStrTok(data,',',&save);
    if(data!=NULL)
    {
        if(data!=NULL)
        {
            *day=atoi(data);
            if(*day>=1 && *day<=31)
            {
                /*Parse minutes*/
                data = xStrTok(NULL,',',&save);
                if(data!=NULL)
                {
                    *month=atoi(data);
                    if(*month>=1 && *month<=12)
                    {
                        /*Parse seconds*/
                        data = xStrTok(NULL,',',&save);
                        if(data!=NULL)
                        {
                            *year=atoi(data);
                            if(*year>=2000 && *year<=2050)
                            {
                                rv=pdTRUE;
                            }        
                        }
                    }            
                }
            }
        }
    }
    return rv;
}

static void TaskButton( void *parameters )
{
    t_btnstate bstate=B_IDLE;
    uint8_t bpressed=0;
    uint16_t stimer=0;
    for( ; ; )
    {
        /*Determine if button is pressed*/
        bpressed = (xReadButton()==0);
        switch(bstate)
        {
            case B_IDLE:
            {
                if(bpressed)
                {
                    bstate=B_SINGLE;
                    stimer=0;
                }
            }
            break;
            case B_SINGLE:
            {
                if(!bpressed)
                {
                    /*Button has been released go wait for the 2nd click*/
                    bstate=B_WAITDOUBLE;
                    stimer=0;
                }
                else
                {
                    if(stimer>=TSINGLE_2_LONG_MS)
                    {
                        /*Long Click Detected*/
                        cbLongClick();
                        bstate=B_WAITDEPRESSFLONG;
                    }
                }
            }
            break;
            case B_WAITDOUBLE:
            {
                if(bpressed)
                {
                    /*Double Click Detected*/
                    cbDoubleClick();
                    bstate=B_WAITDEPRESS;
                }
                else
                {
                    if(stimer>=TSINGLE_2_DOUBLE_MS)
                    {
                        /*Time expired and only Single Click Detected*/
                        cbSingleClick();
                        bstate=B_WAITDEPRESS;
                    }
                }
            }
            break;
            case B_WAITDEPRESSFLONG:
            {
                if(!bpressed)
                {                    
                    /*LONG RELEASE DETECTED*/
                    cbLongRelease();
                    bstate=B_IDLE;
                }
            }
            break;
            case B_WAITDEPRESS:
            {
                if(!bpressed)
                {                    
                    bstate=B_IDLE;
                }
            }
            break;
            default:
            break;
        }
        stimer += TPOLL_RATE_MS;
        vTaskDelay( TPOLL_RATE_MS );        
    }
}
    
static void cbSingleClick(void)
{
    /*if the alarm is active disable it*/
    xTimerStop(TmrDisableAlarm,portMAX_DELAY);
    AlarmActive=0;
}
static void cbDoubleClick(void)
{
    /*if the alarm is active disable it*/
    xTimerStop(TmrDisableAlarm,portMAX_DELAY);
    AlarmActive=0;
    /*If the alarm is set, clear it*/
    if(bSetAlarm==1)
    {
        SEGGER_RTT_printf(0, "ALARM OFF\r\n");
        bSetAlarm=0;
        vUnsetAlarm();
    }
}
static void cbLongClick(void)
{
    /*if the alarm is active disable it*/
    xTimerStop(TmrDisableAlarm,portMAX_DELAY);
    AlarmActive=0;
    BtnHeld=1;
}
static void cbLongRelease(void)
{
    BtnHeld=0;
}

/*This task only prints and goes to suspended again until next button event*/
static void TaskSerial( void *parameters )
{
    char serialmsg[MAX_SERIALMSG_LEN];   
    uint32_t msglen=0; 
    for( ; ; )
    {

        if(xQueueReceive(serQueue,&serialmsg,portMAX_DELAY)==pdTRUE)
        {
            msglen = strlen(serialmsg);
            //SEGGER_RTT_printf(0, serialmsg);
            vSerialSend((uint8_t *)serialmsg,msglen);
        }        
    }
}