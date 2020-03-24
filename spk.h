//*****************************************************************************
//
// spk.h - Speaker module
//
//
//*****************************************************************************

#ifndef __SPK_H__
#define __SPK_H__
#include <stdint.h>


int spkInit(void);

int spkWrite(
	uint16_t *sampleP,
	uint16_t numOfSample);


#endif // __SPK_H__
