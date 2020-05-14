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
#include "MiscFunctions.h"
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
uint8_t MiscFunction_StringCompare(const uint8_t * StringBase, const uint8_t * StringToCompare, uint16_t AmountOfCharacters)
{
	uint8_t bStatus = STRING_OK;
	uint8_t bStringOffset = 0;

	while(AmountOfCharacters--)
	{
		/* Compare each character, early exit just after a character mismatch */
		if(StringBase[bStringOffset] != StringToCompare[bStringOffset])
		{
			bStatus = STRING_ERROR;
			AmountOfCharacters = 0;
		}
		bStringOffset++;
	}

	return (bStatus);
}

void MiscFunctions_MemCopy(const void * Source, void * Destination, uint16_t DataSize)
{
	uint8_t * SourceAsByte = (uint8_t*)Source;
	uint8_t * DestinationAsByte = (uint8_t*)Destination;
	uint32_t * SourceAsWord = (uint32_t*)Source;
	uint32_t * DestinationAsWord = (uint32_t*)Destination;
	uint32_t DataOffset = 0;
	uint8_t AddressModulo;

	while(DataSize)
	{
		AddressModulo = (uint32_t)&DestinationAsByte[DataOffset] % 4;
		AddressModulo |= (uint32_t)&SourceAsByte[DataOffset] % 4;

		if((DataSize >= 4)&&(!AddressModulo))
		{
			DestinationAsWord[DataOffset/4] = SourceAsWord[DataOffset/4];
			DataOffset += 4;
			DataSize -= 4;
		}
		else
		{
			DestinationAsByte[DataOffset] = SourceAsByte[DataOffset];
			DataOffset += 1;
			DataSize --;
		}
	}
}

uint8_t MiscFunctions_SearchInString(const uint8_t * Source, uint16_t SourceSize, const uint8_t * StringToSearch, uint16_t StringToSearchSize)
{
	uint16_t SourceOffset = 0;
	uint16_t StringToSearchOffset = 0;
	uint16_t Status = STRING_ERROR;

	(void)SourceSize;
	while(Source[SourceOffset] != '\0')
	{
		if(Source[SourceOffset] == StringToSearch[StringToSearchOffset])
		{
			StringToSearchOffset++;
		}
		else
		{
			StringToSearchOffset = 0;
		}

		if(StringToSearchOffset == StringToSearchSize)
		{
			Status = STRING_OK;
			break;
		}

		SourceOffset++;
	}

	return(Status);
}

uint8_t MiscFunctions_StringCopyUntilToken(const uint8_t * Source, uint8_t * Destination, uint8_t Token)
{
	uint8_t SourceOffset = 0;

	while((Source[SourceOffset] != Token) && (Source[SourceOffset] != '\0'))
	{
		Destination[SourceOffset] = Source[SourceOffset];

		SourceOffset++;
	}

	return SourceOffset;
}

void MiscFunctions_MemClear(void * Source, uint16_t DataSize)
{
	uint32_t DataOffset = 0;
	uint8_t * SourceData = (uint8_t *)Source;
	while(DataSize--)
	{
		SourceData[DataOffset] = 0;
		DataOffset++;

	}
}


uint16_t MiscFunctions_IntegerToAscii(uint32_t Data, uint8_t *AsciiBuffer)
{
	uint32_t CurrentValue = Data;
	uint32_t DataToConvert;
	uint16_t StringSize = 0;

	if(Data)
	{

		while(CurrentValue)
		{
			DataToConvert = CurrentValue % 10;
			AsciiBuffer[StringSize] = '0' + DataToConvert;
			StringSize++;
			CurrentValue /= 10;
		}
	}
	else
	{
		AsciiBuffer[0] = '0';
		StringSize = 1;
	}

	MiscFunctions_StringReverse(AsciiBuffer);

	/* add null terminator */
	AsciiBuffer[StringSize]= '\0';

	return (StringSize);
}

void MiscFunctions_StringReverse(uint8_t * StringToReverse)
{
	uint8_t PreviousData;
	uint16_t StartIndex = 0;
	uint16_t EndIndex;

	EndIndex = strlen((char*)StringToReverse) - 1U;

	if(EndIndex >= 1)
	{
		while(StartIndex < EndIndex)
		{
			PreviousData = StringToReverse[StartIndex];
			StringToReverse[StartIndex] = StringToReverse[EndIndex];
			StringToReverse[EndIndex] = PreviousData;

			StartIndex++;
			EndIndex--;
		}
	}

}

uint8_t * MiscFunctions_FindTokenInString(uint8_t * StringToSearch, uint8_t Token)
{
	uint8_t StringOffset = 0;
	uint8_t * StringAtToken = NULL;

	while(StringToSearch[StringOffset] != '\0')
	{
		if(StringToSearch[StringOffset] == Token)
		{
			StringAtToken = &StringToSearch[StringOffset + 1];
			break;
		}

		StringOffset++;
	}

	return StringAtToken;

}

uint32_t MiscFunctions_AsciiToUnsignedInteger(uint8_t *AsciiString)
{
	uint32_t NewData = 0;
	uint16_t StringOffset = 0;

	/* keep converting until a non-number ascii is found */
	while((AsciiString[StringOffset] >= '0') && (AsciiString[StringOffset] <= '9'))
	{
		NewData = (NewData * 10) + (AsciiString[StringOffset] - '0');
		StringOffset++;
	}

	return NewData;
}

uint8_t ReverseBitsInByte(uint8_t DataToReverse)
{
	uint8_t ReversedBits = 0;

	ReversedBits = (uint8_t)(((DataToReverse * 0x0802U & 0x22110U) | (DataToReverse * 0x8020U & 0x88440U)) * 0x10101U >> 16);


	return ReversedBits;
}

__attribute__((naked))
void MiscFunctions_BlockingDelay(uint32_t TargetDelay)
{
	/* The delay is at R0 */
	/* divide by 2 since delay is 2 instructions */
	__asm(  ".syntax unified \n"
			"LSRS R0, #1 \n"
			"DELAY: \n"
			"SUBS R0, #1 \n"
			"BNE DELAY \n"
			"BX LR \n");
}
