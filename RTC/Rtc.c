/*HEADER******************************************************************************************
BSD 3-Clause License

Copyright (c) 2020, Carlos Neri
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**END********************************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Includes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include "MiscFunctions.h"
#include "Rtc.h"
#include "fsl_rtc.h"
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#define RTC_SECONDS_IN_A_DAY (86400U)

#define RTC_SECONDS_IN_A_HOUR (3600U)

#define RTC_SECONDS_IN_A_MINUTE (60U)

#define RTC_DAYS_IN_A_YEAR (365U)

#define RTC_YEAR_RANGE_START (1970U)

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	uint32_t AlarmSeconds;
	rtc_alarm_callback_t Callback;
	void * Args;
}rtc_alarm_t;

typedef struct
{
	rtc_alarm_t * Alarm;
	uint8_t Offset;
}rtc_active_alarm_t;
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

static void Rtc_ServeAlarms(void);

static void Rtc_SelectAndSetAlarm(void);

static rtc_status_t Rtc_SetAlarm(uint32_t AlarmInSeconds, uint8_t * AlarmHandle, rtc_alarm_callback_t AlarmCallback, void * CallbackArgs);

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////

static bool isRtcConfigured = false;

static rtc_alarm_t RtcAlarms[RTC_SUPPORTED_ALARMS];

static rtc_active_alarm_t ActiveAlarm =
{
		.Alarm = NULL,
		.Offset = 0xFF,
};

static uint16_t AvailableAlarms = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////
rtc_status_t Rtc_Init(rtc_date_t * RTcInitDate)
{
	rtc_config_t RtcConfig;
	rtc_status_t Status = RTC_WRONG_PARAMETER;
	rtc_datetime_t InitialDate;
	uint16_t AlarmsOffset = 0;

	if(isRtcConfigured == false)
	{
		RTC_GetDefaultConfig(&RtcConfig);
		RTC_Init(RTC, &RtcConfig);

		RTC_ClearStatusFlags(RTC, kRTC_AlarmInterruptEnable);
		RTC_ClearStatusFlags(RTC, kRTC_TimeInvalidFlag);

		if(RTcInitDate != NULL)
		{
			InitialDate.year = RTcInitDate->Year;
			InitialDate.month = RTcInitDate->Month;
			InitialDate.day = RTcInitDate->Day;
			InitialDate.hour = RTcInitDate->Hour;
			InitialDate.minute = RTcInitDate->Minutes;
			InitialDate.second = RTcInitDate->Seconds;

			RTC_SetDatetime(RTC, &InitialDate);
		}

		RTC_StartTimer(RTC);

		RTC_EnableInterrupts(RTC, kRTC_AlarmInterruptEnable);

		EnableIRQ(RTC_IRQn);

		Status = RTC_OK;

		isRtcConfigured = true;

		while(AlarmsOffset < RTC_SUPPORTED_ALARMS)
		{
			RtcAlarms[AlarmsOffset].AlarmSeconds = 0;
			RtcAlarms[AlarmsOffset].Callback = NULL;
			RtcAlarms[AlarmsOffset].Args = NULL;
			AlarmsOffset++;
		}
	}

	return Status;
}

rtc_status_t Rtc_GetCurrentDate(rtc_date_t * RtcDate)
{
	rtc_status_t Status = RTC_WRONG_PARAMETER;
	rtc_datetime_t CurrentDate;

	if(RtcDate != NULL)
	{
		RTC_GetDatetime(RTC,&CurrentDate);

		RtcDate->Year		=	CurrentDate.year;
		RtcDate->Month		=	CurrentDate.month;
		RtcDate->Day		=	CurrentDate.day ;
		RtcDate->Hour		=	CurrentDate.hour;
		RtcDate->Minutes	=	CurrentDate.minute;
		RtcDate->Seconds	=	CurrentDate.second;
		RtcDate->Weekday = Rtc_DateToWeekDay(RtcDate);

		Status = RTC_OK;
	}

	return Status;
}

void Rtc_SetCurrentSecondsCounter(uint32_t CurrentSeconds)
{
	RTC_StopTimer(RTC);
	RTC->TSR = CurrentSeconds;
	RTC_StartTimer(RTC);
}

uint32_t Rtc_GetCurrentSecondsCounter(void)
{
	uint32_t RtcRead;

	/* RM recommends two consecutive reads */
	RtcRead = RTC->TSR;
	RtcRead = RTC->TSR;

	return RtcRead;
}

