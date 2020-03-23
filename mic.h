//*****************************************************************************
//
// mic.h - Microphone module
//
//
//*****************************************************************************

#ifndef __MIC_H__
#define __MIC_H__
#include <stdint.h>

int micInit(void);

int micReadBlock(uint16_t *blockP);

int micStart(void);

int micStop(void);

#endif // __MIC_H__
