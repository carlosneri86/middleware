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

#include <stdlib.h>
#include <stdbool.h>
#include "SW_Timer.h"
#include "FixedPoint.h"
#include "ADC.h"
#include "AMT1001.h"
#include "pin_mux.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

/* taken from DS humidity plot. 33.33 */
#define AMT1001_HUMIDITY_SLOPE_Q6	(2133)

#define AMT1001_THERMISTOR_R0_VALUE	(INTEGER_TO_FIXED(10000,6))

#define AMT1001_THERMISTOR_VOLTAGE	(INTEGER_TO_FIXED(5,6))

#define AMT1001_THERMISTOR_LUT_SIZE (71U)

#define AMT1001_THERMISTOR_MIN_TEMP	(-10)

#define AMT1001_THERMISTOR_MAX_TEMP	(60)

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	uint32_t HumidityMeasurement;
	uint32_t HumidityAccumulator;
	int8_t TemperatureMeasurement;
	uint32_t TemperatureAccumulator;
	amt1001_callback_t Callback;
	uint8_t SamplingOffset;
	bool isMeasurementOnGoing;
}amt1001_t;

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

static void AMT1001_TimerCallback(void * Params);

static int8_t AMT1001_CalculateTemperature(uint32_t TemperatureAdcData);

static uint8_t AMT1001_CalculateHumidity(uint32_t HumidityAdcData);

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

static amt1001_t MeasurementInstance;

static swtimer_t SamplingTimer;

static 	q6_t AdcVoltage;

static q6_t AdcResolution;

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////
void AMT1001_Init(amt1001_callback_t AppCallback)
{
	/* allocate the timer to be used for sampling */

	MeasurementInstance.HumidityAccumulator = 0;
	MeasurementInstance.HumidityMeasurement = 0;
	MeasurementInstance.TemperatureAccumulator = 0;
	MeasurementInstance.TemperatureMeasurement = 0;
	MeasurementInstance.SamplingOffset = 0;
	MeasurementInstance.isMeasurementOnGoing = false;
	MeasurementInstance.Callback = AppCallback;

	SamplingTimer = SWTimer_AllocateChannel(AMT1001_READINGS_SAMPLING_MS, AMT1001_TimerCallback, NULL);

	ADC_Init();

	AdcResolution = ADC_GetResolution();

	AdcResolution = (1 << AdcResolution) - 1;

	AdcResolution = INTEGER_TO_FIXED(AdcResolution,6);

	BOARD_AMT1001();
}

void AMT1001_StartMeasurement(void)
{
	if(MeasurementInstance.isMeasurementOnGoing == false)
	{
		MeasurementInstance.HumidityAccumulator = 0;
		MeasurementInstance.TemperatureAccumulator = 0;
		MeasurementInstance.SamplingOffset = 0;
		MeasurementInstance.isMeasurementOnGoing = true;

		AdcVoltage = ADC_GetVoltageReference();

		SWTimer_EnableTimer(SamplingTimer);
	}
}

uint8_t AMT1001_GetHumidityMeasurement(void)
{
	return MeasurementInstance.HumidityMeasurement;
}

int8_t AMT1001_GetTemperatureMeasurement(void)
{
	return MeasurementInstance.TemperatureMeasurement;
}

static uint8_t AMT1001_CalculateHumidity(uint32_t HumidityAdcData)
{
	q6_t HumidityVoltage;
	uint8_t HumidityPercentage;

	/* first change the ADC reading to voltage */
	HumidityVoltage = ADC_GetVoltageFromAdc(HumidityAdcData);

	/* now apply the slope */
	HumidityVoltage = FixedPoint_Multiplication(HumidityVoltage,AMT1001_HUMIDITY_SLOPE_Q6,6);

	/* get only the integer */
	HumidityPercentage = (uint8_t)FIXED_TO_INTEGER(HumidityVoltage,6);

	return HumidityPercentage;
}

