/*
 * cbuff_dma.h -- Microcontroller circular buffer for DMA operation.
 *
 */

#ifndef CBUFF_DMA_H_
#define CBUFF_DMA_H_


typedef struct 
{
	// Buffer space that this circular buffer manages
	// Best performance if this is first since then &cbuff_t == buff
	unsigned char *buff;

	// Start and end indices of valid data in the buffer
	// Read from start; write to end
	int start;
	int end;

	// Number of bytes in the buffer
	int numOfAvail;

	// Number of bytes remaining in the buffer
	int numOfFree;

	// Number of bytes transferring by DMA
	int numOfXfer;

	// Number of bytes transferred in a DMA transfer block
	int xferSize;

	// Number of overrun events
	int numOfOverrun;

	// Fix size of the buffer
	int size;
}cbuff_dma_t;

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
	int transferSize);

/*
 * purpose: reset a DMA buffer
 * parameter: 
 *     buff: DMA buffer reference
 * return:
 *  1- success, 0- failure
 */
int
cbuff_dma_reset(cbuff_dma_t *buff);

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
	int len);

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
	int *dataLenP);

/*
 * purpose: for driver to confirm data read from DMA buffer
 * parameter: 
 *     buff: DMA buffer reference
 * return:
 *  other positive integer- number of bytes read, 0- failure
 */
int
cbuff_dma_dequeue_driver2(cbuff_dma_t *buff);

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
	int len);

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
	int *dataLenP);

/*
 * purpose: for driver to confirm data written into DMA buffer
 * parameter: 
 *     buff: DMA buffer reference
 * return:
 *  positive integer- number of bytes written, 0- failure
 */
int
cbuff_dma_enqueue_driver2(cbuff_dma_t *buff);


#endif /* CBUFF_DMA_H_ */
