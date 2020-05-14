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

#ifndef SW_TIMER_H_
#define SW_TIMER_H_

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Includes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////
//! Error flag for SW timer
#define SWTIMER_ERROR	(1)
//! Time based in milliseconds. Used to calculate on precompile each SW timer period
#define SWTIMER_BASE_TIME	(100)
//! Maximum number of supported timers */
#define SWTIMER_MAX_TIMERS	(15UL)
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef uint8_t swtimer_t;

typedef enum
{
	SWTIMER_ENABLED = 0,
	SWTIMER_DISABLED,
}swtimerstatus_t;
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                Function-like Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

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
/*!
 *	@brief	Initializes the SW timer service
 *
 *	@param	dwIRQtoEnable		[in]	Interrupt source to enable
 *
 * 	@return	void
 *
*/
void SWTimer_Init(void);
/*!
 *	@brief	Allocates and configures a SW Timer channel
 *
 *	@param	dwCounter			[in]	Amount of milliseconds for the timer
 *
 *	@param	pTimerCallback	[in]	Callback to be executed when the timer reaches zero
 *
 * 	@return	uint8_t			Possible error after allocating the channel
 * 	@retval	SWTIMER_ERROR	There are no channels available
 * 	@retval	Any value between 0 and SWTIMER_MAX_TIMERS is a valid channel
 *
*/
swtimer_t SWTimer_AllocateChannel(uint32_t Counter, void (* pTimerCallback)(void*), void * Args);
/*!
 *	@brief	The selected timer is started
 *
 *	@param	bTimerToEnable			[in]	Timer to start
 *
 * 	@return	void
 *
*/
void SWTimer_EnableTimer(swtimer_t TimerToEnable);
/*!
 *	@brief	Stops the selected timer
 *
 *	@param	TimerToDisable			[in]	Timer to stop
 *
 * 	@return	void
 *
*/
void SWTimer_DisableTimer(swtimer_t TimerToDisable);
/*!
 *	@brief	Makes the selected timer available
 *
 *	@param	TimerToRelease			[in]	Timer to make available
 *
 * 	@return	void
 *
*/
void SWTimer_ReleaseTimer(swtimer_t TimerToRelease);
/*!
 *	@brief	Changes the selected timer period
 *
 *	@param	bTimerToEnable			[in]	Timer to update
 *
 * 	@return	void
 *
*/
void SWTimer_UpdateCounter(swtimer_t TimerToUpdate, uint32_t NewCounter);
/*!
 *	@brief	Checks if a timer is active
 *
 *	@param	bTimerToEnable			[in]	Timer to query status
 *
 * 	@return	Enabled/disabled
 *
*/
swtimerstatus_t SWTimer_TimerStatus(swtimer_t TimerToQuery);
/*!
 *	@brief	Main task for the SW timers
 *
 *	@param	void
 *
 * 	@return	void
 *
 * 	@note This function must be called periodically
 *
*/
void SWTimer_ServiceTimers(void);
/*!
 *	@brief	Shutdown HW timer and reset all counters
 *
 *	@param	void
 *
 * 	@return	void
 *
 *
*/
void SWTimer_StopTimers (void);
/*!
 *	@brief	Restarts HW timer
 *
 *	@param	void
 *
 * 	@return	void
 *
 *
*/
void SWTimer_StartTimers (void);

/*! @} End of Software Timers */
/*! @} End of Service Layer */
#if defined(__cplusplus)
}
#endif // __cplusplus


#endif /* SW_TIMER_H_ */
///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////