static int8_t AMT1001_CalculateTemperature(uint32_t TemperatureAdcData)
{
	q6_t TemperatureVoltage;
	q6_t ThermistorResistance;
	q6_t CurrentMinDifference = INT32_MAX;
	q6_t LutDifference;
	uint32_t LutIndex = 0;
	uint32_t LutMinIndex = 0xFF;
	int8_t FinalTemperature;
	const static q6_t TemperatureResistanceLut[AMT1001_THERMISTOR_LUT_SIZE] =
	{
			2884,	2749,	2621,	2499,	2384,	2275,	2172,	2074,	1981,	1893,
			1809,	1730,	1654,	1582,	1514,	1450,	1388,	1329,	1274,	1220,
			1170,	1122,	1076,	1032,	990,	951,	913,	877,	842,	809,
			778,	748,	719,	691,	665,	640,	616,	593,	571,	550,
			530,	511,	492,	474,	457,	441,	425,	411,	396,	382,
			369,	356,	344,	333,	321,	311,	300,	290,	281,	271,
			262,	254,	246,	238,	230,	223,	216,	209,	202,	196,
			190,
	};
	/* LUT is computed from -10 to 60 C, taking the resistance from the device DS and converted to Q6*/

	/* first convert the ADC reading to voltage and then to thermistor resistance */
	TemperatureVoltage = ADC_GetVoltageFromAdc(TemperatureAdcData);

	/* to compute resistance use R0 * ((Vdda/Vo) - 1), where, R0 is 10K 	*/
	/* Vdda is the thermistor reference, meaning, 5 V and V0 is the ADC reading */

	ThermistorResistance = FixedPoint_Division(AMT1001_THERMISTOR_VOLTAGE, TemperatureVoltage, 6);

	ThermistorResistance -= INTEGER_TO_FIXED(1,6);

	ThermistorResistance = FixedPoint_Multiplication(ThermistorResistance, AMT1001_THERMISTOR_R0_VALUE, 6);

	/* Now that we have the thermistor resistance, look for the closest reading from LUT */

	while(LutIndex < AMT1001_THERMISTOR_LUT_SIZE)
	{
		LutDifference = TemperatureResistanceLut[LutIndex] - ThermistorResistance;
		LutDifference = abs(LutDifference);

		if(CurrentMinDifference > LutDifference)
		{
			CurrentMinDifference = LutDifference;
			LutMinIndex = LutIndex;
		}

		LutIndex++;
	}

	/* With the index, compute temperature as T = (MinIndex * LUT_STEP) + AMT1001_THERMISTOR_MIN_TEMP */
	/* Given that the LUT_STEP is 1 degree, we optimize the multiplication */

	FinalTemperature = LutMinIndex + AMT1001_THERMISTOR_MIN_TEMP;

	if((FinalTemperature < AMT1001_THERMISTOR_MIN_TEMP) || (FinalTemperature > AMT1001_THERMISTOR_MAX_TEMP))
	{
		FinalTemperature = 0;
	}

	return FinalTemperature;
}

static void AMT1001_TimerCallback(void * Params)
{
	MeasurementInstance.HumidityAccumulator += ADC_GetConversion(AMT1001_HUMIDITY_ADC_CHANNEL);

	MeasurementInstance.TemperatureAccumulator += ADC_GetConversion(AMT1001_TEMPERATURE_ADC_CHANNEL);

	MeasurementInstance.SamplingOffset++;

	if(MeasurementInstance.SamplingOffset >= AMT1001_AVERAGED_READINGS)
	{
		SWTimer_DisableTimer(SamplingTimer);

		MeasurementInstance.HumidityAccumulator /= AMT1001_AVERAGED_READINGS;
		MeasurementInstance.TemperatureAccumulator /= AMT1001_AVERAGED_READINGS;

		MeasurementInstance.HumidityMeasurement = AMT1001_CalculateHumidity(MeasurementInstance.TemperatureAccumulator);

		MeasurementInstance.TemperatureMeasurement = AMT1001_CalculateTemperature(MeasurementInstance.TemperatureAccumulator);

		MeasurementInstance.isMeasurementOnGoing = false;

		MeasurementInstance.Callback(MeasurementInstance.HumidityMeasurement,MeasurementInstance.TemperatureMeasurement);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////
