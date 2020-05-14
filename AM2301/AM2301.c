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

#include "fsl_common.h"
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "MiscFunctions.h"
#include "AM2301.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#define AM2301_START_OF_DATA	(0x01)

#define AM2301_TOTAL_DATA_BITS	(40)

#define AM2301_1_5_MS_DELAY		(48000)

#define	AM2301_70_US_DELAY		(2170)

#define	AM2301_50_US_DELAY		(1440)

#define AM2301_TOTAL_DATA_BYTES	(AM2301_TOTAL_DATA_BITS / 8)

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

static status_t AM2301_ReadData(uint8_t * DataBuffer);

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////

extern uint32_t SystemCoreClock;

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////

void AM2301_Init(void)
{
	BOARD_AM2301();
}

status_t AM2301_GetMeasurement(uint16_t * Humidity, int16_t * Temperature)
{
	uint8_t ReceivedBuffer[AM2301_TOTAL_DATA_BYTES];
	status_t Status = kStatus_Fail;

	Status = AM2301_ReadData(&ReceivedBuffer[0]);

	if(Status == kStatus_Success)
	{
		*Humidity = (uint16_t)((ReceivedBuffer[0] << 8) & 0xFF00);
		*Humidity |= (uint16_t)((ReceivedBuffer[1]) & 0xFF);

		*Temperature = (int16_t)((ReceivedBuffer[2] << 8) & 0xFF00);
		*Temperature |= (int16_t)((ReceivedBuffer[3]) & 0xFF);
	}

	return Status;
}

static status_t AM2301_ReadData(uint8_t * DataBuffer)
{
	volatile uint32_t * SdaPort;
	uint32_t DataBufferOffset = 0;
	uint32_t DataBitCounter = 0;
	uint32_t StartData = 0;
	uint32_t EdgeTimeout = AM2301_50_US_DELAY;
	uint16_t InputData = 0;
	uint32_t Primask;
	status_t Status = kStatus_Fail;
	uint8_t Parity;
    /* Initialize GPIO functionality on pin PTC0 (pin 43)  */

	SdaPort = &BOARD_AM2301_SDA_PORT->PCR[BOARD_AM2301_SDA_PIN];

	Primask = DisableGlobalIRQ();

	/* first generate the start signal, this is, pulling SDA to 0 for ~800 uS */
	BOARD_AM2301_SDA_GPIO->PDDR |=	(1 << BOARD_AM2301_SDA_PIN);
	GPIO_PortClear(BOARD_AM2301_SDA_GPIO, 1 << BOARD_AM2301_SDA_PIN);

	MiscFunctions_BlockingDelay(AM2301_1_5_MS_DELAY);

	/* set as input so the pull-up will force the 1 */
	BOARD_AM2301_SDA_GPIO->PDDR &=	~(1 << BOARD_AM2301_SDA_PIN);


	/* now we must read the start signal */
	/* there's a 20 uS period after releasing start, then, 80 us for 0 and 80 for 1 of */
	/* the start signal, hence, wait for 6, so we catch the 0 at the middle	*/
	/* leave ~70 uS for the delay */
	MiscFunctions_BlockingDelay(AM2301_70_US_DELAY);

	StartData = GPIO_PinRead(BOARD_AM2301_SDA_GPIO,BOARD_AM2301_SDA_PIN);

	StartData <<= 1;

	MiscFunctions_BlockingDelay(AM2301_70_US_DELAY);

	StartData |= GPIO_PinRead(BOARD_AM2301_SDA_GPIO,BOARD_AM2301_SDA_PIN);

	if(StartData == AM2301_START_OF_DATA)
	{
		/* wait some time to try and align to the center of the first bit */
		/* it should be ~65 but since there was code within this statement */
		/* account for that */
		MiscFunctions_BlockingDelay(AM2301_70_US_DELAY);

		/* now iterate for the data */
		/* each bit starts with a ~50 us in low. The actual bit value is the high length */
		/* when 0, this is ~25 us, when 1 is ~75 us */

		/* we'll wait for each bit rising edge and wait ~50 us */
		/* if is a 0, we'll read in the low portion of the next bit */
		/* if is a 1, we'll get it as this is almost half of the bit */

		while(DataBufferOffset < AM2301_TOTAL_DATA_BYTES)
		{
			InputData = 0;
			DataBitCounter = 0;
			*SdaPort |= PORT_PCR_ISF_MASK;

			do
			{
				EdgeTimeout = AM2301_50_US_DELAY;

				/* wait for the edge or timeout so we don't hang */
				while((*SdaPort & PORT_PCR_ISF_MASK) == 0)
				{
					EdgeTimeout--;
					if(EdgeTimeout == 0)
					{
						break;
					}
				}

				*SdaPort |= PORT_PCR_ISF_MASK;

				MiscFunctions_BlockingDelay(AM2301_50_US_DELAY);

				InputData |= GPIO_PinRead(BOARD_AM2301_SDA_GPIO,BOARD_AM2301_SDA_PIN);

				InputData <<= 1;

				DataBitCounter++;

			}while(DataBitCounter < 8);

			InputData >>= 1;

			DataBuffer[DataBufferOffset] = InputData;

			DataBufferOffset++;
		}

		Parity = DataBuffer[0] + DataBuffer[1] + DataBuffer[2] + DataBuffer[3];

		if(Parity == DataBuffer[4])
		{
			Status = kStatus_Success;
		}
	}

	EnableGlobalIRQ(Primask);

	return Status;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////
