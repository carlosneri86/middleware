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
#ifndef ATCOMMANDSPLATFORM_H_
#define ATCOMMANDSPLATFORM_H_


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Includes Section
///////////////////////////////////////////////////////////////////////////////////////////////////
#include "fsl_lpuart.h"
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#define AT_COMMANS_PLAT_UART	(LPUART0)

#define AT_COMMANDS_PLAT_TIMER	(TPM1)
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum
{
	ATCOMMANDS_PLATFORM_DATA_RECEIVED = 0,
	ATCOMMANDS_PLATFORM_DATA_OK,
	ATCOMMANDS_PLATFORM_ERROR = 0xFF
}AtCommandsPlatformStatus_t;

typedef void (*AtCommandsPlatformCallback_t)(uint8_t);

typedef void (*AtCommandsPlatformTimeoutCallback_t)(void);
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

void AtCommands_PlatformUartInit  (uint32_t BaudRate, AtCommandsPlatformCallback_t Callback);

void AtCommands_PlatformUartSend (uint8_t * CommandBuffer, uint16_t BufferSize);

AtCommandsPlatformStatus_t AtCommands_PlatformUartRxStatus(uint8_t * NewData);

uint8_t AtCommands_PlatformUartRead (void);

void AtCommands_PlatformUartEnableTx(bool isEnabled);

void AtCommands_PlatformUartEnableRx(bool isEnabled);

void AtCommands_PlatformAssertReset(void);

void AtCommands_PlatformDeassertReset(void);

void AtCommand_PlatformCharacterTimeoutInit(AtCommandsPlatformTimeoutCallback_t Callback, uint32_t Timeout);

void AtCommand_PlatformCharacterTimeoutStart(void);

void AtCommand_PlatformCharacterTimeoutStop(void);

void AtCommand_PlatformCharacterTimeoutRefresh(void);

#if defined(__cplusplus)
}
#endif // __cplusplus


#endif /* ATCOMMANDSPLATFORM_H_ */
///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////
