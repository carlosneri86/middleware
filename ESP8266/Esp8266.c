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
#ifdef FSL_RTOS_FREE_RTOS
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#endif
#include "MiscFunctions.h"
#include "AtCommands.h"
#include "RingBuffer.h"
#include "SW_Timer.h"
#include "state_machine.h"
#include "Esp8266.h"
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#define	PARAMETERS_BUFFER_SIZE				(512)

#define APMODE_DEFAULT_CHANNEL				(11)

#define ESP8266_TIMEOUT						(100)

#define ESP8266_RESET_TIMER					(1000)

#define ESP8266_DISCONNECT_COUNTER			(5)

#ifdef FSL_RTOS_FREE_RTOS
#define ESP8266_STACK_SIZE					(256)

#define ESP8266_TASK_PRIORITY				(configMAX_PRIORITIES - 3)

#define ESP8266_SELF_EVENT					(1 << 0)

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                       Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum
{
	ESP8266_STATUS_SERVER_ENABLED = 0,
	ESP8266_STATUS_AP_ENABLED,
	ESP8266_STATUS_WIFI_CONNECTED,
	ESP8266_STATUS_MUX_ENABLED
}esp8266_internal_commands_status_t;

typedef enum
{
	ESP8266_SM_PROCESSING = 0,
	ESP8266_WIFI_CONNECTED,
	ESP8266_COMMAND_GENERATE_EVENT,
	ESP8266_OK_RECEIVED,
	ESP8366_COMMAND_TIMEOUT_EVENT,
	ESP8366_START_TIMER_EVENT
}esp8266_commands_status_t;

typedef enum
{
	ESP8266_IDLE_STATE,
	ESP8266_WAIT_OK_STATE,
	ESP8266_START_SERVER_STATE,
	ESP8266_SEND_DATA_STATE,
	ESP8266_TIMEOUT_STATE,
	ESP8266_INIT_DONE_STATE,
	ESP8266_ENABLE_MULT_CONNECTIONS_STATE,
	ESP8266_DISABLE_ECHO_STATE,
	ESP8266_MAX_STATE
}esp8266_states_t;

