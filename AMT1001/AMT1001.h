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
#ifndef _AMT1001_H_
#define _AMT1001_H_
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Includes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#define AMT1001_AVERAGED_READINGS		(10)

#define AMT1001_READINGS_SAMPLING_MS	(1000)

#define AMT1001_HUMIDITY_ADC_CHANNEL	(1)

#define AMT1001_TEMPERATURE_ADC_CHANNEL	(14)

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef void (* amt1001_callback_t)(uint8_t , int8_t);

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

void AMT1001_Init(amt1001_callback_t AppCallback);

void AMT1001_StartMeasurement(void);

uint8_t AMT1001_GetHumidityMeasurement(void);

int8_t AMT1001_GetTemperatureMeasurement(void);

#if defined(__cplusplus)
}
#endif // __cplusplus


#endif /* WEATHERSERVICE_AMT1001_H_ */
///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////