rtc_status_t Rtc_SetAlarmByDate(rtc_date_t * AlarmDate, uint8_t * AlarmHandle, rtc_alarm_callback_t AlarmCallback, void * AlarmCallbackArgs)
{
	rtc_datetime_t AlarmDateTime;
	rtc_status_t Status = RTC_WRONG_PARAMETER;
	uint32_t AlarmSeconds;

	if((AlarmDate != NULL) && (AlarmCallback != NULL))
	{

		AlarmDateTime.year = AlarmDate->Year;
		AlarmDateTime.month = AlarmDate->Month;
		AlarmDateTime.day = AlarmDate->Day;
		AlarmDateTime.hour = AlarmDate->Hour;
		AlarmDateTime.minute = AlarmDate->Minutes;
		AlarmDateTime.second = AlarmDate->Seconds;

		AlarmSeconds = RTC_ConvertDatetimeToSeconds(&AlarmDateTime);

		Status = Rtc_SetAlarmBySeconds(AlarmSeconds, AlarmHandle, AlarmCallback, AlarmCallbackArgs);

	}

	return Status;
}

rtc_status_t Rtc_SetAlarmBySeconds(uint32_t AlarmSeconds, uint8_t * AlarmHandle, rtc_alarm_callback_t AlarmCallback, void * AlarmCallbackArgs)
{
	rtc_status_t Status = RTC_WRONG_PARAMETER;
	uint32_t CurrentSeconds;

	if(AlarmCallback != NULL)
	{
		CurrentSeconds = Rtc_GetCurrentSecondsCounter();

		if(CurrentSeconds < AlarmSeconds)
		{

			Status = Rtc_SetAlarm(AlarmSeconds, AlarmHandle, AlarmCallback, AlarmCallbackArgs);
		}
	}

	return Status;
}

rtc_status_t Rtc_GetAlarmStatusInSeconds(uint32_t * AlarmRemainingSeconds, uint8_t * AlarmHandle)
{
	uint32_t CurrentAlarm;
	uint32_t CurrentTime;
	rtc_status_t Status = RTC_NO_ALARM_SET;

	/* compute remaining seconds in alarm and the handle that is used */
	assert(AlarmRemainingSeconds);

	if(ActiveAlarm.Alarm != NULL)
	{
		CurrentAlarm = RTC->TAR;

		if(CurrentAlarm)
		{
			CurrentTime = Rtc_GetCurrentSecondsCounter();

			*AlarmRemainingSeconds = CurrentAlarm - CurrentTime;

			*AlarmHandle = ActiveAlarm.Offset;

			Status = RTC_OK;
		}
	}

	return Status;
}

void Rtc_AlarmCancel(uint8_t AlarmHandle)
{
	/* Cancel the requested alarm */
	/* If is the active one, trigger a new selection */
	if(AlarmHandle < RTC_SUPPORTED_ALARMS)
	{
		RtcAlarms[AlarmHandle].AlarmSeconds = 0;
		RtcAlarms[AlarmHandle].Callback = NULL;
		RtcAlarms[AlarmHandle].Args = NULL;
		CLEAR_FLAG(AvailableAlarms,AlarmHandle);

		if(AlarmHandle == ActiveAlarm.Offset)
		{
			CLEAR_FLAG(AvailableAlarms,ActiveAlarm.Offset);

			Rtc_SelectAndSetAlarm();
		}
	}
}

rtc_weekday_t Rtc_DateToWeekDay(const rtc_date_t *Date)
{
	rtc_date_t CurrentDate;
	uint8_t Weekday;

	CurrentDate = *Date;

	/* https://stackoverflow.com/questions/6054016/c-program-to-find-day-of-week-given-date?rq=1*/
	/* Weekday  = (d += m < 3 ? y-- : y - 2, 23*m/9 + d + 4 + y/4- y/100 + y/400)%7; */

	Weekday = (CurrentDate.Day += CurrentDate.Month < 3 ? CurrentDate.Year-- : CurrentDate.Year - 2, 23*CurrentDate.Month/9 + \
			CurrentDate.Day + 4 + CurrentDate.Year/4- CurrentDate.Year/100 + CurrentDate.Year/400)%7;

	return (rtc_weekday_t)Weekday;
}