enum esp8266_commands_t
{
	ESP8266_DISABLE_ECHO_COMMAND = 0,
	ESP8266_RESET_COMMAND,
	ESP8266_RESTORE_COMMAND,
	ESP8266_MODE_COMMAND,
	ESP8266_CONNECT_NWK_COMMAND,
	ESP8266_GET_NWKS_COMMAND,
	ESP8266_DISCONNECT_NWK_COMMAND,
	ESP8266_CONFIG_AP_COMMAND,
	ESP8266_CONFIG_DHCP_COMMAND,
	ESP8266_SET_CONNECTIONS_COMMAND,
	ESP8266_START_SERVER_COMMAND,
	ESP8266_CLOSE_SOCKET_COMMAND,
	ESP8266_SOCKET_SEND_COMMAND,
	ESP8266_CONNECT_TO_SERVER_COMMAND,
	ESP8266_AUTO_CONNECT_COMMAND,
	ESP8266_POLL_STATUS_COMMAND,
	ESP8266_MAX_COMMAND
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

static void Esp8266_AtCommandsCallback(AtCommandsEvent_t Event, uint8_t*Data, uint16_t DataSize);

static bool AtCommand_OkCallback(uint8_t * Parameters, uint16_t ParametersSize);

bool AtCommand_ErrorCallback(uint8_t * Parameters, uint16_t ParametersSize);

void AtCommands_Callback(AtCommandsEvent_t Event, uint8_t* Data, uint16_t DataSize);

bool AtCommand_WifiConnectedCallbak(uint8_t * Parameters, uint16_t ParametersSize);

bool AtCommand_WifiDisconnectedCallbak(uint8_t * Parameters, uint16_t ParametersSize);

bool AtCommand_WifiGotIpCallbak(uint8_t * Parameters, uint16_t ParametersSize);

bool AtCommand_EchoOffCallbak(uint8_t * Parameters, uint16_t ParametersSize);

bool AtCommand_TcpDataReceivedCallbak(uint8_t * Parameters, uint16_t ParametersSize);

bool AtCommand_TcpSendOkCallbak(uint8_t * Parameters, uint16_t ParametersSize);

bool AtCommand_FailCallbak(uint8_t * Parameters, uint16_t ParametersSize);

bool AtCommand_JoinStatus(uint8_t * Parameters, uint16_t ParametersSize);

bool AtCommand_TcpSendDataCallback(uint8_t * Parameters, uint16_t ParametersSize);

void Esp8266_TimerCallback (void * Args);

void Esp8266_ResetTimerCallback(void * Args);

static void Esp8266_IdleState(void);

static void Esp8266_WaitOkState(void);

static void Esp8266_StartServerState(void);

static void Esp8266_SendTcpDataState(void);

static void Esp8266_TimeoutState(void);

static void Esp8266_InitDoneState(void);

static void Esp8266_EnableMultipleConnectionsState(void);

static void Esp8266_DisableEchoState(void);


static void (* Esp8266_StateMachineFunctions[ESP8266_MAX_STATE])(void) =
{
		Esp8266_IdleState,
		Esp8266_WaitOkState,
		Esp8266_StartServerState,
		Esp8266_SendTcpDataState,
		Esp8266_TimeoutState,
		Esp8266_InitDoneState,
		Esp8266_EnableMultipleConnectionsState,
		Esp8266_DisableEchoState
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////

const uint8_t * AtCommandTable[ESP8266_MAX_COMMAND] =
{
		(uint8_t *)"ATE0",				/* 0 - disables echo 						*/
		(uint8_t *)"RST",				/* 1 - resets esp8266 						*/
		(uint8_t *)"RESOTRE",			/* 2 - restore factory settings				*/
		(uint8_t *)"CWMODE_CUR",		/* 3 - AP mode 								*/
		(uint8_t *)"CWJAP_CUR",			/* 4 - Connect to network					*/
		(uint8_t *)"CWLAP",				/* 5 - list available networks				*/
		(uint8_t *)"CWQAP",				/* 6 - Disconnects from network				*/
		(uint8_t *)"CWSAP",				/* 7 - Configures AP mode					*/
		(uint8_t *)"CWDHCP",			/* 8 - Configures DHCP						*/
		(uint8_t *)"CIPMUX",			/* 9 - enables multiple connections			*/
		(uint8_t *)"CIPSERVER",			/* 10 - starts a server						*/
		(uint8_t *)"CIPCLOSE",			/* 11 - close connection					*/
		(uint8_t *)"CIPSEND",			/* 12 - sends data							*/
		(uint8_t *)"CIPSTART",			/* 13 - connects to a tcp server			*/
		(uint8_t *)"CWAUTOCONN",		/* 14 - Enables auto connect				*/
		(uint8_t *)"AT",				/* 15 - polling command						*/

};

const AtCommandResponse_t AtCommandsResponseTable[12] =
{
		{
			(uint8_t*)"OK",
			2,
			AtCommand_OkCallback
		},
		{
			(uint8_t*)"ERROR",
			5,
			AtCommand_ErrorCallback
		},
		{
			(uint8_t*)"WIFI CONNECTED",
			14,
			AtCommand_WifiConnectedCallbak
		},
		{
			(uint8_t*)"WIFI DISCONNECT",
			15,
			AtCommand_WifiDisconnectedCallbak
		},
		{
			(uint8_t*)"WIFI GOT IP",
			11,
			AtCommand_WifiGotIpCallbak
		},
		{
			(uint8_t*)"ATE0",
			4,
			AtCommand_EchoOffCallbak
		},
		{
			(uint8_t*)"+IPD,",
			5,
			AtCommand_TcpDataReceivedCallbak
		},
		{
			(uint8_t*)"SEND OK",
			7,
			AtCommand_TcpSendOkCallbak
		},
		{
			(uint8_t*)"FAIL",
			4,
			AtCommand_FailCallbak
		},
		{
			(uint8_t*)"OK\r\n> ",
			6,
			AtCommand_TcpSendDataCallback
		},
		{
			(uint8_t*)"> ",
			2,
			AtCommand_TcpSendDataCallback
		},
		{
			(uint8_t*)"+CWJAP:",
			7,
			AtCommand_JoinStatus
		},
};

static const uint8_t TcpConnectString[] =
{
		"CONNECT"
};

static const uint8_t TcpDisconnectString[] =
{
		"CLOSED"
};
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Global Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t ParametersBuffer[PARAMETERS_BUFFER_SIZE];

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Static Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////

static uint32_t CommandStatusRegister = 0;

static uint16_t StatusRegister = 0;

static esp8266_callback_t AppGenericEventsCallback;

static esp8266_tcp_callback_t AppTcpCallback;

static esp8266_events_t EventToReport;

static uint16_t ServerPortNumber = 0;

static uint8_t ServerConnectionsAvailable = ESP8266_MAX_CONNECTIONS;

static uint8_t * TcpSendDataBuffer;

static uint16_t TcpSendDataSize;

static state_machine_t Esp8266_States;

static swtimer_t CommandTimer = 0xFF;

static swtimer_t ResetTimer = 0xFF;

static uint8_t DisconnectCounter = ESP8266_DISCONNECT_COUNTER;

#ifdef FSL_RTOS_FREE_RTOS

static EventGroupHandle_t Esp8266_Event = NULL;

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////

void Esp8266_Init(esp8266_callback_t Callback)
{

	AtCommands_Init(Esp8266_AtCommandsCallback, (AtCommandResponse_t*)&AtCommandsResponseTable[0], SIZE_OF_ARRAY(AtCommandsResponseTable));

	AppGenericEventsCallback = Callback;

	EventToReport = ESP8266_CONFIG_DONE_EVENT;

	CommandTimer = SWTimer_AllocateChannel(ESP8266_TIMEOUT,Esp8266_TimerCallback,NULL);
	ResetTimer  = SWTimer_AllocateChannel(ESP8266_RESET_TIMER,Esp8266_ResetTimerCallback,NULL);

	StatusRegister = 0;


	#ifdef FSL_RTOS_FREE_RTOS

	Esp8266_Event = xEventGroupCreate();

	/* create task and event */
	xTaskCreate((TaskFunction_t) Esp8266_Task, (const char*) "Esp8266_task", ESP8266_STACK_SIZE,\
						NULL,ESP8266_TASK_PRIORITY,NULL);

	#endif

	Esp8266_Reset();
}
#ifndef FSL_RTOS_FREE_RTOS
void Esp8266_Task(void)
{

	AtCommands_Task();

	/* some operations are executed directly on AT callbacks 			*/
	/* more complex ones or that require several commands are handled 	*/
	/* by the SM														*/
	Esp8266_StateMachineFunctions[Esp8266_States.CurrentState]();
}
#else
void Esp8266_Task(void * Param)
{
	EventBits_t EventsTriggered;

	while(1)
	{

		EventsTriggered = xEventGroupWaitBits(Esp8266_Event,ESP8266_SELF_EVENT, \
												pdTRUE,pdFALSE,portMAX_DELAY);


		/* some operations are executed directly on AT callbacks 			*/
		/* more complex ones or that require several commands are handled 	*/
		/* by the SM														*/
		Esp8266_StateMachineFunctions[Esp8266_States.CurrentState]();
		(void)EventsTriggered;
	}
}
#endif

esp8266_status_t Esp8266_Reset(void)
{
	esp8266_status_t Status = ESP8266_SUCCESS;

	/* TODO: Polling mechanism to identify if the device is connected or not */
	AtCommands_EnableUartRx(false);

	AtCommands_ResetModule();

	Esp8266_States.NextState = ESP8266_DISABLE_ECHO_STATE;

	//ATCommands_ExecuteCommand((uint8_t*)AtCommandTable[ESP8266_RESET_COMMAND]);

	/* Trigger a timer, more or less 1 seconds (empiric time)			*/
	SWTimer_EnableTimer(ResetTimer);

	return Status;
}

esp8266_status_t Esp8266_SetMode(esp8266_mode_t SelectedMode)
{
	uint8_t NewMode;
	esp8266_status_t Status = ESP8266_SUCCESS;

	if(SelectedMode <= ESP8266_AP_CLIENT_MODE)
	{
		MiscFunctions_MemClear(&ParametersBuffer[0],PARAMETERS_BUFFER_SIZE);

		NewMode = (uint8_t)SelectedMode;

		SET_FLAG(CommandStatusRegister,ESP8266_COMMAND_GENERATE_EVENT);

		EventToReport = ESP8266_CHANGE_MODE_EVENT;

		MiscFunctions_IntegerToAscii(NewMode,&ParametersBuffer[0]);

		ATCommands_SetCommand((uint8_t*)AtCommandTable[ESP8266_MODE_COMMAND],&ParametersBuffer[0]);
	}
	else
	{
		Status = ESP8266_WRONG_PARAMETER;
	}

	return Status;
}

esp8266_status_t Esp8266_DisconnectNetwork(void)
{

	/* the response of this command is 	*/
	/* first OK then WIFI DISCONNECT	*/
	SET_FLAG(CommandStatusRegister,ESP8266_COMMAND_GENERATE_EVENT);

	EventToReport = ESP8266_NETWORK_DISCONNECTED_EVENT;

	ATCommands_ExecuteCommand((uint8_t*)AtCommandTable[ESP8266_DISCONNECT_NWK_COMMAND]);

	return ESP8266_SUCCESS;
}

esp8266_status_t Esp8266_ConnectToNetwork(uint8_t* NetworkSsid, uint8_t* NetworkPassword)
{
	esp8266_status_t Status = ESP8266_SUCCESS;
	uint16_t StringSize;
	uint16_t ParameterOffset;

	if((NetworkSsid != NULL) && (NetworkPassword != NULL))
	{
		MiscFunctions_MemClear(&ParametersBuffer[0],PARAMETERS_BUFFER_SIZE);

		/* AT+CWJAP = "ssid","psw" */
		ParametersBuffer[0] = '"';

		ParameterOffset = 1;

		StringSize = strlen((char*)NetworkSsid);

		MiscFunctions_MemCopy(NetworkSsid, &ParametersBuffer[1], StringSize);

		ParameterOffset += StringSize;

		ParametersBuffer[ParameterOffset] = '"';

		ParameterOffset += 1;

		ParametersBuffer[ParameterOffset] = ',';

		ParameterOffset += 1;

		ParametersBuffer[ParameterOffset] = '"';

		ParameterOffset += 1;

		StringSize = strlen((char*)NetworkPassword);

		MiscFunctions_MemCopy(NetworkPassword, &ParametersBuffer[ParameterOffset], StringSize);

		ParameterOffset += StringSize;

		ParametersBuffer[ParameterOffset] = '"';

		ParameterOffset += 1;

		ATCommands_SetCommand((uint8_t*)AtCommandTable[ESP8266_CONNECT_NWK_COMMAND],&ParametersBuffer[0]);
	}
	else
	{
		Status = ESP8266_WRONG_PARAMETER;
	}

	return Status;
}

esp8266_status_t Esp8266_ApSettings(uint8_t * ApSsid, uint8_t * ApPassword, esp8266_apsecurity_t Security)
{
	esp8266_status_t Status = ESP8266_WRONG_PARAMETER;
	uint16_t StringSize;
	uint16_t ParameterOffset;

	if((Security < ESP8266_APSECURITY_INVALID) && (ApSsid != NULL) && (ApPassword != NULL))
	{
		/* AT+CWSAP="ssid","psw",channel,security */

		MiscFunctions_MemClear(&ParametersBuffer[0],PARAMETERS_BUFFER_SIZE);

		ParametersBuffer[0] = '"';

		ParameterOffset = 1;

		StringSize = strlen((char*)ApSsid);

		MiscFunctions_MemCopy(ApSsid, &ParametersBuffer[1], StringSize);

		ParameterOffset += StringSize;

		ParametersBuffer[ParameterOffset] = '"';

		ParameterOffset += 1;

		ParametersBuffer[ParameterOffset] = ',';

		ParameterOffset += 1;

		ParametersBuffer[ParameterOffset] = '"';

		ParameterOffset += 1;

		StringSize = strlen((char*)ApPassword);

		MiscFunctions_MemCopy(ApPassword, &ParametersBuffer[ParameterOffset], StringSize);

		ParameterOffset += StringSize;

		ParametersBuffer[ParameterOffset] = '"';

		ParameterOffset += 1;

		ParametersBuffer[ParameterOffset] = ',';

		ParameterOffset += 1;

		ParameterOffset += MiscFunctions_IntegerToAscii(APMODE_DEFAULT_CHANNEL,&ParametersBuffer[ParameterOffset]);

		ParametersBuffer[ParameterOffset] = ',';

		ParameterOffset += 1;

		ParameterOffset += MiscFunctions_IntegerToAscii((uint32_t)Security,&ParametersBuffer[ParameterOffset]);

		SET_FLAG(CommandStatusRegister,ESP8266_COMMAND_GENERATE_EVENT);
		EventToReport = ESP8266_AP_READY_EVENT;

		ATCommands_SetCommand((uint8_t*)AtCommandTable[ESP8266_CONFIG_AP_COMMAND],&ParametersBuffer[0]);

		Status = ESP8266_SUCCESS;
	}

	return Status;
}

esp8266_status_t Esp8266_StartServer(uint16_t PortNumber, esp8266_tcp_callback_t TcpCallback)
{

	MiscFunctions_MemClear(&ParametersBuffer[0],PARAMETERS_BUFFER_SIZE);

	AppTcpCallback = TcpCallback;

	ParametersBuffer[0] = (uint8_t)'1';

	ParametersBuffer[1] = (uint8_t)',';

	(void)MiscFunctions_IntegerToAscii(PortNumber,&ParametersBuffer[2]);

	SET_FLAG(CommandStatusRegister,ESP8266_COMMAND_GENERATE_EVENT);

	EventToReport = ESP8266_SERVER_CREATED_EVENT;

	ATCommands_SetCommand((uint8_t*)AtCommandTable[ESP8266_START_SERVER_COMMAND],&ParametersBuffer[0]);

	return ESP8266_SUCCESS;
}

esp8266_status_t Esp8266_ShutdownServer(void)
{
	MiscFunctions_MemClear(&ParametersBuffer[0],PARAMETERS_BUFFER_SIZE);

	ParametersBuffer[0] = (uint8_t)'0';

	SET_FLAG(CommandStatusRegister,ESP8266_COMMAND_GENERATE_EVENT);

	EventToReport = ESP8266_SERVER_SHUTDOWN_EVENT;

	ATCommands_SetCommand((uint8_t*)AtCommandTable[ESP8266_START_SERVER_COMMAND],&ParametersBuffer[0]);

	return ESP8266_SUCCESS;
}

esp8266_status_t Esp8266_TcpSendData(uint32_t ConnectionNumber, uint8_t * DataToSend, uint16_t DataSize)
{
	uint16_t ParameterOffset = 0;


	MiscFunctions_MemClear(&ParametersBuffer[0],PARAMETERS_BUFFER_SIZE);
	/* back up to send later */
	TcpSendDataBuffer = DataToSend;

	TcpSendDataSize = DataSize;

	/* AT+CIPSEND = Connection,DataSize 		*/
	/* Once that send, module will reply with >	*/
	/* and the data must be sent then 			*/
	ParameterOffset = MiscFunctions_IntegerToAscii(ConnectionNumber,&ParametersBuffer[0]);

	ParametersBuffer[ParameterOffset] = ',';

	ParameterOffset++;

	MiscFunctions_IntegerToAscii(DataSize,&ParametersBuffer[ParameterOffset]);

	ATCommands_SetCommand((uint8_t*)AtCommandTable[ESP8266_SOCKET_SEND_COMMAND],&ParametersBuffer[0]);

	return ESP8266_SUCCESS;

}

esp8266_status_t Esp8266_TcpClose(uint32_t ConnectionNumber)
{
	/*CIPCLOSE = X*/
	MiscFunctions_MemClear(&ParametersBuffer[0],PARAMETERS_BUFFER_SIZE);

	(void)MiscFunctions_IntegerToAscii(ConnectionNumber,&ParametersBuffer[0]);

	ATCommands_SetCommand((uint8_t*)AtCommandTable[ESP8266_CLOSE_SOCKET_COMMAND],&ParametersBuffer[0]);

	return ESP8266_SUCCESS;
}

esp8266_status_t Esp8266_ConnectToTcpServer(uint8_t * IpAddressString, uint16_t PortNumber, esp8266_tcp_callback_t TcpCallback)
{
	esp8266_status_t Status = ESP8266_WIFI_NOT_CONNECTED;
	uint16_t StringSize;
	uint16_t ParameterOffset = 0;

	MiscFunctions_MemClear(&ParametersBuffer[0],PARAMETERS_BUFFER_SIZE);

	/* first check that we're connected to a wifi */
	if(CHECK_FLAG(StatusRegister,ESP8266_STATUS_WIFI_CONNECTED))
	{
		/* check if we have MUX enabled */
		if(CHECK_FLAG(StatusRegister,ESP8266_STATUS_MUX_ENABLED))
		{
			/*AT+CIPSTART=link,"TCP","ip",port*/

			ParameterOffset += MiscFunctions_IntegerToAscii(0,&ParametersBuffer[ParameterOffset]);

			ParametersBuffer[ParameterOffset] = ',';

			ParameterOffset += 1;

			MiscFunctions_MemCopy((uint8_t*)"\"TCP\",\"", &ParametersBuffer[ParameterOffset], 7);

			ParameterOffset += 7;

			StringSize = strlen((char*)IpAddressString);

			MiscFunctions_MemCopy(IpAddressString, &ParametersBuffer[ParameterOffset], StringSize);

			ParameterOffset += StringSize;

			ParametersBuffer[ParameterOffset] = '"';

			ParameterOffset += 1;

			ParametersBuffer[ParameterOffset] = ',';

			ParameterOffset += 1;

			ParameterOffset += MiscFunctions_IntegerToAscii(PortNumber,&ParametersBuffer[ParameterOffset]);

			AppTcpCallback = TcpCallback;

			ATCommands_SetCommand((uint8_t*)AtCommandTable[ESP8266_CONNECT_TO_SERVER_COMMAND],&ParametersBuffer[0]);
		}
	}

	return (Status);

}

esp8266_status_t Esp8266_AutoConnect(bool isEnabled)
{
	MiscFunctions_MemClear(&ParametersBuffer[0],PARAMETERS_BUFFER_SIZE);

	if(isEnabled)
	{
		ParametersBuffer[0] = (uint8_t)'1';
	}
	else
	{
		ParametersBuffer[0] = (uint8_t)'0';
	}

	SET_FLAG(CommandStatusRegister,ESP8266_COMMAND_GENERATE_EVENT);

	EventToReport = ESP8266_AUTOCONN_EVENT;

	ATCommands_SetCommand((uint8_t*)AtCommandTable[ESP8266_AUTO_CONNECT_COMMAND],&ParametersBuffer[0]);

	return ESP8266_SUCCESS;
}

static void Esp8266_IdleState(void)
{

}

static void Esp8266_WaitOkState(void)
{
	if(CHECK_FLAG(CommandStatusRegister,ESP8266_OK_RECEIVED))
	{
		CLEAR_FLAG(CommandStatusRegister,ESP8266_OK_RECEIVED);

		if(!(CHECK_FLAG(CommandStatusRegister,ESP8366_START_TIMER_EVENT)))
		{
			/* next state */
			Esp8266_States.CurrentState = Esp8266_States.NextState;
			#ifdef FSL_RTOS_FREE_RTOS
			xEventGroupSetBits(Esp8266_Event, ESP8266_SELF_EVENT);
			#endif
		}
		else
		{
			CLEAR_FLAG(CommandStatusRegister,ESP8366_START_TIMER_EVENT);
			Esp8266_States.CurrentState = ESP8266_TIMEOUT_STATE;
			SWTimer_EnableTimer(CommandTimer);

		}
	}
}

static void Esp8266_StartServerState(void)
{
	/* AT+CIPSERVER = 1,port */

	MiscFunctions_MemClear(&ParametersBuffer[0],PARAMETERS_BUFFER_SIZE);

	ParametersBuffer[0] = (uint8_t)'1';

	ParametersBuffer[1] = (uint8_t)',';

	(void)MiscFunctions_IntegerToAscii(ServerPortNumber,&ParametersBuffer[2]);

	ATCommands_SetCommand((uint8_t*)AtCommandTable[ESP8266_START_SERVER_COMMAND],&ParametersBuffer[0]);

	SET_FLAG(CommandStatusRegister,ESP8266_COMMAND_GENERATE_EVENT);
	SET_FLAG(StatusRegister,ESP8266_STATUS_MUX_ENABLED);

	Esp8266_States.CurrentState = ESP8266_IDLE_STATE;

	EventToReport = ESP8266_SERVER_CREATED_EVENT;
}

static void Esp8266_SendTcpDataState(void)
{
	Esp8266_States.CurrentState = ESP8266_IDLE_STATE;

	ATCommands_SendCustomCommand(TcpSendDataBuffer,TcpSendDataSize);
}

static void Esp8266_TimeoutState(void)
{
	if(CHECK_FLAG(CommandStatusRegister,ESP8366_COMMAND_TIMEOUT_EVENT))
	{
		CLEAR_FLAG(CommandStatusRegister,ESP8366_COMMAND_TIMEOUT_EVENT);
		/* next state */
		Esp8266_States.CurrentState = Esp8266_States.NextState;

		#ifdef FSL_RTOS_FREE_RTOS
		xEventGroupSetBits(Esp8266_Event, ESP8266_SELF_EVENT);
		#endif
	}
}

static void Esp8266_InitDoneState(void)
{
	Esp8266_States.CurrentState = ESP8266_IDLE_STATE;
	Esp8266_States.NextState = ESP8266_IDLE_STATE;

	SET_FLAG(StatusRegister,ESP8266_STATUS_MUX_ENABLED);

	AppGenericEventsCallback(ESP8266_CONFIG_DONE_EVENT,ESP8266_EVENT_OK_STATUS);
}

static void Esp8266_DisableEchoState(void)
{
	EventToReport = ESP8266_CONFIG_DONE_EVENT;

	SET_FLAG(CommandStatusRegister,ESP8266_SM_PROCESSING);
	/* if echo was already disabled, the state machine will handle multiple connections */
	/* if echo wasn't disabled, then, echo off callback handles multiple connections */
	Esp8266_States.CurrentState = ESP8266_WAIT_OK_STATE;
	Esp8266_States.NextState = ESP8266_ENABLE_MULT_CONNECTIONS_STATE;

	AtCommands_EnableUart(true);

	ATCommands_SendCustomCommand((uint8_t*)AtCommandTable[ESP8266_DISABLE_ECHO_COMMAND], 4);
}


static void Esp8266_EnableMultipleConnectionsState(void)
{
	/* once echo is off, enable multiple connections for default*/
	SET_FLAG(CommandStatusRegister,ESP8266_SM_PROCESSING);

	ParametersBuffer[0] = (uint8_t)'1';

	ATCommands_SetCommand((uint8_t*)AtCommandTable[ESP8266_SET_CONNECTIONS_COMMAND],&ParametersBuffer[0]);

	/* once MUX is enabled, then start the server */
	Esp8266_States.CurrentState = ESP8266_WAIT_OK_STATE;
	Esp8266_States.NextState = ESP8266_INIT_DONE_STATE;

	#ifdef FSL_RTOS_FREE_RTOS
	xEventGroupSetBits(Esp8266_Event, ESP8266_SELF_EVENT);
	#endif
}

void Esp8266_ResetTimerCallback(void * Args)
{
	SWTimer_DisableTimer(ResetTimer);

	Esp8266_States.CurrentState = Esp8266_States.NextState;

	#ifdef FSL_RTOS_FREE_RTOS
	xEventGroupSetBits(Esp8266_Event, ESP8266_SELF_EVENT);
	#endif
}

void Esp8266_TimerCallback (void * Args)
{
	SET_FLAG(CommandStatusRegister,ESP8366_COMMAND_TIMEOUT_EVENT);
	SWTimer_DisableTimer(CommandTimer);

	#ifdef FSL_RTOS_FREE_RTOS
	xEventGroupSetBits(Esp8266_Event, ESP8266_SELF_EVENT);
	#endif
}

static void Esp8266_AtCommandsCallback(AtCommandsEvent_t Event, uint8_t*Data, uint16_t DataSize)
{
	uint16_t NewConnectionHandle;
	static uint16_t ConnectionToRefused = 0xFF;
	uint8_t StringStatus;

	if(Event == ATCOMMANDS_RESPONSE_NOT_FOUND_EVENT)
	{
		/* process custom responses */

		/* for now, the only custom response is the connection which comes on the form of 	*/
		/* x,CONNECT or x,CLOSE - since each connection starts with a different number					*/
		/*	 might be easier to process here than generate N callbacks						*/

		/* get the connection number */
		NewConnectionHandle = MiscFunctions_AsciiToUnsignedInteger(Data);
		/* now just confirm is "CONNECT" or "CLOSE" the rest of the text								*/
		StringStatus = MiscFunctions_SearchInString(Data,0,&TcpConnectString[0],sizeof(TcpConnectString) - 1u);

		if(StringStatus == STRING_OK)
		{
			/* confirm we can handle this connection or reject it otherwise AT+CIPCLOSE=X		*/
			if(ServerConnectionsAvailable)
			{
				/* ACK the upper layer a new connection is ready through the TCP callback			*/
				/* connection ID																	*/
				AppTcpCallback(ESP8266_TCP_SERVER_NEW_CONNECTION_EVENT,NewConnectionHandle,NULL,0);
				ServerConnectionsAvailable--;
			}
			else
			{
				MiscFunctions_MemClear(&ParametersBuffer[0],PARAMETERS_BUFFER_SIZE);

				(void)MiscFunctions_IntegerToAscii(NewConnectionHandle,&ParametersBuffer[0]);

				ConnectionToRefused = NewConnectionHandle;

				ATCommands_SetCommand((uint8_t*)AtCommandTable[ESP8266_CLOSE_SOCKET_COMMAND],&ParametersBuffer[0]);
			}
		}
		else
		{
			StringStatus = MiscFunctions_SearchInString(Data,0,&TcpDisconnectString[0],sizeof(TcpDisconnectString) - 1u);

			if(StringStatus == STRING_OK)
			{
				/* just report when a valid connection was closed */
				if(ConnectionToRefused != NewConnectionHandle)
				{
					AppTcpCallback(ESP8266_TCP_SERVER_CONNECTION_CLOSED_EVENT,NewConnectionHandle,NULL,0);
					ServerConnectionsAvailable++;
				}
				else
				{
					ConnectionToRefused = 0xFF;
				}
			}
		}

	}

	if(Event == ATCOMMANDS_COMMAND_TIMEOUT_ERROR_EVENT)
	{
		DisconnectCounter--;

		if(DisconnectCounter)
		{
			AppGenericEventsCallback(ESP8266_ERROR_EVENT,ESP8266_EVENT_ERROR_STATUS);
		}
		else
		{
			DisconnectCounter = ESP8266_DISCONNECT_COUNTER;
			AppGenericEventsCallback(ESP8266_ERROR_EVENT,ESP8266_EVENT_DEVICE_UNRESPONSIVE_STATUS);
		}
		CLEAR_FLAG(CommandStatusRegister,ESP8266_COMMAND_GENERATE_EVENT);
		CLEAR_FLAG(CommandStatusRegister,ESP8266_SM_PROCESSING);

		Esp8266_States.CurrentState = ESP8266_IDLE_STATE;
	}
}

bool AtCommand_OkCallback(uint8_t * Parameters, uint16_t ParametersSize)
{
	bool Status = false;

	/* generate an event if required or process this on the SM */
	if(CHECK_FLAG(CommandStatusRegister,ESP8266_COMMAND_GENERATE_EVENT))
	{
		CLEAR_FLAG(CommandStatusRegister,ESP8266_COMMAND_GENERATE_EVENT);
		/* let upper layer know we're done */
		AppGenericEventsCallback(EventToReport,ESP8266_EVENT_OK_STATUS);

	}
	else if(CHECK_FLAG(CommandStatusRegister,ESP8266_SM_PROCESSING))
	{
		SET_FLAG(CommandStatusRegister,ESP8266_OK_RECEIVED);
		CLEAR_FLAG(CommandStatusRegister,ESP8266_SM_PROCESSING);
		#ifdef FSL_RTOS_FREE_RTOS
		xEventGroupSetBits(Esp8266_Event, ESP8266_SELF_EVENT);
		#endif
	}

	if(ParametersSize)
	{
		Status = true;
	}

	return Status;
}

bool AtCommand_ErrorCallback(uint8_t * Parameters, uint16_t ParametersSize)
{
	bool Status = false;

	/* report error to upper layer */
	AppGenericEventsCallback(ESP8266_ERROR_EVENT,ESP8266_EVENT_ERROR_STATUS);

	if(ParametersSize)
	{
		Status = true;
	}

	return Status;
}

bool AtCommand_WifiConnectedCallbak(uint8_t * Parameters, uint16_t ParametersSize)
{
	bool Status = false;

	SET_FLAG(CommandStatusRegister,ESP8266_WIFI_CONNECTED);

	if(ParametersSize)
	{
		Status = true;
	}

	return Status;
}

bool AtCommand_WifiDisconnectedCallbak(uint8_t * Parameters, uint16_t ParametersSize)
{
	bool Status = false;

	/* clear internal flags and confirm disconnect is ok */
	CLEAR_FLAG(CommandStatusRegister,ESP8266_WIFI_CONNECTED);
	CLEAR_FLAG(StatusRegister,ESP8266_STATUS_WIFI_CONNECTED);


	AppGenericEventsCallback(EventToReport,ESP8266_EVENT_OK_STATUS);

	if(ParametersSize)
	{
		Status = true;
	}

	return Status;
}

bool AtCommand_WifiGotIpCallbak(uint8_t * Parameters, uint16_t ParametersSize)
{
	bool Status = false;

	/* Report connection only when there's IP */
	if(CHECK_FLAG(CommandStatusRegister,ESP8266_WIFI_CONNECTED))
	{
		SET_FLAG(CommandStatusRegister,ESP8266_COMMAND_GENERATE_EVENT);
		SET_FLAG(StatusRegister,ESP8266_STATUS_WIFI_CONNECTED);
		EventToReport = ESP8266_NETWORK_CONNECTED_EVENT;

	}
	else
	{
		/* if we got here without wifi connect, something is wrong */
		AppGenericEventsCallback(ESP8266_ERROR_EVENT,ESP8266_EVENT_ERROR_STATUS);
	}

	if(ParametersSize)
	{
		Status = true;
	}

	return Status;
}

bool AtCommand_EchoOffCallbak(uint8_t * Parameters, uint16_t ParametersSize)
{
	bool Status = false;

	/* do nothing */
	/* as we need ATE0 and OK */
	/* Furthermore, the SM is waiting for OK */
	if(ParametersSize)
	{
		Status = true;
	}

	return Status;
}

bool AtCommand_FailCallbak(uint8_t * Parameters, uint16_t ParametersSize)
{
	bool Status = false;

	/* something went wrong */
	AppGenericEventsCallback(ESP8266_ERROR_EVENT,ESP8266_EVENT_ERROR_STATUS);

	if(ParametersSize)
	{
		Status = true;
	}

	return Status;
}

bool AtCommand_JoinStatus(uint8_t * Parameters, uint16_t ParametersSize)
{
	uint32_t JoinStatus;
	bool Status = false;

	JoinStatus = MiscFunctions_AsciiToUnsignedInteger(Parameters);

	if(JoinStatus)
	{
		/* something went wrong */
		AppGenericEventsCallback(ESP8266_ERROR_JOINING_WIFI_EVENT,ESP8266_EVENT_OK_STATUS);
	}

	if(ParametersSize > 1)
	{
		Status = true;
	}

	return Status;
}

bool AtCommand_TcpSendDataCallback(uint8_t * Parameters, uint16_t ParametersSize)
{
	bool Status = false;

	ATCommands_SendCustomCommand(TcpSendDataBuffer,TcpSendDataSize);

	if(ParametersSize)
	{
		Status = true;
	}

	return Status;
}

bool AtCommand_TcpDataReceivedCallbak(uint8_t * Parameters, uint16_t ParametersSize)
{
	uint32_t ConnectionNumber;
	uint32_t DataSize;
	uint8_t * DataSizeString;
	uint8_t * ReceivedData;

	/* data is received on the following command: +IPD,connection,datasize:data... */

	/* extract connection, data size and pointer to the data */
	ConnectionNumber = MiscFunctions_AsciiToUnsignedInteger(Parameters);

	DataSizeString = MiscFunctions_FindTokenInString(Parameters, ',');

	DataSize = MiscFunctions_AsciiToUnsignedInteger(DataSizeString);

	ReceivedData = MiscFunctions_FindTokenInString(DataSizeString, ':');

	/* upper layer must copy the data, after returning callback, the data is not valid */
	AppTcpCallback(ESP8266_TCP_SERVER_DATA_RECEIVED_EVENT,ConnectionNumber,ReceivedData,DataSize);

	return false;
}

bool AtCommand_TcpSendOkCallbak(uint8_t * Parameters, uint16_t ParametersSize)
{
	bool Status = false;

	AppTcpCallback(ESP8266_TCP_SERVER_DATA_SENT_EVENT,0xFF,NULL,0);

	if(ParametersSize)
	{
		Status = true;
	}

	return Status;
}

/* EOF */
