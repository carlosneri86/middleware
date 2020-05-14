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

#include "pin_mux.h"
#include "fsl_gpio.h"
#include "DebugPins.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#define DEBUG_PINS_MAX_PINS		(4)

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	GPIO_Type * GpioPort;
	uint32_t Pin;
}debug_pins_t;

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

static debug_pins_t DebugPins[DEBUG_PINS_MAX_PINS] =
{
	{
		BOARD_DEBUGPINS_DEBUG_PIN_1_GPIO,
		BOARD_DEBUGPINS_DEBUG_PIN_1_PIN,
	},
	{
		BOARD_DEBUGPINS_DEBUG_PIN_2_GPIO,
		BOARD_DEBUGPINS_DEBUG_PIN_2_PIN,
	},
	{
		BOARD_DEBUGPINS_DEBUG_PIN_3_GPIO,
		BOARD_DEBUGPINS_DEBUG_PIN_3_PIN,
	},
	{
		BOARD_DEBUGPINS_DEBUG_PIN_4_GPIO,
		BOARD_DEBUGPINS_DEBUG_PIN_4_PIN,
	}
};
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////

void DebugPins_Init(void)
{
	BOARD_DebugPins();
}

void DebugPins_TogglePin(uint8_t PinToToggle)
{
	if(PinToToggle < DEBUG_PINS_MAX_PINS)
	{
		GPIO_PortToggle(DebugPins[PinToToggle].GpioPort, 1 << DebugPins[PinToToggle].Pin);
	}
}

void DebugPins_SetPin(uint8_t PinToSet)
{
	if(PinToSet < DEBUG_PINS_MAX_PINS)
	{
		GPIO_PinWrite(DebugPins[PinToSet].GpioPort, DebugPins[PinToSet].Pin, 1);
	}
}

void DebugPins_ClearPin(uint8_t PinToClear)
{
	if(PinToClear < DEBUG_PINS_MAX_PINS)
	{
		GPIO_PinWrite(DebugPins[PinToClear].GpioPort, DebugPins[PinToClear].Pin, 0);
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////
