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
#include "fsl_port.h"
#include "pin_mux.h"
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

void AtCommandsPlatform_Callback(UART_Type *base, uart_handle_t *handle, status_t status, void *userData);

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////
uart_handle_t UartHandle;

uart_transfer_t TxUartTransfer;

uart_transfer_t RxUartTransfer;

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
	uart_config_t config;
	uint32_t ClockFrequency;

	BOARD_InitEsp8266();

	ReportDataCallback = Callback;

    UART_GetDefaultConfig(&config);

    config.baudRate_Bps = BaudRate;
    config.enableTx = true;
    config.enableRx = true;

    ClockFrequency = CLOCK_GetFreq(kCLOCK_BusClk);

    UART_Init(AT_COMMANS_PLAT_UART, &config, ClockFrequency);

    UART_TransferCreateHandle(AT_COMMANS_PLAT_UART, &UartHandle, AtCommandsPlatform_Callback, NULL);

    RxUartTransfer.data = &DataReceived;
    RxUartTransfer.dataSize = 1;

    UART_TransferReceiveNonBlocking(AT_COMMANS_PLAT_UART, &UartHandle, &RxUartTransfer, NULL);
}

void AtCommands_PlatformUartSend(uint8_t * CommandBuffer, uint16_t BufferSize)
{
	TxUartTransfer.data = CommandBuffer;
	TxUartTransfer.dataSize = BufferSize;

	UART_TransferSendNonBlocking(AT_COMMANS_PLAT_UART, &UartHandle, &TxUartTransfer);
}

uint8_t AtCommands_PlatformUartRead (void)
{
	isDataReady = false;
	UART_TransferReceiveNonBlocking(AT_COMMANS_PLAT_UART, &UartHandle, &RxUartTransfer, NULL);

	return DataReceived;
}

AtCommandsPlatformStatus_t AtCommands_PlatformUartRxStatus(uint8_t * NewData)
{
	AtCommandsPlatformStatus_t Status = ATCOMMANDS_PLATFORM_ERROR;

	if(isDataReady == true)
	{
		*NewData = DataReceived;
		isDataReady = false;
		UART_TransferReceiveNonBlocking(AT_COMMANS_PLAT_UART, &UartHandle, &RxUartTransfer, NULL);
		Status = ATCOMMANDS_PLATFORM_DATA_RECEIVED;
	}

	return Status;
}

void AtCommands_PlatformUartEnableRx(bool isEnabled)
{
	UART_EnableRx(AT_COMMANS_PLAT_UART,isEnabled);

	if(isEnabled)
	{
		PORT_SetPinMux(BOARD_INITESP8266_LCD_P42_PORT, BOARD_INITESP8266_LCD_P42_PIN, kPORT_MuxAlt3);
	}
	else
	{
		PORT_SetPinMux(BOARD_INITESP8266_LCD_P42_PORT, BOARD_INITESP8266_LCD_P42_PIN, kPORT_PinDisabledOrAnalog);
	}
}

void AtCommands_PlatformUartEnableTx(bool isEnabled)
{
	UART_EnableTx(AT_COMMANS_PLAT_UART,isEnabled);

	if(isEnabled)
	{
		PORT_SetPinMux(BOARD_INITESP8266_LCD_P43_PORT, BOARD_INITESP8266_LCD_P43_PIN, kPORT_MuxAlt3);
	}
	else
	{
		PORT_SetPinMux(BOARD_INITESP8266_LCD_P43_PORT, BOARD_INITESP8266_LCD_P43_PIN, kPORT_PinDisabledOrAnalog);
	}
}

void AtCommandsPlatform_Callback(UART_Type *base, uart_handle_t *handle, status_t status, void *userData)
{
	if(kStatus_UART_RxIdle == status)
	{
		isDataReady = true;
		ReportDataCallback(DataReceived);
		UART_TransferReceiveNonBlocking(AT_COMMANS_PLAT_UART, &UartHandle, &RxUartTransfer, NULL);
	}
	if(kStatus_UART_RxHardwareOverrun == status)
	{
		UART_ClearStatusFlags(AT_COMMANS_PLAT_UART,kUART_RxOverrunFlag);
		ErrorCounter++;
	}
}

void UART2_FLEXIO_IRQHandler(void)
{
    UART_TransferHandleIRQ(UART2, &UartHandle);
}
/* EOF */
