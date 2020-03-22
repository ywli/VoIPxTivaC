/*
 * cbuff_dma.c -- Microcontroller circular buffer for DMA operation.
 *
 *
 */

#include "cbuff_dma.h"
#define DMA_TRANSFER_MAX 1024
/*
 * purpose: initialize a DMA buffer
 * parameter: 
 *     buff: DMA buffer reference
 *     staticbuff: static array reference
 *     bufSize: size of the static array reference
 *     blockSize: size of a DMA transfer block
 * return:
 *  1- success, 0- failure
 * 
 */
int
cbuff_dma_init(
	cbuff_dma_t *buff, 
	unsigned char *staticbuff, 
	int bufSize, 
	int transferSize)
{
	if ((bufSize % transferSize) != 0)
	{
		/* error tbd */
		return 0;
	}

	/* initialize buffer */
	buff->buff = staticbuff;
	buff->size = bufSize;
	buff->xferSize = transferSize;

	cbuff_dma_reset(buff);

	return 1;
}

/*
 * purpose: reset a DMA buffer
 * parameter: 
 *     buff: DMA buffer reference
 * return:
 *  1- success, 0- failure
 */
int
cbuff_dma_reset(cbuff_dma_t *buff)
{
	buff->start = 0;
	buff->end = 0;
	buff->numOfAvail = 0;
	buff->numOfXfer = 0;
	buff->numOfFree = buff->size;
	buff->numOfOverrun = 0;

	return 1;
}

/*
 * purpose: for application to put data into DMA buffer
 * parameter: 
 *     buff: DMA buffer reference
 *     dataP: input data reference
 *     len: input data length
 * return:
 *  other positive integer- number of bytes written, 0- failure
 */
int
cbuff_dma_enqueue_app(
	cbuff_dma_t *buff, 
	unsigned char *dateP, 
	int len)
{
	// Assume everything will be queued
	int end = buff->end;
	int size = buff->size;
	int avail = buff->numOfAvail;

	// Not enough space in the buffer
	if (avail < len)
	{
		return 0;
	}

	// Update counters before len is modified in the for-loop
	buff->numOfAvail += len;
	buff->numOfFree -= len;

	// Loop to end of data or end of buffer, whichever was decided above
	for (; len > 0; len--)
	{
		// Copy byte
		buff->buff[end++] = *(dateP++);

		// Wrap around if need be
		if (end >= size)
		{
			end = 0;
		}
	}

	// Update buffer with new end value
	buff->end = end;

	return len;
}

/*
 * purpose: for driver to get data from DMA buffer
 * parameter: 
 *     buff: DMA buffer reference
 *     dataPP: output data reference
 *     dataLenP: output data length
 * return:
 *  other positive integer- number of bytes read, 0- failure
 */
int
cbuff_dma_dequeue_driver1(
	cbuff_dma_t *buff, 
	unsigned char **dataPP, 
	int *dataLenP)
{
	int start = buff->start;
	int size = buff->size;
	int avail = buff->numOfAvail;
	int len;

	/* DMA buffer empty */
	if (buff->numOfAvail > 0) 
	{
		return 0;
	}

	/* block is already transferring */
	if (buff->numOfXfer != 0)
	{
		return 0;
	}

    /* avoid wrapping */
    len = ((start + avail) > size) ? size - start: avail;

	/* avoid oversize */
	len = (len > DMA_TRANSFER_MAX)? DMA_TRANSFER_MAX: len;

    /* return */
    *dataPP = &buff->buff[start];
    *dataLenP = len;

	/* update buffer */
	buff->start = (start + len) % size;
	buff->numOfAvail -= len;
	buff->numOfXfer = len;

	return len;
}

/*
 * purpose: for driver to confirm data read from DMA buffer
 * parameter: 
 *     buff: DMA buffer reference
 * return:
 *  other positive integer- number of bytes read, 0- failure
 */
int
cbuff_dma_dequeue_driver2(cbuff_dma_t *buff)
{
	int xfer;
	
	xfer = buff->numOfXfer;

	/* block transfer confirmed */
	buff->numOfXfer = 0;
	buff->numOfFree += xfer;

	return xfer;
}

/*
 * purpose: for application to get data from DMA buffer
 * parameter: 
 *     buff: DMA buffer reference
 *     destP: output data reference
 *     len: output data length
 * return:
 *  positive integer- number of bytes read, 0- failure
 */
int
cbuff_dma_dequeue_app(
	cbuff_dma_t *buff, 
	unsigned char *destP, 
	int len)
{
	int start = buff->start;
	int size = buff->size;

	// If buffer does not contain len bytes 
	if (len > buff->numOfAvail) { return 0;}

	// Update counters before len is modified in the for-loop
	buff->numOfAvail -= len;
	buff->numOfFree += len;

	// copy bytes
	for (; len > 0; len--)
	{
		*(destP++) = buff->buff[start++];

		// Wrap around if need be
		if (start >= size)
		{
			start = 0;
		}
	}

	// Update buffer with new start value
	buff->start = start;

	return len;
}

/*
 * purpose: for driver to put data into DMA buffer
 * parameter: 
 *     buff: DMA buffer reference
 *     dataPP: data reference
 *     dataLenP: data length
 * return:
 *  positive integer- number of bytes written, 0- failure
 */
int
cbuff_dma_enqueue_driver1(
	cbuff_dma_t *buff, 
	unsigned char **dataPP, 
	int *dataLenP)
{
	// Assume everything will be queued
	int end = buff->end;
	int size = buff->size;
	int xfer = buff->numOfXfer;
	int xferSize = buff->xferSize;
	int free = buff->numOfFree;
	int tmp;

	/* determine size of next DMA transfer */
	end = (end + xfer) % size;

	/* adjust to avoid memory address wrapping */
	if ((end + xferSize) > size)
	{
		/* error tbd */
		return 0;
	}

	/* return */
	*dataPP = &buff->buff[end];
	*dataLenP = xferSize;

	/* update accumulative on-going transfer size */
	buff->numOfXfer += xferSize;

	/* if buffer overflow */
	tmp = xferSize - free;
	if (tmp > 0)
	{
		/* update buffer */
		buff->start += tmp;
		buff->numOfAvail -= tmp;
		buff->numOfFree = 0;

		/* increment number of overflow events */
		buff->numOfOverrun++;
	}
	else
	{
		buff->numOfFree -= xferSize;
	}

	return xferSize;
}

/*
 * purpose: for driver to confirm data written into DMA buffer
 * parameter: 
 *     buff: DMA buffer reference
 * return:
 *  positive integer- number of bytes written, 0- failure
 */
int
cbuff_dma_enqueue_driver2(cbuff_dma_t *buff)
{
	// Assume everything will be queued
	int end = buff->end;
	int size = buff->size;
	int xferSize = buff->xferSize;

	/* update buffer */
	buff->end = (end + xferSize) % size;
	buff->numOfAvail += xferSize;
	buff->numOfXfer -= xferSize;

	return xferSize;
}
