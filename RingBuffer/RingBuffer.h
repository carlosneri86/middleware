#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Includes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>


///////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Typedef Section
///////////////////////////////////////////////////////////////////////////////////////////////////
/*!
 * @brief Ring buffer flags used to control the read/write of data.
 */
typedef enum
{
	RING_BUFFER_FULL = 0,
	RING_BUFFER_EMPTY,
	RING_BUFFER_PATTERN_FOUND,
	RING_BUFFER_PATTERN_NOT_FOUND,
	RING_BUFFER_NOT_ENOUGH_SPACE,
}RingBufferStatus_t;

/*!
 * @brief Ring buffer configuration.
 */
typedef struct
{
	uint8_t	*  pWritePointer;
	uint8_t	*  pReadPointer;
	uint32_t StartAddress;
	uint32_t EndAddress;
	uint32_t BufferSize;
	uint32_t BufferStatus;
}RingBuffer_t;

/*!
 * @brief Ring buffer status for the FSM.
 */
typedef enum
{
	DMA_TO_RINGBUFFER_COMPLETE = 0,
	RINGBUFFER_TO_DMA_COMPLETE,
}eRingBufferStatus;

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
/*!
 * @brief Initialize the ring buffer.
 *
 * @param spRingBuffer pointer to the ring buffer.
 * @param pStartAddress pointer to the beginning of the start address.
 * @param BufferSize ring buffer size.
 * @return void.
 */
void RingBuffer_Init(RingBuffer_t * spRingBuffer, uint8_t * pStartAddress, uint32_t BufferSize);

/*!
 * @brief Write several bytes into the ring buffer.
 *
 * @param psRingBuffer pointer to the ring buffer.
 * @param pOutData pointer to the buffer that is going to be written.
 * @param SizeOfDataToWrite size of the data that will be written.
 * @return void.
 */
void RingBuffer_WriteBuffer(RingBuffer_t * psRingBuffer, uint8_t * pOutData, uint32_t SizeOfDataToWrite);

/*!
 * @brief Write single data into the ring buffer.
 *
 * @param psRingBuffer pointer to the ring buffer.
 * @param pOutData pointer to the buffer that is going to be written.
 * @return void.
 */
void RingBuffer_WriteData(RingBuffer_t * psRingBuffer, uint8_t * pOutData);

/*!
 * @brief Read single data from the ring buffer.
 *
 * @param psRingBuffer pointer to the ring buffer.
 * @param pOutData pointer to the buffer that is going to be read.
 * @return void.
 */
void RingBuffer_ReadData(RingBuffer_t * psRingBuffer, uint8_t * pData);

/*!
 * @brief Read several bytes from the ring buffer
 *
 * @param psRingBuffer pointer to the ring buffer.
 * @param pOutData pointer to the buffer that is going to be read.
 * @return void.
 */
void RingBuffer_ReadBuffer(RingBuffer_t * psRingBuffer, uint8_t * pDataIn, uint32_t DataToRead);

/*!
 * @brief Read data using a non blocking function.
 *
 * @param psRingBuffer pointer to the ring buffer.
 * @param pOutData pointer to the buffer that is going to be read.
 * @return void.
 */
void RingBuffer_Reset(RingBuffer_t * spRingBuffer);

/*!
 * @brief Return the current available space.
 *
 * @param psRingBuffer pointer to the ring buffer.
 * @return current available space.
 */
uint32_t RingBuffer_SpaceAvailable(RingBuffer_t * psRingBuffer);


uint32_t RingBuffer_DataAvailable(RingBuffer_t * psRingBuffer);
#if defined(__cplusplus)
}
#endif // __cplusplus


#endif /* RINGBUFFER_H_ */
///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////

