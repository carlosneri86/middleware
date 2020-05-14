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
#include "board.h"
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "Relay.h"
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	GPIO_Type * RelayPortBase;
	uint32_t RelayPinMask;
	uint8_t RelayPin;
	uint8_t RelayLogic;
}relay_port_t;

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////

static relay_port_t const RelayPorts[RELAY_MAX] =
{
		{
			.RelayPortBase = BOARD_RELAY_RELAY_0_GPIO,
			.RelayPin = BOARD_RELAY_RELAY_0_PIN,
			.RelayPinMask = 1<< BOARD_RELAY_RELAY_0_PIN,
			.RelayLogic = 0
		},
		{
			.RelayPortBase = BOARD_RELAY_RELAY_1_GPIO,
			.RelayPin = BOARD_RELAY_RELAY_1_PIN,
			.RelayPinMask = 1<< BOARD_RELAY_RELAY_1_PIN,
			.RelayLogic = 0
		},
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////

void Relay_Initialization(void)
{
	uint8_t RelayCount = RELAY_MAX;
	gpio_pin_config_t RelayConfig =
	{
		kGPIO_DigitalOutput, 0,
	};

	BOARD_Relay();

	while(RelayCount--)
	{
		RelayConfig.outputLogic = RelayPorts[RelayCount].RelayLogic;
		GPIO_PinInit(RelayPorts[RelayCount].RelayPortBase, RelayPorts[RelayCount].RelayPin, &RelayConfig);

		Relay_Disable(RelayCount);
	}
}

void Relay_Enable(relay_t SelectedRelay)
{
	if(SelectedRelay < RELAY_MAX)
	{
		GPIO_PinWrite(RelayPorts[SelectedRelay].RelayPortBase, RelayPorts[SelectedRelay].RelayPin, (RelayPorts[SelectedRelay].RelayLogic & 0x01));
	}
}

void Relay_Disable(relay_t SelectedRelay)
{
	if(SelectedRelay < RELAY_MAX)
	{
		GPIO_PinWrite(RelayPorts[SelectedRelay].RelayPortBase, RelayPorts[SelectedRelay].RelayPin, (~RelayPorts[SelectedRelay].RelayLogic) & 0x01);

	}
}

bool Relay_GetStatus(relay_t SelectedRelay)
{
	uint32_t RelayValue;
	bool RelayStatus = false;

	if(SelectedRelay < RELAY_MAX)
	{
		RelayValue = GPIO_PinRead(RelayPorts[SelectedRelay].RelayPortBase, RelayPorts[SelectedRelay].RelayPin);

		if(RelayPorts[SelectedRelay].RelayLogic)
		{
			RelayStatus = RelayValue ? true : false;
		}
		else
		{
			RelayStatus = RelayValue ? false : true;
		}
	}

	return(RelayStatus);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////


