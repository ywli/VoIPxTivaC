//*****************************************************************************
//
// rtp.h - RTP module
//
//
//*****************************************************************************

#ifndef __RTP_H__
#define __RTP_H__
#include <stdint.h>

int rtp_init(void);

int rtp_write(
	uint8_t* packetP,
	const int16_t *sampleP,
	uint16_t sampleLen);

#endif // __RTP_H__
