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
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "fsl_tpm.h"
#include "AtCommandsPlatform.h"
#include "DebugPins.h"
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

void AtCommandsPlatform_Callback(LPUART_Type *base, lpuart_handle_t *handle, status_t status, void *userData);

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////
lpuart_handle_t UartHandle;

lpuart_transfer_t TxUartTransfer;

lpuart_transfer_t RxUartTransfer;

uint8_t DataReceived;

volatile bool isDataReady;

uint16_t ErrorCounter = 0;

static AtCommandsPlatformCallback_t ReportDataCallback = NULL;

static AtCommandsPlatformTimeoutCallback_t ReportTimeoutCallback = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////


void AtCommands_PlatformUartInit (uint32_t BaudRate, AtCommandsPlatformCallback_t Callback)
{
	lpuart_config_t config;
	uint32_t ClockFrequency;

	BOARD_InitEsp8266();

	ReportDataCallback = Callback;

	LPUART_GetDefaultConfig(&config);

    config.baudRate_Bps = BaudRate;
    config.enableTx = true;
    config.enableRx = true;

    ClockFrequency = CLOCK_GetFreq(kCLOCK_McgInternalRefClk);

    LPUART_Init(AT_COMMANS_PLAT_UART, &config, ClockFrequency);

    LPUART_TransferCreateHandle(AT_COMMANS_PLAT_UART, &UartHandle, AtCommandsPlatform_Callback, NULL);

    RxUartTransfer.data = &DataReceived;
    RxUartTransfer.dataSize = 1;

    LPUART_TransferReceiveNonBlocking(AT_COMMANS_PLAT_UART, &UartHandle, &RxUartTransfer, NULL);
}

void AtCommands_PlatformUartSend(uint8_t * CommandBuffer, uint16_t BufferSize)
{
	TxUartTransfer.data = CommandBuffer;
	TxUartTransfer.dataSize = BufferSize;

	LPUART_TransferSendNonBlocking(AT_COMMANS_PLAT_UART, &UartHandle, &TxUartTransfer);
}

uint8_t AtCommands_PlatformUartRead (void)
{
	isDataReady = false;
	LPUART_TransferReceiveNonBlocking(AT_COMMANS_PLAT_UART, &UartHandle, &RxUartTransfer, NULL);

	return DataReceived;
}

AtCommandsPlatformStatus_t AtCommands_PlatformUartRxStatus(uint8_t * NewData)
{
	AtCommandsPlatformStatus_t Status = ATCOMMANDS_PLATFORM_ERROR;

	if(isDataReady == true)
	{
		*NewData = DataReceived;
		isDataReady = false;
		LPUART_TransferReceiveNonBlocking(AT_COMMANS_PLAT_UART, &UartHandle, &RxUartTransfer, NULL);
		Status = ATCOMMANDS_PLATFORM_DATA_RECEIVED;
	}

	return Status;
}

void AtCommands_PlatformUartEnableRx(bool isEnabled)
{
	LPUART_EnableRx(AT_COMMANS_PLAT_UART,isEnabled);

	if(isEnabled)
	{
		PORT_SetPinMux(BOARD_INITESP8266_ESP8266_RX_PORT, BOARD_INITESP8266_ESP8266_RX_PIN, kPORT_MuxAlt2);
	}
	else
	{
		PORT_SetPinMux(BOARD_INITESP8266_ESP8266_RX_PORT, BOARD_INITESP8266_ESP8266_RX_PIN, kPORT_PinDisabledOrAnalog);
	}
}

void AtCommands_PlatformAssertReset(void)
{
	GPIO_PinWrite(BOARD_INITESP8266_ESP8266_RESET_GPIO, BOARD_INITESP8266_ESP8266_RESET_PIN, 0);
}

void AtCommands_PlatformDeassertReset(void)
{
	GPIO_PinWrite(BOARD_INITESP8266_ESP8266_RESET_GPIO, BOARD_INITESP8266_ESP8266_RESET_PIN, 1);
}

void AtCommands_PlatformUartEnableTx(bool isEnabled)
{
	LPUART_EnableTx(AT_COMMANS_PLAT_UART,isEnabled);

	if(isEnabled)
	{
		PORT_SetPinMux(BOARD_INITESP8266_ESP8266_TX_PORT, BOARD_INITESP8266_ESP8266_TX_PIN, kPORT_MuxAlt2);
	}
	else
	{
		PORT_SetPinMux(BOARD_INITESP8266_ESP8266_TX_PORT, BOARD_INITESP8266_ESP8266_TX_PIN, kPORT_PinDisabledOrAnalog);
	}
}

void AtCommandsPlatform_Callback(LPUART_Type *base, lpuart_handle_t *handle, status_t status, void *userData)
{
	if(kStatus_LPUART_RxIdle == status)
	{
		isDataReady = true;
		ReportDataCallback(DataReceived);
		LPUART_TransferReceiveNonBlocking(AT_COMMANS_PLAT_UART, &UartHandle, &RxUartTransfer, NULL);
	}
	if(kStatus_LPUART_RxHardwareOverrun == status)
	{
		LPUART_ClearStatusFlags(AT_COMMANS_PLAT_UART,kLPUART_RxOverrunFlag);
		ErrorCounter++;
	}
}

void AtCommand_PlatformCharacterTimeoutInit(AtCommandsPlatformTimeoutCallback_t Callback, uint32_t Timeout)
{
	tpm_config_t TpmInfo;
	uint32_t TpmClock;

	/* using TPM as character timeout to enable low power */
	/* allows SW timer to extend its period as this timer is only used when UART COMMs happen */
	/* which is not that often */

	TPM_GetDefaultConfig(&TpmInfo);

	TpmInfo.prescale = kTPM_Prescale_Divide_4;

	TPM_Init(AT_COMMANDS_PLAT_TIMER, &TpmInfo);

	TpmClock = CLOCK_GetFreq(kCLOCK_Osc0ErClk);

	TpmClock /= 4;

    TPM_SetTimerPeriod(AT_COMMANDS_PLAT_TIMER, MSEC_TO_COUNT(Timeout, TpmClock));

    TPM_EnableInterrupts(AT_COMMANDS_PLAT_TIMER, kTPM_TimeOverflowInterruptEnable);

    EnableIRQ(TPM1_IRQn);

    ReportTimeoutCallback = Callback;
}

void AtCommand_PlatformCharacterTimeoutStart(void)
{

	TPM_StartTimer(AT_COMMANDS_PLAT_TIMER, kTPM_SystemClock);

}

 void AtCommand_PlatformCharacterTimeoutStop(void)
{
	TPM_StopTimer(AT_COMMANDS_PLAT_TIMER);
}

 void AtCommand_PlatformCharacterTimeoutRefresh(void)
{
	/* to refresh, stop the timer, write CNT to clear it and re-start the timer */
	AtCommand_PlatformCharacterTimeoutStop();

	AT_COMMANDS_PLAT_TIMER->CNT = 0;

	AtCommand_PlatformCharacterTimeoutStart();
}

void TPM1_IRQHandler(void)
{
	/* Clear interrupt flag.*/
	TPM_ClearStatusFlags(AT_COMMANDS_PLAT_TIMER, kTPM_TimeOverflowFlag);

	ReportTimeoutCallback();
}

/* EOF */
