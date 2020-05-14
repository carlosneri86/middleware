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

#ifndef _RTC_H_
#define _RTC_H_

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Includes Section
///////////////////////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	RTC_OK = 0,
	RTC_ERROR,
	RTC_WRONG_PARAMETER,
	RTC_NO_ALARM_SET
}rtc_status_t;

typedef enum
{
	RTC_SUNDAY = 0,
	RTC_MONDAY,
	RTC_TUESDAY,
	RTC_WEDNESDAY,
	RTC_THURSDAY,
	RTC_FRIDAY,
	RTC_SATURDAY
}rtc_weekday_t;

typedef struct
{
	uint16_t Year;
	uint8_t Month;
	uint8_t Day;
	uint8_t Hour;
	uint8_t Minutes;
	uint8_t Seconds;
	rtc_weekday_t Weekday;
}rtc_date_t;

typedef void (* rtc_alarm_callback_t)(void*);

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                Function-like Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#define RTC_SUPPORTED_ALARMS	(5)

#define RTC_TIMEZONE_ADJUSTMENT	(-5)

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Extern Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Extern Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus

rtc_status_t Rtc_Init(rtc_date_t * RTcInitDate);

rtc_status_t Rtc_GetCurrentDate(rtc_date_t * RtcDate);

rtc_status_t Rtc_SetAlarmByDate(rtc_date_t * AlarmDate, uint8_t * AlarmHandle, rtc_alarm_callback_t AlarmCallback, void * AlarmCallbackArgs);

rtc_status_t Rtc_SetAlarmBySeconds(uint32_t AlarmSeconds, uint8_t * AlarmHandle, rtc_alarm_callback_t AlarmCallback, void * AlarmCallbackArgs);

void Rtc_SetCurrentSecondsCounter(uint32_t CurrentSeconds);

uint32_t Rtc_GetCurrentSecondsCounter(void);

rtc_status_t Rtc_GetAlarmStatusInSeconds(uint32_t * AlarmRemainingSeconds, uint8_t * AlarmHandle);

void Rtc_AlarmCancel(uint8_t AlarmHandle);

rtc_weekday_t Rtc_DateToWeekDay(const rtc_date_t *Date);

#if defined(__cplusplus)
}
#endif // __cplusplus


#endif /* _RTC_H_ */
///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////