static rtc_status_t Rtc_SetAlarm(uint32_t AlarmInSeconds, uint8_t * AlarmHandle, rtc_alarm_callback_t AlarmCallback, void * CallbackArgs)
{
	rtc_status_t Status = RTC_ERROR;
	uint8_t AlarmOffset = 0;

	while(AlarmOffset < RTC_SUPPORTED_ALARMS)
	{
		/* First look for a an empty Alarm */
		if(CHECK_FLAG(AvailableAlarms,AlarmOffset) == 0)
		{
			SET_FLAG(AvailableAlarms,AlarmOffset);

			RtcAlarms[AlarmOffset].AlarmSeconds = AlarmInSeconds;
			RtcAlarms[AlarmOffset].Callback = AlarmCallback;
			RtcAlarms[AlarmOffset].Args = CallbackArgs;
			*AlarmHandle = AlarmOffset;

			break;
		}

		AlarmOffset++;
	}

	/* If the alarm was set, now, make sure the closest one is active */
	if(AlarmOffset < RTC_SUPPORTED_ALARMS)
	{
		Rtc_SelectAndSetAlarm();

		Status = RTC_OK;
	}

	return Status;
}

static void Rtc_ServeAlarms(void)
{
	uint8_t CurrentAlarmOffset = ActiveAlarm.Offset;

	/* Execute callback */
	if(ActiveAlarm.Alarm->Callback != NULL)
	{
		ActiveAlarm.Alarm->Callback(ActiveAlarm.Alarm->Args);
	}

	/* clear the current alarm */
	RtcAlarms[CurrentAlarmOffset].AlarmSeconds = 0;
	RtcAlarms[CurrentAlarmOffset].Args = NULL;
	RtcAlarms[CurrentAlarmOffset].Callback = NULL;

	CLEAR_FLAG(AvailableAlarms,ActiveAlarm.Offset);

	/* now sweep for the next alarm */
	Rtc_SelectAndSetAlarm();
}

static void Rtc_SelectAndSetAlarm(void)
{
	uint32_t MinAlarm = UINT32_MAX;
	uint8_t SelectedAlarm = UINT8_MAX;
	uint8_t AlarmOffset = 0;

	/*  the next alarm is the immediate in time */
	while(AlarmOffset < RTC_SUPPORTED_ALARMS)
	{
		if(CHECK_FLAG(AvailableAlarms,AlarmOffset) != 0)
		{
			if(MinAlarm > RtcAlarms[AlarmOffset].AlarmSeconds)
			{
				MinAlarm = RtcAlarms[AlarmOffset].AlarmSeconds;
				SelectedAlarm = AlarmOffset;
			}
		}

		AlarmOffset++;
	}

	/* confirm the new alarm selection and write it */
	if(SelectedAlarm < RTC_SUPPORTED_ALARMS)
	{
		RTC->TAR = RtcAlarms[SelectedAlarm].AlarmSeconds;

		ActiveAlarm.Alarm = &RtcAlarms[SelectedAlarm];
		ActiveAlarm.Offset = SelectedAlarm;
	}
	else
	{
		/* No alarm to set */
		RTC->TAR = 0;
		ActiveAlarm.Alarm  = NULL;
		ActiveAlarm.Offset = 0xFF;
	}

}

void RTC_IRQHandler(void)
{
    uint32_t status = RTC_GetStatusFlags(RTC);

    if (status & kRTC_AlarmFlag)
    {
        /* Clear alarm flag */
        RTC_ClearStatusFlags(RTC, kRTC_AlarmInterruptEnable);

        Rtc_ServeAlarms();
    }
    else if (status & kRTC_TimeInvalidFlag)
    {
        /* Clear timer invalid flag */
        RTC_ClearStatusFlags(RTC, kRTC_TimeInvalidFlag);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////


