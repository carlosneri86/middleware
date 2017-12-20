/*HEADER******************************************************************************************
* File name: DataLogger.c
* Date: Jan 1, 2016
* Author: B22385
*
**END********************************************************************************************/
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Includes Section
///////////////////////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#ifdef FSL_RTOS_FREE_RTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#endif

#include "DataLogger.h"
#include "fsl_gpio.h"
#include "fsl_sysmpu.h"
#include "diskio.h"
#include "ff.h"
#include "fsl_host.h"
#include "Rtc.h"
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef FSL_RTOS_FREE_RTOS
#define DATA_LOGGER_STACK_SIZE			(2048)

#define DATA_LOGGER_TASK_PRIORITY		1

#define DATA_LOGGER_MAX_LOG				5
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef FSL_RTOS_FREE_RTOS
typedef enum
{
	DATA_LOGGER_POST_EVENT = 0,
	DATA_LOGGER_MOUNT_EVENT,
	DATA_LOGGER_UNMOUNT_EVENT
}datalogger_events_t;

typedef struct
{
	datalogger_events_t Event;
	uint8_t LogMessage[100];
	uint16_t LogMessageSize;
	uint8_t LogData[100];
	uint16_t LogDataSize;
}datalogger_event_t;
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

static void DataLogger_SystemConfigure(void);

static void DataLogger_MountFs(void);

static void DataLogger_UnMountFs(void);

extern void CardInsertDetectHandle(void);

#ifdef FSL_RTOS_FREE_RTOS

void Datalogger_Task (void * param);

void static DataLogger_WriteMessage(datalogger_event_t * EventPost);

#endif
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////
static const uint8_t DataLogger_EndOfLine[] =
{
		"\r\n"
};
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////
static rtc_date_t RtcDate;

static FATFS FileSystemHandler;

#ifdef FSL_RTOS_FREE_RTOS
static xQueueHandle DataLoggerMessageQueue;
#endif

static bool isCardPresent = false;

static int8_t LogMessage[DATALOGGER_MAX_LOG_MESSAGE];
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef FSL_RTOS_FREE_RTOS
uint32_t DataLogger_Init(void)
{
	FRESULT FileSystemStatus;
	uint32_t DataLoggerInitStatus = DATA_LOGGER_ERROR;

	DataLogger_SystemConfigure();

	if(!DataLogger_IsCardPresent())
	{
		/* Wait for SD Card insertion */
		while (!DataLogger_IsCardPresent());
	}

	FileSystemStatus = disk_initialize(SDDISK);

	if(FileSystemStatus == FR_OK)
	{
		/* now that the card is there, mount the file system */
		FileSystemStatus = f_mount(&FileSystemHandler,"2:/",0);

		if(FileSystemStatus == FR_OK)
		{
			FileSystemStatus = f_chdrive("2:/");

			if(FileSystemStatus == FR_OK)
			{
				DataLoggerInitStatus = DATA_LOGGER_OK;
			}
		}
	}
	return(DataLoggerInitStatus);
}
#else

uint32_t DataLogger_Init(void)
{
	uint32_t DataLoggerInitStatus = DATA_LOGGER_OK;

	DataLogger_SystemConfigure();

	/* create the queue for app messages */
	DataLoggerMessageQueue = xQueueCreate(DATA_LOGGER_MAX_LOG, sizeof(datalogger_event_t));

	/* create task and the queue to send the data */
	xTaskCreate((TaskFunction_t) Datalogger_Task, (const char*) "Datalogger_task", DATA_LOGGER_STACK_SIZE,\
						NULL,DATA_LOGGER_TASK_PRIORITY,NULL);

	return (DataLoggerInitStatus);
}


#endif


