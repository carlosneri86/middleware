///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Includes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "fsl_common.h"
#include "fsl_adc16.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////


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
	}
}

uint32_t ADC_GetConversion(uint8_t Channel)
{
	adc16_channel_config_t ChannelConfig;

	ChannelConfig.channelNumber = Channel;

	ChannelConfig.enableInterruptOnConversionCompleted = false;

	 ADC16_SetChannelConfig(ADC0, 0U, &ChannelConfig);

	 /* poll the conversion done */
	 while (0U == (kADC16_ChannelConversionDoneFlag &
				   ADC16_GetChannelStatusFlags(ADC0, 0U)))
	 {
	 }

	 return ADC16_GetChannelConversionValue(ADC0, 0U);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////

