#ifndef _SERIAL_PLATFORM_H_
#define _SERIAL_PLATFORM_H_


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Includes Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	SERIAL_PLATFORM_DATA_RECEIVED = 0,
	SERIAL_PLATFORM_DATA_OK,
	SERIAL_PLATFORM_ERROR = 0xFF
}serialplatformstatus_t;

typedef void (*serialplatformcallback_t)(uint8_t);

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

void SerialPlatform_Init(uint32_t BaudRate, serialplatformcallback_t Callback);

void SerialPlatform_SendNonBlocking(uint8_t * CommandBuffer, uint16_t BufferSize);

void SerialPlatform_SendBlocking(uint8_t * CommandBuffer, uint16_t BufferSize);

serialplatformstatus_t SerialPlatform_RxStatus(uint8_t * NewData);

uint8_t SerialPlatform_Read(void);

#if defined(__cplusplus)
}
#endif // __cplusplus


#endif /* _SERIAL_PLATFORM_H_ */
///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////