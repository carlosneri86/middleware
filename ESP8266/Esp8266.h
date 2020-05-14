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
#ifndef ESP8266_H_
#define ESP8266_H_

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Includes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////
#define ESP8266_MAX_CONNECTIONS	(1)
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum
{
	ESP8266_CLIENT_MODE = 1,
	ESP8266_AP_MODE,
	ESP8266_AP_CLIENT_MODE
}esp8266_mode_t;


typedef enum
{
	ESP8266_SUCCESS = 0,
	ESP8266_WRONG_PARAMETER,
	ESP8266_WIFI_NOT_CONNECTED
}esp8266_status_t;

typedef enum
{
	ESP8266_CONFIG_DONE_EVENT = 0,
	ESP8266_NETWORK_CONNECTED_EVENT,
	ESP8266_NETWORK_DISCONNECTED_EVENT,
	ESP8266_CHANGE_MODE_EVENT,
	ESP8266_AP_READY_EVENT,
	ESP8266_SERVER_CREATED_EVENT,
	ESP8266_SERVER_SHUTDOWN_EVENT,
	ESP8266_ERROR_EVENT,
	ESP8266_ERROR_JOINING_WIFI_EVENT,
	ESP8266_INVALID_EVENT,
	ESP8266_AUTOCONN_EVENT,
}esp8266_events_t;

typedef enum
{
	ESP8266_APSECURITY_OPEN = 0,
	ESP8266_APSECURITY_WEP,
	ESP8266_APSECURITY_WPA,
	ESP8266_APSECURITY_WPA2,
	ESP8266_APSECURITY_WPA_WPA2,
	ESP8266_APSECURITY_INVALID
}esp8266_apsecurity_t;

typedef enum
{
	ESP8266_EVENT_OK_STATUS = 0,
	ESP8266_EVENT_ERROR_STATUS,
	ESP8266_EVENT_DEVICE_UNRESPONSIVE_STATUS
}esp8266_event_status_t;

typedef enum
{
	ESP8266_TCP_SERVER_NEW_CONNECTION_EVENT = 0,
	ESP8266_TCP_SERVER_DATA_RECEIVED_EVENT,
	ESP8266_TCP_SERVER_DATA_SENT_EVENT,
	ESP8266_TCP_SERVER_CONNECTION_CLOSED_EVENT
}esp8266_tcp_events_t;

typedef void (*esp8266_callback_t)(esp8266_events_t, esp8266_event_status_t);

typedef void (*esp8266_tcp_callback_t)(esp8266_tcp_events_t, uint16_t, uint8_t*, uint16_t);
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                Function-like Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Extern Constants Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Extern Variables Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                Function Prototypes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus

void Esp8266_Init(esp8266_callback_t Callback);

#ifndef FSL_RTOS_FREE_RTOS
void Esp8266_Task(void);
#else
void Esp8266_Task(void * Param);
#endif

esp8266_status_t Esp8266_DisconnectNetwork(void);

esp8266_status_t Esp8266_Reset(void);

esp8266_status_t Esp8266_SetMode(esp8266_mode_t SelectedMode);

esp8266_status_t Esp8266_ConnectToNetwork(uint8_t* NetworkSsid, uint8_t* NetworkPassword);

esp8266_status_t Esp8266_ApSettings(uint8_t * ApSsid, uint8_t * ApPassword, esp8266_apsecurity_t Security);

esp8266_status_t Esp8266_StartServer(uint16_t PortNumber, esp8266_tcp_callback_t TcpCallback);

esp8266_status_t Esp8266_ShutdownServer(void);

esp8266_status_t Esp8266_TcpSendData(uint32_t ConnectionNumber, uint8_t * DataToSend, uint16_t DataSize);

esp8266_status_t Esp8266_TcpClose(uint32_t ConnectionNumber);

esp8266_status_t Esp8266_ConnectToTcpServer(uint8_t * IpAddressString, uint16_t PortNumber, esp8266_tcp_callback_t TcpCallback);

esp8266_status_t Esp8266_AutoConnect(bool isEnabled);

#if defined(__cplusplus)
}
#endif // __cplusplus

#endif /* ESP8266_H_ */
///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////



