/*HEADER******************************************************************************************
* File name: DataLogger.c
* Date: Jan 1, 2016
* Author: Carlos Neri
*
**END********************************************************************************************/
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Includes Section
///////////////////////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "DataLogger.h"
#include "fsl_gpio.h"
#include "fsl_sysmpu.h"
#include "diskio.h"
#include "ff.h"
#include "Rtc.h"
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

static void DataLogger_SystemConfigure(void);

static bool DataLogger_IsCardPresent(void);

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

static int8_t LogMessage[DATALOGGER_MAX_LOG_MESSAGE];
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////

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


static void DataLogger_SystemConfigure(void)
{
    gpio_pin_config_t PinDetect =
    {
        kGPIO_DigitalInput, 0,
    };

	/* Initialize the SDCard detect pin */
	/* The data logger assumes */
	/* the SDCard is present at boot and */
	/* does not support hot plug */
	GPIO_PinInit(DATALOGGER_PIN_DETECT_GPIO, DATALOGGER_PIN_DETECT, &PinDetect);

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
