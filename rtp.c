// TI library
#include "ez_rtp.h"
#include "g722.h"
#include <stdint.h>

typedef struct
{
	struct rtp_header rtpTxHdr; // header, state of RTP protocol
	G722_ENC_CTX rtpEncoder;    // encoder
}rtpControlBlock_t;

rtpControlBlock_t rtpCb;

int rtp_init(void)
{
	/* initialize header */
	rtpCb.rtpTxHdr.bitfields = 0;//tbd
	rtpCb.rtpTxHdr.sequence_number = 0;//tbd
	rtpCb.rtpTxHdr.timestamp = 0;//tbd
	rtpCb.rtpTxHdr.ssrc = 0;//tbd
	rtpCb.rtpTxHdr.csrc[0] = 0;//tbd

	/* initialize encoder */
	if (g722_encoder_config(
			&rtpCb.rtpEncoder,
			64000,
			G722_SAMPLE_RATE_8000) != 0)
	{
		/* error tbd */
	}

	return 0;
}

/** 
 * Generate RTP packet
 * param: packetP (uint8_t*) -> packet reference
 * param: packetLen (uint16_t)	-> packet size in bytes
 * param: hdrP (struct rtp_header*)	-> packet header output
 * param: sampleP (int16_t*) 	-> sample output
 * param: sampleLenP (uint16_t*) 	-> number of sample output
 * return: (int)			-> 0 on failure, 1 on success
**/
int rtp_write(
	uint8_t* packetP,
	const int16_t *sampleP,
	uint16_t sampleLen) 
{
	int res;
	uint8_t* wrP;
	struct rtp_header* hdrP;

	/* initialize working variables */
	wrP = packetP;
	hdrP = &rtpCb.rtpTxHdr;

	/* update header variable */
	hdrP->sequence_number++;
	hdrP->timestamp++;//tbd

	/* generate header - 12 bytes */
	*wrP++ = (uint8_t) ((hdrP->bitfields >> 8) & 0xFF);
	*wrP++ = (uint8_t) ((hdrP->bitfields >> 0) & 0xFF);
	*wrP++ = (uint8_t) ((hdrP->sequence_number >> 8) & 0xFF);
	*wrP++ = (uint8_t) ((hdrP->sequence_number >> 0) & 0xFF);
	*wrP++ = (uint8_t) ((hdrP->timestamp >> 24) & 0xFF);
	*wrP++ = (uint8_t) ((hdrP->timestamp >> 16) & 0xFF);
	*wrP++ = (uint8_t) ((hdrP->timestamp >>  8) & 0xFF);
	*wrP++ = (uint8_t) ((hdrP->timestamp >>  0) & 0xFF);
	*wrP++ = (uint8_t) ((hdrP->ssrc >> 24) & 0xFF);
	*wrP++ = (uint8_t) ((hdrP->ssrc >> 16) & 0xFF);
	*wrP++ = (uint8_t) ((hdrP->ssrc >>  8) & 0xFF);
	*wrP++ = (uint8_t) ((hdrP->ssrc >>  0) & 0xFF);
	
	/* audio samples 320-bytes */
	#if 0
	res = g722_encode(
			&rtpCb.rtpEncoder,
			sampleP,
			sampleLen,
			wrP);
	if (res <= 0)
	{
		/* error tbd */
		return -1;
	}
	wrP += res;
	#else
	int16_t *dstSampleP;
	dstSampleP = (int16_t *) wrP;
	/* 8ksps, 1011Hz, 8sp one cycle, max amp.: 32k */
	static const uint16_t spkTestData[] = 
	{
		0x3e80, 0x6ab2, 0x7d00, 0x6ab2, 0x3e80, 0x124e, 0x0000, 0x124e
	};
	for (res = 0; res<sampleLen; res++)
	{
		//dstSampleP[res] = spkTestData[(res * 3 ) % 8];//3k tone
		dstSampleP[res] = spkTestData[(res * 1 ) % 8];//1k tone
		//dstSampleP[res] = sampleP[res];//true voice
	}
	wrP += sampleLen * sizeof(int16_t);
	// for (res = 0; res < 332; res++)
	// {
	// 	packetP[res] = 0;
	// }
	#endif

	return (wrP - packetP);
}
