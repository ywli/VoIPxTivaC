// TI library
#include "ez_rtp.h"
#include "g722.h"
#include <stdint.h>

typedef struct
{
	G722_ENC_CTX rtpEncoder;
	G722_DEC_CTX rtpDecoder;
	struct rtp_header rtpTxHdr;
	struct rtp_header rtpRxHdr;
}rtpControlBlock_t;

rtpControlBlock_t rtpCb;

int rtp_open()
{

	if (g722_encoder_config(
			&rtpCb.rtpEncoder,
			64000,
			G722_SAMPLE_RATE_8000) != 0)
	{
		/* error tbd */
	}
	
	rtpCb.rtpTxHdr.bitfields = 0;//tbd
	

	return 0;
}
/** 
 * Get audio samples from a RTP packet.
 * param: packetP (uint8_t*) -> packet reference
 * param: packetLen (uint16_t)	-> packet size in bytes
 * param: hdrP (struct rtp_header*)	-> packet header output
 * param: sampleP (int16_t*) 	-> sample output
 * param: sampleLenP (uint16_t*) 	-> number of sample output
 * return: (int)			-> 0 on failure, 1 on success
**/
int rtp_read(
	uint8_t* packetP,  
	uint16_t packetLen,
	struct rtp_header* hdrP,
	int16_t* sampleP,
	uint16_t* sampleLenP)
{
	// uint8_t* rdP;
	// uint16_t sampleLen;
	// int res;

	// if (packetLen < 12)
	// {
	// 	return 0;
	// }
	// rdP = packetP;

	// /* parse header */
	// hdrP->bitfields = (rdP[0] << 8) |
	//                   (rdP[1] << 0);
	// rdP += 2;

	// hdrP->sequence_number = (rdP[0] << 8) |
	//                         (rdP[1] << 0);
	// rdP += 2;

	// hdrP->timestamp = (rdP[0] << 24) |
	//                   (rdP[1] << 16) |
	// 				  (rdP[2] <<  8) |
	// 				  (rdP[3] <<  0);
	// rdP += 4;

	// hdrP->ssrc = (rdP[0] << 24) |
	//              (rdP[1] << 16) |
	// 			 (rdP[2] <<  8) |
	// 			 (rdP[3] <<  0);
	// rdP += 4;

	// /* bypass csrc */
	// rdP += (GET_CSRC_COUNT(hdrP->bitfields) * 4);
	

	// /* get audio samples */

	// /* number of samples */
	// sampleLen = packetLen;
	// sampleLen -= (rdP - packetP);
	
	// /* deced data */
	// res = g722_decode(
	// 		&decoderState, 
	// 		rdP,
	// 		sampleLen, 
	// 		sampleP);
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
	uint16_t* packetLenP,
	int16_t* sampleP,
	uint16_t sampleLen) 
{
	uint8_t* wrP;
	int res;
	struct rtp_header* hdrP;
	hdrP = &rtpCb.rtpTxHdr;
	wrP = packetP;

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
	res = g722_encode(
			&rtpCb.rtpEncoder,
			sampleP,
			sampleLen,
			wrP);
	wrP += sampleLen;

	*packetLenP = (wrP - packetP);
	return res;
}
