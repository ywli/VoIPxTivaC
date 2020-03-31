/*
 * cbuff_dma.h -- Microcontroller circular buffer for DMA operation.
 *
 */

#ifndef DMA_BUFFER_H_
#define DMA_BUFFER_H_


typedef struct 
{
	// Buffer space that this circular buffer manages
	// Best performance if this is first since then &cbuff_t == buff
	unsigned char *buff;

	// Number of bytes in the buffer
	int buffSize;

	// Number of bytes in one unit 
	int unitSize;

	// Start and end indices of valid data in the buffer
	// Read from start; write to end
	int start; //point to the first available unit
	int end;   //point to the last available unit + 1

	// Number of available units
	int numOfAvail;

	// Number of free unit space
	int numOfFree;

	// Number of transferring units
	int numOfXfer;

	// Number of all units
	int numOfTotal;
}dmaBuffer_t;

#define DMA_BUFFER_PUT_OPT_APP_PUT_UNIT_1  1
#define DMA_BUFFER_PUT_OPT_APP_PUT_UNIT_2  2
#define DMA_BUFFER_PUT_OPT_DRV_PUT_UNIT_1  3
#define DMA_BUFFER_PUT_OPT_DRV_PUT_UNIT_2  4

#define DMA_BUFFER_GET_OPT_APP_GET_UNIT_1  1
#define DMA_BUFFER_GET_OPT_APP_GET_UNIT_2  2
#define DMA_BUFFER_GET_OPT_DRV_GET_UNIT_1  3
#define DMA_BUFFER_GET_OPT_DRV_GET_UNIT_2  4

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
	int unitSize);

/*
 * purpose: reset a DMA buffer
 * parameter: 
 *     buff: DMA buffer reference
 * return:
 *  
 */
int
dmaBufferReset(dmaBuffer_t *buff);

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
	int option);

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
	int option);

#endif /* DMA_BUFFER_H_ */
