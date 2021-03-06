//*****************************************************************************
//
// mic.h - Microphone module
//
//
//*****************************************************************************
#ifndef __MIC_H__
#define __MIC_H__
#include <stdint.h>
#include "testMode.h"

/* test feature */
/* use fake 1011 tone data */
#define MIC_TEST_1011_TONE   TEST_MODE_MIC_1011_TONE

/* DC blocking filter */
#define MIC_DC_BLOCK_FILTER  1  //1-IIR, 2-subtract DC

/* microphone sampling rate */
#define MIC_SAMPLE_RATE_HZ   8000

/* 
 * audio block definition 
 */
/* time duration of a block in ms */
#define MIC_BLOCK_MS         20
/* number of samples in a block */
#define MIC_BLOCK_NUM_OF_SP  (MIC_SAMPLE_RATE_HZ * MIC_BLOCK_MS / 1000)
/* size of block in bytes */
#define MIC_BLOCK_SIZE       (MIC_BLOCK_NUM_OF_SP * sizeof(uint16_t))
/* number of blocks */
#define MIC_BLOCK_NUM_OF     10

/*
 * DMA definition
 */
/* DMA channel index */
#define MIC_DMA_CH            14
#define MIC_DMA_ENC           0
#define MIC_DMA_MODE          DMA_MODE_PINGPONG
#define MIC_DMA_DIR           DMA_DIR_IO_TO_RAM
#define MIC_DMA_ELEMENT_SIZE  2

/* audio block definition */
typedef struct 
{
	int16_t micDataBlock[MIC_BLOCK_NUM_OF_SP];
}micDataBlock_t;

/** 
 * Initialize module
 * param: none
 * return: (int) -> execution status
**/
int micInit(void);

/** 
 * Get one audio block
 * param: none
 * return: (micDataBlock_t*) -> 
 *       retrieved audio block reference on valid address,
 *       0 on block not available
**/
micDataBlock_t* micBlockGet(void);

/** 
 * Start audio capturing
 * param: none
 * return: (int) -> execution status
**/
int micStart(void);

/** 
 * Stop audio capturing
 * param: none
 * return: (int) -> execution status
**/
int micStop(void);

#endif // __MIC_H__
