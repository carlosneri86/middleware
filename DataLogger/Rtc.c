///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Includes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
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
static rtc_datetime_t InitialDate;

static rtc_datetime_t CurrentDate;

static bool isRtcConfigured = false;
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////
rtc_status_t Rtc_Init(rtc_date_t * RTcInitDate)
{
	rtc_config_t RtcConfig;
	rtc_status_t Status = RTC_WRONG_PARAMETER;

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

			RTC_SetDatetime(RTC, &InitialDate);

			RTC_StartTimer(RTC);

			Status = RTC_OK;
			isRtcConfigured = true;
		}
	}

	return Status;
}

rtc_status_t Rtc_GetCurrentDate(rtc_date_t * RtcDate)
{
	rtc_status_t Status = RTC_WRONG_PARAMETER;

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

///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////


