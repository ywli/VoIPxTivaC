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

/* MIPS measure */
#define TEST_MODE_MIPS_MEASURE         1 //1- test MIPS mode, measure TX task MIPS

/** 
 * Initialize timer
 * param: none
 * return: none
**/
void testModeTimerInit(void);

/** 
 * Start timer
 * param: none
 * return: none
**/
void testModeTimerTic(void);

/** 
 * Stop timer
 * param: none
 * return: (long) -> timer counter value
**/
long testModeTimerToc();

/** 
 * Test TX task MIPS
 * param: none
 * return: none
**/
void testModeMipsMeasure(void);

#endif // __TEST_MODE_H__
