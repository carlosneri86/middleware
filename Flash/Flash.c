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

#include <stdbool.h>
#include "fsl_flash.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "Flash.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#define FLASH_ENTER_CRITIAL	(taskENTER_CRITICAL())

#define FLASH_EXIT_CRITIAL	(taskEXIT_CRITICAL())

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

static flash_config_t FlashConfig;

static SemaphoreHandle_t FlashMutex;

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////

void Flash_Init(void)
{
	static bool isInitialized = false;

	if(isInitialized == false)
	{
		/* initialize flash */
		(void)FLASH_Init(&FlashConfig);

		FlashMutex = xSemaphoreCreateMutex();

		isInitialized = true;
	}
}

status_t Flash_Erase(uint32_t EraseAddress, uint16_t Bytes)
{
	status_t Status = kStatus_Fail;
	BaseType_t MutexStatus;

	MutexStatus = xSemaphoreTake(FlashMutex, portMAX_DELAY);

	if(MutexStatus == pdTRUE)
	{

		FLASH_ENTER_CRITIAL;

		Status = FLASH_Erase(&FlashConfig,(uint32_t)EraseAddress,Bytes,kFTFx_ApiEraseKey);

		FLASH_EXIT_CRITIAL;

		xSemaphoreGive(FlashMutex);
	}

	return Status;
}

status_t Flash_Write(uint8_t * DataToWrite, uint32_t ProgramAddress, uint16_t Bytes)
{
	status_t Status = kStatus_Fail;
	BaseType_t MutexStatus;

	MutexStatus = xSemaphoreTake(FlashMutex, portMAX_DELAY);

	if(MutexStatus == pdTRUE)
	{

		FLASH_ENTER_CRITIAL;

		Status = FLASH_Program(&FlashConfig,ProgramAddress,DataToWrite,Bytes);

		FLASH_EXIT_CRITIAL;

		xSemaphoreGive(FlashMutex);
	}

	return Status;
}

uint16_t Flash_GetSectorSize(void)
{
	uint32_t SectorSize;

	FLASH_GetProperty(&FlashConfig, kFLASH_PropertyPflash0SectorSize, &SectorSize);

	return (uint16_t)SectorSize;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////