#ifndef FSL_RTOS_FREE_RTOS
uint32_t DataLogger_PostEvent(uint8_t * pLogMessage, uint8_t *pLogData, uint16_t LogDataSize)
{
	uint32_t	BytesWritten;
	FIL			LogFile;
	uint32_t	FileNewLine;
	FRESULT 	FileSystemStatus;
	uint32_t	MessageSize;
	uint16_t	MessageOffset = 0;
	uint32_t	PostEventStatus = DATA_LOGGER_ERROR;

	/* TODO: change strcat and sprintf with custom ones */

	/* check if the card still there*/
	if(DataLogger_IsCardPresent())
	{
		/* Open the file, move to the last line and log the new entry*/
		FileSystemStatus = f_open(&LogFile, DATALOGGER_FILE_NAME, FA_WRITE | FA_OPEN_ALWAYS);

		if(FileSystemStatus == FR_OK)
		{
			FileNewLine = f_size(&LogFile);

			/* Set file pointer to the start of new line in text file */
			FileSystemStatus = f_lseek(&LogFile, FileNewLine);

			if(FileSystemStatus == FR_OK)
			{
				/* get the current time stamp and fill the buffer*/
				Rtc_GetCurrentDate(&RtcDate);

				memset(&LogMessage[0],0,DATALOGGER_MAX_LOG_MESSAGE);

				sprintf((char*)&LogMessage[0],"%.2d-%.2d-%d, %.2d:%.2d:%.2d, ",RtcDate.Day,\
						RtcDate.Month,RtcDate.Year,RtcDate.Hour,\
						RtcDate.Minutes,RtcDate.Seconds);

				/* Add the application message if available*/
				if(pLogMessage != NULL)
				{
					strcat((char*)&LogMessage[0], (const char*)pLogMessage);
					/* Calculate the current string size*/
					MessageSize = strlen((const char *)&LogMessage[0]);
					LogMessage[MessageSize] = ',';
					MessageSize += 1;
				}
				/* Add application data if available */
				if(LogDataSize)
				{

					/* Parse the data and add it to the message buffer */
					while(LogDataSize--)
					{
						sprintf((char*)&LogMessage[MessageSize],"%d,",pLogData[MessageOffset]);
						MessageSize = strlen((const char *)&LogMessage[0]);
						MessageOffset++;
					}
				}
				/* Set the end of line */
				strcat((char*)&LogMessage[0],(const char *)&DataLogger_EndOfLine[0]);
				MessageSize = strlen((const char *)&LogMessage[0]);
				/* Write the file */
				FileSystemStatus = f_write(&LogFile,&LogMessage[0], MessageSize, &BytesWritten);

				/* Close the log file */
				FileSystemStatus = f_close(&LogFile);

				PostEventStatus = DATA_LOGGER_OK;
			}
		}
	}
	return(PostEventStatus);
}
#else


uint32_t DataLogger_PostEvent(uint8_t * pLogMessage, uint8_t *pLogData, uint16_t LogDataSize)
{
	datalogger_event_t MessageToPost;
	uint32_t	PostEventStatus = DATA_LOGGER_ERROR;

	if(isCardPresent)
	{
		MessageToPost.LogMessageSize = strlen((char*)pLogMessage);

		strncpy((char*)&MessageToPost.LogMessage[0],(char*)pLogMessage,MessageToPost.LogMessageSize);


		memcpy(&MessageToPost.LogData, pLogData, LogDataSize);

		MessageToPost.Event = DATA_LOGGER_POST_EVENT;

		xQueueSend(DataLoggerMessageQueue, &MessageToPost, 0);

		PostEventStatus = DATA_LOGGER_OK;
	}

	return (PostEventStatus);
}

void Datalogger_Task (void * param)
{
	datalogger_event_t MessageToPost;


	while(1)
	{
		xQueueReceive(DataLoggerMessageQueue, &MessageToPost, portMAX_DELAY);


		switch(MessageToPost.Event)
		{
			case DATA_LOGGER_POST_EVENT:
			{
				DataLogger_WriteMessage(&MessageToPost);
			}
			break;

			case DATA_LOGGER_MOUNT_EVENT:
			{
				DataLogger_MountFs();
			}
			break;

			case DATA_LOGGER_UNMOUNT_EVENT:
			{
				DataLogger_UnMountFs();
			}
			break;

			default:

			break;
		}
	}

}

