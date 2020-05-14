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

#include "FreeRTOS.h"
#include "semphr.h"
#include "fsl_common.h"
#include "fsl_adc16.h"
#include "FixedPoint.h"
#include "ADC.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#define ADC_BANDGAP_CHANNEL (27)

#define ADC_BANDGAP_VOLTAGE (INTEGER_TO_FIXED(1,6))

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

static SemaphoreHandle_t AdcMutex;

static q6_t AdcVoltageReference;

static q6_t AdcResolotionCounts;

static uint8_t AdcResolutionBits;

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////

void ADC_Init(void)
{
	static bool isInit = false;
	adc16_config_t Config;

	if(isInit == false)
	{
		ADC16_GetDefaultConfig(&Config);

		Config.referenceVoltageSource = kADC16_ReferenceVoltageSourceValt;

		ADC16_Init(ADC0, &Config);
		ADC16_EnableHardwareTrigger(ADC0, false);

		ADC16_SetHardwareAverage(ADC0, kADC16_HardwareAverageCount16);

		ADC16_DoAutoCalibration(ADC0);

		AdcMutex = xSemaphoreCreateMutex();

		AdcVoltageReference =ADC_GetVoltageReference();

		AdcResolutionBits = ADC_GetResolution();

		AdcResolotionCounts = INTEGER_TO_FIXED(((1 << AdcResolutionBits) - 1),6);
	}
}

uint32_t ADC_GetConversion(uint8_t Channel)
{
	adc16_channel_config_t ChannelConfig;
	uint32_t AdcReading;

	xSemaphoreTake(AdcMutex, portMAX_DELAY);

	ChannelConfig.channelNumber = Channel;

	ChannelConfig.enableInterruptOnConversionCompleted = false;

	 ADC16_SetChannelConfig(ADC0, 0U, &ChannelConfig);

	 /* poll the conversion done */
	 while (0U == (kADC16_ChannelConversionDoneFlag &
				   ADC16_GetChannelStatusFlags(ADC0, 0U)))
	 {
	 }

	 AdcReading = ADC16_GetChannelConversionValue(ADC0, 0U);

	 xSemaphoreGive(AdcMutex);

	 return AdcReading;
}

q6_t ADC_GetVoltageReference(void)
{
	q6_t BandgapReading;
	q6_t VoltageReference;

	/* Vdda = (Bandgap * AdcResolution) / AdcReading */
	BandgapReading = INTEGER_TO_FIXED(ADC_GetConversion(ADC_BANDGAP_CHANNEL),6);

	VoltageReference = FixedPoint_Multiplication(AdcResolotionCounts, ADC_BANDGAP_VOLTAGE,6);

	VoltageReference = FixedPoint_Division(VoltageReference, BandgapReading, 6);

	return VoltageReference;
}

uint8_t ADC_GetResolution(void)
{
	uint32_t ConfigValue;
	uint8_t Resolution;
	static const uint8_t AdcResolutions[4] =
	{
		8, 	/* kADC16_ResolutionSE8Bit */
		10,	/*  kADC16_ResolutionSE10Bit */
		12, /* kADC16_ResolutionSE12Bit */
		16, /* kADC16_ResolutionSE16Bit */
	};

	ConfigValue = ADC0->CFG1;

	Resolution = (ConfigValue & ADC_CFG1_MODE_MASK) >> ADC_CFG1_MODE_SHIFT;

	return AdcResolutions[Resolution];
}

q6_t ADC_GetVoltageFromAdc(uint32_t AdcReading)
{
	q6_t AdcFixed;
	q6_t FinalVoltage;

	/* Vin = (Vdda * ADCReading) / ADCResolution */

	AdcFixed = INTEGER_TO_FIXED(AdcReading,6);

	FinalVoltage = FixedPoint_Multiplication(AdcFixed, AdcVoltageReference, 6);

	FinalVoltage = FixedPoint_Division(FinalVoltage, AdcResolotionCounts, 6);

	return FinalVoltage;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////


