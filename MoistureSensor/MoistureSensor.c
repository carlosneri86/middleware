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
#include "ADC.h"
#include "MoistureSensor.h"
#include "FixedPoint.h"
#include "SW_Timer.h"
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#define LINEAR_A_FACTOR_Q7 	(-6)

#define LINEAR_SLOPE_Q7		(24782)

#define MOISTURESENSOR_SAMPLING_TIMER	(1000)

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

static void MoistureSensor_TimerCallback(void * Args);

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

static swtimer_t SensorSamplingTimer = 0xFF;

static uint32_t SampleAccumulator;

static uint32_t LastMeasurement;

static uint8_t SamplesToAverage;

static uint8_t SampleCounter = 0;
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////

void MoistureSensor_Init(uint8_t SamplesAverage)
{
	ADC_Init();

	SensorSamplingTimer = SWTimer_AllocateChannel(MOISTURESENSOR_SAMPLING_TIMER, MoistureSensor_TimerCallback,NULL);

	SampleAccumulator = 0;

	SamplesToAverage = SamplesAverage;
}

uint8_t MoistureSensor_GetReading(void)
{
	return LastMeasurement;
}

void MoistureSensor_Start(void)
{
	SampleAccumulator = 0;

	SampleCounter = 0;

	SWTimer_EnableTimer(SensorSamplingTimer);
}

void MoistureSensor_Stop(void)
{
	SWTimer_DisableTimer(SensorSamplingTimer);
}

static void MoistureSensor_TimerCallback(void * Args)
{
	uint32_t AdcReading;
	int32_t MoistureFixed;
	uint8_t MoisturePercentage;
	uint32_t AdcReadingFixed;

	AdcReading = ADC_GetConversion(MOISTURE_SENSOR_CHANNEL);

	SampleAccumulator += AdcReading;

	SampleCounter++;

	if(SampleCounter >= SamplesToAverage)
	{
		/* now change the reading to percentage */
		/* this process requires offline readings of the sensor */
		/* dry and totally wet to make the correct translation  */

		/* The linear conversion from 0% - 100% with the given numbers is 	*/
		/* f(x) = 0.47*X + 193.61											*/

		/*MoistureFixed = ((LINEAR_A_FACTOR_Q7 * (AdcReading << 7)) >> 7) + LINEAR_SLOPE_Q7;*/

		AdcReadingFixed = INTEGER_TO_FIXED(SampleAccumulator,7);

		AdcReadingFixed = FixedPoint_Division(AdcReadingFixed, INTEGER_TO_FIXED(SamplesToAverage,7), 7);

		MoistureFixed = FixedPoint_Multiplication(AdcReadingFixed, LINEAR_A_FACTOR_Q7, 7);

		MoistureFixed += LINEAR_SLOPE_Q7;

		MoisturePercentage = FIXED_TO_INTEGER(MoistureFixed,7);

		if(MoisturePercentage > 100)
		{
			MoisturePercentage = 100;
		}

		LastMeasurement = MoisturePercentage;

		SampleAccumulator = 0;

		SampleCounter = 0;

	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////
