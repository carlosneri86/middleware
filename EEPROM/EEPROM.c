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
#include "EEPROM.h"
#include "MiscFunctions.h"
#include "CRC.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#define EEPROM_HEADER_MAGIC_WORD	(0XDEADBEEF)

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	uint32_t Header;
	uint16_t TagCounter;
	uint16_t Offset;
	uint8_t Data[EEPROM_MAX_ALLOWED_DATA_BYTES];
	uint16_t Crc;
}eeprom_tag_t;


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

static status_t Eeeprom_WriteToFlash(eeprom_tag_t * TagData, uint32_t Address);

static status_t Eeeprom_EraseFlash(void);

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////

extern uint32_t EEPROM_DATA_START_ADDRESS[];

extern uint32_t EEPROM_DATA_END_ADDRESS[];

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////

static eeprom_tag_t ActiveDataSet;

static eeprom_tag_t DataSetToProgram;

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////

void Eeprom_Init(void)
{
	eeprom_tag_t * DataSet = (eeprom_tag_t *)EEPROM_DATA_START_ADDRESS;
	uint16_t DataSetOffset = 0;
	uint16_t LatestRecord = 0;
	crc_t DataSetCrc;

	/* initialize flash */
	Flash_Init();

	MiscFunctions_MemClear(&ActiveDataSet, sizeof(eeprom_tag_t));

	/* sweep the flash looking for a valid tag */
	while(DataSetOffset < EEPROM_MAX_ALLOWED_DATA_SETS)
	{
		/* will assume if is there, then is a valid record */
		if(DataSet[DataSetOffset].Header == EEPROM_HEADER_MAGIC_WORD)
		{
			if(LatestRecord <= DataSet[DataSetOffset].TagCounter)
			{
				/* CRC is only for the data. Accounting for that and padding (hence the extra -2) */
				DataSetCrc = Crc_FastCalculation((uint8_t*)&DataSet[DataSetOffset], (sizeof(eeprom_tag_t) - \
						sizeof(DataSet[DataSetOffset].Crc) - 2));

				if(DataSetCrc == DataSet[DataSetOffset].Crc)
				{
					MiscFunctions_MemCopy(&DataSet[DataSetOffset], &ActiveDataSet, sizeof(eeprom_tag_t));
					LatestRecord = DataSet[DataSetOffset].TagCounter;
				}
			}
		}

		DataSetOffset++;
	}

}

status_t Eeprom_WriteData(void * NewData, uint16_t Size)
{
	uint32_t ProgramAddress;
	status_t Status;
	uint16_t ActiveOffsetBackup;

	assert(NewData != NULL);

	MiscFunctions_MemClear(&DataSetToProgram, sizeof(eeprom_tag_t));

	/* Fill the new data */

	DataSetToProgram.Header = EEPROM_HEADER_MAGIC_WORD;

	/* avoid 0xFFFF */
	DataSetToProgram.TagCounter = ActiveDataSet.TagCounter + 1 != UINT16_MAX ? ActiveDataSet.TagCounter + 1 : 0;

	DataSetToProgram.Offset = (ActiveDataSet.Offset + 1) % EEPROM_MAX_ALLOWED_DATA_SETS;

	MiscFunctions_MemCopy(NewData, &DataSetToProgram.Data[0], Size);

	DataSetToProgram.Crc = Crc_FastCalculation((uint8_t*)&DataSetToProgram, (sizeof(eeprom_tag_t) - \
			sizeof(DataSetToProgram.Crc) - 2));

	if(DataSetToProgram.Offset)
	{
		/* compute the new address offset and then write */
		ProgramAddress = (uint32_t)((uint32_t)EEPROM_DATA_START_ADDRESS + (DataSetToProgram.Offset * \
				sizeof(eeprom_tag_t)));

		Status = Eeeprom_WriteToFlash(&DataSetToProgram, ProgramAddress);
	}
	else
	{
		/* if we just wrapped around, have to erase everything, copy the current */
		/* then the new one														 */
		Eeeprom_EraseFlash();

		/* WARNING: if power goes off at this stage, we may be left without any records	*/
		/* Must implement a 2 pages EEPROM so there's always where to copy from			*/

		ActiveOffsetBackup = ActiveDataSet.Offset;

		ActiveDataSet.Offset = 0;

		ActiveDataSet.Crc = Crc_FastCalculation((uint8_t*)&ActiveDataSet, (sizeof(eeprom_tag_t) - \
							sizeof(ActiveDataSet.Crc) - 2));

		Status = Eeeprom_WriteToFlash(&ActiveDataSet, (uint32_t)EEPROM_DATA_START_ADDRESS);


		if(Status == kStatus_Success)
		{
			DataSetToProgram.Offset = 1;

			DataSetToProgram.Crc = Crc_FastCalculation((uint8_t*)&DataSetToProgram, (sizeof(eeprom_tag_t) - \
						sizeof(DataSetToProgram.Crc) - 2));

			/* compute the new address offset and then write */
			ProgramAddress = (uint32_t)((uint32_t)EEPROM_DATA_START_ADDRESS + sizeof(eeprom_tag_t));

			Status = Eeeprom_WriteToFlash(&DataSetToProgram, ProgramAddress);
		}
		else
		{
			ActiveDataSet.Offset = ActiveOffsetBackup;
		}
	}

	/* only mark it as the new if last programming was successful */
	if(Status == kStatus_Success)
	{
		ActiveDataSet = DataSetToProgram;
	}

	return Status;
}

status_t Eeprom_ReadData(uint8_t * Buffer, uint16_t DataToCopy)
{
	status_t Status = kStatus_Fail;

	/* just make sure the active data set is valid */
	/* and truncate to max bytes if needed			*/
	if(ActiveDataSet.Header == EEPROM_HEADER_MAGIC_WORD)
	{
		if(DataToCopy > EEPROM_MAX_ALLOWED_DATA_BYTES)
		{
			DataToCopy = EEPROM_MAX_ALLOWED_DATA_BYTES;
		}

		MiscFunctions_MemCopy(&ActiveDataSet.Data[0], Buffer, DataToCopy);

		Status = kStatus_Success;
	}

	return Status;
}

static status_t Eeeprom_WriteToFlash(eeprom_tag_t * TagData, uint32_t Address)
{
	return Flash_Write((uint8_t *)TagData, Address, sizeof(eeprom_tag_t));
}

static status_t Eeeprom_EraseFlash(void)
{
	status_t Status;
	uint32_t AddressToErase;
	uint16_t TotalDataSetSize;
	uint16_t SectorsToErase;
	uint16_t RemainingSectorBytes;
	uint16_t SectorOffset = 0;
	uint16_t SectorSize;

	SectorSize = Flash_GetSectorSize();

	TotalDataSetSize = (sizeof(eeprom_tag_t) * EEPROM_MAX_ALLOWED_DATA_SETS);

	SectorsToErase = TotalDataSetSize / SectorSize;

	RemainingSectorBytes = TotalDataSetSize % SectorSize;

	/* make sure all sectors to be erased are accounted for */
	if(RemainingSectorBytes)
	{
		SectorsToErase += 1;
	}

	/* erase sector by sector */
	while(SectorOffset < SectorsToErase)
	{
		AddressToErase = (uint32_t)((uint32_t)EEPROM_DATA_START_ADDRESS + (SectorSize * SectorOffset));

		Status = Flash_Erase(AddressToErase,SectorSize);

		if(Status == kStatus_Success)
		{
			SectorOffset++;
		}
		else
		{
			break;
		}
	}

	return Status;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////
