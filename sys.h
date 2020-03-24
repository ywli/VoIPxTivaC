//*****************************************************************************
//
// mic.h - Microphone module
//
//
//*****************************************************************************
#include <stdint.h>

#ifndef __SYS_H__
#define __SYS_H__

#define DMA_TRANSFER_SIZE_MAX 1024

struct dmaTableEntry
{
	volatile uint32_t srcAddr;
	volatile uint32_t dstAddr;
	volatile uint32_t chCtl;
	volatile uint32_t reserved;
};

extern struct dmaTableEntry dmaTable[64];
int sysInit(void);


#endif // __SYS_H__
