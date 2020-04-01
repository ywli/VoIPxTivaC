/*
 * rtp.c -- RTP protocol
 *
 *
 */
/* standard library */
#include <stdint.h>

/* 3-rd party resources */
#include "ez_rtp.h"
#include "g722.h"

/* project resource */
#include "common.h"
#include "rtp.h"

typedef struct
{
	struct rtp_header rtpTxHdr; // header, state of RTP protocol
	G722_ENC_CTX rtpEncoder;    // encoder
}rtpControlBlock_t;

rtpControlBlock_t rtpCb;

int rtpInit(void)
{
	/* initialize header tbd */
	rtpCb.rtpTxHdr.bitfields = 0;
	rtpCb.rtpTxHdr.sequence_number = 0;
	rtpCb.rtpTxHdr.timestamp = 0;
	rtpCb.rtpTxHdr.ssrc = 0;
	rtpCb.rtpTxHdr.csrc[0] = 0;

	/* initialize encoder */
	if (g722_encoder_config(
			&rtpCb.rtpEncoder,
			64000,
			G722_SAMPLE_RATE_8000) != 0)
	{
		return COMMON_RETURN_STATUS_FAILURE;
	}

	return COMMON_RETURN_STATUS_SUCCESS;
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
int rtpWrite(
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

	/* update header variable tbd */
	hdrP->sequence_number++;
	hdrP->timestamp++;

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
	
	/* audio samples */
	res = g722_encode(
			&rtpCb.rtpEncoder,
			sampleP,
			sampleLen,
			wrP);
	if (res <= 0)
	{
		return COMMON_RETURN_STATUS_FAILURE;
	}
	wrP += res;

	return (wrP - packetP);
}
