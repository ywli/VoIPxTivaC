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
#include "mic.h"
#include "wifi.h"
#include "rtp.h"
#include "abm.h"
#include "txTask.h"

void txInit(void)
{
    /* initialize mic module */
    micInit();

    /* initialize wifi module */
    wifiInit();

    /* initialize rtp module */
    rtpInit();
}

void txLoop()
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
        return;
    }
    
    /* initialize buffer */
    wifiPktP = wifiPktSend1();
    if (wifiPktP == 0)
    {
        /* TX busy */
        abmAbort();
        return;
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
        return;
    }
    
    /* send RTP packet */
    wifiPktSend2();

    return;
}

/* from microphone to Wifi */
void txTask(void *pvParameters)
{
    for (;;)
    {
        txLoop();
    }
}

