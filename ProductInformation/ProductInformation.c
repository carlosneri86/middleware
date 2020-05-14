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

#include "Flash.h"
#include "CRC.h"
#include "MiscFunctions.h"
#include "ProductInformation.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#define PRODUCT_INFO_CRC_SIZE		(sizeof(productinfo_t) - 4)

#define PRODUCT_INFO_SIZE			(sizeof(productinfo_t))

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

static const char ProductInfoTag[12] =
{
		"PRODUCT_INFO"
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////

extern uint32_t PRODUCT_DATA_BASE_ADDR[];

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////

void ProductInformation_ReadData(productinfo_t * ProductInfo)
{
	productinfo_t * CurrentInfo = (productinfo_t *)PRODUCT_DATA_BASE_ADDR;
	crc_t CurrentCrc;
	uint8_t HeaderCheck;

	/* First check if the header matches what we expect. */
	HeaderCheck = MiscFunction_StringCompare((const uint8_t *)&ProductInfoTag[0], (const uint8_t*)&CurrentInfo->DataHeader[0], 12);

	if(HeaderCheck == STRING_OK)
	{
		/* now verify the CRC is valid */
		CurrentCrc = Crc_FastCalculation((uint8_t* const)&CurrentInfo->DataHeader[0],PRODUCT_INFO_CRC_SIZE);

		if(CurrentCrc == CurrentInfo->InfoCrc)
		{
			/* everything matches, copy the information */
			MiscFunctions_MemCopy(&CurrentInfo->DataHeader[0], &ProductInfo->DataHeader[0], PRODUCT_INFO_SIZE);
		}
		else
		{
			MiscFunctions_MemClear(&ProductInfo->DataHeader[0], PRODUCT_INFO_SIZE);
		}
	}
	else
	{
		MiscFunctions_MemClear(&ProductInfo->DataHeader[0], PRODUCT_INFO_SIZE);
	}

}

bool ProductInformation_WriteData(uint8_t * ProjectName, uint16_t NameSize, uint32_t Version, uint8_t * CommitHash)
{
	productinfo_t DataToSave;
	status_t Status;
	crc_t DataCrc;
	bool Result = true;

	Flash_Init();


	MiscFunctions_MemClear(&DataToSave.DataHeader[0], PRODUCT_INFO_SIZE);

	MiscFunctions_MemCopy(&ProductInfoTag[0], &DataToSave.DataHeader[0], 12);

	if(ProjectName != NULL)
	{
		if(NameSize > PRODUCT_INFO_PROJECT_NAME_MAX_SIZE)
		{
			NameSize = PRODUCT_INFO_PROJECT_NAME_MAX_SIZE;
		}

		MiscFunctions_MemCopy(ProjectName, &DataToSave.ProjectName[0], NameSize);
	}
	else
	{
		MiscFunctions_MemClear(&DataToSave.ProjectName[0], PRODUCT_INFO_PROJECT_NAME_MAX_SIZE);
	}

	DataToSave.ProjectVersion = Version;

	if(CommitHash != NULL)
	{
		MiscFunctions_MemCopy(CommitHash, &DataToSave.ProjectCommitHash[0], 40);
	}
	else
	{
		MiscFunctions_MemClear(&DataToSave.ProjectCommitHash[0], 40);
	}

	DataCrc = Crc_FastCalculation(&DataToSave.DataHeader[0],PRODUCT_INFO_CRC_SIZE);

	DataToSave.InfoCrc = (uint16_t)DataCrc;


	Status = Flash_Erase((uint32_t)PRODUCT_DATA_BASE_ADDR,PRODUCT_INFO_SIZE);

	if (Status == kStatus_Success)
	{
		Status = Flash_Write(&DataToSave.DataHeader[0],(uint32_t)PRODUCT_DATA_BASE_ADDR,PRODUCT_INFO_SIZE);

		if (Status != kStatus_Success)
		{
			Result = false;
		}
	}

	return Result;

}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////
