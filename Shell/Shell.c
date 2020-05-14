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
#include "Shell.h"
#include "SerialPlatform.h"
#include "MiscFunctions.h"
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#define SHELL_CHARACTER_RECEIVED_FLAG	(0)

#define SHELL_COMMAND_PENDING_FLAG		(1)

#define SHELL_CLEAR_SCREEN				(Shell_WriteString("\033[2J\033[H"))

#define SHELL_PROMPT					(Shell_WriteString(ShellPrompt))

#define SHELL_BACKSPACE					(Shell_WriteString("\b \b"))

#define SHELL_NEW_LINE					(Shell_WriteString("\n\r"))

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

static void Shell_ReceiveCallback (uint8_t NewReceivedData);

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

static volatile uint16_t ShellStatus = 0;

static volatile uint8_t NewCharacterRecevied;

static shell_command_t * ApplicationCommands;

static uint8_t ApplicationCommandsTotal;

static uint8_t ShellCommandBuffer[SHELL_COMMAND_SIZE_MAX];

static uint8_t ShellCommandCurrentOffset;

static uint8_t ShellArgs[SHELL_COMMAND_ARGS_MAX][SHELL_COMMAND_ARGS_SIZE_MAX];

static uint8_t * ShellArgsList[SHELL_COMMAND_ARGS_MAX];

static char * ShellPrompt;

const char ShellWelcome[] =
{
		"\n\r"
		"*************************************\n\r"
		"                 SHELL               \n\r"
		"*************************************\n\r"
		"Build Date "
};
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////

shell_status_t Shell_Init(shell_command_t * CommandTable, uint8_t CommandTableSize, char * Prompt)
{
	shell_status_t Status = SHELL_WRONG_PARAMETER;

	if(CommandTable != NULL)
	{
		if(CommandTableSize < SHELL_COMMANDS_MAX)
		{
			if(Prompt != NULL)
			{
				ApplicationCommands = CommandTable;

				ApplicationCommandsTotal = CommandTableSize;

				ShellPrompt = Prompt;

				ShellCommandCurrentOffset = 0;

				MiscFunctions_MemClear(&ShellCommandBuffer[0],SHELL_COMMAND_SIZE_MAX);

				SerialPlatform_Init(SHELL_BAUDRATE,Shell_ReceiveCallback);

				SHELL_CLEAR_SCREEN;

				Shell_WriteString((char*)&ShellWelcome[0]);

				Shell_WriteString(__DATE__);

				SHELL_NEW_LINE;

				SHELL_PROMPT;

				Status = SHELL_OK;
			}
		}
	}

	return Status;
}

shell_status_t Shell_ClearScreen(void)
{
	SHELL_CLEAR_SCREEN;

	return (SHELL_OK);
}

void Shell_NewLine(void)
{
	SHELL_NEW_LINE;
}

void Shell_WriteString(char * TextToWrite)
{
	uint8_t StringSize;

	StringSize = strlen(TextToWrite);
	SerialPlatform_SendBlocking((uint8_t*)TextToWrite,StringSize);
}

void Shell_WriteNumber(uint32_t NumberToPrint)
{
	uint8_t AsciiBuffer[6]=
	{
		0
	};

	MiscFunctions_IntegerToAscii(NumberToPrint,&AsciiBuffer[0]);

	Shell_WriteString((char*)&AsciiBuffer[0]);
}

void Shell_WriteCharacter(uint8_t DataToPrint)
{
	SerialPlatform_SendBlocking(&DataToPrint,1);
}

void Shell_AsynchCommandDone(void)
{
	CLEAR_FLAG(ShellStatus,SHELL_COMMAND_PENDING_FLAG);
	SHELL_NEW_LINE;
	SHELL_PROMPT;
}

