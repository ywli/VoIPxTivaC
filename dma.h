//*****************************************************************************
//
// dma.h - DMA module
//
//
//*****************************************************************************
#ifndef __DMA_H__
#define __DMA_H__

/* DMA channel direction */
#define DMA_DIR_IO_TO_RAM 1
#define DMA_DIR_RAM_TO_IO 0

/* DMA channel mode */
#define DMA_MODE_BASIC    0
#define DMA_MODE_PINGPONG 1

/** 
 * Initialize DMA
 * param: none
 * return: (int) -> execution status
**/
int dmaInit(void);

/** 
 * Initialize DMA channel
 * param: dmaChId -> DMA channel ID
 * param: dmaChEnc -> DMA channel assignment encoding
 *                    0- Enc. 0
 *                    1- Enc. 1
 *                    2- Enc. 2
 *                    3- Enc. 3
 *                    4- Enc. 4
 * param: dmaMode -> DMA channel assignment encoding
 *                     DMA_MODE_BASIC- basic mode
 *                     DMA_MODE_PINGPONG- ping-pong mode
 * param: direction -> DMA channel direction
 *                     DMA_DIR_IO_TO_RAM- from IO to RAM
 *                     DMA_DIR_RAM_TO_IO- from RAM to IO
 * param: elementSize -> DMA transfer size, 
 *                       1- 8-bit, 2- 16-bit, 4- 32-bit
 * return: execution status
**/
int dmaChInit(
    int dmaChId,
    int dmaChEnc,
    int dmaMode,
    int direction,
    int elementSize);

/** 
 * Request DMA transfer
 * param: dmaChId -> DMA channel ID
 * param: srcAddr -> DMA transfer source address
 * param: dstAddr -> DMA transfer destination address
 * param: elementSize -> number of items in the transfer
 * return: execution status
 *         COMMON_RETURN_STATUS_SUCCESS- transfer requested
 *         COMMON_RETURN_STATUS_FAILURE- transfer not requested due to on-going transfer
**/
int dmaChRequest(
    uint8_t dmaChId,
    uint32_t *srcAddr,
    uint32_t *dstAddr,
    uint32_t numOfElement);

#endif // __DMA_H__
