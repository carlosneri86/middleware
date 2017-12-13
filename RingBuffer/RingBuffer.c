//////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Includes Section
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include "RingBuffer.h"
///////////////////////////////////////////////////////////////////////////////////////////////////
//                                   Defines & Macros Section
///////////////////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Functions Section
///////////////////////////////////////////////////////////////////////////////////////////////////

/*FUNCTION**********************************************************************
 *
 * Function Name : RingBuffer_Init
 * Description   : initialize the ring buffer.
 *
 *END**************************************************************************/
void RingBuffer_Init(RingBuffer_t * spRingBuffer, uint8_t * pStartAddress, uint32_t BufferSize)
{
	spRingBuffer->StartAddress = (uint32_t)pStartAddress;
	spRingBuffer->EndAddress   = ((uint32_t)(pStartAddress) + (uint32_t)(BufferSize) - (uint32_t)(1));
	spRingBuffer->BufferSize   = BufferSize;
	spRingBuffer->pReadPointer = pStartAddress;
	spRingBuffer->pWritePointer = pStartAddress;
	spRingBuffer->BufferStatus = 0;

}

/*FUNCTION**********************************************************************
 *
 * Function Name : RingBuffer_Reset
 * Description   : re-configure the read and write pointers
 *
 *END**************************************************************************/
void RingBuffer_Reset(RingBuffer_t * spRingBuffer)
{
	spRingBuffer->pReadPointer = (uint8_t *)spRingBuffer->StartAddress;
	spRingBuffer->pWritePointer = (uint8_t *)spRingBuffer->StartAddress;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : RingBuffer_WriteBuffer
 * Description   : Write several bytes into the ring buffer
 *
 *END**************************************************************************/
void RingBuffer_WriteBuffer(RingBuffer_t * psRingBuffer, uint8_t * pOutData, uint32_t SizeOfDataToWrite)
{

	while(SizeOfDataToWrite)
	{
		*(psRingBuffer->pWritePointer) = *pOutData;

		psRingBuffer->pWritePointer++;
		pOutData++;
		SizeOfDataToWrite--;

		/* send to the beginning the pointer */
		if(((uint32_t)psRingBuffer->pWritePointer) > psRingBuffer->EndAddress)
		{
			psRingBuffer->pWritePointer = ((uint8_t*)psRingBuffer->StartAddress);
		}
	}

	/* check for errors */
	if(((uint32_t)psRingBuffer->pWritePointer) == ((uint32_t)psRingBuffer->pReadPointer))
	{
		//psRingBuffer->BufferStatus |= (1<<RING_BUFFER_OVERFLOW);
	}

}

/*FUNCTION**********************************************************************
 *
 * Function Name : RingBuffer_WriteData
 * Description   : Write single data into the ring buffer
 *
 *END**************************************************************************/
void RingBuffer_WriteData(RingBuffer_t * psRingBuffer, uint8_t * pOutData)
{
	/* store the data */
	*(psRingBuffer->pWritePointer) = (*pOutData);
	psRingBuffer->pWritePointer++;
	/* send to the beginning the pointer */
	if(((uint32_t)psRingBuffer->pWritePointer) > psRingBuffer->EndAddress)
	{
		psRingBuffer->pWritePointer = ((uint8_t*)psRingBuffer->StartAddress);
	}
	/* check for errors */
	if(((uint32_t)psRingBuffer->pWritePointer) == ((uint32_t)psRingBuffer->pReadPointer))
	{
		//psRingBuffer->BufferStatus |= (1<<RING_BUFFER_OVERFLOW);
	}
}

/*FUNCTION**********************************************************************
 *
 * Function Name : RingBuffer_ReadData
 * Description   : Read single data from the ring buffer
 *
 *END**************************************************************************/
void RingBuffer_ReadData(RingBuffer_t * psRingBuffer, uint8_t * pData)
{

	*pData = *(psRingBuffer->pReadPointer);
	psRingBuffer->pReadPointer++;
	/* send to the beginning the pointer */
	if(((uint32_t)psRingBuffer->pReadPointer) > psRingBuffer->EndAddress)
	{
		psRingBuffer->pReadPointer = ((uint8_t*)psRingBuffer->StartAddress);
	}
	/* check for errors */
	if(((uint32_t)psRingBuffer->pWritePointer) == ((uint32_t)psRingBuffer->pReadPointer))
	{
		//psRingBuffer->BufferStatus |= (1<<RING_BUFFER_OVERFLOW);
	}

}

/*FUNCTION**********************************************************************
 *
 * Function Name : RingBuffer_ReadBuffer
 * Description   : Read data several bytes from the ring buffer
 *
 *END**************************************************************************/
void RingBuffer_ReadBuffer(RingBuffer_t * psRingBuffer, uint8_t* pDataIn, uint32_t DataToRead)
{
	while(DataToRead)
	{
		*pDataIn = *(psRingBuffer->pReadPointer);

		psRingBuffer->pReadPointer++;
		pDataIn++;
		DataToRead--;

		/* send to the beginning the pointer */
		if(((uint32_t)psRingBuffer->pReadPointer) > psRingBuffer->EndAddress)
		{
			psRingBuffer->pReadPointer = ((uint8_t*)psRingBuffer->StartAddress);
		}
	}

	/* check for errors */
	if(((uint32_t)psRingBuffer->pWritePointer) == ((uint32_t)psRingBuffer->pReadPointer))
	{
		//psRingBuffer->BufferStatus |= (1<<RING_BUFFER_OVERFLOW);
	}
}

/*FUNCTION**********************************************************************
 *
 * Function Name : RingBuffer_SpaceAvailable
 * Description   : Returns the current space available in the ring buffer
 *
 *END**************************************************************************/
uint32_t RingBuffer_SpaceAvailable(RingBuffer_t * psRingBuffer)
{
	uint32_t SpaceAvailable;

	/* calcualted current available space */
	if(((uint32_t)psRingBuffer->pWritePointer) > ((uint32_t)psRingBuffer->pReadPointer))
	{
		SpaceAvailable = ((uint32_t)psRingBuffer->pWritePointer - (uint32_t)psRingBuffer->pReadPointer);
	}
	else
	{
		SpaceAvailable = (psRingBuffer->BufferSize - ((uint32_t)psRingBuffer->pReadPointer - (uint32_t)psRingBuffer->pWritePointer));
	}

	return(SpaceAvailable);
}
/*FUNCTION**********************************************************************
 *
 * Function Name : RingBuffer_DataAvailable
 * Description   : Returns the current space occupied in the ring buffer
 *
 *END**************************************************************************/
uint32_t RingBuffer_DataAvailable(RingBuffer_t * psRingBuffer)
{
	uint32_t SpaceAvailable;

	/* calcualted current available space */
	if(((uint32_t)psRingBuffer->pWritePointer) > ((uint32_t)psRingBuffer->pReadPointer))
	{
		SpaceAvailable = ((uint32_t)psRingBuffer->pWritePointer - (uint32_t)psRingBuffer->pReadPointer);
	}
	else
	{
		SpaceAvailable = (psRingBuffer->BufferSize - ((uint32_t)psRingBuffer->pReadPointer - (uint32_t)psRingBuffer->pWritePointer));
	}

	return(SpaceAvailable);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////////////////////////

