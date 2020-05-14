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
#include "led.h"
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "board.h"
#include "SW_Timer.h"
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	GPIO_Type * LedPortBase;
	uint32_t LedPinMask;
	uint8_t LedPin;
	uint8_t LedTimer;
}led_port_t;
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

void Led_FlashingTimerCallback(void * Args);

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////

static led_port_t const LedsPorts[LED_MAX] =
{
		{
			.LedPortBase = BOARD_LED_RED_GPIO,
			.LedPin = BOARD_LED_RED_GPIO_PIN,
			.LedPinMask = 1<< BOARD_LED_RED_GPIO_PIN
		},
		{
			.LedPortBase = BOARD_LED_GREEN_GPIO,
			.LedPin = BOARD_LED_GREEN_GPIO_PIN,
			.LedPinMask = 1<< BOARD_LED_GREEN_GPIO_PIN
		},
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////

static uint8_t Led_FlashingTimer;

static uint8_t Led_ToFlash;

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////

void Led_Initialization(void)
{
	uint8_t LedCount = LED_MAX;
	gpio_pin_config_t LedConfig =
	{
		kGPIO_DigitalOutput, 0,
	};

	BOARD_InitLEDsPins();

	while(LedCount--)
	{
		GPIO_PinInit(LedsPorts[LedCount].LedPortBase, LedsPorts[LedCount].LedPin, &LedConfig);
		Led_TurnOff((led_t)LedCount);
	}

	Led_FlashingTimer = SWTimer_AllocateChannel(LED_FLASHING_RATE_MS,Led_FlashingTimerCallback,NULL);

}

void Led_TurnOn(led_t LedToHandle)
{
	GPIO_PortClear(LedsPorts[LedToHandle].LedPortBase, LedsPorts[LedToHandle].LedPinMask);
}

void Led_TurnOff(led_t LedToHandle)
{
	GPIO_PortSet(LedsPorts[LedToHandle].LedPortBase, LedsPorts[LedToHandle].LedPinMask);
}

void Led_Toggle(led_t LedToHandle)
{
	GPIO_PortToggle(LedsPorts[LedToHandle].LedPortBase, LedsPorts[LedToHandle].LedPinMask);
}

void Led_StartFlashing(led_t LedToFlash)
{
	Led_ToFlash |= (1 << LedToFlash);

	SWTimer_EnableTimer(Led_FlashingTimer);
}

void Led_StopFlashing(led_t LedToFlash)
{
	Led_ToFlash &= ~(1<<LedToFlash);

	Led_TurnOff(LedToFlash);

	if(!Led_ToFlash)
	{
		SWTimer_DisableTimer(Led_FlashingTimer);
	}
}

void Led_FlashingTimerCallback(void * Args)
{
	uint8_t LedFlashing = LED_MAX;

	while(LedFlashing--)
	{
		if(Led_ToFlash & (1<<LedFlashing))
		{
			Led_Toggle((led_t)LedFlashing);
		}
	}

}
///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////


