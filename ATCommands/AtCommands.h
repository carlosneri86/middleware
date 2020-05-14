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
#ifndef ATCOMMANDS_H_
#define ATCOMMANDS_H_


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Includes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#define AT_COMMANDS_BAUDRATE	(115200)

#define AT_COMMANDS_TIMEOUT_MS	(2000)

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	const uint8_t * Response;
	uint16_t ResponseSize;
	bool (*ResponseCallback)(uint8_t * , uint16_t );
}AtCommandResponse_t;

typedef enum
{
	ATCOMMANDS_OK = 0,
	ATCOMMANDS_ERROR,
	ATCOMMANDS_WRONG_PARAMETER,
}AtCommandsStatus_t;

typedef enum
{
	ATCOMMANDS_RESPONSE_NOT_FOUND_EVENT = 0,
	ATCOMMANDS_CUSTOM_RESPONSE_EVENT,
	ATCOMMANDS_COMMAND_TIMEOUT_ERROR_EVENT,
}AtCommandsEvent_t;

typedef void (* AtCommand_callback_t)(AtCommandsEvent_t, uint8_t*, uint16_t);
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

#ifndef FSL_RTOS_FREE_RTOS
void AtCommands_Task(void);
#else
void AtCommands_Task(void * param);
#endif

void AtCommands_Init(AtCommand_callback_t AppCallback, AtCommandResponse_t * ResponseTable, uint16_t AmountCommands);

void AtCommands_ResetModule(void);

AtCommandsStatus_t ATCommands_SetCommand(uint8_t * CommandToSend, uint8_t *Parameters);

AtCommandsStatus_t ATCommands_ExecuteCommand(uint8_t * CommandToSend);

AtCommandsStatus_t ATCommands_SendCustomCommand(uint8_t *CommandToSend, uint16_t CommandSize);

void AtCommands_EnableUart(bool isEnabled);

void AtCommands_EnableUartRx(bool isEnabled);

void AtCommands_EnableUartTx(bool isEnabled);

#if defined(__cplusplus)
}
#endif // __cplusplus


#endif /* ATCOMMANDS_H_ */
///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////
