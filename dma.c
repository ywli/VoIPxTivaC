/* standard library */
#include <stdint.h>

/* TI resource */
#include "tm4c123gh6pm.h"

/* project resource */
#include "common.h"
#include "dma.h"
#include "abm.h"

#define DMA_NUM_OF_ELEMENT 1023

struct dmaTableEntry
{
	volatile uint32_t *srcAddr;
	volatile uint32_t *dstAddr;
	volatile uint32_t chCtl;
	volatile uint32_t reserved;
};

/* 1k alignment */
#pragma DATA_ALIGN (dmaTable, 1024)
struct dmaTableEntry dmaTable[64];

/** 
 * Initialize DMA
 * param: none
 * return: (int) -> execution status
**/
int dmaInit(void)
{
	/* 1- DMA clock */
	SYSCTL_RCGCDMA_R |= SYSCTL_RCGCDMA_R0;
	while ((SYSCTL_PRDMA_R & SYSCTL_PRDMA_R0) == 0)
	{};

	/* 2- enable uDMA controller */
	UDMA_CFG_R |= UDMA_CFG_MASTEN;
	while((UDMA_STAT_R & UDMA_STAT_MASTEN) == 0)
	{};
	
	/* 3- control table base */
	UDMA_CTLBASE_R = (uint32_t) &dmaTable[0];

    return COMMON_RETURN_STATUS_SUCCESS;
}

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
    int elementSize)
{
    volatile uint32_t * regAddr; // channel assignmnet register
    uint32_t mask;
    uint32_t shift;
    uint32_t ctlReg; //control word for DMA table entry

	/* channel attribute */
	
	/* configure channel assignment */
    regAddr = &UDMA_CHMAP0_R + (dmaChId / 8);
    shift = (dmaChId % 8) * 4;
    mask = 0xff << shift;
    *regAddr &= ~mask;
    *regAddr |=  (dmaChEnc << shift) & mask;

	/* 1- default channel priority level */
	//UDMA_PRIOSET_R = 0x00000000;//tbd

	/* 2- use primary control structure */
	//UDMA_ALTSET_R = 0x00000000;//tbd

	/* 3- respond to both single and burst requests */
	//UDMA_USEBURSTCLR_R = 0x00000000;//tbd
	
	/* 4- clear mask */
	UDMA_REQMASKCLR_R |= (1 << dmaChId);

    /* DMA table */
    ctlReg = 0;
    /* element size */
    if (elementSize == 1)
    {
        ctlReg |= UDMA_CHCTL_DSTSIZE_8 | UDMA_CHCTL_SRCSIZE_8;
        if (direction == DMA_DIR_IO_TO_RAM)
        {
            ctlReg |= UDMA_CHCTL_DSTINC_8 | UDMA_CHCTL_SRCINC_NONE;
        }
        else
        {
            ctlReg |= UDMA_CHCTL_DSTINC_NONE | UDMA_CHCTL_SRCINC_8;
        }
    }
    else if (elementSize == 2)
    {
        ctlReg |= UDMA_CHCTL_DSTSIZE_16 | UDMA_CHCTL_SRCSIZE_16;
        if (direction == DMA_DIR_IO_TO_RAM)
        {
            ctlReg |= UDMA_CHCTL_DSTINC_16 | UDMA_CHCTL_SRCINC_NONE;
        }
        else
        {
            ctlReg |= UDMA_CHCTL_DSTINC_NONE | UDMA_CHCTL_SRCINC_16;
        }
    }
    else if (elementSize == 4)
    {
        ctlReg |= UDMA_CHCTL_DSTSIZE_32 | UDMA_CHCTL_SRCSIZE_32;
        if (direction == DMA_DIR_IO_TO_RAM)
        {
            ctlReg |= UDMA_CHCTL_DSTINC_32 | UDMA_CHCTL_SRCINC_NONE;
        }
        else
        {
            ctlReg |= UDMA_CHCTL_DSTINC_NONE | UDMA_CHCTL_SRCINC_32;
        }
    }
    else
    {
        abmAbort();
    }

    /* arb size */
    ctlReg |= UDMA_CHCTL_ARBSIZE_1;//tbd

    /* next burst */
    ctlReg |= UDMA_CHCTL_NXTUSEBURST;//tbd

    /* DMA mode */
    if (dmaMode == DMA_MODE_PINGPONG)
    {
        ctlReg |= UDMA_CHCTL_XFERMODE_PINGPONG;
    }
    else if (dmaMode == DMA_MODE_BASIC)
    {
        ctlReg |= UDMA_CHCTL_XFERMODE_BASIC;
    }
    else
    {
        abmAbort();
    }

    /* set control word for the DMA table entry */
	dmaTable[dmaChId].chCtl = ctlReg;

    return COMMON_RETURN_STATUS_SUCCESS;
}

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
    uint32_t numOfElement)
{
    uint32_t offset;
    uint32_t ctlReg;
    uint32_t u32tmp;

    /* primary entry */
    ctlReg = dmaTable[dmaChId].chCtl;

    /* read remain transfer size */
    u32tmp = ((ctlReg & UDMA_CHCTL_XFERSIZE_M) >> UDMA_CHCTL_XFERSIZE_S);

    /* primary/alternative switching in ping-pong mode hanlding */
    if ((ctlReg & UDMA_CHCTL_XFERMODE_M) == UDMA_CHCTL_XFERMODE_PINGPONG)
    {
        /* if primary entry is still busy, assume alternative entry is idle */
        if (u32tmp != 0)
        {
            /* alternative */
            dmaChId += 32;
            ctlReg = dmaTable[dmaChId].chCtl;
            u32tmp = ((ctlReg & UDMA_CHCTL_XFERSIZE_M) >> UDMA_CHCTL_XFERSIZE_S);
        }
    }

    /* if DMA still running */
    if (u32tmp != 0)
    {
        return COMMON_RETURN_STATUS_FAILURE;
    }
    
    if (numOfElement > DMA_NUM_OF_ELEMENT)
    {
        return COMMON_RETURN_STATUS_FAILURE;
    }

    /* 
     * calculate memory offset 
     */
    u32tmp = ctlReg & UDMA_CHCTL_SRCSIZE_M;
    if (u32tmp == UDMA_CHCTL_SRCSIZE_8)
    {
        offset = 1 * numOfElement;
    }
    else if (u32tmp == UDMA_CHCTL_SRCSIZE_16)
    {
        offset = 2 * numOfElement;
    }
    else if (u32tmp == UDMA_CHCTL_SRCSIZE_32)
    {
        offset = 4 * numOfElement;
    }
    else
    {
        abmAbort();
    }

    /* 
     * apply memory offset 
     */
    /* source of IO */
    if ((ctlReg & UDMA_CHCTL_SRCINC_M) == UDMA_CHCTL_SRCINC_NONE)
    {
        /* destination (RAM) address offset */
        dstAddr = (uint32_t *) (((uint8_t *) dstAddr) + offset);
    }
    /* destination of IO */
    else if ((ctlReg & UDMA_CHCTL_DSTINC_M) == UDMA_CHCTL_DSTINC_NONE)
    {
        /* source (RAM) address offset */
        srcAddr = (uint32_t *) (((uint8_t *) srcAddr) + offset);
    }
    else
    {
        abmAbort();
    }

    /* 
     * DMA channel control description 
     */
    dmaTable[dmaChId].srcAddr = srcAddr;
    dmaTable[dmaChId].dstAddr = dstAddr;
    dmaTable[dmaChId].chCtl &= ~UDMA_CHCTL_XFERSIZE_M;
    dmaTable[dmaChId].chCtl |= (((numOfElement - 1) << UDMA_CHCTL_XFERSIZE_S) & UDMA_CHCTL_XFERSIZE_M);

	/* enable uDMA channel */
	UDMA_ENASET_R |= (1 << dmaChId);

    return COMMON_RETURN_STATUS_SUCCESS;
}
