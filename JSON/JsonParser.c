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
#include <string.h>
#include "fsl_common.h"
#include "MiscFunctions.h"
#include "JsonParser.h"

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

status_t JsonParser_Init(jsonparser_t * InitInstance)
{
	assert(InitInstance);

	jsmn_init(&InitInstance->Parser);

	MiscFunctions_MemClear(&InitInstance->ParsedJson[0],JSON_PARSER_MAX_TOKENS * sizeof(InitInstance->ParsedJson[0]));

	return kStatus_Success;
}

status_t JsonParser_Parse(jsonparser_t * Instance, uint8_t * InputBuffer, uint16_t BufferSize)
{
	status_t Status = kStatus_Fail;
	int32_t TokenCount;

	assert(Instance);
	assert(InputBuffer);

	/* just parse the received json */

	if(BufferSize)
	{
		TokenCount = jsmn_parse(&Instance->Parser, (char*)InputBuffer, BufferSize, &Instance->ParsedJson[0], \
				SIZE_OF_ARRAY(Instance->ParsedJson));

		if(TokenCount > 0)
		{
			Instance->Tokens = TokenCount;
			Instance->JsonBuffer = InputBuffer;
			Status = kStatus_Success;
		}
	}

	return Status;
}

/* get token index */
int32_t JsonParser_GetTokenIndex(jsonparser_t * Instance, uint8_t * const TokenToFind)
{
	int32_t FoundToken = -1;
	uint16_t TokenOffset = 0;
	uint16_t TokenStart;
	uint16_t TokenSize;
	uint8_t StringStatus;

	assert(Instance);
	assert(TokenToFind);

	/* sweep the string tokens until the requested one is found */

	/* only sweep when there are tokens */
	if(Instance->Tokens)
	{
		TokenSize = strlen((char*)TokenToFind);

		if(TokenSize > 0)
		{

			while(TokenOffset < Instance->Tokens)
			{

				if(Instance->ParsedJson[TokenOffset].type == JSMN_STRING)
				{
					TokenStart = Instance->ParsedJson[TokenOffset].start;

					StringStatus = MiscFunction_StringCompare(TokenToFind,&Instance->JsonBuffer[TokenStart],TokenSize);

					if(StringStatus == STRING_OK)
					{
						FoundToken = TokenOffset;
						break;
					}
				}

				TokenOffset++;
			}
		}
	}

	return FoundToken;
}

status_t JasonParser_GetStringToken(jsonparser_t * Instance, int32_t TokenIndex, uint8_t * Buffer)
{
	status_t Status = kStatus_Fail;
	uint16_t StringSize;
	uint16_t TokenOffset;

	assert(Instance);
	assert(Buffer);

	if(TokenIndex > 0)
	{
		if(Instance->ParsedJson[TokenIndex].type == JSMN_STRING)
		{
			StringSize = Instance->ParsedJson[TokenIndex].end -  Instance->ParsedJson[TokenIndex].start;

			TokenOffset = Instance->ParsedJson[TokenIndex].start;

			MiscFunctions_MemClear((uint8_t*)Buffer, StringSize + 1);

			strncpy((char*)Buffer,(char*)&Instance->JsonBuffer[TokenOffset],StringSize);

			Status = kStatus_Success;
		}
	}

	return Status;

}

status_t JasonParser_GetIntegerToken(jsonparser_t * Instance, int32_t TokenIndex, uint32_t * TokenDataBuffer)
{
	status_t Status = kStatus_Fail;
	uint8_t * StringRemainder;
	uint16_t TokenOffset;

	assert(Instance);
	assert(TokenDataBuffer);

	if(TokenIndex > 0)
	{
		if(Instance->ParsedJson[TokenIndex].type == JSMN_PRIMITIVE)
		{

			TokenOffset = Instance->ParsedJson[TokenIndex].start;

			*TokenDataBuffer = strtoul((char*)&Instance->JsonBuffer[TokenOffset], (char**)&StringRemainder, 10);

			Status = kStatus_Success;
		}
	}

	return Status;

}

/* EOF */
