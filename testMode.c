/*
 * testMode.c -- test utilities
 *
 *
 */

/* standard library */
#include <stdint.h>

/* TI library */
#include "tm4c123gh6pm.h"

/* project resource */
#include "common.h"
#include "txTask.h"

/** 
 * Initialize timer
 * param: none
 * return: none
**/
void testModeTimerInit(void)
{
	/* 
	 * configure trigger source timer 
	 */

	/* 1 disable timer, p.737 */
    WTIMER5_CTL_R = 0;

	/* 2 config register 64-bit timer, p.727 */
    WTIMER5_CFG_R = 0;

	/* 3 configure timer mode, p.729 */
	WTIMER5_TAMR_R = TIMER_TAMR_TAMR_1_SHOT | 
                     TIMER_TAMR_TACDIR;

	return;
}

/** 
 * Start timer
 * param: none
 * return: none
**/
void testModeTimerTic(void)
{
    /* reset timer value */
    WTIMER5_TAV_R = 0;
    WTIMER5_TBV_R = 0;

    /* start timer */
    WTIMER5_CTL_R |= TIMER_CTL_TAEN | TIMER_CTL_TBEN;
}

/** 
 * Stop timer
 * param: none
 * return: (long) -> timer counter value
**/
long testModeTimerToc()
{
    long tm = 0;

    /* read timer value */
    tm = WTIMER5_TAV_R;
    tm |= (WTIMER5_TBV_R << 32);

    /* stop the timer */
    WTIMER5_CTL_R = 0;

    /* reset timer value */
    WTIMER5_TAV_R = 0;
    WTIMER5_TBV_R = 0;

    return tm;
}


#define TEST_MODE_MIPS_SAMPLE_NUM_OF 50
unsigned long testModeMips[TEST_MODE_MIPS_SAMPLE_NUM_OF];
extern int txLoop(void);

/** 
 * Test TX task MIPS
 * param: none
 * return: none
**/
void testModeMipsMeasure(void)
{
    int cnt = 0;
    testModeTimerInit();

    while (1)
    {
        /* tic */
        testModeTimerTic();

        /* DUT */
        if (txLoop() == COMMON_RETURN_STATUS_SUCCESS)
        {
            /* toc */
            testModeMips[cnt % TEST_MODE_MIPS_SAMPLE_NUM_OF] = testModeTimerToc();

            cnt++;
        }
    }
}
