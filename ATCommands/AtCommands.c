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
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#ifdef FSL_RTOS_FREE_RTOS
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#endif
#include "AtCommands.h"
#include "AtCommandsPlatform.h"
#include "SW_Timer.h"
#include "MiscFunctions.h"
#include "RingBuffer.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#define AT_COMMAND_BUFFER_SIZE				(2048)

#define AT_COMMAND_RESPONSE_BUFFER_SIZE		(2048)

#define AT_COMMAND_COMMAND					(uint8_t*)"AT+"

#define AT_COMMAND_COMMAND_SIZE				3

#define AT_COMMAND_EOF						"\r\n"

#define AT_COMMAND_EOF_SIZE					2

#define AT_COMMAND_SOF						"\r\n"

#define AT_COMMAND_SOF_SIZE					2

#define AT_RESPONSE_TIMEOUT					(30000)

#define AT_CHARACTER_TIMEOUT				(40)

#ifdef FSL_RTOS_FREE_RTOS
#define AT_COMMAND_STACK_SIZE				(256)

#define AT_COMMAND_TASK_PRIORITY			(configMAX_PRIORITIES - 2)

#define ATCOMMANDS_NEW_FRAME_EVENT			(1 << 0)

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	ATCOMMANDS_SET_COMMAND = 0,
	ATCOMMANDS_EXECUTE_COMMAND,
	ATCOMMANDS_GET_COMMAND,
	ATCOMMANDS_TEST_COMMAND,
	ATCOMMANDS_SIMPLE_COMMAND
}AtCommandsType_t;

typedef enum
{
	ATCOMMANDS_NEW_FRAME_FLAG = 0,
	ATCOMMANDS_INVALID_FLAG
}AtCommandsPacketProcessing_t;

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

static uint16_t AtCommands_BuildCommand(uint8_t * CommandToSend, uint8_t *CommandParameters, AtCommandsType_t CommandType);

static void AtCommands_ProcessData(void);

static void AtCommands_DataReceived(uint8_t DataReceived);

void AtCommands_ResponseTimeoutCallback (void * Args);

void AtCommands_CharacterTimeoutCallback (void);

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////
const static uint8_t CommandEndOfFrame[AT_COMMAND_EOF_SIZE] =
{
		AT_COMMAND_EOF
};

