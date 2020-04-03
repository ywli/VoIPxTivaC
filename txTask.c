/*
 * tx.c -- transmit task
 *
 *
 */

/* standard library */
#include <stdint.h>

/* TI resource */
#include "tm4c123gh6pm.h"

/* FreeRTOS includes */
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* project resources */
#include "common.h"
#include "mic.h"
#include "wifi.h"
#include "rtp.h"
#include "abm.h"
#include "txTask.h"

/** 
 * Initialize TX task
 * param: none
 * return: none
**/
void txInit(void)
{
    /* initialize mic module */
    micInit();

    /* initialize wifi module */
    wifiInit();

    /* initialize rtp module */
    rtpInit();
}

/** 
 * one loop of TX task
 * param: none
 * return: (int) -> execution status
**/
int txLoop(void)
{
    wifiXferBlock_t *wifiPktP;
    micDataBlock_t *audioBlockP;

    /* 
    * read audio block 
    */
    audioBlockP = micBlockGet();
    /* no data */
    if (audioBlockP == 0)
    {
        return COMMON_RETURN_STATUS_FAILURE;
    }
    
    /* initialize buffer */
    wifiPktP = wifiPktSend1();
    if (wifiPktP == 0)
    {
        /* TX busy */
        abmAbort();
        return COMMON_RETURN_STATUS_FAILURE;
    }
    
    /* 
    * generate RTP packet 
    */
    wifiPktP->wifiPktSize = rtpWrite(
                wifiPktP->wifiPktP,
                &audioBlockP->micDataBlock[0],
                MIC_BLOCK_NUM_OF_SP);
    if (wifiPktP->wifiPktSize <= 0)
    {
        /* error */
        abmAbort();
        return COMMON_RETURN_STATUS_FAILURE;
    }
    
    /* send RTP packet */
    wifiPktSend2();

    return COMMON_RETURN_STATUS_SUCCESS;
}

/** 
 * TX task
 * param: pvParameters-> task argument
 * return: none
**/
void txTask(void *pvParameters)
{
    for (;;)
    {
        txLoop();
    }
}

