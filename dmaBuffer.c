/*
 * dmaBuffer.c -- Buffer for DMA operation
 *
 *
 */

#include "common.h"
#include "dmaBuffer.h"

/*
 * purpose: initialize a DMA buffer
 * parameter: 
 *     buff: DMA buffer reference
 *     staticbuff: static array reference
 *     bufSize: size of the static array reference
 *     unitSize: size of a DMA transfer block
 * return:
 *  
 */
int
dmaBufferInit(
	dmaBuffer_t *buff, 
	void *staticbuff, 
	int bufSize, 
	int unitSize)
{
	/* input check */
	if ((bufSize % unitSize) != 0)
	{
		/* error */
		return COMMON_RETURN_STATUS_FAILURE;
	}

	/* initialize buffer */
	buff->buff = staticbuff;
	buff->buffSize = bufSize;
	buff->unitSize = unitSize;

	dmaBufferReset(buff);

	return COMMON_RETURN_STATUS_SUCCESS;
}

/*
 * purpose: reset a DMA buffer
 * parameter: 
 *     buff: DMA buffer reference
 * return:
 *  
 */
int
dmaBufferReset(dmaBuffer_t *buff)
{
	int i;

	buff->start = 0;
	buff->end = 0;
	buff->numOfAvail = 0;
	buff->numOfXfer = 0;
	buff->numOfFree = (buff->buffSize / buff->unitSize);
	buff->numOfTotal = buff->numOfFree;

	/* zero buffer space */
	for(i = 0; i<buff->buffSize; i++)
	{
		buff->buff[i] = 0xff;
	}

	return COMMON_RETURN_STATUS_SUCCESS;
}



/*
 * purpose: put data to buffer
 * parameter: 
 *     buff: buffer reference
 *     option: 
 * return:
 *  valid address- unit reference
 *  0 failure or not required
 */
//tbd: overrun condition 
void *
dmaBufferPut(
	dmaBuffer_t *buff,
	int option)
{
	// Assume everything will be queued
	int unitSize = buff->unitSize;
	int end = buff->end;
	int numOfFree = buff->numOfFree;
	int numOfXfer = buff->numOfXfer;
	int numOfTotal = buff->numOfTotal;
	
	if (option == DMA_BUFFER_PUT_OPT_APP_PUT_UNIT_1)
	{
		/* case 1: get unit reference pointer */
		return (numOfFree == 0) ? 0 : (void *) &buff->buff[end * unitSize];
	}
	else if(option == DMA_BUFFER_PUT_OPT_APP_PUT_UNIT_2)
	{
		/* case 2: confirm unit loaded */
		buff->end = (end + 1) % numOfTotal;
		buff->numOfAvail++;
		buff->numOfFree--;
		return 0;
	}
	else if (option == DMA_BUFFER_PUT_OPT_DRV_PUT_UNIT_1)
	{
		/* case 3: prepare unit for DMA */
		if ((numOfFree == 0) && (numOfXfer == 0))
		{
			return 0;
		}
		else if (numOfFree == 0)
		{
			/* overrun */
			buff->numOfXfer++;
			buff->numOfAvail--;
		}
		else
		{
			buff->numOfXfer++;
			buff->numOfFree--;
		}
		
		return (void *) &buff->buff[((end + numOfXfer) % numOfTotal) * unitSize];
	}
	else if (option == DMA_BUFFER_PUT_OPT_DRV_PUT_UNIT_2)
	{
		/* case 4: confirm unit DMA transfer */
		buff->end = (end + 1) % numOfTotal;
		buff->numOfAvail++;
		buff->numOfXfer--;
		return 0;
	}
	else
	{
		/* option error */
		return 0;
	}
}

/*
 * purpose: get data from buffer
 * parameter: 
 *     buff: buffer reference
 *     option: 
 * return:
 *  valid address- unit reference
 *  0 failure or not required
 */
//tbd: overrun condition 
void *
dmaBufferGet(
	dmaBuffer_t *buff,
	int option)
{
	// Assume everything will be queued
	int unitSize = buff->unitSize;
	int start = buff->start;
	int numOfAvail = buff->numOfAvail;
	int numOfTotal = buff->numOfTotal;

	/* no data */
	if (numOfAvail == 0)
	{
		return 0;
	}
	
	if (option == DMA_BUFFER_GET_OPT_APP_GET_UNIT_1)
	{
		/* case 1: get unit reference pointer */
		return (void *)&buff->buff[start * unitSize];
	}
	else if(option == DMA_BUFFER_GET_OPT_APP_GET_UNIT_2)
	{
		/* case 2: confirm unit loaded */
		buff->start = (start + 1) % numOfTotal;
		buff->numOfAvail--;
		buff->numOfFree++;
		return 0;
	}
	else if (option == DMA_BUFFER_GET_OPT_DRV_GET_UNIT_1)
	{
		/* case 3: prepare unit for DMA */
		buff->numOfXfer++;
		buff->numOfAvail--;
		return (void *)&buff->buff[start * unitSize];
	}
	else if (option == DMA_BUFFER_GET_OPT_DRV_GET_UNIT_2)
	{
		/* case 4: confirm unit DMA transfer */
		buff->start = (start + 1) % numOfTotal;
		buff->numOfXfer--;
		buff->numOfFree++;
		return 0;
	}
	else
	{
		/* option error */
		return 0;
	}
}
