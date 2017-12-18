///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Includes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdlib.h>
#ifdef FSL_RTOS_FREE_RTOS
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#endif
#include "SW_Timer.h"
#include "fsl_lptmr.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////
/* Timer instance */
#define TIMER_INSTANCE  LPTMR0

#ifdef FSL_RTOS_FREE_RTOS
#define SWTIMERS_STACK_SIZE				(1024)

#define SWTIMERS_TASK_PRIORITY			configMAX_PRIORITIES - 3

#define SWTIMERS_TIMER_EVENT			(1)
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * SW Timer type
 */
typedef struct
{
	uint32_t CounterReload; /**< Timer reload value */
	uint32_t Counter;		/**< Timer counter */
	void (* SWTimer_Callback)(void);
}SWTimer_t;
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////
/* SWTimer task */
void SWTimer_SWTimerTask (void * param);

static void SWTimer_PlatformTimerInit(void);

static void SWTimer_PlatformTimerStop(void);

static void SWTimer_PlatformTimerStart(void);

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////
/* Array with enable masks for each supported timer */
const static uint16_t  SWTimer_gEnableMasks[SWTIMER_MAX_TIMERS] =
{
#if SWTIMER_MAX_TIMERS > 0
		(1<<0),
#endif
#if SWTIMER_MAX_TIMERS > 1
		(1<<1),
#endif
#if SWTIMER_MAX_TIMERS > 2
		(1<<2),
#endif
#if SWTIMER_MAX_TIMERS > 3
		(1<<3),
#endif
#if SWTIMER_MAX_TIMERS > 4
		(1<<4),
#endif
#if SWTIMER_MAX_TIMERS > 5
		(1<<5),
#endif
#if SWTIMER_MAX_TIMERS > 6
		(1<<6),
#endif
#if SWTIMER_MAX_TIMERS > 7
		(1<<7),
#endif
#if SWTIMER_MAX_TIMERS > 8
		(1<<8),
#endif
#if SWTIMER_MAX_TIMERS > 9
		(1<<9),
#endif
#if SWTIMER_MAX_TIMERS > 10
		(1<<10),
#endif
#if SWTIMER_MAX_TIMERS > 11
		(1<<11),
#endif
#if SWTIMER_MAX_TIMERS > 12
		(1<<12),
#endif
#if SWTIMER_MAX_TIMERS > 13
		(1<<13),
#endif
#if SWTIMER_MAX_TIMERS > 14
		(1<<14),
#endif
#if SWTIMER_MAX_TIMERS > 15
		(1<<15)
#endif
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////
/* Flag to signalize a timer ISR */
volatile uint8_t SWTimer_TimerIsrFlag = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////
/* Holds the enabled timers. One per bit */
static uint16_t SWTimer_gTimersEnabled = 0;
/* SW Timer elements array */
static SWTimer_t SWTimers_gCounters[SWTIMER_MAX_TIMERS];

static uint8_t isPlatformTimerEnabled = 0;

/* Amount of timers allocated */
static uint8_t SWTimer_Allocated = 0;
#ifdef FSL_RTOS_FREE_RTOS
static EventGroupHandle_t SWTimer_Event = NULL;
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////
/*FUNCTION**********************************************************************
 *
 * Function Name : SWTimer_Init
 * Description   : Software timer initialization using the PIT module.
 *
 *END**************************************************************************/
void SWTimer_Init(void)
{
	SWTimer_PlatformTimerInit();

	#ifdef FSL_RTOS_FREE_RTOS

	/* create task and event */
	xTaskCreate((TaskFunction_t) SWTimer_SWTimerTask, (const char*) "SWTimers_task", SWTIMERS_STACK_SIZE,\
						NULL,SWTIMERS_TASK_PRIORITY,NULL);
	#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SWTimer_AllocateChannel
 * Description   : Allocate a channel and configures it.
 *
 *END**************************************************************************/
uint8_t SWTimer_AllocateChannel(uint32_t Counter, void (* pTimerCallback)(void))
{
	uint8_t TimerOffset = 0;

	while(TimerOffset < SWTIMER_MAX_TIMERS)
	{
		/* If the bit is 0, means the timer is free */
		/* if is 1, means the timer is allocated and must move to the next*/
		if(!(SWTimer_Allocated & 1<<TimerOffset))
		{
			SWTimer_Allocated |= (1<<TimerOffset);
			/* Load the counter and the reload value */
			SWTimers_gCounters[TimerOffset].Counter = Counter/SWTIMER_BASE_TIME;
			SWTimers_gCounters[TimerOffset].CounterReload = Counter/SWTIMER_BASE_TIME;

			/* Set the timer callback */
			if(pTimerCallback != NULL)
			{
				SWTimers_gCounters[TimerOffset].SWTimer_Callback = pTimerCallback;
			}

			/* exit the cycle*/
			break;
		}
		else
		{
			TimerOffset++;
		}
	}

	/* send error in case there wasn't any timer available*/
	if(TimerOffset > SWTIMER_MAX_TIMERS)
	{
		TimerOffset = 0xFF;
	}

	return(TimerOffset);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SWTimer_EnableTimer
 * Description   : The selected timer is started.
 *
 *END**************************************************************************/
void SWTimer_EnableTimer(uint8_t TimerToEnable)
{
	/* Set the proper timer bit */
	if(SWTIMER_MAX_TIMERS > TimerToEnable)
	{
		SWTimer_gTimersEnabled |= SWTimer_gEnableMasks[TimerToEnable];

		if(isPlatformTimerEnabled == 0)
		{
			isPlatformTimerEnabled = 1;
			SWTimer_StartTimers();
		}
	}
}

/*FUNCTION**********************************************************************
 *
 * Function Name : Changes the selected timer period
 * Description   : Calculation of the sample needed by the application.
 *
 *END**************************************************************************/
void SWTimer_UpdateCounter(uint8_t TimerToUpdate, uint32_t NewCounter)
{
	/* Change the counter period */
	if(SWTIMER_MAX_TIMERS > TimerToUpdate)
	{
		SWTimers_gCounters[TimerToUpdate].Counter = NewCounter/SWTIMER_BASE_TIME;
		SWTimers_gCounters[TimerToUpdate].CounterReload = NewCounter/SWTIMER_BASE_TIME;
	}
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SWTimer_DisableTimer
 * Description   : Stops the selected timer.
 *
 *END**************************************************************************/
void SWTimer_DisableTimer(uint8_t TimerToDisable)
{
	/* Shutdown the timer */
	if(SWTIMER_MAX_TIMERS > TimerToDisable)
	{
		SWTimer_gTimersEnabled &= ~SWTimer_gEnableMasks[TimerToDisable];
		SWTimers_gCounters[TimerToDisable].Counter = SWTimers_gCounters[TimerToDisable].CounterReload;
	}

	if(!SWTimer_gTimersEnabled)
	{
		SWTimer_PlatformTimerStop();
		isPlatformTimerEnabled = 0;
	}
}
/*FUNCTION**********************************************************************
 *
 * Function Name : SWTimer_ReleaseTimer
 * Description   : Makes the timer available
 *
 *END**************************************************************************/
void SWTimer_ReleaseTimer(uint8_t TimerToRelease)
{
	/* Shutdown the timer */
	if(SWTIMER_MAX_TIMERS > TimerToRelease)
	{
		SWTimers_gCounters[TimerToRelease].Counter = 0;
		SWTimer_Allocated &= ~SWTimer_gEnableMasks[TimerToRelease];
	}
}
/*FUNCTION**********************************************************************
 *
 * Function Name : SWTimer_ReleaseTimer
 * Description   : Makes the timer available
 *
 *END**************************************************************************/
swtimerstatus_t SWTimer_TimerStatus(swtimer_t TimerToQuery)
{
	swtimerstatus_t Status = SWTIMER_DISABLED;

	if(SWTIMER_MAX_TIMERS > TimerToQuery)
	{
		if(SWTimer_gTimersEnabled & SWTimer_gEnableMasks[TimerToQuery])
		{
			 Status = SWTIMER_ENABLED;
		}
	}

	return (Status);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SWTimer_ServiceTimers
 * Description   : Main task for the SW timers.
 *
 *END**************************************************************************/
void SWTimer_ServiceTimers(void)
{
	uint8_t MaxCounter = SWTIMER_MAX_TIMERS - 1;

	/* Confirm there's a HW timer event */
#ifdef FSL_RTOS_FREE_RTOS
	EventBits_t EventsTriggered;

	EventsTriggered = xEventGroupWaitBits(SWTimer_Event,
											SWTIMERS_TIMER_EVENT,
											pdTRUE,
											pdFALSE,
											portMAX_DELAY);

	if(EventsTriggered & SWTIMERS_TIMER_EVENT)
	{
#else

	if(SWTimer_TimerIsrFlag)
	{
		SWTimer_TimerIsrFlag = 0;
#endif



		/* execute only when there's at least one timer enabled */
		if(SWTimer_gTimersEnabled)
		{
			/* Update all timers and execute callbacks in case the period is met */
			do
			{
				if(SWTimer_gTimersEnabled&SWTimer_gEnableMasks[MaxCounter])
				{
					SWTimers_gCounters[MaxCounter].Counter--;

					if(!SWTimers_gCounters[MaxCounter].Counter)
					{
						SWTimers_gCounters[MaxCounter].SWTimer_Callback();
						SWTimers_gCounters[MaxCounter].Counter = SWTimers_gCounters[MaxCounter].CounterReload;
					}
				}

			}while(MaxCounter--);
		}
	}
}
/*FUNCTION**********************************************************************
 *
 * Function Name : SWTimer_StopTimers
 * Description   : Stops all timers
 *
 *END**************************************************************************/
void SWTimer_StopTimers (void)
{
	uint8_t TimerOffset = SWTIMER_MAX_TIMERS;

	/* set all timers to initial count */
	while(TimerOffset--)
	{
		SWTimers_gCounters[TimerOffset].Counter = SWTimers_gCounters[TimerOffset].CounterReload;
	}

	SWTimer_PlatformTimerStop();
}
/*FUNCTION**********************************************************************
 *
 * Function Name : SWTimer_StartTimers
 * Description   : Re-start timers
 *
 *END**************************************************************************/
void SWTimer_StartTimers (void)
{
	SWTimer_PlatformTimerStart();
}
/*FUNCTION**********************************************************************
 *
 * Function Name : AudioApp_SWTimerTask
 * Description   : Initialize the audio buffer handler.
 *
 *END**************************************************************************/
void SWTimer_SWTimerTask (void * param)
{
	#ifdef FSL_RTOS_FREE_RTOS
	SWTimer_Event = xEventGroupCreate();
	#endif

	while(1)
	{
		/* service the timers*/
		SWTimer_ServiceTimers();

		#ifndef FSL_RTOS_FREE_RTOS
		break;
		#endif
	}
}


static void SWTimer_PlatformTimerInit(void)
{
	lptmr_config_t TimerConfig;
	uint32_t TimerClock;
	uint8_t Prescaler;
	/* Initialize HW timer. Usually the time base is 1ms*/
	LPTMR_GetDefaultConfig(&TimerConfig);

	/* use the oscerclk as clock source */
	TimerConfig.prescalerClockSource = kLPTMR_PrescalerClock_3;
	/* reduce the clock to match needs */
	TimerConfig.bypassPrescaler = false;
	TimerConfig.value = kLPTMR_Prescale_Glitch_3;
	Prescaler = (1 << (TimerConfig.value + 1));

	/* Init pit module */
	LPTMR_Init(TIMER_INSTANCE, &TimerConfig);

	TimerClock = CLOCK_GetFreq(kCLOCK_Osc0ErClk);
	TimerClock /= Prescaler;

	/* Set timer period for channel 0 */
	LPTMR_SetTimerPeriod(TIMER_INSTANCE, MSEC_TO_COUNT(SWTIMER_BASE_TIME, TimerClock));

	/* Enable timer interrupts for channel 0 */
	LPTMR_EnableInterrupts(TIMER_INSTANCE, kLPTMR_TimerInterruptEnable);

	/* Enable at the NVIC */
	NVIC_SetPriority(LPTMR0_IRQn, 2);
	EnableIRQ(LPTMR0_IRQn);
}

static void SWTimer_PlatformTimerStart(void)
{
	LPTMR_StartTimer(TIMER_INSTANCE);
}

static void SWTimer_PlatformTimerStop(void)
{
	LPTMR_StopTimer(TIMER_INSTANCE);
}

void LPTMR0_IRQHandler(void)
{
	#ifdef FSL_RTOS_FREE_RTOS
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	#endif
	/* Clear interrupt flag.*/
	LPTMR_ClearStatusFlags(TIMER_INSTANCE, kLPTMR_TimerCompareFlag);

	#ifdef FSL_RTOS_FREE_RTOS
	xEventGroupSetBitsFromISR(SWTimer_Event, SWTIMERS_TIMER_EVENT, &xHigherPriorityTaskWoken);
	#else
	SWTimer_TimerIsrFlag = 1;
	#endif
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////
