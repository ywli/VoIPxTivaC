//*****************************************************************************
//
// rtp.h - RTP module
//
//
//*****************************************************************************

#ifndef __RTP_H__
#define __RTP_H__
#include <stdint.h>


#define RTP_STATUS_SUCCESS 1
#define RTP_STATUS_FAILURE 0

int rtpInit(void);

int rtpWrite(
	uint8_t* packetP,
	const int16_t *sampleP,
	uint16_t sampleLen);

#endif // __RTP_H__
