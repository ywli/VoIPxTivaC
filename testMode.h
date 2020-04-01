//*****************************************************************************
//
// testMode.h - test mode operation
//
//
//*****************************************************************************
#ifndef __TEST_MODE_H__
#define __TEST_MODE_H__

/* use canned 1011 tone */
#define TEST_MODE_MIC_1011_TONE        0 //1-enable, 0-disable

/* use canned packet */
#define TEST_MODE_WIFI_CANNED_TX_PKT   0 // 1- send canned packets, 0- send regular packets

/* G722 test mode */
#define TEST_MODE_G722_PCM_BYPASS      0 //1- PCM encoding, 0- normal operation

#endif // __TEST_MODE_H__
