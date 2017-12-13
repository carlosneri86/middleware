/*HEADER******************************************************************************************
* File name: MiscFunctions.c
* Date: Jun 3, 2016
* Author: Carlos Neri
*
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

void MiscFunctions_MemCopy(const uint8_t * Source, uint8_t * Destination, uint16_t DataSize)
{
	uint32_t * SourceAsWord = (uint32_t*)Source;
	uint32_t * DestinationAsWord = (uint32_t*)Destination;
	uint32_t DataOffset = 0;
	uint8_t AddressModulo;

	while(DataSize)
	{
		AddressModulo = (uint32_t)&Destination[DataOffset] % 4;
		AddressModulo |= (uint32_t)&Source[DataOffset] % 4;

		if((DataSize >= 4)&&(!AddressModulo))
		{
			DestinationAsWord[DataOffset/4] = SourceAsWord[DataOffset/4];
			DataOffset += 4;
			DataSize -= 4;
		}
		else
		{
			Destination[DataOffset] = Source[DataOffset];
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

void MiscFunctions_MemClear(uint8_t * Source, uint16_t DataSize)
{
	uint32_t DataOffset = 0;

	while(DataSize--)
	{
		Source[DataOffset] = 0;
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
