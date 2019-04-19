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

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////


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

static rtc_alarm_callback_t RtcAlarmApplicatinoCallback;

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////
rtc_status_t Rtc_Init(rtc_date_t * RTcInitDate)
{
	rtc_config_t RtcConfig;
	rtc_status_t Status = RTC_WRONG_PARAMETER;
	rtc_datetime_t InitialDate;

	if(isRtcConfigured == false)
	{
		if(RTcInitDate != NULL)
		{
			RTC_GetDefaultConfig(&RtcConfig);
			RTC_Init(RTC, &RtcConfig);

			InitialDate.year = RTcInitDate->Year;
			InitialDate.month = RTcInitDate->Month;
			InitialDate.day = RTcInitDate->Day;
			InitialDate.hour = RTcInitDate->Hour;
			InitialDate.minute = RTcInitDate->Minutes;
			InitialDate.second = RTcInitDate->Seconds;


			RTC_ClearStatusFlags(RTC, kRTC_AlarmInterruptEnable);
			RTC_ClearStatusFlags(RTC, kRTC_TimeInvalidFlag);

			RTC_SetDatetime(RTC, &InitialDate);

			RTC_StartTimer(RTC);

			RTC_EnableInterrupts(RTC, kRTC_AlarmInterruptEnable);

			EnableIRQ(RTC_IRQn);

			Status = RTC_OK;

			isRtcConfigured = true;

			/* TODO: Implement callbacks with parameter passing */
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

rtc_status_t Rtc_SetAlarmByDate(rtc_date_t * AlarmDate, rtc_alarm_callback_t AlarmCallback)
{
	rtc_status_t Status = RTC_WRONG_PARAMETER;
	rtc_datetime_t CurrentDate;
	status_t AlarmStatus;

	if((AlarmDate != NULL) && (AlarmCallback != NULL))
	{
		CurrentDate.year	=	AlarmDate->Year;
		CurrentDate.month	=	AlarmDate->Month;
		CurrentDate.day		=	AlarmDate->Day ;
		CurrentDate.hour	=	AlarmDate->Hour;
		CurrentDate.minute	=	AlarmDate->Minutes;
		CurrentDate.second	=	AlarmDate->Seconds;

		AlarmStatus = RTC_SetAlarm(RTC, &CurrentDate);

		if(AlarmStatus == kStatus_Success)
		{
			Status = RTC_OK;
		}

		RtcAlarmApplicatinoCallback = AlarmCallback;

	}

	return Status;
}

rtc_status_t Rtc_SetAlarmBySeconds(uint32_t AlarmSeconds, rtc_alarm_callback_t AlarmCallback)
{
	rtc_status_t Status = RTC_WRONG_PARAMETER;
	uint32_t CurrentSeconds;

	if(AlarmCallback != NULL)
	{
		CurrentSeconds = Rtc_GetCurrentSecondsCounter();

		if(CurrentSeconds < AlarmSeconds)
		{

			RtcAlarmApplicatinoCallback = AlarmCallback;

			RTC->TAR = AlarmSeconds;

			Status = RTC_OK;
		}
	}

	return Status;
}

rtc_status_t Rtc_GetAlarmStatusInSeconds(uint32_t * AlarmRemainingSeconds)
{
	uint32_t CurrentAlarm;
	uint32_t CurrentTime;
	rtc_status_t Status = RTC_NO_ALARM_SET;

	assert(AlarmRemainingSeconds);

	CurrentAlarm = RTC->TAR;

	if(CurrentAlarm)
	{
		CurrentTime = Rtc_GetCurrentSecondsCounter();

		*AlarmRemainingSeconds = CurrentAlarm - CurrentTime;

		Status = RTC_OK;
	}

	return Status;
}

void Rtc_AlarmCancel(void)
{
	RTC->TAR = 0;
}

void RTC_IRQHandler(void)
{
    uint32_t status = RTC_GetStatusFlags(RTC);

    if (status & kRTC_AlarmFlag)
    {
        /* Clear alarm flag */
        RTC_ClearStatusFlags(RTC, kRTC_AlarmInterruptEnable);

        RtcAlarmApplicatinoCallback();
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


