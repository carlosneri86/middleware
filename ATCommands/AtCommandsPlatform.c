/*HEADER******************************************************************************************
* Filename: AtCommands.c
* Date: Mar 12, 2016
* Author: Carlos Neri
*
**END********************************************************************************************/
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Includes Section
///////////////////////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "fsl_lpsci.h"
#include "AtCommandsPlatform.h"
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

void AtCommandsUartPlatformCallback(UART0_Type *base, lpsci_handle_t *handle, status_t status, void *userData);

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////
lpsci_handle_t UartHandle;

lpsci_transfer_t TxUartTransfer;

lpsci_transfer_t RxUartTransfer;

uint8_t DataReceived;

volatile bool isDataReady;

uint16_t ErrorCounter = 0;

static AtCommandsPlatformCallback_t ReportDataCallback = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////


void AtCommands_PlatformUartInit (uint32_t BaudRate, AtCommandsPlatformCallback_t Callback)
{
	lpsci_config_t config;
	uint32_t ClockFrequency;

	ReportDataCallback = Callback;

	CLOCK_SetLpsci0Clock(0x1U);

    LPSCI_GetDefaultConfig(&config);

    config.baudRate_Bps = BaudRate;
    config.enableTx = true;
    config.enableRx = true;

    ClockFrequency = CLOCK_GetFreq(kCLOCK_CoreSysClk);

    LPSCI_Init(UART0, &config, ClockFrequency);

    LPSCI_TransferCreateHandle(UART0, &UartHandle, AtCommandsUartPlatformCallback, NULL);

    RxUartTransfer.data = &DataReceived;
    RxUartTransfer.dataSize = 1;

    LPSCI_TransferReceiveNonBlocking(UART0, &UartHandle, &RxUartTransfer, NULL);
}

void AtCommands_PlatformUartSend(uint8_t * CommandBuffer, uint16_t BufferSize)
{
	TxUartTransfer.data = CommandBuffer;
	TxUartTransfer.dataSize = BufferSize;

	LPSCI_TransferSendNonBlocking(UART0, &UartHandle, &TxUartTransfer);
}

uint8_t AtCommands_PlatformUartRead (void)
{
	isDataReady = false;
	LPSCI_TransferReceiveNonBlocking(UART0, &UartHandle, &RxUartTransfer, NULL);

	return DataReceived;
}

AtCommandsPlatformStatus_t AtCommands_PlatformUartRxStatus(uint8_t * NewData)
{
	AtCommandsPlatformStatus_t Status = ATCOMMANDS_PLATFORM_ERROR;

	if(isDataReady == true)
	{
		*NewData = DataReceived;
		isDataReady = false;
		LPSCI_TransferReceiveNonBlocking(UART0, &UartHandle, &RxUartTransfer, NULL);
		Status = ATCOMMANDS_PLATFORM_DATA_RECEIVED;
	}

	return Status;
}

void AtCommandsUartPlatformCallback(UART0_Type *base, lpsci_handle_t *handle, status_t status, void *userData)
{
	if(kStatus_LPSCI_RxIdle == status)
	{
		isDataReady = true;
		ReportDataCallback(DataReceived);
		LPSCI_TransferReceiveNonBlocking(UART0, &UartHandle, &RxUartTransfer, NULL);
	}
	if(kStatus_LPSCI_RxHardwareOverrun == status)
	{
		LPSCI_ClearStatusFlags(UART0,kLPSCI_RxOverrunFlag);
		ErrorCounter++;
	}
}


/* EOF */
