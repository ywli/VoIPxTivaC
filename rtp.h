//*****************************************************************************
//
// rtp.h - RTP module
//
//
//*****************************************************************************

#ifndef __RTP_H__
#define __RTP_H__
#include <stdint.h>

/** 
 * Initialize module
 * param: none
 * return: none
**/
int rtpInit(void);

/** 
 * Generate RTP packet
 * param: packetP (uint8_t*) -> packet reference
 * param: sampleP (int16_t*) 	-> sample output
 * param: sampleLenP (uint16_t*) 	-> number of sample output
 * return: (int) -> 0- failure, postive integer- packet size in bytes
**/
int rtpWrite(
	uint8_t* packetP,
	const int16_t *sampleP,
	uint16_t sampleLen);

#endif // __RTP_H__