void static DataLogger_WriteMessage(datalogger_event_t * EventPost)
{
	uint32_t	BytesWritten;
	FIL			LogFile;
	uint32_t	FileNewLine;
	FRESULT 	FileSystemStatus;
	uint32_t	MessageSize;
	uint16_t	MessageOffset = 0;
	uint16_t    LogSize;

	if(isCardPresent)
	{
		/* Open the file, move to the last line and log the new entry*/
		FileSystemStatus = f_open(&LogFile, DATALOGGER_FILE_NAME, FA_WRITE | FA_OPEN_ALWAYS);

		if(FileSystemStatus == FR_OK)
		{
			FileNewLine = f_size(&LogFile);

			/* Set file pointer to the start of new line in text file */
			FileSystemStatus = f_lseek(&LogFile, FileNewLine);

			if(FileSystemStatus == FR_OK)
			{
				/* get the current time stamp and fill the buffer*/
				Rtc_GetCurrentDate(&RtcDate);

				memset(&LogMessage[0],0,DATALOGGER_MAX_LOG_MESSAGE);

				sprintf((char*)&LogMessage[0],"%.2d-%.2d-%d, %.2d:%.2d:%.2d, ",RtcDate.Day,\
						RtcDate.Month,RtcDate.Year,RtcDate.Hour,\
						RtcDate.Minutes,RtcDate.Seconds);

				/* Add the application message if available*/
				if(EventPost->LogMessageSize > 0)
				{
					strcat((char*)&LogMessage[0], (const char*)&EventPost->LogMessage[0]);
					/* Calculate the current string size*/
					MessageSize = EventPost->LogMessageSize;
					LogMessage[MessageSize] = ',';
					MessageSize += 1;
				}
				/* Add application data if available */
				if(EventPost->LogDataSize)
				{
					LogSize = EventPost->LogDataSize;
					/* Parse the data and add it to the message buffer */
					while(LogSize--)
					{
						sprintf((char*)&LogMessage[MessageSize],"%d,",EventPost->LogData[MessageOffset]);
						MessageSize = strlen((const char *)&LogMessage[0]);
						MessageOffset++;
					}
				}
				/* Set the end of line */
				strcat((char*)&LogMessage[0],(const char *)&DataLogger_EndOfLine[0]);
				MessageSize = strlen((const char *)&LogMessage[0]);
				/* Write the file */
				FileSystemStatus = f_write(&LogFile,&LogMessage[0], MessageSize, &BytesWritten);

				/* Close the log file */
				FileSystemStatus = f_close(&LogFile);
			}
		}
	}
}
#endif

static void DataLogger_MountFs(void)
{
	FRESULT FileSystemStatus;

	FileSystemStatus = disk_initialize(SDDISK);

	if(FileSystemStatus == FR_OK)
	{
		/* now that the card is there, mount the file system */
		FileSystemStatus = f_mount(&FileSystemHandler,"2:/",0);

		if(FileSystemStatus == FR_OK)
		{
			FileSystemStatus = f_chdrive("2:/");
			isCardPresent = true;
		}
	}
}

static void DataLogger_UnMountFs(void)
{
	f_mount(NULL,"2:/",0);
	isCardPresent = false;
}

static void DataLogger_SystemConfigure(void)
{
	/* required by the SDHC driver */
	SYSMPU_Enable(SYSMPU, false);

    /* Set a start date time and start RTC */
	RtcDate.Year = 2017U;
	RtcDate.Month = 12U;
	RtcDate.Day = 10U;
	RtcDate.Hour = 11U;
	RtcDate.Minutes = 10U;
	RtcDate.Seconds = 0U;

    Rtc_Init(&RtcDate);
}

static bool DataLogger_IsCardPresent(void)
{
	uint32_t SDCardDetectPin;
	bool IsPresent = true;

	SDCardDetectPin = GPIO_ReadPinInput(DATALOGGER_PIN_DETECT_GPIO,DATALOGGER_PIN_DETECT);

	if(!SDCardDetectPin)
	{
		IsPresent = false;
	}

	return (IsPresent);
}

void PORTE_DriverIRQHandler(void)
{
	bool isCardPresent;
#ifdef FSL_RTOS_FREE_RTOS
	datalogger_event_t MessageToPost;
	BaseType_t HigherPriorityTaskWoken = pdFALSE;;
#endif
	CardInsertDetectHandle();

	isCardPresent = DataLogger_IsCardPresent();

	if(isCardPresent)
	{
		#ifdef FSL_RTOS_FREE_RTOS
		MessageToPost.Event = DATA_LOGGER_MOUNT_EVENT;
		#endif
	}
	else
	{
		#ifdef FSL_RTOS_FREE_RTOS
		MessageToPost.Event = DATA_LOGGER_UNMOUNT_EVENT;
		#endif
	}

	#ifdef FSL_RTOS_FREE_RTOS
	xQueueSendFromISR(DataLoggerMessageQueue, &MessageToPost, &HigherPriorityTaskWoken);
	#endif
}