const static uint8_t CommandStartOfFrame[AT_COMMAND_SOF_SIZE] =
{
		AT_COMMAND_SOF
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////

static AtCommand_callback_t ApplicationCallback;

static swtimer_t CommandResponseTimeout = 0xFF;

//static swtimer_t CharacterTimeout = 0xFF;

static uint8_t CommandBuffer[AT_COMMAND_BUFFER_SIZE];

static uint8_t CommandsRingBuffer[AT_COMMAND_BUFFER_SIZE];

static uint8_t ResponseBuffer[AT_COMMAND_RESPONSE_BUFFER_SIZE];

#ifndef FSL_RTOS_FREE_RTOS
static uint16_t PacketProcessingFlags = 0;
#else
static EventGroupHandle_t AtCommand_Event = NULL;
#endif

static AtCommandResponse_t * CommandResponseTable;

static uint16_t CommandResponseTableSize;

RingBuffer_t ResponseRingBuffer;


#if AT_COMMANDS_DEBUG == 1

static uint8_t CommandDebugBuffer[AT_COMMAND_BUFFER_SIZE * 2];

static uint16_t CommandDebugCounter = 0;

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////

void AtCommands_Init(AtCommand_callback_t AppCallback, AtCommandResponse_t * ResponseTable, uint16_t AmountCommands)
{

	AtCommands_PlatformUartInit(AT_COMMANDS_BAUDRATE,AtCommands_DataReceived);

	RingBuffer_Init(&ResponseRingBuffer,&CommandsRingBuffer[0],SIZE_OF_ARRAY(CommandsRingBuffer));

	CommandResponseTimeout = SWTimer_AllocateChannel(AT_RESPONSE_TIMEOUT,AtCommands_ResponseTimeoutCallback,NULL);

#if 0
	CharacterTimeout = SWTimer_AllocateChannel(AT_CHARACTER_TIMEOUT,AtCommands_CharacterTimeoutCallback,NULL);
#else
	AtCommand_PlatformCharacterTimeoutInit(AtCommands_CharacterTimeoutCallback,AT_CHARACTER_TIMEOUT);
#endif
	if(AppCallback != NULL)
	{
		ApplicationCallback = AppCallback;
	}

	if(ResponseTable != NULL)
	{
		CommandResponseTable = ResponseTable;
		CommandResponseTableSize = AmountCommands;
	}

	#ifdef FSL_RTOS_FREE_RTOS

	AtCommand_Event = xEventGroupCreate();

	/* create task and event */
	xTaskCreate((TaskFunction_t) AtCommands_Task, (const char*) "AtCommands_task", AT_COMMAND_STACK_SIZE,\
						NULL,AT_COMMAND_TASK_PRIORITY,NULL);


	#endif
}

void AtCommands_ResetModule(void)
{
	AtCommands_PlatformAssertReset();
#ifdef FSL_RTOS_FREE_RTOS
	vTaskDelay(50/portTICK_PERIOD_MS);
#else
	/* empiric time */
	MiscFunctions_BlockingDelay(2000);
#endif
	AtCommands_PlatformDeassertReset();
}


#ifndef FSL_RTOS_FREE_RTOS
void AtCommands_Task(void)
{

	if(CHECK_FLAG(PacketProcessingFlags,ATCOMMANDS_NEW_FRAME_FLAG))
	{
		CLEAR_FLAG(PacketProcessingFlags,ATCOMMANDS_NEW_FRAME_FLAG);

		AtCommands_ProcessData();
	}
}
#else
void AtCommands_Task(void * param)
{
	EventBits_t EventsTriggered;

	while(1)
	{

		EventsTriggered = xEventGroupWaitBits(AtCommand_Event,ATCOMMANDS_NEW_FRAME_EVENT, \
												pdTRUE,pdFALSE,portMAX_DELAY);

		if(EventsTriggered & ATCOMMANDS_NEW_FRAME_EVENT)
		{
			AtCommands_ProcessData();
		}
	}

}
#endif

void AtCommands_ProcessData(void)
{
	uint32_t FrameSize;
	uint16_t FrameOffset = 0;
	uint8_t Status;
	uint16_t CommandOffset;
	uint16_t ParameterSize;
	uint8_t * NextCommand;
	bool isSearchingNewCommand = true;
	bool KeepSearching;


	/* read the frame from the FIFO, make sure the first two bytes are SOF 	*/
	/* if not, report as custom data?										*/
	FrameSize = RingBuffer_DataAvailable(&ResponseRingBuffer);


	RingBuffer_ReadBuffer(&ResponseRingBuffer,&ResponseBuffer[0],FrameSize);

	CommandOffset = CommandResponseTableSize;

	/* compare with all the table */
	while(CommandOffset--)
	{
		if(isSearchingNewCommand == true)
		{
			Status = MiscFunction_StringCompare(&CommandStartOfFrame[0],&ResponseBuffer[FrameOffset],AT_COMMAND_SOF_SIZE);

			if(Status == STRING_OK)
			{
				/* start comparing after SOF */
				FrameOffset += AT_COMMAND_SOF_SIZE;
			}

			isSearchingNewCommand = false;
		}

		Status = MiscFunction_StringCompare(CommandResponseTable[CommandOffset].Response,&ResponseBuffer[FrameOffset],\
				CommandResponseTable[CommandOffset].ResponseSize);

		if(Status == STRING_OK)
		{
			/* found it, now check if there are any parameters */
			FrameOffset += CommandResponseTable[CommandOffset].ResponseSize;

			if(FrameSize > (FrameOffset + AT_COMMAND_EOF_SIZE))
			{

				ParameterSize = FrameSize - FrameOffset;
				KeepSearching = CommandResponseTable[CommandOffset].ResponseCallback(&ResponseBuffer[FrameOffset],ParameterSize);
			}
			else
			{
				KeepSearching = CommandResponseTable[CommandOffset].ResponseCallback(NULL,0);
			}

			if(!KeepSearching)
			{
				break;
			}
			else
			{
				/* if we need to keep searching commands, find where the current command ends 	*/
				/* and set the offset to the beginning of the next command						*/
				/* this process assumes the current command ended with EOF (\r\n)				*/
				/* any special cases will end up as a command not found							*/
				isSearchingNewCommand = true;
				CommandOffset = CommandResponseTableSize;
				NextCommand = MiscFunctions_FindTokenInString(&ResponseBuffer[FrameOffset], CommandEndOfFrame[0]);
				FrameOffset += ((uint32_t)&NextCommand[1] - (uint32_t)&ResponseBuffer[FrameOffset]);
			}
		}
	}

	/* a rollover means we went through all the table without success */
	if(CommandOffset == 0xFFFF)
	{
		ApplicationCallback(ATCOMMANDS_RESPONSE_NOT_FOUND_EVENT,&ResponseBuffer[0],FrameSize);
	}
}

AtCommandsStatus_t ATCommands_ExecuteCommand(uint8_t * CommandToSend)
{
	uint16_t CommandSize;
	AtCommandsStatus_t Status = ATCOMMANDS_OK;

	CommandSize = AtCommands_BuildCommand(CommandToSend, NULL, ATCOMMANDS_EXECUTE_COMMAND);

	if(CommandSize > 0)
	{
		/* once the command is built, send it */
		SWTimer_EnableTimer(CommandResponseTimeout);
		AtCommands_PlatformUartSend(&CommandBuffer[0],CommandSize);
	}
	else
	{
		Status = ATCOMMANDS_WRONG_PARAMETER;
	}

	return Status;
}

AtCommandsStatus_t ATCommands_SendCustomCommand(uint8_t *CommandToSend, uint16_t CommandSize)
{
	uint16_t CommandBufferOffset = 0;
	AtCommandsStatus_t Status = ATCOMMANDS_OK;

	if(CommandToSend != NULL)
	{
		CommandBufferOffset = CommandSize;

		MiscFunctions_MemCopy(CommandToSend,&CommandBuffer[0],CommandSize);
		/* add the end of frame */
		MiscFunctions_MemCopy(&CommandEndOfFrame[0],&CommandBuffer[CommandBufferOffset],AT_COMMAND_EOF_SIZE);

		CommandBufferOffset += AT_COMMAND_EOF_SIZE;

		/* once the command is built, send it */
		SWTimer_EnableTimer(CommandResponseTimeout);
		AtCommands_PlatformUartSend(&CommandBuffer[0],CommandBufferOffset);
	}
	else
	{
		Status = ATCOMMANDS_WRONG_PARAMETER;
	}

	return Status;
}

AtCommandsStatus_t ATCommands_SetCommand(uint8_t * CommandToSend, uint8_t *Parameters)
{

	uint16_t CommandSize;
	AtCommandsStatus_t Status = ATCOMMANDS_OK;

	CommandSize = AtCommands_BuildCommand(CommandToSend, Parameters, ATCOMMANDS_SET_COMMAND);

	if(CommandSize > 0)
	{
		/* once the command is built, send it */
		SWTimer_EnableTimer(CommandResponseTimeout);
		AtCommands_PlatformUartSend(&CommandBuffer[0],CommandSize);
	}
	else
	{
		Status = ATCOMMANDS_WRONG_PARAMETER;
	}

	return Status;
}

static uint16_t AtCommands_BuildCommand(uint8_t * CommandToSend, uint8_t *CommandParameters, AtCommandsType_t CommandType)
{
	uint16_t CommandSize;
	uint16_t CommandBufferOffset = 0;
	uint16_t ParameterSize;

	if(CommandToSend != NULL)
	{
		/* fill the buffer with set command: 	*/
		/* AT + COMMAND = PARAMETER				*/
		CommandSize = strlen((char*)CommandToSend);

		MiscFunctions_MemClear(&CommandBuffer[0],sizeof(CommandBuffer));

		MiscFunctions_MemCopy(AT_COMMAND_COMMAND,&CommandBuffer[0],AT_COMMAND_COMMAND_SIZE);

		CommandBufferOffset += AT_COMMAND_COMMAND_SIZE;

		MiscFunctions_MemCopy(CommandToSend,&CommandBuffer[CommandBufferOffset],CommandSize);

		CommandBufferOffset += CommandSize;


		switch(CommandType)
		{
			case ATCOMMANDS_SET_COMMAND:
			{
				CommandBuffer[CommandBufferOffset] = '=';

				CommandBufferOffset += 1;

				ParameterSize = strlen((char*)CommandParameters);

				MiscFunctions_MemCopy(CommandParameters,&CommandBuffer[CommandBufferOffset],ParameterSize);

				CommandBufferOffset += ParameterSize;

			}
			break;
			case ATCOMMANDS_GET_COMMAND:
			{
				CommandBuffer[CommandBufferOffset] = '?';

				CommandBufferOffset += 1;

			}
			case ATCOMMANDS_TEST_COMMAND:
			{
				CommandBuffer[CommandBufferOffset] = '=';

				CommandBufferOffset += 1;

				CommandBuffer[CommandBufferOffset] = '?';

				CommandBufferOffset += 1;

			}
			break;
			/* fall through */
			case ATCOMMANDS_EXECUTE_COMMAND:
			default:
			break;
		}

		/* add the end of frame */
		MiscFunctions_MemCopy(&CommandEndOfFrame[0],&CommandBuffer[CommandBufferOffset],AT_COMMAND_EOF_SIZE);

		CommandBufferOffset += AT_COMMAND_EOF_SIZE;
	}


	return CommandBufferOffset;
}

void AtCommands_ResponseTimeoutCallback (void * Args)
{
	SWTimer_DisableTimer(CommandResponseTimeout);

	ApplicationCallback(ATCOMMANDS_COMMAND_TIMEOUT_ERROR_EVENT,NULL,0);
}

void AtCommands_CharacterTimeoutCallback (void)
{
	#ifdef FSL_RTOS_FREE_RTOS
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	#endif

	/* shutdown both timers */
	AtCommand_PlatformCharacterTimeoutStop();
	SWTimer_DisableTimer(CommandResponseTimeout);

	/* signal the event or process it here? */


	#ifndef FSL_RTOS_FREE_RTOS

	SET_FLAG(PacketProcessingFlags,ATCOMMANDS_NEW_FRAME_FLAG);

	#else

	xEventGroupSetBitsFromISR(AtCommand_Event, ATCOMMANDS_NEW_FRAME_EVENT,&xHigherPriorityTaskWoken);

	#endif
}

void AtCommands_EnableUart(bool isEnabled)
{
	AtCommands_PlatformUartEnableRx(isEnabled);

	AtCommands_PlatformUartEnableTx(isEnabled);

	RingBuffer_Reset(&ResponseRingBuffer);
}

void AtCommands_EnableUartRx(bool isEnabled)
{
	AtCommands_PlatformUartEnableRx(isEnabled);

	RingBuffer_Reset(&ResponseRingBuffer);
}

void AtCommands_EnableUartTx(bool isEnabled)
{
	AtCommands_PlatformUartEnableTx(isEnabled);
}

static void AtCommands_DataReceived(uint8_t DataReceived)
{
	//swtimerstatus_t Status;
	/* push the new character */
	RingBuffer_WriteData(&ResponseRingBuffer,&DataReceived);

#if 0
	/* Check if it was enabled, if not enable it */
	Status = SWTimer_TimerStatus(CharacterTimeout);


	if(Status == SWTIMER_ENABLED)
	{
		/* reset the timer. If this timer reaches 0, means we're not longer getting data 	*/
		/* so let's process it 																*/
		SWTimer_UpdateCounter(CharacterTimeout,AT_CHARACTER_TIMEOUT);
		AtCommand_PlatformCharacterTimeoutRefresh();
	}
	else
	{
		SWTimer_EnableTimer(CharacterTimeout);
	}
#else
	AtCommand_PlatformCharacterTimeoutRefresh();
#endif
}

/* EOF */