void Shell_Task(void)
{
	static uint8_t EofCounter = 0;
	uint8_t CommandOffset;
	uint8_t CommandSize;
	uint8_t CommandTextStatus;
	shell_command_status_t CommandStatus;
	uint8_t * ArgPosition;
	uint8_t ArgsCounter;
	uint8_t * ArgLastPosition;

	if(CHECK_FLAG(ShellStatus,SHELL_CHARACTER_RECEIVED_FLAG))
	{
		CLEAR_FLAG(ShellStatus,SHELL_CHARACTER_RECEIVED_FLAG);

		/* ignore character if there's a pending asynch command */
		if(!CHECK_FLAG(ShellStatus,SHELL_COMMAND_PENDING_FLAG))
		{
			if(NewCharacterRecevied == '\r')
			{
				EofCounter++;
			}
			else if((EofCounter == 1) && (NewCharacterRecevied == '\n'))
			{
				if(ShellCommandCurrentOffset > 2)
				{
					/* enter was received, start parsing */
					CommandOffset = ApplicationCommandsTotal;

					while(CommandOffset--)
					{
						CommandSize = strlen(ApplicationCommands[CommandOffset].CommandText);

						CommandTextStatus = MiscFunction_StringCompare((uint8_t*)ApplicationCommands[CommandOffset].CommandText, &ShellCommandBuffer[0], CommandSize);

						if(STRING_OK == CommandTextStatus)
						{
							break;
						}
					}

					SHELL_NEW_LINE;
					/* roll over means there's no match */
					if(CommandOffset != 0xFF)
					{
						/* now look for parameters if needed*/
						if(ApplicationCommands[CommandOffset].CommandArgsCount)
						{
							ArgsCounter = 0;
							ArgLastPosition = &ShellCommandBuffer[0];

							while(ArgsCounter < ApplicationCommands[CommandOffset].CommandArgsCount)
							{
								ArgPosition = MiscFunctions_FindTokenInString(ArgLastPosition, ' ');

								if(ArgPosition != NULL)
								{
									/* function returns the address if the token, hence, point to the next */
									MiscFunctions_MemClear(&ShellArgs[ArgsCounter][0],SHELL_COMMAND_ARGS_SIZE_MAX);
									MiscFunctions_StringCopyUntilToken(ArgPosition, &ShellArgs[ArgsCounter][0], ' ');
									ShellArgsList[ArgsCounter] = &ShellArgs[ArgsCounter][0];

									ArgLastPosition = ArgPosition;
									ArgsCounter++;
								}
								else
								{
									break;
								}
							}

							CommandStatus = ApplicationCommands[CommandOffset].Command(&ShellArgsList[0],ArgsCounter);
						}
						else
						{
							CommandStatus = ApplicationCommands[CommandOffset].Command(NULL,0);
						}

						if(SHELL_COMMAND_DONE == CommandStatus)
						{
							SHELL_NEW_LINE;
							SHELL_PROMPT;
						}
						else
						{
							SET_FLAG(ShellStatus,SHELL_COMMAND_PENDING_FLAG);
						}
					}
					else
					{
						Shell_WriteString("\n\rCommand Not Found\n\r");
						SHELL_NEW_LINE;
						SHELL_PROMPT;
					}

					/* once command is done, refresh everything */
					EofCounter = 0;
					ShellCommandCurrentOffset = 0;

					MiscFunctions_MemClear(&ShellCommandBuffer[0],SHELL_COMMAND_SIZE_MAX);
				}
				else
				{
					SHELL_NEW_LINE;
					SHELL_PROMPT;
					EofCounter = 0;
					ShellCommandCurrentOffset = 0;

					MiscFunctions_MemClear(&ShellCommandBuffer[0],SHELL_COMMAND_SIZE_MAX);
				}
			}
			else if(NewCharacterRecevied == 0x8)
			{
				if(ShellCommandCurrentOffset)
				{
					ShellCommandCurrentOffset--;
					ShellCommandBuffer[ShellCommandCurrentOffset] = 0;

					SHELL_BACKSPACE;
				}
			}
			else
			{
				ShellCommandBuffer[ShellCommandCurrentOffset] = NewCharacterRecevied;
				ShellCommandCurrentOffset++;
				Shell_WriteCharacter(NewCharacterRecevied);
			}
		}
	}
}

static void Shell_ReceiveCallback (uint8_t NewReceivedData)
{
	NewCharacterRecevied = NewReceivedData;

	SET_FLAG(ShellStatus,SHELL_CHARACTER_RECEIVED_FLAG);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////


